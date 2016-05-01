# **Stackistry**

## lucky imaging tool

Copyright (C) 2016 Filip Szczerek (ga.software@yahoo.com)

version 0.0.1 (2016-05-01)

*This program comes with ABSOLUTELY NO WARRANTY. This is free software, licensed under GNU General Public License v3 or any later version and you are welcome to redistribute it under certain conditions. See the LICENSE file for details.*

----------------------------------------

- 1\. Introduction
- 2\. Input/output formats support
- 3\. User interface overview
  - 3\.1\. Frame selection
  - 3\.2\. Processing settings
  - 3\.3\. Image stabilization
  - 3\.4\. Visualization
- 4\. Downloading
- 5\. Building from source code
  - 5\.1\. Building under Linux (and similar platforms)
  - 5\.2\. Building under MS Windows
- 6\. Change log


----------------------------------------
## 1. Introduction

**Stackistry** implements the *lucky imaging* principle of astronomical imaging: creating a high-quality still image out of a series of many (possibly thousands) low quality ones (blurred, deformed, noisy). The resulting *image stack* typically requires post-processing, including sharpening (e.g. via deconvolution). Such post-processing is not performed by Stackistry.

For Windows binary distributions, use ``stackistry.bat`` to start the program (you can also create a shortcut to it). Note that currently on some versions of Windows (e.g. 8.1, but not Vista or Server 2012) there is an error message (“Program Gio has stopped working” or similar) on starting; it does not affect Stackistry’s operation.


----------------------------------------
## 2. Input/output formats support

Supported input formats:

- AVI: uncompressed DIB (mono or RGB)
- SER: mono or RGB
- BMP: 8-, 24- and 32-bit uncompressed
- TIFF: 8 and 16 bits per channel mono or RGB uncompressed

Supported output formats:

- BMP: 8- and 24-bit uncompressed
- TIFF: 16-bit mono or RGB uncompressed

AVI files up to 2 GiB are supported. In case of 64-bit builds of Stackistry, there are no size limits for the remaining formats (other than the available memory).


----------------------------------------
## 3. User interface overview

Most of the menu options have corresponding toolbar buttons or context menu options (opened via right-click on the job list).

Multiple jobs can be selected in the job list using common key combinations (``Ctrl-click``, ``Shift-click``, ``Ctrl-A``, ``Shift-End`` etc.). The following commands are executed for all selected jobs:

- ``Edit/Processing settings...``
- ``Edit/Remove from list``
- ``Processing/Start processing``

While processing is in progress, no jobs can be removed or added.


----------------------------------------
### 3.1. Frame selection

The user can exclude some frames of a video from processing by using the ``Edit/Select frames...`` option (the option is active only if a single job is selected). In the selection dialog, multiple frames can be selected in the frame list using common key combinations (``Ctrl-click``, ``Shift-click``, ``Ctrl-A``, ``Shift-End`` etc.). Pressing ``Space`` toggles active state of selected frames; ``Del`` deactivates them.


----------------------------------------
### 3.2. Processing settings

Processing settings can be adjusted for multiple jobs at once by selecting them in the job list and using the ``Edit/Processing settings...`` option. Jobs being edited are shown in the list at the top of ``Processing settings`` dialog.

- Video stabilization anchors placement

Automatic placement should be reliable in most cases. If manual placement is chosen, the anchor selection dialog will be displayed when processing of a job is about to start.

- Reference points placement

Automatic placement currently works best for solar/lunar surface videos. For solar prominence videos with an overexposed disk and for planetary videos it may add some points in suboptimal positions (where their alignment will be unreliable). If this happens, manual placement should be chosen; the placement dialog will be displayed for a job during its processing, when the points need to be defined (i.e. after the quality estimation step). Avoid placing the points at areas with little or no detail. Use sunspots, filaments, knotted prominence areas, craters, cloud belts etc.

- Stacking criterion

The stacking criterion concerns both the reference point alignment as well as the image stacking steps. Note that increasing the percentage (or number) of stacked frame fragments also increases processing time and may produce image stack of worse quality (due to blurring by inclusion of poor-quality fragments).

- Automatic saving of image stack

If not disabled, the resulting image stack name is the same as input video file with ``_stacked`` suffix. For image series, it is simply ``stack``.

Regardless of this setting, every completed job’s image stack can be saved via ``File/Save stacked image...``.

- Flat-field

A flat-field is an image used to compensate any brightness variations caused by the optical train (vignetting, etalon “sweet spot”, Newton rings etc.). Stackistry can use a flat-field created by an external tool (provided it is in one of the supported input formats); it can also create its own via ``File/Create flat-field from video...``. The video used for this purpose must show a uniformly lit and defocused view. Its absolute brightness is not important, however the tone curve must have the same characteristics as the video that is being processed (e.g. the camera’s gamma setting, if present, must be the same; shutter & gain settings may differ). For best results, use a video at least a few tens of frames long, so that any noise is averaged out.


----------------------------------------
### 3.3. Image stabilization

With its emphasis on unattended batch processing of multiple jobs, Stackistry usually does a good job of placing video stabilization anchors automatically. If it is not the case, use the ``Edit/Set video stabilization anchors...`` option (or select manual anchor placement in job’s processing settings, see section 3.2). As with reference points, avoid placing anchors at areas with little or no detail.

Multiple anchors can be added, e.g. if the user knows that due to image drift some anchors will move out of view (Stackistry will then switch to one of the remaining anchors). Note that even if this happens to all user-defined anchors, Stackistry adds new one(s) automatically as needed.

When there is no severe image drift, one anchor is sufficient.


----------------------------------------
### 3.4. Visualization

Toggled by ``Processing/Show visualization...`` (can be done at any time), this feature show a “visual diagnostic output” during processing. It can be used e.g. to verify that video stabilization anchors are handled correctly, to see if the reference points have been placed in adequate positions and are successfully tracked. Note that enabled visualization slows down processing.


----------------------------------------
## 4. Downloading

Source code and MS Windows executables can be downloaded from:
    
    https://github.com/GreatAttractor/stackistry/releases

    
----------------------------------------
## 5. Building from source code

Building from sources requires a C++ compiler toolchain (with C++11 support) and GTKmm 3.0 and *libskry* libraries.


----------------------------------------
### 5.1. Building under Linux (and similar platforms)

A GNU Make-compatible Makefile is provided. To build, install GTKmm 3.0 libraries (whose packages are usually named ``gtkmm30`` and ``gtkmm30-devel`` or similar), download (from ``https://github.com/GreatAttractor/libskry/releases``) and build *libskry*, navigate to the Stackistry source folder, set values in Makefile’s ``User-configurable variables`` section and execute:

```
$ make
```

This produces ``./bin/stackistry`` executable. It can be moved to any location, as long as the ``./icons`` folder is placed at the same level as ``./bin``.


----------------------------------------
### 5.2. Building under MS Windows

Building has been tested using the supplied Makefile in the MinGW/MSYS environment. Stackistry executable (``stackistry.exe``) is produced by performing the same steps as in section **5.1** (in the MSYS shell).

**Platform-specific notes**

MSYS2 installer can be downloaded from ``https://msys2.github.io/`` (follow the instructions to install). Once installed, start the MSYS shell and install required tools and libraries. For 64-bit GCC, execute:

```
$ pacman -S make base-devel mingw-w64-x86_64-toolchain
```

(the 32-bit package is called ``mingw-w64-i686-toolchain``).

To install 64-bit GTK and GTKmm, execute:

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
export PATH=/mingw64/bin:$PATH
```

Once built, Stackistry can be launched from MSYS shell (``./bin/stackistry.exe``). In order to run it directly from Windows Explorer, the necessary runtime DLLs must be placed in specific relative locations (see a Stackistry binary Windows distribution for reference).

----------------------------------------
## 6. Change log

```
0.0.1 (2016-05-01)
    Initial revision.
```
