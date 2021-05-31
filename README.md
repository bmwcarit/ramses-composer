<!--
SPDX-License-Identifier: MPL-2.0

This file is part of Ramses Composer
(see https://github.com/GENIVI/ramses-composer).

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
-->
# Ramses Composer

![](styles/ramses-composer-logo.png)

The authoring tool for the RAMSES rendering ecosystem. Find the [user manual here](https://github.com/GENIVI/ramses-composer-docs).

## Setup

To build Ramses Composer you first need to checkout and initialize it's dependencies:

```console
> git clone https://github.com/GENIVI/ramses-composer ramses-composer
> cd ramses-composer
\raco> git submodule update --init --recursive
```

## Environment variables

If your Qt installation is not in the default location (Currently: ```C:/Qt/5.15.2/msvc2019_64```),
set the environment variable ```RACO_QT_BASE``` to it. 

## Build with CMake

```console
\raco> mkdir build
\raco> cd build
\raco\build> cmake ..
\raco\build> cmake --build . --target RaCoEditor --config <CONFIG>       # <CONFIG> either Release or Debug
```

Ramses Composer is built on Windows 10 with Visual Studio 2019 and on Ubuntu 18.04 with gcc 7.5.0.

## Starting Ramses Composer

The executable can be found in:
```console
\raco\build\release\bin\<CONFIG>\RamsesComposer.exe       # <CONFIG> either Release or Debug
```

Starting RaCoEditor with an extra console showing stdout (Windows only):
```console
\raco\build\release\bin\<CONFIG>\RamsesComposer.exe -c
```

Starting RaCoEditor with an extra console and configured ramses framework log levels:
```console
\raco\build\release\bin\<CONFIG>\RamsesComposer.exe -c -r '--log-level-contexts-filter info:RAPI,off:RPER,debug:RRND,off:RFRA,off:RDSM,info:RCOM --log-level-console trace'
```

RaCoEditor can also be given the initial project file as an command line argument:
```console
\raco\build\release\bin\<CONFIG>\RamsesComposer.exe <PATH_TO_PROJECT_FILE>
```
The ```<PATH_TO_PROJECT_FILE>``` has to be either an absolute path or relative to the current working directory.

### Starting on Linux (Ubuntu 18.04)

The Linux release is developer-only. To start it, it is necessary to install Qt 5.15.2 on the machine. Once Qt 5.15.2 is 
installed, Ramses Composer Headless and Ramses Composer can be started using the command-line 

```
LD_LIBRARY_PATH="./;/<QtRootDir>/5.15.2/gcc_64/lib" ./RaCoHeadless
```
resp.
```
LD_LIBRARY_PATH="./;/<QtRootDir>/5.15.2/gcc_64/lib" ./RamsesComposer
```

It is also necessary to provide a qt.conf next to the RaCoHeadless / RamsesComposer executable with the contents

```
[Paths]
Plugins="<QtRootDir>/5.15.2/gcc_64/plugins"
```

Adjust ```<QtRootDir>``` in both cases to the directory in which Qt 5.15.2 was installed.

If you see the error message 
```
Could not load the Qt platform plugin "xcb" in "" even though it was found.
```
when running RamsesComposer a few xcb libraries might be missing. You can try running Ramses Composer with
```
QT_DEBUG_PLUGINS=1;LD_LIBRARY_PATH="./;/<QtRootDir>/5.15.2/gcc_64/lib" ./RamsesComposer
```
and look for a line looking like
```
Cannot load library /(...)/Qt/5.15.2/gcc_64/plugins/platforms/libqxcb.so: (libxcb-icccm.so.4: cannot open shared object file: No such file or directory)
```
and install the package required to install the missing .so file (in this case 'sudo apt install libxkb-icccm4' fixes the issue).

### Locations (Windows)

The Ramses Composer will create an directory within the windows documents folder:
```
%userprofile%\Documents\RaCo
```
Files in this directory are:
* settings.ini - Saves the layout and geomtry information of the Qt application.
* recent_files.ini - Saves the list of recent files opened with Ramses Composer.
* RaCo.log - Main log file.

These locations can be modified in [PathManager.h](headless/libCommon/include/ramses_composer/PathManager.h). 

## Windows Subsystem for Linux (Ubuntu 18.04)

The linux build needs newer versions of CMake and Qt than available from the Ubuntu 18.04 repositories. Install CMake >= 3.19 and Qt 5.15.2.

Installing Qt into /usr/local/opt/Qt/5.15.2 can be done like this:
```console
apt-get install python3-pip
python3 -m pip install --uprade pip
python3 -m pip install aqtinstall
python3 -m aqt install --outputdir /usr/local/opt/Qt 5.15.2 linux desktop
```

The environment variable QTBASEDIR needs to be set to the Qt base directory when running CMake, e.g. to /usr/local/opt/Qt in the example above.

To build ramses renderer dependent project you also need to install OpenGL dependencies:

```console
sudo apt install libegl1-mesa
sudo apt install libegl1-mesa-dev
```

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
We are using predifined log contexts (e.g. ```COMMON```, ```PROPERTY_BROWSER```) which are
predeclared in ```log.h```. If you need a new log context add it there and also initialize
a logger for this context during ```log_system::init()```.
Available log levels are ```TRACE, DEBUG, INFO, WARNING, ERROR, CRITICAL```.

Further information about capabailites and formatting can be found at [spdlog](https://github.com/gabime/spdlog) and [fmt](https://github.com/fmtlib/fmt).

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
raco_package_add_test_resouces(libRamsesBase_test
${CMAKE_CURRENT_SOURCE_DIR} # source directory for the resources below
    res/basic.frag
    res/basic.vert
    res/bunny.ctm)
```
This example will create the setup for having a working directory with the specified resources already copied when using the testing/RacoBaseTest.h fixture.

## Third Party Components

The UI is based on [Qt](www.qt.io). Qt is used as Open Source under the LGPL 3 license in the form of unmodified dynamic libraries from Qt 5.15.2. You can find the [source code here](https://github.com/GENIVI/ramses-composer/releases/download/v0.8.1/qt-src-5.12.2.tgz). 

Ramses Composer uses a number of third party libraries:

* assimp
* googletest
* OpenCTM-1.0.3
* Qt Advanced Docking System
* RAMSES 
* RAMSES logic
* spdlog

Their source code and respective licenses can be found in the ```third_party/``` folder.
	
## License

Ramses Composer is published under the [Mozilla Public License 2.0](LICENSE.txt).

Some icons originate from the [Google Material Design](https://material.io/resources/icons/?style=baseline) ([Apache 2.0 license](https://github.com/google/material-design-icons/blob/master/LICENSE)).

There are some example files included in ```resources/```. For Meshes taken from the Khronos glTF library, their individual licenses [are listed here](resources\meshes\README.md). All other meshes, Lua scripts, shaders and textures are also under MPL 2.0.

