// Copyright (C) 2026 - zsliu98
// This file is part of ZLEqualizer
//
// ZLEqualizer is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLEqualizer is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLEqualizer. If not, see <https://www.gnu.org/licenses/>.

#include "fork_updater_platform.hpp"

#include "fork_update_logic.hpp"

#if JUCE_WINDOWS
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <shellapi.h>
#include <objbase.h>
#endif

namespace zlupdate::platform {
#if JUCE_WINDOWS
    namespace {
        void moduleAnchor() {
        }
    }

    juce::File getInstalledBundle() {
        HMODULE module = nullptr;
        if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                reinterpret_cast<LPCWSTR>(&moduleAnchor), &module)) {
            return {};
        }
        std::array<wchar_t, 32768> buffer{};
        const auto length = GetModuleFileNameW(module, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (length == 0 || length >= buffer.size()) {
            return {};
        }
        // `<bundle>.vst3\Contents\x86_64-win\<binary>.vst3`: walk up to the bundle folder
        auto dir = juce::File(juce::String(buffer.data())).getParentDirectory();
        for (int i = 0; i < 4 && dir != dir.getParentDirectory(); ++i) {
            if (dir.isDirectory() && dir.getFileName() == kBundleName) {
                return dir;
            }
            dir = dir.getParentDirectory();
        }
        return {};
    }

    LaunchResult launchElevatedScript(const juce::File& script) {
        const auto params = juce::String("-NoProfile -ExecutionPolicy Bypass -WindowStyle Hidden -File \"")
            + script.getFullPathName() + "\"";
        const auto hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
        SHELLEXECUTEINFOW info{};
        info.cbSize = sizeof(info);
        info.fMask = SEE_MASK_NOASYNC | SEE_MASK_FLAG_NO_UI;
        info.lpVerb = L"runas";
        info.lpFile = L"powershell.exe";
        info.lpParameters = params.toWideCharPointer();
        info.nShow = SW_HIDE;
        const auto ok = ShellExecuteExW(&info) != FALSE;
        const auto error = ok ? ERROR_SUCCESS : GetLastError();
        if (SUCCEEDED(hr)) {
            CoUninitialize();
        }
        if (ok) {
            return LaunchResult::kOk;
        }
        return error == ERROR_CANCELLED ? LaunchResult::kCancelled : LaunchResult::kFailed;
    }
#else
    juce::File getInstalledBundle() {
        return {};
    }

    LaunchResult launchElevatedScript(const juce::File&) {
        return LaunchResult::kFailed;
    }
#endif
}
