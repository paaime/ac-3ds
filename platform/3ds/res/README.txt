Animal Crossing 3DS
===================
Version @VERSION@

Animal Crossing (GameCube, USA) on the New Nintendo 3DS. It runs the game from
your own disc image: nothing of the game is in this download.

What you need
-------------
- A New 3DS, New 3DS XL or New 2DS XL with custom firmware (Luma3DS) and the
  Homebrew Launcher.
- An original 3DS or 2DS hasn't been tested. The game warns you and lets you
  try. Use the .cia there, since it gets the most memory. Expect it to be slow
  and to stop in busy places when memory runs out: save often.
- Your own disc image of Animal Crossing (USA), GAFE01, first release
  (.iso, .gcm or .ciso). RVZ and NKit images must be converted to ISO first
  (Dolphin: right-click the game, Convert File).
- For sound, the DSP firmware dump: sdmc:/3ds/dspfirm.cdc. If you don't have
  it, open the Rosalina menu (L + Down + SELECT), Miscellaneous options,
  Dump DSP firmware.

Installing
----------
1. Make the folder sdmc:/3ds/ac3ds/ on the SD card and put your disc image in
   it. On a Mac, copy it with Finder or "cp -X" and remove any "._" file that
   appears next to it.
2. Then either:
   - ac3ds.cia: install it with FBI. "Animal Crossing 3DS" appears on the
     HOME Menu.
   - ac3ds.3dsx: copy it to sdmc:/3ds/ and start it from the Homebrew
     Launcher. Open the Homebrew Launcher through a game (or Download Play),
     not from the Rosalina menu, or the game doesn't get enough memory.

The first start reads the disc image for a few seconds. If something is
wrong, the bottom screen says what and how to fix it.

Playing
-------
Buttons as on the GameCube: A, B, X, Y, START, the D-pad, L and R; ZL or ZR
is Z; the Circle Pad is the stick and the C-Stick the C-stick.

The bottom screen shows your town's map: tap an acre to see what's there.
Two buttons in its top-right corner:
- Save (the gyroid): saves anywhere in town, as the gyroid in front of your
  house does, and puts you back where you were.
- Options: the port's settings (skip Mr. Resetti, faster text, the camera,
  sharp textures, widescreen, no weeds, and more). Tap Back for the map.

Town ordinances, as in New Leaf: talk to Pelly at the post office (Phyllis
at night) and choose Ordinances. Filing one costs 20,000 Bells; repealing it
is free. One at a time, shared by the whole town, and only residents can
change it:
- Early Bird: villagers keep to a day two hours earlier; Tom Nook's shop
  opens at 6 AM (at 10 on raffle day, as usual).
- Night Owl: villagers keep to a day two hours later; the shop stays open
  until midnight.
- Bell Boom: Tom Nook pays 20% more, and his shop and catalog cost 20% more.
  Turnips keep their price.
- Beautiful Town: no new weeds and no cockroaches.

NES games (the NES furniture in your house) run in the middle of the top
screen; hold L and R and press ZL to leave. Clu Clu Land D is a Famicom Disk
System game and doesn't run.

Saves
-----
sdmc:/3ds/ac3ds/save/card_a/DobutsunomoriP_MURA.gci, with three backups
(.bak1 to .bak3). They are GameCube save files, the same as Dolphin's and the
PC port's: you can carry a town between them. The ordinance is kept in a
spare byte of the save, which they ignore.

If something goes wrong, the log is sdmc:/3ds/ac3ds/ac3ds.log.

Not available: the Game Boy Advance link (the island's GBA features, the
e-Reader) and the island from another town.

Credits and licences
--------------------
- The Animal Crossing decompilation, ACreTeam/ac-decomp (CC0 1.0).
- The PC port, flyngmt/ACGC-PC-Port (MIT), which this port builds on.
- The 3DS port (MIT).
- QuickNES, the NES emulator, by Shay Green, as maintained by libretro
  (GNU LGPL 2.1 or later; changed for this port).
- libctru and citro3d (zlib licence).
The licences are in the LICENSES folder. Source code of this release,
including QuickNES with its changes: @SOURCE_URL@

Animal Crossing is a trademark of Nintendo. This project isn't affiliated with
or endorsed by Nintendo, and contains no game data.
