<!--
SPDX-License-Identifier: MPL-2.0

This file is part of Ramses Composer
(see https://github.com/bmwcarit/ramses-composer-docs).

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
-->
# Introduction
![](./images/sample_project.png)

## What is Ramses Composer?

Ramses Composer is an interactive authoring tool for the [RAMSES rendering ecosystem](https://github.com/bmwcarit/ramses).
While RAMSES provides an efficient OpenGL 3D rendering engine it used to be quite challenging to harness
it as the whole setup of scenes had to be done programmatically in C++. This is where Ramses Composer
comes in. It is a WYSIWYG editor that allows you to create and set up a scene interactively in a user
interface and immediately see the effect of your actions in a preview.

![](images/ramses_toolchain.png)

Ramses Composer strives to make the features of RAMSES accessible in an intuitive UI. The individual
3D assets of a project are created in other tools like Blender or Photoshop and then imported, configured
and connected into a working scene graph. As RAMSES is geared towards efficient performance on embedded
targets, you will often use optimized versions of the assets rather than direct exports from major
desktop applications. You can also add dynamic behaviour and business logic using scripts. Additionally,
Ramses Composer checks your input, helps to avoid mistakes and gives you some tools to structure and
maintain your project. The scene is stored in a JSON-based .rca file which is human-readable to some
degree and can be inspected and processed in text-based tools (diff, Git, grep etc.).

Ramses Composer runs instances of the RAMSES renderer and RAMSES logic internally to create a working
scene and display it in the preview window - with live updates of everything that happens  in the scene
graph as well as in the external assets. This scene can then be exported into RAMSES files to be used
in a target application or with a player.

Ramses Composer runs on Windows as well as Ubuntu 18 Linux. The screenshots in this manual refer to the
Windows version.

## User interface overview

The different views of the user interface can be rearranged by drag&drop to form whatever layout you
prefer to work with. Just play around with it until you feel comfortable. If you accidentially closed
a view, you can bring it up again from the _View_ menu. The default layout contains the main views:

__Scene Graph__: This tree view is the heart of Ramses Composer and displays the scene graph with all
the elements of your scene. The elements can be rearranged by drag&drop. Multiple elements can be
selected and cut/copied/pasted whenever possible. The order in the scene graph also determines
the rendering order.

__Resources__: The resource view shows all the resources like meshes or textures used by the scene. By
clicking on the column headers, it can be sorted by name or by type. It is possible to use resources
which are included from other projects. Such external resources are colored in green.

__Prefabs__: This view shows a list of reusable components. Find out more about Prefabs [here](../prefabs/README.md).
It is possible to use prefabs which are defined in and included from other projects.
Such external prefabs are colored in green.

__Undo Stack__: Here you can see all the changes you made to the scene. It is possible
to undo and redo multiple changes by selecting the according line in this view.

__Property Browser__: Here you can see and change all the properties of the object
last selected in the Scene Graph, Resources or Prefabs view. It is possible to lock
a property browser to permanently  stick with the same object using the lock button
on top. You can also add additional property browsers from the _View_ menu to observe
and configure multiple objects.

__Ramses Preview__: Here you see what your scene will look like. Note that you can
select different methods for zooming the preview on the top. _Original fit_ will give
you the exact resolution of the camera. If your scene has multiple cameras, the
preview will show the camera you modified last. Take note that you can currently
create only one preview via the _View_ menu.

### Layouts

Ramses Composer saves your current view layout when closing. This current layout will get
restored upon relaunching Ramses Composer or loading a project.

You can save, load and delete custom layouts, or restore the default layout.  All these
options are available in the _View_ → _Layouts_ menu.

Saved layouts are program-global - loading a project will not load a custom layout.

## Asset types and tools

__Meshes__ are created in a 3D modelling tool. Ramses Composer is designed and tested to
work well with Blender. It supports the [Khronos group glTF](https://www.khronos.org/gltf/)
as a modern format for meshes.

__Textures__ can be created in any graphics application. There are 2D textures and cube
maps. Ramses Composer currently supports PNG images (the most common type with 8bit
RGBA colors). They can be mipmapped by activating the "Generate Mipmaps" flag
in the Property Browser.

__Shaders__ in RAMSES are written in the [OpenGL Shading Language](https://en.wikipedia.org/wiki/OpenGL_Shading_Language),
or to be more specific in GLSL ES 3.0. For editing them, a plain text editor with syntax highlighting is recommended,
e.g. Notepad++.

__LUA Scripts__ add dynamic behaviour and business logic to a scene. They are simple
text files which can be edited either with a text editor or a dedicated LUA IDE like
[ZeroBrane Studio](https://studio.zerobrane.com/).

## Ramses Composer Editor

_RamsesComposer.exe_ is the interactive scene editor normally used for creating RAMSES scenes and logic.

![](images/raco_editor.png)

Currently, Ramses Composer has no integrated log view. To get some deeper insight
into what the application and RAMSES are doing or to look for helpful details in
case of problems, start Ramses Composer with the command line parameter **-c**. This
will open an additional console window with the log output, both from Ramses Composer
and the RAMSES client.

**Command line options**

	-h, --help                                	Displays help on commandline options.
	--help-all                                	Displays help including Qt specific options.
	-v, --version                             	Displays version information.
	-c, --console                             	Open with std out console.
	-a, --framework-arguments <default-args>  	Override arguments passed to the ramses framework.
	-d, --nodump                              	Don't generate crash dumps on unhandled exceptions.
	-p, --project <project-path>              	Load a scene from specified path.
	-t, --trace-messages-ramses               	Enable trace-level Ramses log messages.
	-f, --featurelevel <feature-level>        	RamsesLogic feature level (-1, 1 ... 2)
	-r, --run <script-path>                   	Run Python script. Specify arguments for python script by writing '--' before arguments.
 	-y, --pythonpath <python-path>				Directory to add to python module search path.

## Ramses Composer Headless

_RaCoHeadless.exe_ is a version of Ramses Composer without user interface, renderer or UI
dependencies. It is intended for use with build systems (e.g. to generate scene binaries
on the fly), for test automation, or for scripted asset generation.

![](images/raco_headless.png)

Currently it can be used for loading projects as a smoke test or for automating the export
of Ramses Composer scenes to Logic and RAMSES files.

**Command line options**

	-h, --help                          	Displays help on commandline options.
	--help-all                          	Displays help including Qt specific options.
	-v, --version                       	Displays version information.
	-p, --project <project-path>        	Load a scene from specified path.
	-e, --export <export-path>          	Export Ramses scene and logic to path.
											File extensions are added automatically (ignored if '-r' is used).
	-c, --compress                      	Compress Ramses scene on export (ignored if '-r' is used).
	-d, --nodump                        	Don't generate crash dumps on unhandled exceptions.
	-l, --loglevel <log-level>          	Maximum information level that should be printed as console log output. Possible options:
											0 (off), 1 (critical), 2 (error), 3 (warn), 4 (info), 5 (debug), 6 (trace).
	-r, --run <script-path>             	Run Python script.
	-o, --outlogfile <log-file-name>    	File name to write log file to.
	-f, --featurelevel <feature-level>  	RamsesLogic feature level (-1, 1 ... 2)
 	-y, --pythonpath <python-path>			Directory to add to python module search path.


## Feature Levels

To allow easier upgrades of RAMSES and RAMSES Logic in the toolchain RAMSES Logic allows to selectively enable only a subset of features using feature levels. To support this in RamsesComposer every project has a feature level which is used when exporting using RAMSES and RAMSES Logic. By keeping the feature level of an existing project  when switching to a newer RamsesComposer version featuring a newer RAMSES Logic the exported files can still be loaded by an older RAMSES Logic supporting the feature level of the project. A RamsesComposer project has to be upgraded explicitly by the user to a new feature level if desired. Upgrading a RamsesComposer project to a newer feature level is needed to access the features provided by that feature level.

The feature level for new projects can be set using the `Preferences Dialog` in the GUI application and via the `-f` commandline option in the headless application.

To upgrade a project to a higher feature level the `-f` commandline option can be used which will attempt to upgrade the initially loaded project given using the `-p` commandline option to the desired feature level. The GUI application also allows a feature level upgrade via the `File/Upgrade Feature Level` menu. The new feature level can't be lower than the current feature level of the project or the upgrade will fail.

A project may reference external projects with the same or lower feature levels but externally referenced projects must not have a higher feature level than the master project.

The following table contains a list of the features enabled in RamsesComposer at various feature levels together with the corresponding RAMSES Logic and RamsesComposer versions making them available.

Feature Level | RAMSES Logic Version | RamsesComposer Version | Feature 
-|-|-|-
1 | <=1.0.3 | <= 1.2.0 | Base Features
2 | 1.1.0 |  1.3.0 | - added AnchorPoint type <br> - linkable RenderPass properties <br> - added `enabled` Node property <br> - frustum planes for PerspectiveCameras
3 | 1.2.0 | 1.5.0 | dynamic control of render order via links in RenderLayer `renderableTags` properties
4 | 1.3.0 | 1.6.0 | added Skin type
5 | 1.4.0 | 1.6.0 | added support for LuaModules in LuaInterfaces
5 | 1.4.0 | 1.9.0 | linkable `instanceCount` MeshNode 



