Quake2
======

My own Quake 2 modification. Has a lot of unique features.

Source code: https://github.com/gildor2/Quake2

For more details, please visit http://www.gildor.org/en/projects/quake2

## Just a brief list of major features

- Support for game levels (maps) from other games: Quake 1, Half-Life, Quake 3, Kingpin. Those levels are playable with standard Quake 2 game
  and mods, level entities are converted into Q2 at loading time.
- Transparent support for Quake 3 player models: when one of clients will not have one, or when client is playing with different engine
  (original Q2 etc) - standard model will be used. Q3 player models has a possibility to use Q2 weapon models.
- Full network compatibility with original Quake 2 servers and clients.
- Improved user interface: it's still looking like Quake 2, but has things originally missing there, like level screenshots etc.
- More customization of player, for example a possibility to stylize railgun effect (like Q3A does).
- Possibility to customize levels, mainly to add FX to it. Called "level patch".
- Some eye-candy features like lens-flares, directional lighting etc.
- Heavily optimized renderer (actually made a entirely new renderer), using fixed pipeline OpenGL functions. One of features is correct
  dynamic directional lighting for game models.
- Gameplay features like "camper sounds", "falling sounds".
- Reviewed collision and game physics system, fixed many bugs there (and sorry, some exploits used by "hardcore gamers" will not work anymore).
- Lots of debugging console and 3D stuff which is possibly not interesting for most of people.

More detailed list of additions to the engine could be found in [4.XX_Changes.txt](4.XX_Changes.txt).

I didn't find a good name for this engine modifications, don't like things like "Quake 2 Pro", "Quake 2 XP", "Quake 2 2019", "Super Duper Quake 2"
etc. So I simply called it "Quake 2 4.0", as the recent vanilla Quake 2 has version 3.21.