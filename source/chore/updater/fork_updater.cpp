// Copyright (C) 2026 - zsliu98
// This file is part of ZLEqualizer
//
// ZLEqualizer is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLEqualizer is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLEqualizer. If not, see <https://www.gnu.org/licenses/>.

#include "fork_updater.hpp"

#include <array>
#include <vector>

#include <juce_events/juce_events.h>

#include "fork_updater_platform.hpp"

namespace zlupdate {
    namespace {
        constexpr int kCheckTimeoutMS = 5000;
        constexpr int kDownloadTimeoutMS = 10000;
        constexpr size_t kMaxJSONBytes = 1 << 20;
        constexpr juce::int64 kMaxZipBytes = 300 * 1024 * 1024;

        const juce::String kHeaders =
            "User-Agent: ZLEqualizer-fork-updater\r\nAccept: application/vnd.github+json, application/octet-stream\r\n";

        // single-quoted PowerShell string literal
        juce::String quote(const juce::String& s) {
            return "'" + s.replace("'", "''") + "'";
        }
    }

    ForkUpdater::ForkUpdater(std::function<void()> on_change) :
        juce::Thread("ZLForkUpdater"), on_change_(std::move(on_change)) {
    }

    ForkUpdater::~ForkUpdater() {
        {
            const juce::ScopedLock sl(lock_);
            if (active_stream_ != nullptr) {
                active_stream_->cancel();
            }
        }
        stopThread(15000);
    }

    ForkUpdater::State ForkUpdater::getState() const {
        const juce::ScopedLock sl(lock_);
        return state_;
    }

    ReleaseInfo ForkUpdater::getRelease() const {
        const juce::ScopedLock sl(lock_);
        return release_;
    }

    bool ForkUpdater::didServerReply() const {
        const juce::ScopedLock sl(lock_);
        return server_replied_;
    }

    void ForkUpdater::setState(const State state) {
        const juce::ScopedLock sl(lock_);
        state_ = state;
    }

    void ForkUpdater::notifyChange() {
        juce::MessageManager::callAsync([weak = std::weak_ptr<int>(life_), this]() {
            // the updater (and with it `life_`) is destroyed on the message thread, like this lambda runs
            if (weak.lock() && on_change_) {
                on_change_();
            }
        });
    }

    void ForkUpdater::startCheck() {
        if (isThreadRunning() || getState() != State::kIdle) {
            return;
        }
        {
            const juce::ScopedLock sl(lock_);
            state_ = State::kChecking;
            server_replied_ = false;
            task_ = Task::kCheck;
        }
        startThread(juce::Thread::Priority::low);
    }

    void ForkUpdater::setKnownRelease(const ReleaseInfo& info) {
        const juce::ScopedLock sl(lock_);
        if (state_ == State::kIdle) {
            release_ = info;
            state_ = State::kAvailable;
        }
    }

    void ForkUpdater::startInstall() {
        if (getState() != State::kAvailable) {
            return;
        }
        // a finished check thread may still be returning from run()
        stopThread(2000);
        {
            const juce::ScopedLock sl(lock_);
            state_ = State::kDownloading;
            task_ = Task::kInstall;
        }
        startThread(juce::Thread::Priority::low);
    }

    void ForkUpdater::run() {
        Task task;
        {
            const juce::ScopedLock sl(lock_);
            task = task_;
        }
        if (task == Task::kCheck) {
            runCheck();
        } else {
            runInstall();
        }
        if (!threadShouldExit()) {
            notifyChange();
        }
    }

    juce::File ForkUpdater::getWorkDirectory() {
        const auto local = juce::SystemStats::getEnvironmentVariable("LOCALAPPDATA", {});
        const auto root = local.isNotEmpty()
            ? juce::File(local)
            : juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
        return root.getChildFile("ZLEqualizerFork").getChildFile("updates");
    }

    void ForkUpdater::runCheck() {
        const juce::URL url(juce::String("https://api.github.com/repos/") + kRepo + "/releases/latest");
        juce::WebInputStream stream(url, false);
        stream.withExtraHeaders(kHeaders).withConnectionTimeout(kCheckTimeoutMS).withNumRedirectsToFollow(3);
        {
            const juce::ScopedLock sl(lock_);
            active_stream_ = &stream;
        }
        juce::String body;
        int status = 0;
        if (!threadShouldExit() && stream.connect(nullptr)) {
            status = stream.getStatusCode();
            if (status == 200) {
                juce::MemoryOutputStream out;
                std::array<char, 8192> buffer{};
                while (!threadShouldExit() && out.getDataSize() < kMaxJSONBytes) {
                    const auto n = stream.read(buffer.data(), static_cast<int>(buffer.size()));
                    if (n <= 0) {
                        break;
                    }
                    out.write(buffer.data(), static_cast<size_t>(n));
                }
                body = out.toUTF8();
            }
        }
        {
            const juce::ScopedLock sl(lock_);
            active_stream_ = nullptr;
        }
        if (threadShouldExit()) {
            return;
        }
        // 404 (no releases yet), rate limits, offline, malformed JSON: stay silent
        const auto info = status == 200 ? findNewerRelease(body, kForkBuild) : std::nullopt;
        const juce::ScopedLock sl(lock_);
        server_replied_ = status > 0;
        if (info.has_value()) {
            release_ = *info;
            state_ = State::kAvailable;
        } else {
            state_ = State::kIdle;
        }
    }

    bool ForkUpdater::downloadFile(const juce::String& url, const juce::File& target,
                                   const juce::int64 max_bytes, const int timeout_ms) {
        juce::WebInputStream stream{juce::URL(url), false};
        stream.withExtraHeaders(kHeaders).withConnectionTimeout(timeout_ms).withNumRedirectsToFollow(5);
        {
            const juce::ScopedLock sl(lock_);
            active_stream_ = &stream;
        }
        bool ok = false;
        const auto part = target.getSiblingFile(target.getFileName() + ".part");
        part.deleteFile();
        if (!threadShouldExit() && stream.connect(nullptr) && stream.getStatusCode() == 200) {
            if (juce::FileOutputStream out(part); out.openedOk()) {
                std::vector<char> buffer(64 * 1024);
                juce::int64 total = 0;
                ok = true;
                while (!threadShouldExit()) {
                    const auto n = stream.read(buffer.data(), static_cast<int>(buffer.size()));
                    if (n < 0) {
                        ok = false;
                        break;
                    }
                    if (n == 0) {
                        break;
                    }
                    total += n;
                    if (total > max_bytes || !out.write(buffer.data(), static_cast<size_t>(n))) {
                        ok = false;
                        break;
                    }
                }
                ok = ok && !threadShouldExit() && stream.isExhausted();
                out.flush();
            }
        }
        {
            const juce::ScopedLock sl(lock_);
            active_stream_ = nullptr;
        }
        if (ok) {
            target.deleteFile();
            ok = part.moveFileTo(target);
        }
        if (!ok) {
            part.deleteFile();
        }
        return ok;
    }

    juce::String ForkUpdater::makeScript(const juce::File& staged_bundle, const juce::File& target_bundle,
                                         const juce::File& log_file) {
        juce::String s;
        s << "$ErrorActionPreference = 'Stop'\r\n"
            << "$target = " << quote(target_bundle.getFullPathName()) << "\r\n"
            << "$staged = " << quote(staged_bundle.getFullPathName()) << "\r\n"
            << "$log = " << quote(log_file.getFullPathName()) << "\r\n"
            << "$bin = Join-Path $target 'Contents\\x86_64-win\\ZL Equalizer 2.vst3'\r\n"
            << "$backup = $target + '.old'\r\n"
            << "function Log($m) { Add-Content -LiteralPath $log -Value ('{0} {1}' -f (Get-Date -Format s), $m) }\r\n"
            << "$mutex = New-Object System.Threading.Mutex($false, 'Global\\ZLEqualizerForkUpdate')\r\n"
            << "if (-not $mutex.WaitOne(0)) { Log 'another updater is running'; exit 3 }\r\n"
            << "Log 'waiting for the plugin to be released'\r\n"
            << "$deadline = (Get-Date).AddMinutes(30)\r\n"
            << "$done = $false\r\n"
            << "while (-not $done -and (Get-Date) -lt $deadline) {\r\n"
            << "  try {\r\n"
            << "    if (Test-Path -LiteralPath $bin) {\r\n"
            << "      $fs = [IO.File]::Open($bin, 'Open', 'ReadWrite', 'None')\r\n"
            << "      $fs.Close()\r\n"
            << "    }\r\n"
            << "    if (Test-Path -LiteralPath $backup) { Remove-Item -LiteralPath $backup -Recurse -Force }\r\n"
            << "    if (Test-Path -LiteralPath $target) { Move-Item -LiteralPath $target -Destination $backup }\r\n"
            << "    try {\r\n"
            << "      Copy-Item -LiteralPath $staged -Destination $target -Recurse\r\n"
            << "      if (-not (Test-Path -LiteralPath $bin)) { throw 'binary missing after copy' }\r\n"
            << "    } catch {\r\n"
            << "      Log ('copy failed, restoring: ' + $_)\r\n"
            << "      if (Test-Path -LiteralPath $target) { Remove-Item -LiteralPath $target -Recurse -Force }\r\n"
            << "      if (Test-Path -LiteralPath $backup) { Move-Item -LiteralPath $backup -Destination $target }\r\n"
            << "      throw\r\n"
            << "    }\r\n"
            << "    if (Test-Path -LiteralPath $backup) { Remove-Item -LiteralPath $backup -Recurse -Force }\r\n"
            << "    $done = $true\r\n"
            << "  } catch {\r\n"
            << "    Log ('retry: ' + $_)\r\n"
            << "    Start-Sleep -Seconds 3\r\n"
            << "  }\r\n"
            << "}\r\n"
            << "if ($done) { Log 'updated'; exit 0 } else { Log 'gave up after 30 minutes'; exit 2 }\r\n";
        return s;
    }

    void ForkUpdater::runInstall() {
        const auto release = getRelease();
        const auto fail = [this]() { setState(State::kFailed); };

        const auto target_bundle = platform::getInstalledBundle();
        if (!target_bundle.isDirectory()) {
            return fail();
        }
        const auto work = getWorkDirectory();
        if (!work.createDirectory().wasOk()) {
            return fail();
        }
        const auto zip_file = work.getChildFile(getAssetName(release.build));
        if (!downloadFile(release.asset_url, zip_file, kMaxZipBytes, kDownloadTimeoutMS)) {
            return fail();
        }

        // verify the archive and stage it
        const auto stage = work.getChildFile("staging-" + juce::String(release.build));
        stage.deleteRecursively();
        if (!stage.createDirectory().wasOk()) {
            return fail();
        }
        {
            juce::ZipFile zip(zip_file);
            bool has_binary = false;
            for (int i = 0; i < zip.getNumEntries(); ++i) {
                if (zip.getEntry(i)->filename.replaceCharacter('\\', '/') == kBundleBinary) {
                    has_binary = true;
                    break;
                }
            }
            if (!has_binary || !zip.uncompressTo(stage).wasOk()) {
                return fail();
            }
        }
        if (stage.getChildFile(kBundleBinary).getSize() <= 0 || threadShouldExit()) {
            return fail();
        }

        const auto script = work.getChildFile("apply-update-" + juce::String(release.build) + ".ps1");
        const auto script_text = makeScript(stage.getChildFile(kBundleName), target_bundle,
                                            work.getChildFile("update.log"));
        // UTF-16 with BOM: Windows PowerShell reads non-ASCII paths correctly
        if (!script.replaceWithText(script_text, true, true, "\r\n")) {
            return fail();
        }
        switch (platform::launchElevatedScript(script)) {
        case platform::LaunchResult::kOk:
            setState(State::kReady);
            break;
        case platform::LaunchResult::kCancelled:
            setState(State::kCancelled);
            break;
        case platform::LaunchResult::kFailed:
            fail();
            break;
        }
    }
}
