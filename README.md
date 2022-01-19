For EOSRP.160 using [stm32doom](https://github.com/floppes/stm32doom)

Important!

- Set camera to Movie Mode
- Enter `HDMI info disp` menu and set it to `With info`
  - The code of Doom and Magic Lantern is loaded to the 4K clean HDMI buffers. If you set your camera to clean HDMI mode,
    code will get overwritten by cameras rendering code and will execute random instructions, which may damage your camera.
- Set camera to Manual mode and keep it in this mode to play Doom.

- Update your cameras firmware to version 1.6.0

- Enable SCRIPT and BOOT flag in camera
  - To do so, you can use this tool on Windows: https://chdk.fandom.com/wiki/EOScard
  
- Format an SD card in camera
- Create a `DOOM` folder in SD cards root directory
- Download `DOOM1.WAD` file of Doom Shareare version from https://www.wad-archive.com/wad/5b2e249b9c5133ec987b3ea77596381dc0d6bc1d/download/DOOM1.WAD and copy it into DOOM directory

To compile:

```
cd platform/EOSRP.160
make clean && make -j4

- copy `autoexec.bin` to SD cards root directory

Attributions:

-[kitor](https://github.com/kitor)
-[names_are_hard](https://github.com/reticulatedpines)
-[turtius](https://github.com/turtiustrek)

Magic Lantern
=============

Magic Lantern (ML) is a software enhancement that offers increased
functionality to the excellent Canon DSLR cameras.
  
It's an open framework, licensed under GPL, for developing extensions to the
official firmware.

Magic Lantern is not a *hack*, or a modified firmware, **it is an
independent program that runs alongside Canon's own software**. 
Each time you start your camera, Magic Lantern is loaded from your memory
card. Our only modification was to enable the ability to run software
from the memory card.

ML is being developed by photo and video enthusiasts, adding
functionality such as: HDR images and video, timelapse, motion
detection, focus assist tools, manual audio controls much more.

For more details on Magic Lantern please see [http://www.magiclantern.fm/](http://www.magiclantern.fm/)
