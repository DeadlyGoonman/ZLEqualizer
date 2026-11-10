// Copyright (C) 2026 - zsliu98
// This file is part of ZLEqualizer
//
// ZLEqualizer is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLEqualizer is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLEqualizer. If not, see <https://www.gnu.org/licenses/>.

#pragma once

#include <atomic>
#include <functional>
#include <memory>

#include "fork_update_logic.hpp"

namespace zlupdate {
    // Checks GitHub for a newer fork release and installs it. All network and disk work runs
    // on one background thread; `on_change` is always called on the message thread.
    class ForkUpdater final : private juce::Thread {
    public:
        enum class State {
            kIdle,
            kChecking,
            kAvailable,
            kDownloading,
            kReady,
            kCancelled,
            kFailed
        };

        explicit ForkUpdater(std::function<void()> on_change);

        ~ForkUpdater() override;

        // message thread: asks GitHub for the latest release
        void startCheck();

        // message thread: uses an already known release without asking GitHub
        void setKnownRelease(const ReleaseInfo& info);

        // message thread: downloads and stages the release, then launches the elevated installer
        void startInstall();

        State getState() const;

        ReleaseInfo getRelease() const;

        // true if the last check got an HTTP answer (any status) from the server
        bool didServerReply() const;

    private:
        enum class Task {
            kCheck,
            kInstall
        };

        std::function<void()> on_change_;
        std::shared_ptr<int> life_{std::make_shared<int>(0)};

        mutable juce::CriticalSection lock_;
        State state_{State::kIdle};
        ReleaseInfo release_;
        bool server_replied_{false};
        Task task_{Task::kCheck};
        juce::WebInputStream* active_stream_{nullptr};

        void run() override;

        void runCheck();

        void runInstall();

        void setState(State state);

        void notifyChange();

        bool downloadFile(const juce::String& url, const juce::File& target, juce::int64 max_bytes, int timeout_ms);

        static juce::File getWorkDirectory();

        static juce::String makeScript(const juce::File& staged_bundle, const juce::File& target_bundle,
                                       const juce::File& log_file);
    };
}
