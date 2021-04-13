EmulationStation FCAMOD for rk3326 devices
=======================

This is a fork of EmulationStation FCAMOD by fabricecaruso containing additions for the Odroid Go Advance device and clones. 

Building
========

EmulationStation uses some C++11 code, which means you'll need to use at least g++-4.7 on Linux to compile.

EmulationStation has a few dependencies. For building, you'll need CMake, SDL2, FreeImage, FreeType, cURL and RapidJSON.  You also should probably install the `fonts-droid-fallback` package which contains fallback fonts for Chinese/Japanese/Korean characters, but ES will still work fine without it (this package is only used at run-time).

**On Odroid Go Advance Ubuntu 18.04, 19.10 or 20.04 distros or development OS:**
All of this be easily installed with `apt-get`:
```bash
sudo apt update -y && sudo apt-get install -y libboost-system-dev libboost-filesystem-dev \
  libboost-locale-dev libfreeimage-dev libfreetype6-dev libeigen3-dev libcurl4-openssl-dev \
  libboost-date-time-dev libasound2-dev cmake libsdl2-dev rapidjson-dev libvlc-dev \
  libvlccore-dev vlc-bin libsdl2-mixer-dev
```

Note this Repository uses a git submodule - to checkout the source and all submodules, use

```bash
git clone --recursive https://github.com/christianhaitian/EmulationStation-fcamod.git
```

or 

```bash
git clone https://github.com/christianhaitian/EmulationStation-fcamod.git
cd EmulationStation-fcamod
git submodule update --init
```
If you don't have the go2 headers in either /usr/local/include/go2 or /usr/include/go2, you will also need go2 headers files from [here](https://github.com/OtherCrashOverride/libgo2/tree/master/src) to be copied into a folder named go2 in your /usr/local/include folder.

Then, generate and build the Makefile with CMake:
```bash
cd EmulationStation-fcamod
sudo dpkg -i --force-all libmali-rk-bifrost-g31-rxp0-wayland-gbm_1.7-2+deb10_arm64.deb
dpkg -i libmali-rk-dev_1.7-1+deb10_arm64.deb
cmake .
make (or use make -j2 or -j3 if you have the additional core and memory to handle this to speed up the build)
```

**Special Note**

If you'd like to enable the ability to scrape games using TheGamesDB and/or ScreenScraper.fr, you'll need to provide a developer ID or API Key for the service(s) with a cmake command as follows:

For ScreenScraper.fr:
Use cmake -DSCREENSCRAPER_DEV_LOGIN="devid=<yourdevid>&devpassword=<yourdevpass>"

For TheGamesDB:
Use cmake -DGAMESDB_APIKEY="<YourAPIKey>" 

You can also include a unique name for your build of EmulationStation so that ScreenScraper can provide some stats for your build:
Use cmake -DSCREENSCRAPER_SOFTNAME="<YourBuildNameHere>"

Example cmake line with all of these features enabled:

```
cmake -DSCREENSCRAPER_DEV_LOGIN="devid=TestID&devpassword=TestPassword" -DGAMESDB_APIKEY="someapikeyprovidedbythegamesdb" -DSCREENSCRAPER_SOFTNAME="MyBuildEmulationStation"
```

As of 04/13/2021: 

You can request a Developer ID and Password from screenscraper.fr by creating an account then go to the forum located at: https://www.screenscraper.fr/forumsujets.php?frub=12&numpage=0

You can request a apikey from TheGamesDB by creating an account then go to the forum located at: https://forums.thegamesdb.net/viewforum.php?f=10


current brightness script for es-app/src/guis/GuiMenu.cpp line 78
=================
current_brightness
```
#!/bin/bash

cursound=$(cat /sys/class/backlight/backlight/brightness);
maxsound=255;

echo $((200 * $cursound/$maxsound - 100 * $cursound/$maxsound ))
```

current volume script for es-app/src/guis/GuiMenu.cpp line 78
============
current_volume
```
#!/bin/bash

awk -F'[][]' '/Left:/ { print $2 }' <(amixer sget Playback)
```

Changes in my branch
====================

**System list :** 
- Support for Multiple Emulators/Cores in es_systems.cfg, and setting Emulator/Core per game.

	```xml
	  <command>%HOME%\RetroArch\retroarch.exe -L %HOME%\RetroArch\cores\%CORE%_libretro.dll %ROM%</command>
	  <emulators>
	      <emulator name="mame">
		<cores>
		  <core>mame2003_plus</core>
		  <core>mame2003</core>
		</cores>
	      </emulator>
	      <emulator name="fbalpha">
		<cores>
		  <core>fbalpha2012</core>
		</cores>
	      </emulator>
	    </emulators>
	```
**Grid view :** 
- Animations when size changes and during scrolling.
- Supports having a label. 
	```xml
	<text name="gridtile">
	    <color>969A9E</color>
	    <size>1 0.18</size>
	</text>
	<text name="gridtile_selected">
	    <color>F6FAFF</color>
	</text>
	```	
- Layout can be defined by number of columns and rows ( you had to calculate manually the size of tiles in previous versions ). Zooming the selected item can also be defined simply.
	```xml
	<imagegrid name="gamegrid">
	      <autoLayout>4 3</autoLayout>	
	      <autoLayoutSelectedZoom>1.04</autoLayoutSelectedZoom>
	```	 
- Supports extended padding (top, left, bottom, right) :
	```xml
	<imagegrid name="gamegrid">
	      <padding>0.03 0.13 0.03 0.08</padding>
	```	 
	
- Supports video in the selected item (delay can be defined in the theme)
	```xml
	<imagegrid name="gamegrid">
	      <showVideoAtDelay>700</showVideoAtDelay>      
	```	 	
	
- Theme can define which image to use (image, thumbnail or marquee).
	```xml
	<imagegrid name="gamegrid">
	      <imageSource>marquee</imageSource>
	```	 

- Theme can define the image sizing mode (minSize, maxSize or size). Gridtile items can define a padding.
	```xml
	<gridtile name="default">
	    <padding>24 24</padding>
	    <imageSizeMode>minSize</imageSizeMode>
	```	 
	
- Supports md_image, md_video, md_name items... just like detailed view.
- Ability to override grid size by system.

**Detailed view :** 
- Supports md_video, md_marquee items like video view did : Video view is no longer useful.

**Custom views & Theming:** 	
- Allow creation of custom views, which inherits from one of the basic theme items ( basic, detailed, grid ).
	```xml
	<customView name="Video grid" inherits="grid">
	    <imagegrid name="gamegrid">
	```	    
- Ability to select the view (or customview) to use globally or by system.
- The theme can force the default view to use ( attribute defaultView )
- Fully supports Retropie & Recalbox Themes.
- Carousel supports element "logoPos" : this allows the logo not to be inevitably centered.
- Image loading : the image bytes where duplicated 3 times in memory.
- In previous versions, if a xml element was unknown in the theme, nothing was loaded.
- Support for glows around text 
- Reflection for images ( table reflection effect )
- Gradients for selected menu and list items.
		    
**Optimizations & Fixes:** 	
- Faster loading time, using multithreading.
- Optimized memory usage for files and gamelists.
- The loading sequence displays a progress bar.
- Reviewed SVG loading and size calculation mecanism. Previous versions unloaded/reloaded SVGs each time a new container needed to display it because of a size calculation problem.
- Ability to disable "Preload UI" mecanism. This mecanism is used to preload the UI of gamelists of every system. Disable it adds a small lags when opening
- Don't keep in memory the cache of image filenames when launching games -> It takes a lot of memory for nothing.
- Skip parsing 'downloaded_images' and 'media' folders ( better loading time )
- Added option "Optimize images Vram Use" : Don't load an image in it source resolution if it needs to be displayed smaller -> Resize images in memory to save VRAM. Introduce longer image loading time, but less VRAM use.
- Fixed video starting : Videos started fading even if the video was not available yet ( but not really fading : there was no blending ).
- Software clipping : Avoid rendering clipped items -> They were previously clipped by OpenGl scissors.
- Carousel animation was corrupted if the carousel has to display only one item with <maxLogoCount>1</maxLogoCount>
- Font : Optimization when calculating text extend.
- If XML writer fails, the gamelist.xml file become empty and set to 0Kb -> Added a mecanism to secure that. Also, previous gamelist.xml version is saved as gamelist.xml.old.

**Menus :** 	
- Cleaned menus + changed menu item order (by interest). 
- Full support for menu Theming.
- Separated "Transition style" and "Game launch transition"
- Added option "Boot on gamelist"
- Added option "Hide system view"
- Added option "Display favorites first in gamelist"
- Fixed cutoff gui menus such as scraper and screensaver for rk3326 devices.
- Resized metadata menu to be full sized for rk3326 devices.

**General :** 	
- Localisation (French actually supported)
- OSK : On-screen Keyboard.
- Video elements can be added as extras.
- Fixed : Don't show Games what are marked Hidden in gamelist.
- Added a star icon before the name of the game when it is a favorite.
- Corrected favorites ( and custom lists ) management.
- Don't show Directories that contains only one Game : just Show the game.
- Case insensitive file extensions.
- Stop using "sortname" in gamelists. It is useful.

Je crois que c'est à peu près tout...



Configuring
===========

**~/.emulationstation/es_systems.cfg:**
When first run, an example systems configuration file will be created at `~/.emulationstation/es_systems.cfg`.  `~` is `$HOME` on Linux.  This example has some comments explaining how to write the configuration file. See the "Writing an es_systems.cfg" section for more information.

**Keep in mind you'll have to set up your emulator separately from EmulationStation!**

**~/.emulationstation/es_input.cfg:**
When you first start EmulationStation, you will be prompted to configure an input device. The process is thus:

1. Hold a button on the device you want to configure.  This includes the keyboard.

2. Press the buttons as they appear in the list.  Some inputs can be skipped by holding any button down for a few seconds (e.g. page up/page down).

3. You can review your mappings by pressing up and down, making any changes by pressing A.

4. Choose "SAVE" to save this device and close the input configuration screen.

The new configuration will be added to the `~/.emulationstation/es_input.cfg` file.

**Both new and old devices can be (re)configured at any time by pressing the Start button and choosing "CONFIGURE INPUT".**  From here, you may unplug the device you used to open the menu and plug in a new one, if necessary.  New devices will be appended to the existing input configuration file, so your old devices will remain configured.

**If your controller stops working, you can delete the `~/.emulationstation/es_input.cfg` file to make the input configuration screen re-appear on next run.**


You can use `--help` or `-h` to view a list of command-line options. Briefly outlined here:
```
--resolution [width] [height]   try and force a particular resolution
--gamelist-only                 skip automatic game search, only read from gamelist.xml
--ignore-gamelist               ignore the gamelist (useful for troubleshooting)
--draw-framerate                display the framerate
--no-exit                       don't show the exit option in the menu
--no-splash                     don't show the splash screen
--debug                         more logging
--scrape                        scrape using command line interface
--windowed                      not fullscreen, may be used with --resolution
--vsync [1/on or 0/off]         turn vsync on or off (default is on)
--max-vram [size]               Max VRAM to use in Mb before swapping. 0 for unlimited
--force-kid             Force the UI mode to be Kid
--force-kiosk           Force the UI mode to be Kiosk
--force-disable-filters         Force the UI to ignore applied filters in gamelist
--help, -h                      summon a sentient, angry tuba
```

As long as ES hasn't frozen, you can always press F4 to close the application.


Writing an es_systems.cfg
=========================

Complete configuration instructions at [emulationstation.org](http://emulationstation.org/gettingstarted.html#config).

The `es_systems.cfg` file contains the system configuration data for EmulationStation, written in XML.  This tells EmulationStation what systems you have, what platform they correspond to (for scraping), and where the games are located.

ES will check two places for an es_systems.cfg file, in the following order, stopping after it finds one that works:
* `~/.emulationstation/es_systems.cfg`
* `/etc/emulationstation/es_systems.cfg`

The order EmulationStation displays systems reflects the order you define them in.

**NOTE:** A system *must* have at least one game present in its "path" directory, or ES will ignore it! If no valid systems are found, ES will report an error and quit!

Here's an example es_systems.cfg:

```xml
<!-- This is the EmulationStation Systems configuration file.
All systems must be contained within the <systemList> tag.-->

<systemList>
	<!-- Here's an example system to get you started. -->
	<system>
		<!-- A short name, used internally. -->
		<name>snes</name>

		<!-- A "pretty" name, displayed in the menus and such. This one is optional. -->
		<fullname>Super Nintendo Entertainment System</fullname>

		<!-- The path to start searching for ROMs in. '~' will be expanded to $HOME or %HOMEPATH%, depending on platform.
		All subdirectories (and non-recursive links) will be included. -->
		<path>~/roms/snes</path>

		<!-- A list of extensions to search for, delimited by any of the whitespace characters (", \r\n\t").
		You MUST include the period at the start of the extension! It's also case sensitive. -->
		<extension>.smc .sfc .SMC .SFC</extension>

		<!-- The shell command executed when a game is selected. A few special tags are replaced if found in a command, like %ROM% (see below). -->
		<command>snesemulator %ROM%</command>
		<!-- This example would run the bash command "snesemulator /home/user/roms/snes/Super\ Mario\ World.sfc". -->

		<!-- The platform(s) to use when scraping. You can see the full list of accepted platforms in src/PlatformIds.cpp.
		It's case sensitive, but everything is lowercase. This tag is optional.
		You can use multiple platforms too, delimited with any of the whitespace characters (", \r\n\t"), eg: "genesis, megadrive" -->
		<platform>snes</platform>

		<!-- The theme to load from the current theme set. See THEMES.md for more information.
		This tag is optional; if not set, it will use the value of <name>. -->
		<theme>snes</theme>
	</system>
</systemList>
```

The following "tags" are replaced by ES in launch commands:

`%ROM%`		- Replaced with absolute path to the selected ROM, with most Bash special characters escaped with a backslash.

`%BASENAME%`	- Replaced with the "base" name of the path to the selected ROM. For example, a path of "/foo/bar.rom", this tag would be "bar". This tag is useful for setting up AdvanceMAME.

`%ROM_RAW%`	- Replaced with the unescaped, absolute path to the selected ROM.  If your emulator is picky about paths, you might want to use this instead of %ROM%, but enclosed in quotes.

See [SYSTEMS.md](SYSTEMS.md) for some live examples in EmulationStation.

gamelist.xml
============

The gamelist.xml file for a system defines metadata for games, such as a name, image (like a screenshot or box art), description, release date, and rating.

If at least one game in a system has an image specified, ES will use the detailed view for that system (which displays metadata alongside the game list).

*You can use ES's [scraping](http://en.wikipedia.org/wiki/Web_scraping) tools to avoid creating a gamelist.xml by hand.*  There are two ways to run the scraper:

* **If you want to scrape multiple games:** press start to open the menu and choose the "SCRAPER" option.  Adjust your settings and press "SCRAPE NOW".
* **If you just want to scrape one game:** find the game on the game list in ES and press select.  Choose "EDIT THIS GAME'S METADATA" and then press the "SCRAPE" button at the bottom of the metadata editor.

You can also edit metadata within ES by using the metadata editor - just find the game you wish to edit on the gamelist, press Select, and choose "EDIT THIS GAME'S METADATA."

A command-line version of the scraper is also provided - just run emulationstation with `--scrape` *(currently broken)*.

The switch `--ignore-gamelist` can be used to ignore the gamelist and force ES to use the non-detailed view.

If you're writing a tool to generate or parse gamelist.xml files, you should check out [GAMELISTS.md](GAMELISTS.md) for more detailed documentation.


Themes
======

By default, EmulationStation looks pretty ugly. You can fix that. If you want to know more about making your own themes (or editing existing ones), read [THEMES.md](THEMES.md)!

I've put some themes up for download on my EmulationStation webpage: http://aloshi.com/emulationstation#themes

If you're using RetroPie, you should already have a nice set of themes automatically installed!


-Alec "Aloshi" Lofquist
http://www.aloshi.com
http://www.emulationstation.org
