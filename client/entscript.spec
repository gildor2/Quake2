- if (): cvar, maptype (q2, KP, q3, HL, any text (??)), platform (cvar ??), game (cvar)
- entities:
  - rename class
  - delete
  - set maptype (if specific entity appears)
- fields:
  - set type (for map scaling: angle, vector, distance ... (?))
  - rename
  - delete
  - add

modicication (engine-side; mapscript ?):
  - ref entities (no in client/server, or provide replacement with bbox?) -- fire, flares, smoke (KP), lots of MDX props
  - attach sound to entity (AQ2 have some helicopter maps; all helicopters have a few STATIC sound points; + map "roadrage" ...)
  - modify entity traectory (scripted with Python?) -- rotate helicopters on some maps (should not be too complex to implement)
  ? if 3D sound will be implemented: areas within different EAX presets
  ? if bots will be implemented (Python): AI hints (look UT for samples)
