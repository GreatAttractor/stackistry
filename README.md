# **Stackistry**

## lucky imaging tool

Copyright (C) 2016, 2017 Filip Szczerek (ga.software@yahoo.com)

version 0.3.0 (2017-06-05)

*This program comes with ABSOLUTELY NO WARRANTY. This is free software, licensed under GNU General Public License v3 or any later version and you are welcome to redistribute it under certain conditions. See the LICENSE file for details.*

----------------------------------------

- 1\. Introduction
- 2\. Input/output formats support
- 3\. User interface overview
  - 3\.1\. Frame selection
  - 3\.2\. Processing settings
  - 3\.3\. Image stabilization anchors
  - 3\.4\. Visualization
  - 3\.5\. Output preview
- 4\. Frame quality
- 5\. Downloading
- 6\. Building from source code
  - 6\.1\. Building under Linux (and similar platforms)
  - 6\.2\. Building under MS Windows
  - 6\.3\. UI language
- 7\. Change log


----------------------------------------

![Main window](screenshot.png)


----------------------------------------
## 1. Introduction

**Stackistry** implements the *lucky imaging* principle of astronomical imaging: creating a high-quality still image out of a series of many (possibly thousands) low quality ones (blurred, deformed, noisy). The resulting *image stack* typically requires post-processing, including sharpening (e.g. via deconvolution). Such post-processing is not performed by Stackistry.

For Windows binary distributions, use `stackistry.bat` to start the program (you can also create a shortcut to it).

Video tutorials: https://www.youtube.com/watch?v=_68kEYBXkLw&list=PLCKkDZ7up_-VRMzGQ0bmmiXL39z78zwdE


----------------------------------------
## 2. Input/output formats support

Supported input formats:

- AVI: uncompressed DIB (mono or RGB), Y8/Y800
- SER: mono, RGB, raw color
- BMP: 8-, 24- and 32-bit uncompressed
- TIFF: 8 and 16 bits per channel mono or RGB uncompressed

Supported output formats:

- BMP: 8- and 24-bit uncompressed
- TIFF: 16-bit mono or RGB uncompressed

In case of 64-bit builds of Stackistry, there are no size limits for the input video/image size (other than the available memory). The user can choose to treat mono videos as raw color (enables demosaicing).


----------------------------------------
## 3. User interface overview

Most of the menu options have corresponding toolbar buttons or context menu options (opened via right-click on the job list).

Multiple jobs can be selected in the job list using common key combinations (`Ctrl-click`, `Shift-click`, `Ctrl-A`, `Shift-End` etc.). The following commands are executed for all selected jobs:

- `Edit/Processing settings...`
- `Edit/Remove from list`
- `Processing/Start processing`

While processing is in progress, no jobs can be removed or added.


----------------------------------------
### 3.1. Frame selection

The user can exclude some frames of a video from processing by using the `Edit/Select frames...` option (the option is active only if a single job is selected). In the selection dialog, multiple frames can be selected in the frame list using common key combinations (`Ctrl-click`, `Shift-click`, `Ctrl-A`, `Shift-End` etc.). Pressing `Space` toggles active state of selected frames; `Del` deactivates them.


----------------------------------------
### 3.2. Processing settings

Processing settings can be adjusted for multiple jobs at once by selecting them in the job list and using the `Edit/Processing settings...` option. Jobs being edited are shown in the list at the top of `Processing settings` dialog.

- Video stabilization methods

Stackistry can stabilize videos via stabilization anchors or by using the image intensity centroid (“center of mass”). Using anchors is recommended for solar & lunar videos. Intensity centroid is useful only for planetary videos (where automatic placement of anchors is not always satisfactory).

If manual placement of anchors is chosen, the anchor selection dialog will be displayed when processing of a job is about to start.

- Reference points placement

Automatic placement currently works best for solar/lunar surface videos. For planetary videos it may add some points in suboptimal positions (where their alignment will be unreliable). If this happens, manual placement should be chosen; the placement dialog will be displayed for a job during its processing, when the points need to be defined (i.e. after the quality estimation step). Avoid placing the points at areas with little or no detail. Use sunspots, filaments, knotted prominence areas, craters, cloud belts etc.

- Stacking criterion

The stacking criterion concerns both the reference point alignment as well as the image stacking steps. Note that increasing the percentage (or number) of stacked frame fragments also increases processing time and may produce image stack of worse quality (due to blurring by inclusion of poor-quality fragments).

- Automatic saving of image stack

If not disabled, the resulting image stack name is the same as input video file with `_stacked` suffix. For image series, it is simply `stack`.

Regardless of this setting, every completed job’s image stack can be saved via `File/Save stacked image...`.

- Flat-field

A flat-field is an image used to compensate any brightness variations caused by the optical train (vignetting, etalon “sweet spot”, Newton rings etc.). Stackistry can use a flat-field created by an external tool (provided it is in one of the supported input formats); it can also create its own via `File/Create flat-field from video...`. The video used for this purpose must show a uniformly lit and defocused view. Its absolute brightness is not important, however the tone curve must have the same characteristics as the video that is being processed (e.g. the camera’s gamma setting, if present, must be the same; shutter & gain settings may differ). For best results, use a video at least a few tens of frames long, so that any noise is averaged out.

- Demosaicing of raw color

Enabling the `Treat mono input as raw color` causes Stackistry to demosaic mono videos using the High Quality Linear Interpolation method (Malvar, He, Cutler). Stackistry does not detect the color filter pattern (RGGB, GRBG etc.) automatically, the user must choose the correct one (if it is unknown, can be done by trial and error, until the image in e.g. frame selection dialog is not showing checkerboard pattern nor reversed red/blue channels).

The option can be left disabled for SER raw color videos, as they are demosaiced automatically. However, it may happen that a SER video specifies invalid color filter pattern; in such case, use this option to override it.


----------------------------------------
### 3.3. Image stabilization anchors

With its emphasis on unattended batch processing of multiple jobs, Stackistry usually does a good job of placing video stabilization anchors automatically. If it is not the case, use the `Edit/Set video stabilization anchors...` option (or select manual anchor placement in job’s processing settings, see section 3.2). As with reference points, avoid placing anchors at areas with little or no detail.

Multiple anchors can be added, e.g. if the user knows that due to image drift some anchors will move out of view (Stackistry will then switch to one of the remaining anchors). Note that even if this happens to all user-defined anchors, Stackistry adds new one(s) automatically as needed.

When there is no severe image drift, one anchor is sufficient.


----------------------------------------
### 3.4. Visualization

Toggled by `Processing/Show visualization` (can be done at any time), this feature show a “visual diagnostic output” during processing. It can be used e.g. to verify that video stabilization anchors are handled correctly, to see if the reference points have been placed in adequate positions and are successfully tracked. Note that enabled visualization slows down processing.


----------------------------------------
### 3.5. Output preview

Beside the processing visualization, the right-hand side area of the main window can show the stacked image or the best fragments composite of the currently selected job. The stack is available if the job has finished processing. The composite (consisting of the best fragments of all frames) is available after the job has finished quality estimation.

Both the stack and the best fragments composite can be saved using the `File` menu commands (or the job list’s context menu, accessible by right-click).


----------------------------------------
## 4. Frame quality

Once the quality estimation phase of a job has completed, the frame quality data can be viewed in the quality graph window (toggled by `View/Frame quality graph` or the corresponding toolbar button). The window shows data of the currently selected job. Quality values are normalized to the range [0; 1].

Frame quality data can be exported on demand to a text file (which can be later e.g. imported into a spreadsheet program) via an option in the `File` menu or the quality window’s `Export...` button. Automatic exporting can be enabled in the job settings dialog (the resulting text file will be saved at the same location as the stack, with a `_frame_quality` suffix).

The user can choose (in `Edit/Preferences...`) if inactive frames (section 3.1) are to be included in the exported quality data. For instance, if there are 10 frames and all are active, the result is:

```
Frame;Active frame;Quality
0;0;0.55
1;1;0.56
2;2;0.56
3;3;0.56
4;4;0.56
5;5;0.56
6;6;0.56
7;7;0.56
8;8;0.56
9;9;0.56
```

If frames 3, 4, 5 are inactive and the option to export inactive frames is disabled:

```
Frame;Active frame;Quality
0;0;0.55
1;1;0.56
2;2;0.56
6;3;0.56
7;4;0.56
8;5;0.56
9;6;0.56
```

That is, the `Frame` column contains the absolute frame index, and `Active frame` counts only active frames.

If the inactive frames’ export is enabled, one gets:

```
Frame;Active frame;Quality
0;0;0.55
1;1;0.56
2;2;0.56
3;-1;0
4;-1;0
5;-1;0
6;3;0.56
7;4;0.56
8;5;0.56
9;6;0.56
```

Including inactive frames can e.g. simplify data comparison with output from a solar scintillation monitor.


----------------------------------------
## 5. Downloading

Source code and MS Windows executables can be downloaded from:
    
    https://github.com/GreatAttractor/stackistry/releases

    
----------------------------------------
## 6. Building from source code

Building from sources requires a C++ compiler toolchain (with C++11 support) and gtkmm 3.0 and *libskry* libraries. Versions (tags) of Stackistry and *libskry* should match; alternatively, one can use the latest revisions of both (note that they may be unstable).


----------------------------------------
### 6.1. Building under Linux (and similar platforms)

A GNU Make-compatible Makefile is provided. To build, install gtkmm 3.0 libraries (whose packages are usually named `gtkmm30` and `gtkmm30-devel` or similar), download (from `https://github.com/GreatAttractor/libskry/releases`) and build *libskry*, navigate to the Stackistry source folder, set values in Makefile’s `User-configurable variables` section and execute:

```
$ make
```

This produces `./bin/stackistry` executable. It can be moved to any location, as long as the `./icons` and `./lang` folders are placed at the same level as `./bin`.

If *libskry* is built with *libav* support enabled, Stackistry needs to be linked with *libav*. It is usually available as a package named `ffmpeg-devel` or similar. Otherwise, to build it from sources, execute:

```
$ git clone https://git.ffmpeg.org/ffmpeg.git
$ cd ffmpeg
$ ./configure --disable-programs --enable-gray --disable-muxers --disable-demuxers --enable-demuxer=avi --disable-encoders --disable-decoders --enable-decoder=rawvideo --enable-decoder=bmp --disable-protocols --enable-protocol=file --disable-parsers --disable-devices --disable-hwaccels --disable-bsfs --disable-filters --disable-bzlib --disable-lzma --disable-schannel --disable-xlib --disable-zlib --disable-iconv
$ make
$ make install
```


#### Ubuntu and *libav*

On Ubuntu the following packages are needed: `ffmpeg libavcodec-dev libavformat-dev libavutil-dev`.

In case of Ubuntu 16.04, versions provided by default are too old. Add the following repository before installing the packages:

    sudo add-apt-repository ppa:jonathonf/ffmpeg-3
    sudo apt update


----------------------------------------
### 6.2. Building under MS Windows

Building has been tested using the supplied Makefile in the MinGW/MSYS environment. Stackistry executable (`stackistry.exe`) is produced by performing the same steps as in section **6.1** (in the MSYS shell).

**Platform-specific notes**

MSYS2 installer can be downloaded from `https://msys2.github.io/` (follow the instructions to install). Once installed, start the MSYS shell and install required tools and libraries. For 64-bit GCC, execute:

```
$ pacman -S make base-devel mingw-w64-x86_64-toolchain
```

(the 32-bit package is called `mingw-w64-i686-toolchain`).

To install 64-bit GTK and gtkmm, execute:

```
$ pacman -S mingw-w64-x86_64-gtk3 mingw-w64-x86_64-gtkmm3
```

The package names may change in the future; to list actual package names for given libraries, execute:

```
$ pacman -Ss gtk3
$ pacman -Ss gtkmm
```

Before building, set PATH appropriately. For 64-bit GCC execute:

```
$ export PATH=/mingw64/bin:$PATH
```

Currently MSYS2 does not provide `ffmpeg/libav` package. It can be built from sources as shown in section **6.1**, but first Git and Yasm need to be installed:

```
$ pacman -S git yasm
```

and the *libav*'s `./configure` script needs an additional parameter: `--prefix=$MSYSTEM_PREFIX`.

MSYS mounts the drives as `/<drive_letter>`, so if Stackistry sources have been placed in:

```
C:\Users\MyUsername\Documents\stackistry
```

they can be accessed from MSYS shell at:

```
/c/Users/MyUsername/Documents/stackistry
```

Once built, Stackistry can be launched from MSYS shell (`./bin/stackistry.exe`). In order to run it directly from Windows Explorer, the necessary runtime DLLs and other supporting files must be placed in specific relative locations (see a Stackistry binary Windows distribution for reference).


----------------------------------------
### 6.3. UI Language

Stackistry supports multi-language user interface via the `GNU gettext` package. The current language can be changed in `Edit/Preferences...`.

All translatable strings in the source code are surrounded by the `_()` macro. Adding a new translation consists of the following steps:

- extraction of translatable strings from sources into a PO file by running:
```
$ xgettext -k_ src/*.cpp src/*.h -o stackistry.po
```

- translation of UI strings by editing the `msgstr` entries in `stackistry.po` (translated strings must be in UTF-8)

- converting `stackistry.po` to binary form by running:
```
$ msgfmt stackistry.po -o stackistry.mo
```

- placing `stackistry.mo` in the `lang/<lang_country>/LC_MESSAGES` folder, where `lang` is the ISO 643 language code, and `country` is the ISO 3166 country code (e.g. `de_DE`, `fr_CA`)

- adding the language to the `Utils::Const::languages` array (`src/utils.h`)

Binary distribution of Stackistry needs only the MO (binary) files. Additionally, a Windows distribution needs the following gtkmm language files (in these locations) in Stackistry folder:
```
share/locale/<lang>/LC_MESSAGES/gtk30.mo
share/locale/<lang>/LC_MESSAGES/gtk30-properties.mo
```


----------------------------------------
## 7. Change log

```
0.3.0 (2017-06-05)
  New features:
    - Better AVI support via libav
  Bug fixes:
    - Fixed crashes during triangulation
    
0.2.0 (2017-01-07)
  New features:
    - Image alignment using the intensity centroid (useful for planets)
    - Structure detection for better automatic placement of reference points
    - Zoom function in image view widgets
    - More controls for automatic reference point placement
    - Displaying and exporting of frame quality data
    - Generating the composite of best fragments of all frames
  Bug fixes:
    - Fixed opening AVI files produced by FFMPEG

0.1.1 (2016-08-05)
  New features:
    - Support for Y8/Y800 AVI files
  Enhancements:
    - User-configurable brightness threshold for automatic placement of reference points

0.1.0 (2016-06-06)
  New features:
    - Multilingual user interface support
    - Polish language version

0.0.3 (2016-05-22)
  New features:
    - Demosaicing of raw color images
  Bug fixes:
    - Error on opening SER videos recorded by Genika software
    - Incorrect RGB channel order when saving a BMP

0.0.2 (2016-05-08)
  Bug fixes:
    - Fix errors when stacking a series of TIFFs
    - Use all fragments if criterion is “number of the best”
      and the threshold is more than active images count
    
0.0.1 (2016-05-01)
  Initial revision.
```
