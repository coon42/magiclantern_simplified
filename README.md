Doom for Canon EOS RP at firmware version 1.6.0. Based on [stm32doom](https://github.com/floppes/stm32doom).

**Disclaimer: This is a very hacky and experimental proof of concept of running doom on a DIGIC 8 camera. I don't recommend
to run this in its current state as the user has to consider some precautions for not crashing their camera.
Use the following instructions at own risk!**

**Important! Before you start, please follow these steps or you may damage your camera!
The camera MUST NOT be set to clean HDMI mode!**

How to get it running:

- Disable Clean HDMI mode
  - Set camera to Movie Mode
  - Enter `HDMI info disp` menu and set it to `With info`
    - The code of Doom and Magic Lantern is loaded into the 4K clean HDMI buffers. You must not set your camera to clean HDMI mode or
      code will get overwritten by cameras rendering code and will execute random instructions, which may damage your camera.
  - Set camera to Manual mode and keep it in this mode all the time.
- Ensure that your camera firmware version is 1.6.0 (It won't work with other versions)
- Enable Bootflag in camera
  - Copy `script.req` file and `extend.m` files from `platform/EOSRP.160` directory to SD cards root directory
  - Start camera and press the play button (left to the trashcan button) 
- SD card setup
  - Format an SD card in camera
  - Enable SCRIPT and BOOT flag on SD card:
    - To do so, you can use this tool on Windows: https://chdk.fandom.com/wiki/EOScard 
  - Create a `DOOM` folder in SD cards root directory
  - Download `DOOM1.WAD` file of Doom Sharware version from https://www.wad-archive.com/wad/5b2e249b9c5133ec987b3ea77596381dc0d6bc1d/download/DOOM1.WAD and copy it into DOOM directory.
  - Compile the code and copy `autoexec.bin` to SD cards root directory

To compile:

```
cd platform/EOSRP.160
make clean && make -j4

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
