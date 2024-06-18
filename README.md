<!--
SPDX-License-Identifier: MPL-2.0

This file is part of Ramses Composer
(see https://github.com/bmwcarit/ramses-composer).

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
-->
# Ramses Composer

![](styles/ramses-composer-logo.png)

The authoring tool for the RAMSES rendering ecosystem.
Find the [user documentation here](https://ramses-composer.readthedocs.io/).
Find a broader overview of [the Ramses SDK here](https://ramses-sdk.readthedocs.io/).

## Building 

### Dependencies

Building RamsesComposer requires Qt 5.15.2 and CMake >=3.19 to be installed. Additionally, on Linux Python 3.8, linuxdeployqt, and gcc >=8 are needed.

For platform-specific build instruction see below.

### Setup

Make sure you have Git LFS installed:

```console
> git lfs install
Updated Git hooks.
Git LFS initialized.
```

To build Ramses Composer you first need to checkout and initialize it's dependencies:

```console
> git clone https://github.com/bmwcarit/ramses-composer ramses-composer
> cd ramses-composer
\raco> git submodule update --init --recursive
```

### Environment variables

If your Qt installation is not in the default location (Currently: ```C:/Qt/5.15.2/msvc2019_64```),
set the environment variable ```RACO_QT_BASE``` to it.

### Build with CMake

```console
\raco> mkdir build
\raco> cd build
\raco\build> cmake ..
\raco\build> cmake --build . --target RaCoEditor --config <CONFIG>       # <CONFIG> either RelWithDebInfo or Debug
```

Ramses Composer is built on Windows 10 with Visual Studio 2019 and on Ubuntu 20.04 with gcc 9.4.0.

### Windows

In order to install Qt headers and binaries, download the [official Qt online installer](https://www.qt.io/download-qt-installer) and select Qt 5.15.2 MSVC 2019 64-bit.

### Linux (Ubuntu 20.04)

The linux build needs newer versions of CMake and Qt than available from the Ubuntu 20.04 repositories. Install CMake >= 3.19 and Qt 5.15.2.

Installing Qt into /usr/local/opt/Qt/5.15.2 can be done like this:
```console
apt-get install python3-pip
python3 -m pip install --upgrade pip
python3 -m pip install aqtinstall
python3 -m aqt install --outputdir /usr/local/opt/Qt 5.15.2 linux desktop
```
Alternatively you can use the [official Qt installer](https://www.qt.io/download-qt-installer).

The environment variable QTBASEDIR needs to be set to the Qt base directory when running CMake, e.g. to /usr/local/opt/Qt in the example above.

To build ramses renderer dependent project you also need to install OpenGL dependencies:

```console
sudo apt install libegl1-mesa
sudo apt install libegl1-mesa-dev
```

Further Python 3.8.0 needs to be installed to allow building Ramses Composer:
```console
sudo apt install python3.8-dev
```

## Starting Ramses Composer

### Starting on Windows

The executable can be found in:
```console
\raco\build\release\bin\<CONFIG>\RamsesComposer.exe       # <CONFIG> either RelWithDebInfo or Debug
```

Starting RaCoEditor with an extra console showing stdout (Windows only):
```console
\raco\build\release\bin\<CONFIG>\RamsesComposer.exe -c
```

Starting RaCoEditor with an extra console and configured ramses framework log levels:
```console
\raco\build\release\bin\<CONFIG>\RamsesComposer.exe -c --RAPI 4 --RPER 0 --RRND 5 --RFRA 0 --RDSM 0'
```

RaCoEditor can also be given the initial project file as an command line argument:
```console
\raco\build\release\bin\<CONFIG>\RamsesComposer.exe <PATH_TO_PROJECT_FILE>
```
The ```<PATH_TO_PROJECT_FILE>``` has to be either an absolute path or relative to the current working directory.

### Starting on Linux (Ubuntu 20.04)

The Linux release contains all required Qt shared libraries. Ramses Composer Headless and Ramses Composer can be started using the
provided shell script `./RaCoHeadless.sh` resp. `./RamsesComposer.sh`.

The shell scripts together with the `qt.conf` allow the localization of all required Qt shared libraries.

### The Python environment

Both the Windows and Linux versions of the Ramses Composer Python 3.8 environment can be found in `./bin/python*`. The environment
is [isolated](https://docs.python.org/3/c-api/init_config.html#init-isolated-conf) from the system and contains [pip](https://pip.pypa.io/) for installing
custom modules. For more details how to install modules with pip, please refer to the Python section in the documentation.

### Locations

The various file locations are determined by the [PathManager.h](datamodel/libCore/include/core/PathManager.h). They are all relative to the executable.

#### Log file and Configuration files

The Ramses Composer looks for the the folder
```
../../configfiles
```
relative to its binary path. If the folder does not exist, Ramses Composer will automatically create it on startup.
The folder is also automatically created by CMake and can be found in the zip for the binary files.

Files in this directory are:
* layout.ini - Saves the layout and geometry information of the Qt application.
* RamsesComposer.log - Main log file.
* recent_files.ini - Saves the list of recent files opened with Ramses Composer.

#### Build process

The build process for the Ramses Composer generates a "release" folder in the CMake build directory which matches
the distributed zip-files - so after building Ramses Composer the developer will run Ramses Composer in the
same environment as the user.

## Development

### Logging system
Our logging system facade, which uses [spdlog](https://github.com/gabime/spdlog) as a backend, can be found in
```headless/libLogSystem/```.
To use the logging system link against the target
```raco::LogSystem```.
### Usage
```c++
#include <log_system/log.h>

using raco::log_system::COMMON;

...

int importantValue {43};
LOG_DEBUG(COMMON, "The important value is {}.", importantValue);
LOG(DEBUG, COMMON, "The important value is {}.", importantValue);
LOG_DEBUG_IF(COMMON, importantValue > 1, "The important value is {}.", importantValue);
```
We are using predefined log contexts (e.g. ```COMMON```, ```PROPERTY_BROWSER```) which are
predeclared in ```log.h```. If you need a new log context add it there and also initialize
a logger for this context during ```log_system::init()```.
Available log levels are ```TRACE, DEBUG, INFO, WARNING, ERROR, CRITICAL```.

Further information about capabilities and formatting can be found at [spdlog](https://github.com/gabime/spdlog) and [fmt](https://github.com/fmtlib/fmt).

### Testing
Example of how to use the raco::testing library:

```
set(TEST_LIBRARIES
    libRamsesBase
    raco::testing  # include testing
)
raco_package_add_headless_test(libRamsesBase_test
    "${TEST_SOURCES}"
    "${TEST_LIBRARIES}"
    ${CMAKE_CURRENT_BINARY_DIR}  # set working directory for test
)
raco_package_add_test_resources(libRamsesBase_test
${CMAKE_CURRENT_SOURCE_DIR} # source directory for the resources below
    res/basic.frag
    res/basic.vert
    res/bunny.ctm)
```
This example will create the setup for having a working directory with the specified resources already copied when using the testing/RacoBaseTest.h fixture.


### Project structure visualization

You can use cmake and graphviz to generate a dependency diagram between the cmake targets from inside the build/ directory as follows:
```
cmake --graphviz=deps.dot ..
dot -Tpng -o deps.png deps.dot
```
Graphviz needs to be installed for this.


### Debugging with Visual Studio

To enable the Visual Studio debugger to use better visualization of the RamsesComposer data structures there are two `.natvis` files in the top-level repository directory. Copying them to the directory $HOME/Visual Studio 2022/Visualizers will allow Visual Studio to pick them up automatically.

The attached visualizers are
* raco.natvis adds visualization for RamsesComposer data structures. Feel free to improve this and add more/better visualizations.
* qt5.natvis adds visualization for Qt data structures (e.g. strings).


## Third Party Components

The UI is based on [Qt](https://www.qt.io). Qt is used as Open Source under the LGPL 3 license in the form of unmodified dynamic libraries from Qt 5.15.2. You can find the [source code here](https://github.com/bmwcarit/ramses-composer/releases/download/v0.8.1/qt-src-5.15.2.tgz).

Ramses Composer uses a number of third party libraries:

* boost::spirit
* glm
* googletest
* lodepng
* Lua
* OpenCTM-1.0.3
* pybind11
* Python
* Qt Advanced Docking System
* RAMSES
* spdlog
* tinygltf
* zip

Their source code and respective licenses can be found in the ```third_party/``` folder.

## License

Ramses Composer is published under the [Mozilla Public License 2.0](LICENSE.txt).

Some icons originate from the [Google Material Design](https://material.io/resources/icons/?style=baseline) ([Apache 2.0 license](https://github.com/google/material-design-icons/blob/master/LICENSE)).

There are some example files included in ```resources/```. For Meshes taken from the Khronos glTF library, their individual licenses [are listed here](resources/meshes/README.md). All other meshes, Lua scripts, shaders and textures are also under MPL 2.0.

The ```screenshot_tests/projects/images/``` directory contains the [PNG Suite](http://www.schaik.com/pngsuite/) which is free to use ([License](screenshot_tests/projects/images/PngSuite-2017jul19/PngSuite.LICENSE)).

There are no obligations on the projects you create with Ramses Composer. They are your property and you may use and license them in any way you wish. This applies to the .rca scene file, everything referenced by it and all files exported from Ramses Composer.

