## Fork build 8

**New**
- Shelves: the mouse is now the shelf's plateau. Mouse height = the shelf's full gain, and the shelf reaches ~95% of that gain at the cursor. The ghost curve, the glow point and the placed band all match (no more 2x gain jump between shelf and bell zones).
- Phase curve overlay (Settings > Other > "Phase Curve", default Off): a thin line showing the real phase of all bands for the current filter structure, mapped to +/-180 degrees, with a faint 0 degree reference and labels. Colour: Settings > Colour > "Phase Curve Colour".
- Update check on every open: the plugin asks about a new version each time it opens. "Later" only hides the banner for that session.

**Notes**
- Phase for Matched / Mixed / Zero phase is computed analytically and is very close to, not bit-identical with, the real filter. Dynamic gain, polarity flip and latency are not drawn.

**Install**
- Run `ZLEqualizer-fork-8-windows-installer.exe` (installs to `C:\Program Files\Common Files\VST3`), or unzip the VST3 by hand. Close your DAW first.
