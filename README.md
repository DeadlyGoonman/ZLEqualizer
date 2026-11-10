<p align="center">
  <img src="docs/zlaudio.svg" width="7.5%" hspace="6.068%"/>
  <img src="docs/logo.svg" width="7.5%" hspace="6.068%"/>
</p>

# ZL Equalizer
![pluginval](<https://github.com/ZL-Audio/ZLEqualizer/actions/workflows/cmake_full_test.yml/badge.svg?branch=main>)
[![License](https://img.shields.io/badge/License-AGPLv3-blue.svg)](https://opensource.org/license/agpl-v3)
[![Downloads](https://img.shields.io/github/downloads/ZL-Audio/ZLEqualizer/total)](https://somsubhra.github.io/github-release-stats/?username=ZL-Audio&repository=ZLEqualizer&page=1&per_page=30)

ZL Equalizer is an equalizer plugin.

<img src="https://drive.google.com/uc?export=view&id=1xnUGRs3NvWL-2MZ2i8rg0g0bUnG6MSO2" style="width:750px; max-width: 100%; height: auto" />

## About This Fork

This is an unofficial fork of [ZL-Audio/ZLEqualizer](https://github.com/ZL-Audio/ZLEqualizer) that changes how the curve editor feels to use. It is not affiliated with or supported by ZL Audio.

### Added Features

- **0 dB line always visible**: the summed response line is drawn as a flat 0 dB line even with no bands, and bends as bands are added.
- **Ghost curves**: hovering empty graph space shows a faint outline, with a soft glow, of the band a click would create. The outline morphs smoothly between shapes (for example from a low cut to a bell) and fades in and out.
- **Single click adds a band**: one left click on empty space creates the band shown by the ghost curve. Dragging on empty space still does lasso selection.
- **Automatic cuts**: clicking near the 0 dB line below 75 Hz creates a low cut, and above 17 kHz creates a high cut. Clicking far above or below 0 dB in those ranges creates a shelf.
- **Hover popup**: hovering any band dot opens a compact popup with bypass, frequency, gain, solo, close, filter type, Q and stereo mode. Values can be edited by double-clicking them.
- **Drag from the popup**: dragging the popup's background moves its band, just like dragging the band dot.
- **Scroll wheel sets cut slope**: on low and high cuts, the scroll wheel steps through the slopes 6, 12, 24, 36, 48, 72 and 96 dB/oct and never changes Q.
- **Display refresh rate**: a new "Display" refresh rate option (the default) repaints the UI at the monitor's refresh rate. Existing users can choose it under UI Settings > Refresh Rate.
- **Finer knobs**: drag and scroll wheel sensitivity is halved for all knobs and sliders.
- **Auto-update**: when the editor opens (at most once per 24 hours) the plugin asks this fork's GitHub releases for a newer `fork-N` build and shows a small "Update available" pill with Install / Later. On Windows, Install downloads the release, checks it, and starts a one-prompt (UAC) helper that swaps in the new VST3 as soon as your DAW releases the old one, so restart the DAW to finish. On other systems Install opens the release page. Turn it off under UI Settings > Other > Check for Updates. Only a version request to `api.github.com` is made, with no personal data.

### Releasing

1. Bump the number in `FORK_BUILD` (one integer, for example `2`) and commit it.
2. Tag and push: `git tag fork-2 && git push origin fork-2`.
3. The `Fork Release` workflow builds the Windows VST3 and publishes the release with `ZLEqualizer-fork-2-windows-vst3.zip`. Installed copies with a lower build number then offer the update.

## Build from Source

### Install Dependencies

Please make sure `Clang` (`AppleClang 16+` or `LLVM/Clang 17+`), `cmake 3.25+`, `ninja` are installed and configured on your OS.

On Linux, you can install the remaining dependencies with the following command:

```console
sudo apt-get update && sudo apt install libasound2-dev libx11-dev libxcomposite-dev libxcursor-dev libxext-dev libxinerama-dev libxrandr-dev libxrender-dev libfreetype-dev libfontconfig1-dev libxi-dev
```

### Clone and Build

Once you have set up the environment, you can clone the ZL Equalizer code, populate all submodules, then configure & build the code. Please set:

- the variable `ZL_JUCE_FORMATS` as a list of plugin formats that you want, e.g., `"VST3;LV2"`
  - AAX plug-ins need to be digitally signed using PACE Anti-Piracy's signing tools before they will run in commercially available versions of Pro Tools.
- the variable `ZL_EQ_BAND_NUM` as the number of bands, default 24 bands
  - The plugins built with different `ZL_EQ_BAND_NUM` may NOT be compatible with each other.
- the variable `ZL_HWY_STATIC_TARGET` as the SIMD target
  - If you are on x86-64 and your CPU supports SSE2, set `ZL_HWY_STATIC_TARGET=SSE2`.
  - If you are on x86-64 and your CPU supports SSE2/SSE4, set `ZL_HWY_STATIC_TARGET=SSE4`.
  - If you are on x86-64 and your CPU supports SSE2/SSE4/AVX2, set `ZL_HWY_STATIC_TARGET=AVX2`.
  - If you are on arm64, set `ZL_HWY_STATIC_TARGET=NEON`.
- If there are multiple compilers on your OS, you may need to pass extra flags to maker sure that cmake uses `LLVM/Clang`.
  - On Linux, you may pass `-DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++`.
  - On Windows, you may pass `-DCMAKE_C_COMPILER=clang-cl -DCMAKE_CXX_COMPILER=clang-cl`.

```console
git clone https://github.com/DeadlyGoonman/ZLEqualizer
cd ZLEqualizer
git submodule update --init --recursive
cmake -B Builds -G Ninja -DCMAKE_BUILD_TYPE=Release -DZL_JUCE_FORMATS="VST3;LV2" -DZL_HWY_STATIC_TARGET="NEON" .
cmake --build Builds --config Release
```

After building, the plugins should have been copied to the corresponding folders. If you want to disable the copy process, you can pass `-DZL_JUCE_COPY_PLUGIN=FALSE`, find the binary folders under `Builds/ZLEqualizer_artefacts/Release` and copy them manually.

## AI Transparency

Since October 2026, LLMs have assisted across various stages of this project's development. All code, documentation, and design choices are either authored or reviewed by the maintainer, who takes full responsibility for the quality, safety, and licensing of the final output.

## License

ZL Equalizer is licensed under AGPLv3, as found in the [LICENSE.md](LICENSE.md) file. However, the [logo of ZL Audio](assets/zlaudio.svg) and the [logo of ZL Equalizer](assets/logo.svg) are not covered by this license.

Copyright (c) 2023-2026 [zsliu98](https://github.com/zsliu98)

JUCE framework from [JUCE](https://github.com/juce-framework/JUCE)

JUCE template from [pamplejuce](https://github.com/sudara/pamplejuce)

[nlopt](https://github.com/stevengj/nlopt) by [Steven G. Johnson](https://github.com/stevengj)

[Highway](https://github.com/google/highway) by [Google](https://github.com/google)

[Material Symbols](https://github.com/google/material-design-icons) by [Google](https://github.com/google)

[inter](https://github.com/rsms/inter) by [The Inter Project Authors](https://github.com/rsms/inter)

## References

Yuriy Ivantsov. *On the Ideal Bilinear and Biquadratic Digital Filter*. (2025).

Yuriy Ivantsov. *On the State Space of a Linear Digital Filter*. (2025).

Cleve Moler. [*Makima Piecewise Cubic Interpolation*](https://blogs.mathworks.com/cleve/2019/04/29/makima-piecewise-cubic-interpolation/). MathWorks Blogs. (2019).

Nigel Redmon. *Cascading filters*. (2016).

___

Martin Vicanek. *Matched One-Pole Digital Shelving Filters*. (2019).

Martin Vicanek. *Matched Second Order Digital Filters*. (2016).

Aaron Wishnick. *Time-Varying Filters for Musical Applications*. DAFx. (2014).
