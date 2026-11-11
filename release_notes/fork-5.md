## Fork build 5 (includes builds 3 and 4)

**New**
- Windows installer exe attached to the release (installs to `C:\Program Files\Common Files\VST3`, with uninstaller).
- Drag-to-place: press on the ghost curve and drag to create a band and keep dragging it in one smooth gesture. A plain click still places it.
- Frequency readout (with note name) at the bottom of the graph, follows the mouse.
- Middle-click on a control resets it to default. On the graph it resets Q.
- Undo / redo: Ctrl+Z, Ctrl+Y or Ctrl+Shift+Z.
- Per-band colours and a 0 dB line colour in Settings > Colour.
- Ctrl+X deletes the selected band. Stereo-mode menu on the band popup opens downwards, away from the delete button.
- Settings panel scrolls about 3x faster.

**Changed**
- Low cut / high cut zones are mirrored on the frequency axis: low cut only below ~23.5 Hz, high cut only above 17 kHz. Hovering around 60 Hz now gives a low shelf instead of snapping to a high pass.

**Fixed**
- Hovering the bottom of the graph no longer goes blank after deselecting a band.
- Click when changing a band's type or slope: 50 ms crossfade.
