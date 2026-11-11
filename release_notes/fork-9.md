## Fork build 9

**New**
- Ghost curve now morphs smoothly (about 90 ms) when it changes shape (bell, shelf, cut, notch) instead of popping. The glow point slides along.
- Hysteresis at the zone edges (about 64 Hz and 6.25 kHz): small mouse jitter no longer flips the ghost between a bell and a shelf.
- Curve background gradient: Settings > Colour > "Gradient Top", "Gradient Bottom" (opacity 0 = off) and "Gradient Blend" (Bright = brightness, Meet = where the colours meet, 0 top / 0.5 middle / 1 bottom, Soft = width of the transition).

**Notes**
- The gradient is on by default with subtle dark colours. Set both opacities to 0 to get the plain background back.

**Install**
- Run `ZLEqualizer-fork-9-windows-installer.exe` (installs to `C:\Program Files\Common Files\VST3`), or unzip the VST3 by hand. Close your DAW first.
