<!--
SPDX-License-Identifier: MPL-2.0

This file is part of Ramses Composer
(see https://github.com/bmwcarit/ramses-composer).

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
-->
# Ramses Composer - Change Log

<!--- Template for next release section
## [unreleased]
* **File version number has changed. Files saved with RaCo X.Y.Z cannot be opened by previous versions.**
* **Export now supports ramses-logic feature levels up to U. Scenes exported with feature level U can't be opened with ramses-logic before vX.Y.Z.**

### Known Issues

### Added

### Changes

### Fixes

-->

## [1.10.0] Multiedit, Shader Includes, Drag-and-drop Support, Misc Usability Improvements and Bugfixes

### Added
* Added support for the multiple edit functionality in the property browser.
* Added support for `#include` directives in shader files. Include directives can be nested. The path of the included files must be relative and will be interpreted relative to the path of the shader file containing the include.
* Added support for opening `.rca` files using drag-and-drop.
* Added drag-and-drop support for resource files (`.gltf`, `.glb`, `.ctm`, `.png`, `.vert`, `.frag`, `.geom`, `.def`, `.glsl`). Dropping a resource file on a URI property will fill the property. Dropping a resource file in the treeviews will create a new resource object.
* Added application preference to optionally enable the case-sensitive URI validation. The default is off.
* Added functionality to save screenshots of the preview using the GUI application and exposed via the Python API. 
* Added `resolveUriPropertyToAbsolutePath` function to the Python API to obtain the absolute path from a uri property.
* Added a feature level update python script and updated the python migration scripts in the `python` subdirectory.
    * Use the `migrate_recursive_v1.0.1.py` script to update projects from RaCo version <=1.0.1 to the current version. Keeps feature level.
    * Use the `migrate_recursive.py` script to update projects from RaCo version >=1.1.0 to the current version. Keeps feature level.
    * Use the `upgrade_fl_recursive.py` script to perform a feature level upgrade to a specified feature level. Also updates the projects to the current RaCo version.
    * All scripts will update the main project and all external projects used by it.
    * Detailed usage information is contained in the scripts and can be obtained by running the script with the Python interpreter directly.
* Added section on feature levels to the `Basics/Introduction` chapter of the documentation.

### Changes
* Top-level objects in the scenegraph are now moveable.
* The folding state of non-locked property browsers is preserved and saved in a cache per session for all expandable properties.
* Added a new confirmation dialog to prevent accidental file version upgrade on Save. The appearance of this dialog is controlled via a new option in the preferences dialog.
* Adjust the behaviour of the `-f` commandline option in the GUI application to ensure that projects specified on the commandline will load regardless of the feature level set in the preferences dialog. The behaviour is now as follows:
    * default feature level for new projects
        * set via the preferences dialog in the GUI application
        * set via the `-f` commdandline option in the headless application
    * initial project load feature level
        * set via the `-f` commdandline option in both gui and headless applications.
        * if specified the initial project given using the `-p` option will be upgraded to the given feature level.
        * if no initial project is specified the initial default project will be created with the given feature level.
        * if no `-f` option is used the initial project given the `-p` option will be loaded at the feature level of the project itself.

### Fixes
* Don't optimize away `LuaInterfaces` with invalid links ending on them when exporting a scene. Note that invalid links ending on non-existing properties will not have associated error items indicating an invalid link.


## [1.9.1] Bugfix Release

### Fixes
* Prevent stencil and scissor options in materials and meshnodes from being reset to their defaults by undo/redo operations.


## [1.9.0] Stencil & Scissor support, Linkable instance count, Texture optimization, Misc Bugfixes
* **File version number has changed. Files saved with RaCo 1.9.0 cannot be opened by previous versions.**

### Known Issues
* Setting a MeshNode `instanceCount` property to a negative value via a link will lead to a runtime crash in Ramses.

### Added
* Added support for stencil testing via new stencil-related properties in the `Material` and `MeshNode` options.
* Added support for scissor testing via new properties in the options container of the `Material` and `MeshNode` types.
* Added color write mask suport in the `Material` and `MeshNode` options.
 
### Changes
* Added support for direct list/tuple assignment to properties of vector type in Python API.
* Available at feature level 5: made the `instanceCount` property of `MeshNodes` linkable. 
* Optimize the texture format in Ramses for normal `Texture` objects. No duplicate color channels or channels with constant values are created anymore. This removes warnings about creating empty channels.
* Do not display warnings for unlinked interfaces in Export dialog

### Fixes
* Don't allow to create links between Vec2f/etc properties and struct properties with the same child properties since they can't be created in the LogicEngine.
* Fixed the potential loss of struct uniform values in `MeshNodes` when loading a file created with RamsesComposer V1.7.0 or earlier with V1.8.0. 
* Fix preview scaling to make the scale equal to the device pixel over scene size ratio where the scene size is given by the `Display Size` property in the `ProjectSettings` object.


## [1.8.0] Free Tagging System, Lua Logging, Linkable Struct Uniforms, Misc Bugfixes
* **File version number has changed. Files saved with RaCo 1.8.0 cannot be opened by previous versions.**

### Added
* Introduced free tagging system by adding a `userTags` property to all object types. 
    * The user tags are independent of the normal `tags` property controlling the render setup.
    * They can be accessed in the Python API using the new `getUserTags`/`setUserTags` functions analogous to the normal `setTags`/`getTags` functions.
    * Added filtering by user tags to the tree views.
* A facility generating custom log messages from LuaScripts, LuaInterfaces, and LuaScriptModules using the `rl_logInfo`, `rl_logWarn`, and `rl_logError` functions has been added.
    * Calling these functions will result in a log message being generated by the LogicEngine with the string argument of the function as message. 
    * The logging facility is enabled in normal operation in RamsesComposer but will be disabled during export. This may lead to export failing if the logging functions are called.
* Added full support for linkable struct uniforms. Instead of a flattened property structure for struct and array of struct uniforms the application now builds the same recursive property structure as for LuaScripts. This makes uniforms structs linkable as a whole.
* Added support for the lua saving mode in the export dialog and the Python API. The lua saving mode can be also specified using the '-s' command line option in the headless application.

### Fixes
* "Save As with new ID" will now change all object IDs in the saved project. It is therefore possible to use objects from both the original and the copy as external references in the same project. The `save` function of the Python API had to remove the 'setNewID' flag and doesn't support "Save As with new ID" anymore.
* Fixed missing link validity update for links starting at external objects and ending on local objects when the set of properties of the starting object has changed in the external project.
* Fixed category names for BlitPass and RenderBufferMS types in the resource view.
* Fixed crash when moving multiple interlinked objects out of a Prefab.
* Fixed Ctrl+C shortcut to consistently copy the selection from the Python Runner and the Property Browser. Removed Copy and Paste commands from Main Menu, because they are now pane-specific.


## [1.7.0] External Textures
* **File version number has changed. Files saved with RaCo 1.7.0 cannot be opened by previous versions.**

### Added
* Added new TextureExternal type and support for the corresponding `samplerExternalOES` uniform type.
    * To use `samplerExternalOES` uniforms the shader needs to enable the extension for it with `#extension GL_OES_EGL_image_external_essl3 : require`.
    * This can be used to create textures which may be connected at runtime to external buffers also created at runtime. Since no external buffers can be set up in RamsesComposer these will be rendered in black in the preview.

### Changes
* Update ramses-logic from 1.4.1 to 1.4.2.
* Update ramses from 27.0.128 to 27.0.130.

## [1.6.0] Skinning, Morphing, property copy/paste, modules for Lua interfaces, misc bugfixes

* **File version number has changed. Files saved with RaCo 1.6.0 cannot be opened by previous versions.**
* **Export now supports ramses-logic feature levels up to 5. Scenes exported with feature level 4 can't be opened with ramses-logic before v1.3.0. Scenes exported with feature level 5 can't be opened with ramses-logic before v1.4.1.**
* **Starting at feature level 5 `modules()` statements in LuaInterfaces are not silently ignored anymore. Upgrading a scene to feature level 5 may therefore result in errors appearing in the scene which can be remedied by removing the offending `modules()` statements or supplying a module as needed.**

### Known Issues
* Animations of rotations are currently only working correctly for single-axis rotations due to an issue in the LogicEngine.

### Added
* Added support for vertex skinning and import of skinned meshes from glTF files.
* Added basic support for morphing: morph target position and normal mesh vertex attributes and morph weight animations will now be imported from glTF files.
* Added Copy/Paste functionality to all properties, accessible through their right-click context menu.
* Available at feature level 5: Added support for lua modules used in lua interfaces.
* Added indicator for nodes visibility to SceneGraph and Prefabs view.
* Added release_headless build folder with Ramses Composer Headless. It can be shipped and run independently from Ramses Composer Editor.
* Added "MSAA off" option to Ramses preview window and remove "MSAA 1x" option. The MSAA off option uses nearest neighbor filtering.

### Changes
* Update ramses-logic from 1.3.0 to 1.4.1.
* Update ramses from 27.0.126 to 27.0.128.

### Fixes
* Fixed crash in GLTF import dialog when importing with some items deselected and then trying to import again from the same file.
* Fixed crash on startup in case oldest log file is locked by a running process.
* Restrict feature level to the maximum supported one in the GUI application on startup to avoid crash when the feature level in the ini file is too high.
* Made error message in RenderTarget with mixed multi-sampled and normal buffers more descriptive.
* Prevent Recent Files menu from force downloading OneDrive files.
* Consistently show "Export with errors" in export dialog when RaCoHeadless would not export the scene.
* Fixed broken coloring in Python runner log.
* Consistently use relative paths in the created resource uris when importing from glTF files.
* Fix file not found errors when encountering files with links in their paths. This removes the URI case-sensitivity on Windows systems.
* Fix TracePlayer unintended retention of Lua properties values between trace loads.
* The glTF mesh import has been fixed to correctly transform and normalize the normal vectors when baking all meshes in the glTF file into a single mesh object.

## [1.5.1] Bugfix release

### Changes
* Import dialog items are split into Scene Graph and Resources categories.

### Fixes
* Control of render order by manually changing a renderableTags child property of a RenderLayer now works correctly.
* Make error color in Python runner readable.
* Ignore RenderBufferMS-related errors when exporting as a workaround for a Ramses bug.


## [1.5.0] Multisampling, dynamic render order control, Python API improvements

* **File version number has changed. Files saved with RaCo 1.5.0 cannot be opened by previous versions.**
* **Export now supports ramses-logic feature levels up to 3. Scenes exported with feature level 3 can't be opened with ramses-logic before v1.2.0.**

### Added
* Available at feature level 3: Dynamic control of render order by making the individual `renderableTags` properties for each tag linkable.
* Python API changes
    * Added `get/setTags`, `get/setMaterialFilterTags`, and `get/setRenderableTags` functions to access the `tags`, `materialFilterTags`, and `renderableTags` properties en bloc.
    * The `renderableTags` property in `RenderLayer` objects is now visible from Python to allow linking it.
	* Added `addExternalProject(path)` to add a project as external reference.
	* Added `addExternalReferences(path, type)` to import all objects of a certain type as external references. Type can be either a single string or a list of strings to import multiple types at once. Returns the imported objects.
* Multisampling support
  * Added MSAA option to Ramses preview window.
  * Added RenderBufferMS usertype that handles Ramses TextureSampler2DMS objects.
  * Added Material support for shader uniform type sampler2DMS.
  * Added new example shaders multisampler.(frag/vert).
* Added BlitPass usertype.
* Added support for array uniforms in shaders. Only simple types but not struct are supported as array elements.

### Changes
* Update ramses-logic from 1.1.0 to 1.3.0.
* Update ramses from 27.0.121 to 27.0.126.
* Items in the Resource View are now grouped by their type.
* If exporting in headless fails, the errors causing that failure are now always logged into the console output.
* Upgrade Feature Level menu moved down below Save As...
* TracePlayer now throws an error together with the trace line number, timestamp and the step at which error occurs.
* "Save As with new ID" added.

### Fixes
* Removed double scrollbar in Project Settings.
* Fixed wrong Feature Level number in RCA file after upgrade.
* Fixed misleading error message sometimes appearing for empty texture uniforms.


## [1.4.0] Python API enhancements, various usability improvements and bugfixes

### Added
* Added better support for feature levels in the GUI application
    * Default feature level for new projects can now be set via the preferences dialog.
    * Added "File/Upgrade Feature Level" submenu to upgrade the currently active project to a higher feature level. This will first save the project. Afterwards it will load the project again while upgrading to the desired feature level.
    * Loading a project which uses an external project with a higher feature level will show a dialog allowing the user to upgrade the feature level of the loaded project to the feature level of the external project.
* Added mesh metadata display for unbaked Meshes, e.g. gltf-extras values (currently string values only)
* Python API improvements
    * Added feature level parameters to functions `reset` and `load`.
    * Added `keys()` member function to objects and properties.
    * Added property access via indexing operator `[]` for objects and properties.
    * Added `hasSubstructure()` member function for properties.
    * Added more checking of error conditions.
    * Added `getInstanceById(id)` function to get a specific instance.
    * Added `isRunningInUi()` function to query if RaCo is running in gui application.
    * Added `minFeatureLevel()` and `maxFeatureLevel()` functions to determine currently supported feature levels.
    * Added `isReadOnly()`, `isExternalReference()` and `isResource()` member functions to objects.
    * Added `getPrefab()`, `getPrefabInstance()` and `getOuterContainingPrefabInstance()` member functions to objects. These return either None or the respective prefab/prefabInstance.
    * Added `isReadOnly()`, `isValidLinkStart()` and `isValidLinkEnd()` member functions for properties.
    * Added `getErrors()` function to read active errors as a list of ErrorItem.
    * Added `metadata()` member function to get any mesh metadata for a mesh object.
    * Added `raco_gui` python module, which is only available when running the composer in the GUI and needs to be imported separately.
    * Added `getCurrentSelection()` function to the `raco_gui` module, returning a list of the currently selected editor objects.
    * Added `importGLTF(path, parent)` to import a whole glTF file into the scene. `path` can be either relative to `<projectFolder>/meshes/` or absolute, `parent` argument is optional. If not provided, the object will be attached to the scene root.
    * Added `-y` command line option for both headless and GUI application to set additional python module search paths. Multiple `-y` options may be specified to add multiple directories. The additional paths are added before the default search path.

### Changes
* Made URIs case-sensitive on Windows systems.
* Made Export Dialogue less confusing
  * Removed Export Path Field.
  * Added buttons to select export target files.
  * Export Paths now use relative paths to increase readability.
  * Split the export summary to make it easier to differentiate between Scene Graph items and Resources.
* Errors and Warnings of externally referenced projects are now highlighted in Error View
    * Their visibility can now be toggled on and off.
    * Added a context menu option to directly open the referenced project.
* Changed Python runner in UI from a dialog to a dock with added output functionality.
* Error messages regarding external references now contain more information.
* Object Name tooltips now display the name an object will have after export.
* When importing resources, Git LFS Placeholder files are now detected and yield a helpful error message.
* Retain tree view item name during an in-place edit until user changes it.

### Fixes
* Fixed Lua Runtime errors not being displayed within Error View in nested prefabs.
* Realigned LuaInterface color picker/property expansion behavior with LuaScript behavior.
* Fixed Linux linker errors when compiling with a higher gcc version than 7.5 (i.e. gcc-9 in Ubuntu 20.04 or gcc-11 in Ubuntu 22.04).
* Fixed wrong glTF scenegraph getting shown when trying to import scenegraphs from .gltf files that have not been yet used in the project.
* Changed naming of logfiles: create logfile names based on process id and remove the oldest ones based on modification time.
* Added error item for inconsistent blend option settings in Material and private material of Meshnodes.
* Optimize link removal to speed up saving of large projects.
* Fixed a crash when pasting text from the clipboard without having anything selected.
* Fixed preview "best fit" scaling to avoid showing scrollbar for all scene and preview dock size combinations.
* The origin of the displayed coordinates when mousing over the preview dock is now in the lower left.


## [1.3.0] Ramses Logic feature level 2 support, relinking, search function, UI improvements

* **File version number has changed. Files saved with RaCo 1.3.0 cannot be opened by previous versions.**
* **Export now supports ramses-logic feature levels 1 and 2. Scenes exported with feature level 2 can't be opened with ramses-logic before v1.1.0 or with ramses-logic using feature level 1.**

### Added
* Added support for LogicEngine feature levels
    * The current feature level of a project can be seen in the ProjectSettings object.
    * An application feature level can be set using the '-f' command line option in both the GUI and the headless application. 
    * Creating a new project will use the application feature level. If the application feature level is set to -1 the maximum supported feature level is used.
    * Loading a project will set the current feature level to the project feature level if the application feature level is set to -1. If the application feature level is >=1 the project will be upgraded to the application feature level on load. Downgrading to a lower feature level is not possible.
    * Loading a project will always upgrade all externally referenced project to the feature level of the loaded project.
* Added support for the following features available at LogicEngine feature level 2
    * Added new linkable `enabled` property to Nodes.
    * Allow using planes for the frustum of perspective cameras. Switching is performed using a new `frustumType` property in the PerspectiveCamera. The properties in the frustum container of the camera will be changed accordingly.
    * RenderPass property changes
        * Added a new linkable `renderOnce` property.
        * Renamed `order` to `renderOrder` to match the LogicEngine property name.
        * Made the `enabled`, `renderOrder`, and `clearColor` properties linkable.
    * Added the AnchorPoint user type.
        * Anchor points are LogicEngine objects calculating the viewport coordinates and the depth of a node origin as seen by a camera.
        * Anchor point objects can't be used as external references.
        * See LogicEngine documentation for details.
* Added improved reference editor. The new reference editor follows the same functionality as the link start point editor.
	* Copied objects that are able to be used as references can be set as such a reference by right-clicking on the reference text box in the Property Editor.
* Added tree view object filter ability.
	* Search words can be separated by space to combine multiple searches.
* Added in-place renaming option for tree views.
* Automatically fill the shader paths when the first path is set following a simple naming convention based on file extensions. Two naming conventions are supported
    * Extensions *.vert, *.geom, *.frag, *.def
    * Extensions *.vert.glsl, *.geom.glsl, *.frag.glsl, *.def

### Changes
* Allow relinking of external projects during load when a project file is missing. 
    * A warning message is shown and the user can choose to supply a new project file to load instead of the missing project.
    * Only the loading process if affected, neither the main nor any external project are saved automatically.
    * Due to the removal of redundant information during file save the new project path will only be saved if the external project is directly used by the current project.

### Fixes
* Fixed the ~11 seconds freezing in RaCo Editor, under Linux with Nvidia in performance mode, when the Preview dock is resized.
* Fix trace player to update the outputs of LuaScript objects correctly when locking objects.


## [1.2.0] Trace player, weak links, and running Python script in GUI application
* **File version number has changed. Files saved with RaCo 1.1.2 cannot be opened by previous versions.**

### Added
* Added ability to run Python scripts in the UI version.
    * Use the new command line argument "-r" to run Python scripts before the UI appears.
    * During UI usage, you can run Python scripts using the new dialog "File" -> "Run Script..."
        * Note that Python functions "load()" and "reset()" have been temporarily disabled for this particular use case as they are currently incompatible with scene switches in the UI.
* Added TracePlayer
    * TracePlayer is used for simulation and debugging purposes, where a recorded, or manually defined, animation scenario in a RaCo trace file (.rctrace) can be loaded and played in the scene.
    * rctrace files are JSON based files that should contain so called frames of scene properties data in consecutive timestamps snapshots.
    * TracePlayer parses frames periodically based on RaCo application update time and updates the corresponding LuaScript or LuaInterface object(s) in the scene.
    * For more details with an example, see the TracePlayer section in the Ramses Composer documentation.
* Added support for weak links. Weak links are ignored in the LogicEngine Lua dependency graph which determines the Lua execution order.
    * Weak links can be created in the UI when a loop would be created with a strong link.
    * Strong/weak links are indicated in the property browser by double/single left arrows next to the link menu button.
    * The Python API addLink received an additional argument to allow creating weak links.
* Added "all file" filter for file dialogs that load files (e.g. URIs).

### Changes
* Changed "ramses-framework-arguments" ("-r") command line argument to "framework-arguments" ("-a").
* Icons in read-only state (e.g. link icons of properties in the Property Browser) will retain their color now.
* Removed lua script time_ms hack.

### Fixes
* Fixed incorrect update of PrefabInstance contents when loading a scene with a camera inside a PrefabInstance referenced by a RenderPass.


## [1.1.2] Lua Interface Naming Bugfix

### Fixes
* Change names of LuaInterface objects in the LogicEngine to match the conventions used for LuaScripts. In particular
    * LuaInterface objects outside any PrefabInstance use the LuaInterface name in the LogicEngine.
    * LuaInterfaces which are direct children of a non-nested PrefabInstance are named `<PrefabInstance name>.<LuaInterface name>`
    * All other LuaInterfaces have the object ID of the LuaInterface object appended `<LuaInterface name>-<LuaInterface object ID>`.


## [1.1.1] Lua Interface, Timer, and Animation Bugfixes

### Fixes
* Fixed `migrate_recursive.py` python script to avoid early exit in projects whose external projects graph has multiple paths to some external project.
* Fixed warning about unlinked LuaInterface outputs when exporting by optimizing away LuaInterface objects in the LogicEngine which are unnecessary.
* Fixed crash when attempting to create a link between an empty LuaInterface and an empty LuaScript, both with empty uri.
* Prevent creation of link loops after removing link when multiple links between the same objects are present.
* Fixed links starting on Animation output properties not working in LogicEngine.
* Renamed Animation "animationOutputs" property to "outputs" for consistency with LuaScript and Timer property naming.
* Fixed Timer objects being deleted all the time by "Delete Unused Resources" despite potential links to and from the Timer.



## [1.1.0] Lua Interfaces, various UI improvements
* **File version number has changed. Files saved with RaCo 1.1.0 cannot be opened by previous versions.**
* **Check the suggested migration procedure below for the LuaInterfaces introduced in RamsesComposer 1.1.0 to avoid unnecessary issues.**

### Added
* Added LuaInterface objects
    * LuaInterfaces are created from an interface script similar to the Lua scripts but only containing an interface function and only defining input but not output properties. Interfaces can't use modules either. See the LogicEngine documentation for further details.
    * The input properties of LuaInterfaces can be both starting end ending points of links.
    * Interfaces do not have a run function and no Lua code is evaluated each frame.
    * LuaInterfaces replace the LuaScripts as interfaces for Prefabs. LuaScripts inside PrefabInstances are now completely readonly, i.e. their input property can't be changed or have links ending on them anymore.
    * Migration of Prefab/PrefabInstance interface LuaScripts to LuaInterfaces is performed automatically when loading an old project: new LuaInterface objects are generated for each interface LuaScript and the interface scripts files are generated in an `interfaces/` subdirectory of the project directory. Furthermore the links ending on the interface LuaScripts are rerouted through the newly created LuaInterfaces leading to the same behaviour as before.
    * LuaInterfaces are also allowed outside Prefabs/PrefabInstances.
    * **WARNING** The migration code will generate and write the interface scripts each time it is run. If a file is loaded repeatedly without saving it the interface files will be overwritten each time. To avoid running the migration code repeatedly make sure to save the scenes after loading. A python migration script included  in the release automates this process, see below.
* **Suggested migration procedure**
  * Open main scene, make sure there are no errors, in particular no errors relating to scripts (including missing modules). Make sure all scenes have been migrated to RaCo 1.0.x.
    * If you do not ensure that the scripts have no errors, the LuaInterface objects generated for the scripts will not work as expected!
  * Use the Python migration script on the main scene, e. g. `\bin\RelWithDebInfo\RaCoHeadless.exe -r \python\migrate_recursive.py -l 3 -p .rca
    * Make sure to include the "-l 3" otherwise the console will be full with irrelevant info/debug/trace messages.
    * The script will perform the migration by loading and saving the main scene and all external scenes used by it.
    * The automatic migration might fail if multiple main scenes in the same directory are migrated like this since there might be conflicting versions of the new interface files being generated and written to the same file. This should not happen if there is only a single project file per directory.
  * Open the scene in RaCo again and verify the scene.
  * The generated interface files are now safe to be edited and will not be overwritten on subsequent loads of the project.
  * Make sure the C++ coders loading and using the scene change their code to search for LuaInterface objects instead of LuaScript objects, if the object they try to find is part of a prefab instance.
  * (optional optimization) Strip all simple OUT=IN assignments from the scripts serving as prefab interfaces and link those properties directly to the LuaInterface objects. This can decrease the CPU load of your scene on the target significantly due to the link optimization (see below).
* Added link optimization when exporting: links chains that have only LuaInterfaces in the middle of the chain will be replaced by single link from the chain start to the chain end property.
* Added Ramses log output to RaCo log output.
	* Ramses trace-level log output has to be explicitly activated in the RaCo Editor with the command argument "-t".
* Added custom Texture mipmap support, similar to CubeMap's mipmap support.
* Added tooltips for RaCo properties showing the RaCo-internal property name.
* Added more information to Texture and CubeMap information boxes.
* Allow configuration of standard modules used for lua scripts and modules via a new stdModules property in LuaScript and LuaScriptModule types.
* Added context menu item in the Treeview and Project Browser to open the external project(s) of the selected objects in a separate RamsesComposer instance.
* Added keybord shortcuts for the toolbar menus of the main window.
* Added "u_Resolution" as builtin shader uniform in addition to "u_resolution".

### Changes
* Update from ramses-logic 1.0.2 to 1.1.0
* Update from ramses 27.0.119 to 27.0.121
* Disallowed referencing objects inside Prefabs from objects outside of the same Prefab.
* Disallowed moving top-level objects onto top-level places inside the same objects tree view.
* Extended Texture & Cubemap information to show color channel flow from origin file to shader channels.
* Texture format down-conversion message has been downgraded from warning to information level.
* Texture format up-conversion warning message has been declared as deprecated as it will be upgraded to an error soon.
* Texture format conversion message from palette to any 8-bit format has been downgraded from warning to information level.
* Add python script path as first element of `sys.argv` during script execution with RaCoHeadless.
* Renamed some Node/MeshNode/PerspectiveCamera/OrthographicCamera/PrefabInstance, Timer and LuaScript/LuaInterface properties so they are closer to their ramses-logic counterparts.
	* Attention: This affects Python scripts that directly accesses properties by name.
* Added `projectPath` function to Python API to query the path of the current project.

### Fixes
* Fixed crash when switching scenes after an Export Dialog that shows scene errors has been closed.
* Fixed repeated Ramses DisplayDispatcher "no renderer events" message appearing in log.
* Fixed texture not being loaded when using RG8 mode on grayscale pictures.
* Raise python exception if Python API `load` is called with an empty string as filename argument.
* Fixed crash when saving files with relative paths using Python API.
* Add missing TextureFormat enum to Python API.
* Fixed greyscale PNGs not looking correct when texture format RG8 is selected.
* Fixed goto button not working when the referenced object is not selectable in the scene graphs of the current layout.
* Fixed logging of error messages for projects loaded in the RaCoHeadless application to include errors generated by the Ramses/RamsesLogic engine interface code.
* Fixed link removal in Python API to fail when attempting to remove links ending on read-only objects, e.g. external reference objects or PrefabInstance contents.
* Removed spurious "discard invalid link" warning with empty start and/or endpoints when pasting as external reference.
* Fixed crash when moving Nodes to root level in scenegraph.


## [1.0.1] Fix export differences between RaCoHeadless and Ramses Composer GUI

### Fixes
* Ensure correct export in RaCoHeadless application by flushing the ramses scene before export.
* Remove warning for "time_ms" Lua input parameter.

## [1.0.0] ramses-logic 1.x, Python API, timer, new animations
* **File version number has changed. Files saved with RaCo 1.0.0 cannot be opened by previous versions.**
    * rca files saved with this version will be much smaller than files saved with previous versions.
* **Export file format has changed. Scenes exported with RaCo 1.0.0 / ramses-logic 1.0.2 cannot be opened by ramses-logic versions 0.x.**
* **Breaking Change: ramses-logic 1.0.2 uses a different lua syntax for the type definitions and the interface() and run() function definitions.**
    * See the ramses-logic [CHANGELOG.md](https://github.com/bmwcarit/ramses-logic/blob/master/CHANGELOG.md) for details. 
    * A [migration script provided by ramses-logic](https://github.com/bmwcarit/ramses-logic/tree/master/tools/migrate) that converts the lua syntax is included as migrate_to_v1.0.py in the root folder of the RaCo package.
        * The migration script needs to be run twice - once for adding the  IN,OUT parameter to the interface()/run() functions, and once to update the name of the types.
        * Example for running the migration script in your root project folder on Windows (adjust paths as required):
            * `cd MyProjectFolder`
            * `python \RaCoInstallationFolder\migrate_to_v1.0.py inout .`
            * `python \RaCoInstallationFolder\migrate_to_v1.0.py types .`
        * You might need to install the [click Python module](https://pypi.org/project/click/) using `pip install click` to run the migration script.

### Added
* Added Python API and the ability to run python scripts using the RaCoHeadless application.
    * Added "-r" commandline option to RaCoHeadless application to run python scripts.
    * For a description of the Python API see the PythonAPI.md file.
* Added new Resource type Timer as an interface to the ramses-logic timer.
* Added support for more PNG image formats and Ramses texture formats.
    * Previously imported .pngs for Textures may need to be reconverted as the texture format detection has become more stringent.
    * Only supported PNG color types: R, RG, RGB, RGBA
        * Palette PNGs are loaded and converted for legacy reasons but will always cause a conversion warning.
    * 8-bit PNGs can only be used 8-bit for Ramses texture formats and vice versa with 16-bit PNGs.

### Changes
* Update from ramses-logic 0.15.0 to 1.0.2
* Update from ramses 27.0.116 to 27.0.119
* Nested arrays in LuaScripts are now supported. See the ramses-logic Changelog for details.
* Ramses-logic objects created in RaCo scenes will share the same ramses-logic user ID as the respective RaCo object.
* Animation objects have been simplified and now only have a 'progress' input property.
* The Error View in the dock now also shows errors by external references, and is now consistent with the "Composer Errors" tab in the export dialog. 
* Redundant data has been removed from the rca file - if you are using many prefab instances or external references, expect your files to shrink significantly.

### Fixes
* Always perform a logic engine update when loading a file to avoid inconsistent states when exporting with the headless application.
* Export with headless version will not write exported files anymore when the project can't be loaded.
* The RaCoHeadless application will now return a non-zero exit code if loading a project fails, the export fails or a python script throws an exception.
* Fixed RaCoHeadless crash when a scene extrefed by a loaded scene cannot be found.

### Known Issues
* It is currently not reliably possible to use venv/virtualenv with the RaCoHeadless Python API.

## [0.14.0]
* **Export file format has changed. Scenes exported with RaCo 0.14.0 / ramses-logic 0.15.0 cannot be opened by previous ramses-logic versions.**
* **Breaking change: Due to the update to ramses-logic 0.15.0 it is no longer possible to use variables without the "local" keyword or have global functions outside the "init" function. See the ramses-logic [CHANGELOG.md](https://github.com/bmwcarit/ramses-logic/blob/master/CHANGELOG.md) for details.**

### Added
* When dragging a property slider, you can now hold Shift to increase the magnitude of steps, and for float properties, you can also hold Alt to decrease the magnitude of steps.
* Added goto button for link start/end properties.
* Added tooltip for objects that are part of a PrefabInstance in the Reference drop-down selection.
* Added tooltip for the link button & link editor that shows the full scenegraph path of link property root objects.

### Changes
* Update from ramses-logic 0.14.2 to 0.15.0
* Update from ramses 27.0.115 to 27.0.116
* Added metadata to the exported .rlogic files. The metadata currently included is the generator application, the RamsesComposer version information, and the RamsesComposer file version number.
* The log view now displays a tooltip with the full multi-line message if the mouse is hovered over a message.
* Sort all objects and links in the project file to ensure a stable order when saving repeatedly.
* Change the displayed name of the member properties of the camera viewport and frustum containers to match the internal property name to make it easier to create matching struct definitions in lua.
* Removed redundant information from the .rca files. External projects only indirectly used are removed from the external projects mapping in the saved file.

### Fixes
* Clicking the goto button of an Ext Ref object while having a Project Browser will now always lead to the actually referenced Ext Ref object instead of (sometimes) the object in the Project Browser.

## [0.13.1]
* **File version number has changed. Files saved with RaCo 0.13.1 cannot be opened by RaCo versions 0.12.x or earlier. **

### Known Issues
* RaCoHeadless will crash when trying to load a scene using external references from scenes which cannot be found.
	* The crash will be preceded by an error message like this "External reference update failed: Can't load external project '...' with path '....rca'"
	* If the same scene is opened in the Ramses Composer, the same error message will be displayed in a message box.
	* When this occurs, the best way forward is to restore the file in the stated location.



### Fixes
* Don't reset preview background color to black if the preview is resized or moved.
* Collapsed vector view for floats does round the displayed number again.
* Capturing the ramses-logic output revealed a ramses-logic error message during load caused by attempting to initialize scripts using modules which have not been loaded yet.
* The "Export" button could be disabled without explanation if external references caused entries in the error view.
* Material uniform textures which are unset now show an error message, since exporting them with ramses does not work.

* Fix losing input properties of interface LuaScripts in nested PrefabInstances with externally referenced Prefabs during load. This fix breaks the propagation of the values of new LuaScript interface properties in the Prefab update performed as part of external reference update during load. 
* Fixed Ramses Composer not being able to launch under certain multiple display arrangements on Linux.
* Fixed internal side-effect handling of LuaScript module property updates. 
* Under some circumstances RaCo attempted to delete Lua Modules in ramses-logic before the referencing Lua Scripts when the scene was closed, causing an error to be logged.

### Changes
* The "Export" button in the export dialog is no longer disabled if there are errors in the scene. Instead its label is changed to "Export (with errors)".
* The error view in the export dialog now shows all errors, including the ones caused by external references.


## [0.13.0] Compressed project files, cubemap extensions, log view
* **File version number has changed. Files saved with RaCo 0.13.0 cannot be opened by previous versions.**
* Version was superseded by 0.13.1 while still being a preliminary release.

### Known Issues
* RaCoHeadless will crash when trying to load a scene using external references from scenes which cannot be found.
	* The crash will be preceded by an error message like this "External reference update failed: Can't load external project '...' with path '....rca'"
	* If the same scene is opened in the Ramses Composer, the same error message will be displayed in a message box.
	* When this occurs, the best way forward is to restore the file in the stated location.

### Added
* Added custom CubeMap mipmap support.
* Added optional automatic zipping of project files.
    * With automatic zipping enabled, projects will be saved as ZIP archives that still use the .rca file extension and contain the project JSON file.
    * This option can be enabled/disabled in the Project Settings (disabled by default).
* Added command line argument "--outlogfile" to headless Ramses Composer for changing the log file path.
* New log view allows observing Ramses Composer log output within the application.
* The ramses logic datatype INT64 is now supported.
* Added object duplication feature via context menu or shortcut.

### Changes
* Removed upper limit for glTF mesh TEXCOORD and COLOR attributes.
* Export dialog now displays Ramses Composer scene errors if any are present.
* Ramses Logic log output now appears in Ramses Composer log files.
* Ramses Composer now begins a new log file every time the application is launched.
* The "Field of View" property in the perspective camera has been renamed to "Vert. Field of View".

### Fixes
* Fixed dropping of links from external reference to project local objects when loading a project.
* Fixed crash when dragging around "Scene Id" property value.
* Fixed problem with saving preferences if the entered directory does not exist.
* Improved support for High DPI screens.
* Fixed losing the property values of PrefabInstance interface scripts when pasting PrefabInstances.

## [0.12.0] Bug Fixes and Usability Improvements
* **File version number has changed. Files saved with RaCo 0.12.0 cannot be opened by previous versions.**

### Added
* Whenever a private Material is created, it will now always be created with the same Options as the shared Material.
* New collapsable list entry "External References" serves to group all external references together in the resources and prefab views.
* Added command line argument "--loglevel" to headless Ramses Composer for adjusting log verbosity.
* Added color picker for vector properties in the property browser.
* In the scene graph, prefab, project browser and property browser views, shift click on the arrow symbols will now recursively expand or collapse items.
* A new filtering options menu for the ramses preview widget now allows changing from nearest neighbor sampling to linear sampling.
* Added a ramses-logic-viewer build to RaCo binary folder.
* It is now possible to do simple calculations (like "1920/1080" as aspect ratio) directly in the number inputs of the property browser.
    * Currently allowed operations: addition (+), substraction (-), multiplication (*), division (/), modulo (%), exponentiation (^) and changing precedence using parentheses.
    * Results are calculated immediately and the mathematical expression is not stored.
* The property browser now indicates lua datatypes with tooltips on property labels and with a label in the link editor popup.

### Changes
* Update from ramses-logic 0.13.0 to ramses-logic 0.14.2
* Update from ramses 27.0.114 to 27.0.115
* The default resource directories can now be set in the per-project in the project settings instead of the Ramses Composer preferences.
* The button to open the underlying file for LuaScripts and other objects is now enabled in cases where the object itself cannot be edited (due to being an ExtRef or part of a PrefabInstance).
* Log file size is now limited to 10 MB, with new log files being created once this is exceeded. A maximum of 250 MB of log files can be created before old files get deleted.
* Opening the link editor no longer has the search field pre-filled with the name of the current link, if one exists.
* More details for LuaScriptModule errors in LuaScripts - invalid LuaScriptModule assignments are now also shown as individual errors.
* Removed "Debug">"Add dummy scene" menu element.
* Config and log files moved from program folder to user folder (e.g. %APPDATA%/RamsesComposer on Windows).
* Log file for headless Ramses Composer is now named "RaCoHeadless.log".
* The values of new lua input properties for LuaScripts which are direct children of PrefabInstances are now propagated from the corresponding Prefab LuaScript during external reference updates making it possible to set default values in the external project.
* Optimize simultaneous deletion of many links in scenes with many objects.
* Object IDs of the PrefabInstance children objects are now deterministically determined from the corresponding Prefab child and the PrefabInstance itself.

### Fixes
* The application now handles scenarios where saving configfiles is not possible more gracefully.
* Fixed problems loading projects from paths that contain non-latin characters.
* For empty LuaScript files the correct error message is now shown.
* Fixed Ramses API errors appearing in the log window during new project creation.
* Properties "Flip Vertically" and "Generate Mipmaps" in a texture are now updating the Ramses texture immediately.
* The ramses preview toolbar can no longer be hidden with right click, since this was an unintended feature.
* Removed non-functional "?"-Button from all dialog windows.
* Fixed MeshNodes in PrefabInstances having a different private Material uniform order than their Prefab counterparts after changing Material reference.
* Fixed RaCoHeadless not exporting links.

## [0.11.1] Interim Release - The Tangent Fix

### Added
* Added real-time update of file read-only status in the application title bar.

### Fixes
* Fixed tangents and bitangents being thrown away after glTF import.
* Assets in glTF files that do not specify meshes but are otherwise valid can be imported now.
    * A Mesh that loads a meshless glTF file will still show an error.
    * A baked Mesh that loads a glTF file with no nodes referencing meshes will show an error.
* Fixed Mesh not being properly displayed after getting baked, unbaked, then baked again.
* Fixed missing update of broken link errors by undo/redo in some cases.


## [0.11.0] Lua Modules
* **File version number has changed. Files saved with RaCo 0.11.0 cannot be opened by previous versions.**
* **Export file format has changed. Scenes exported with RaCo 0.11.0 / ramses-logic 0.13.0 cannot be opened by previous ramses-logic versions.**

### Added
* Added multi-selection for deleting, copying, cutting and pasting in Scene Graph, Resources, Prefabs and Project Browser.
* Added Lua module support.
    * The new user type LuaScriptModule is a resource that loads modules from specified Lua files.
    * LuaScripts have a new output entry "Modules" - for each module defined in the Lua script file, this entry will contain a reference drop-down where LuaScriptModules can be selected
    * Nested modules are currently not supported.
* Added support for generating mipmaps for a textures.
    * The texture object has a new option "Generate Mipmaps" which by default is off. If enabled, Ramses will auto-generate mipmaps for the texture.

### Changes
* Update from ramses-logic 0.12.0 to ramses-logic 0.13.0
    * Major performance improvement for large scenes with lots of links alongside few bugfixes
* Update from ramses 27.0.113 to 27.0.114

### Fixes
* Undo / Redo is now properly working for collapsed vector editors in the property browser.
* Fixed visual issue with number inputs in property editor not leaving highlighted state after pressing enter.
* Fixed undo/redo to prevent creation of valid links starting on non-existing properties.
* Fixed drop down boxes with "<empty>" reference in property browser causing a crash when being deactivated.

### Known Issues
* The INT64 type introduced in ramses-logic 0.13.0 is not supported yet.
* The property "timeRange" in Animations introduced in ramses-logic 0.13.0 is not supported yet.
* The new "TimerNode" introduced in ramses-logic 0.13.0 cannot be created in RaCo yet.


## [0.10.0] Animations
* **File version number has changed. Files saved with RaCo 0.10.0 cannot be opened by previous versions.**

### Added
* Added animation support.
    * Currently only animations defined in glTF files are able to be imported.
    * The first new user type AnimationChannel is a low-level data accessor for animation data, akin to the Mesh type.
    * The second new user type Animation is a scene graph object that groups AnimationChannels and contains the channels' linkable outputs.
    * Importing assets via the "Import glTF Assets" option will automatically create links from enabled Animations to the respective enabled Nodes.
* Added quaternion rotation for Nodes.
    * Quaternion rotation is activated by linking a vec4f output to the Rotation property of a Node. The rotation property will then show the converted Euler angles as calculated by ramses-logic.
* Texture and CubeMap resources now have boxes in the property browser describing their size and channel format.
* Added option to set alpha component of display background clear color in project settings.

### Changes
* The property browser now supports localized thousand and decimal seperators for number input (i.e. comma as decimal seperator on German language systems).
* Project files which are not writable are now marked as read-only in the application title.
* Number sliders in the property browser now lock the cursor in place while dragging.
* LuaScript, Material and MeshNode objects now have some of their properties collapsed by default in the property browser. 
* Improved performance of undo/redo of link creation/deletion.

### Fixes
* Improved keyboard navigation in property browser using Tab, Shift-Tab, Esc and Enter.
* If a file cannot be saved (e. g. because it is read-only), the save no longer silently fails but instead displays an appropriate error message.
* An error is now shown for MeshNodes where the material requires an attribute that the mesh data does not provide.
* Fix crash when performing undo after changing lua script file.

## [0.9.3] Update to ramses-logic 0.12.0

### Changes
* Update from ramses-logic 0.11.0 to ramses-logic 0.12.0
    * BREAKING CHANGE: It is no longer possible to use global variables in the Lua Scripts functions.
        * There are now better alternatives:
            * [init() function](https://ramses-logic.readthedocs.io/en/v0.11.0/lua_syntax.html#global-variables-and-the-init-function)
            * [Custom modules](https://ramses-logic.readthedocs.io/en/v0.11.0/lua_syntax.html#custom-modules)
            * Docs for new behavior: see [Lua docs](https://ramses-logic.readthedocs.io/en/v0.12.0/lua_syntax.html#environments-and-isolation)

## [0.9.2]

### Changes
* Update from ramses-logic 0.10.2 to ramses-logic 0.11.0
    * ramses-logic 0.11.0 introduces a new "init()" function for global variables. For details see ramses-logic CHANGELOG/docs.
* Update from ramses 27.0.112 to ramses 27.0.113
* Windows and Linux builds no longer contain debug symbols, reducing the size of the zip files significantly.
* For now remove button "Check Resources Used by Enabled Nodes" from the "Import glTF Assets..." dialog to avoid confusion.
* Reshuffling of library code: 
    * libSerialization is now part of libCore,
    * most annotations in libDataStorage have been moved to libCore,
    * migration code for loading files saved with older versions has been moved from libComponents to libCore.

### Fixes
* Fixed clear flags warning on export for render passes rendering to the default framebuffer.
* Fixed typo in options for "Render Order" property.
* Fixed typo in options for "Material Filter Behaviour" property.
* Suppress logging of misleading error messages to the console during load.

## [0.9.1] Render pipeline bug fixes and minor changes, update ramses-logic to 0.10.2

### Changes
* **File version number has changed. Files saved with RaCo 0.9.1 cannot be opened by previous versions.**
* Update from ramses-logic 0.9.1 to ramses-logic 0.10.2
    * This restores the behaviour of ramses-logic 0.7 (interface method can access things outside its scope). This is a temporary revert and will be reintroduced in an upcoming version again, along other hardening measures!
* Update from ramses 27.0.111 to ramses 27.0.112
* "Import glTF assets..." can no longer import into the root of the scene.
    * This is to avoid the need to tag the imported scene to make it visible.
    * The menu point has been removed from the "File" main menu as well as the context menu for the root.
    * It is now only displayed for object tree entries in which nodes can be created.
* A renderable referenced by a render layer, but not by a render pass now shows the directly referencing render layers in its property browser.
* Render layer property naming changes:
    * The "Sort order" property is now called "Render Order".
    * The options for the "Sort Order" property are now called "Render order value in 'Renderables Tags'" (default) and "Scene graph order".
    * The column header in the tag editor for 'Renderable Tags' specifying the render order is now called "Render Order".
    * The "Incl./Excl. Material Filter" property is now called "Material Filter Behaviour".
* "Optimized" was removed from the options in the "Render Order" render layer property.
    * It is not needed right now, and was unnecessarily confusing.
    * In existing scenes it is replaced with "Determined by render order value".
* The "Ok" and "Cancel" buttons in the tag dialog are now called "Ok" and "Cancel" instead of being displayed as a tick mark and a cross.

### Fixes
* A render pass rendering to the default framebuffer no longer marks its "Target" property in orange, and shows it as "Default Framebuffer" instead of "<empty>".
* A broken render target now stops a render pass rendering to it from rendering, instead of rendering to the default framebuffer.
    * Also: the render pass shows a warning that it won't render due to the broken render target.
* A render target now displays an error if some of its render buffers are either not valid or not set.
    * This is due to https://github.com/bmwcarit/ramses/issues/52
* A render target now displays an error if its first render buffer is not set.
* The sampling parameters are hidden for depth buffers.
* Fixed absolute paths for .ramses & .rlogic files not being recognized in the "Export Project" dialog.
* Disallow RenderPass objects being pasted as external references.
    * This is a temporary workaround and will be reintroduced in later versions.
* Detect and remove link duplicates in the project files when loading a project. Warning messages will be generated in the log if duplicates are detected.
* Fix bug leading to uniform properties referencing deleted texture objects. Invalid references encountered during loading of project files are discarded and a warning message is logged.


## [0.9.0]

### Added
* Added first simple support for offscreen render targets, render passes and render layers.
    * It is possible to add render passes, render targets, render buffers and render layers as individual objects.
    * Render passes contain references to the render layers which they render.
    * A render layer can contain other render layers, nodes and mesh nodes which are rendered when the render layer is rendered.
        * The contents of a render layer are determined via tags: the render layer has a property "Renderable tags" which can contain any number of tags.
        * Nodes, materials and render layers also have a property "Tags" which can contain any number of tags (also cameras, but those tags are currently unused).
        * If a tag in a nodes "Tags" property matches a tag in the render layers "Renderable tags" property, the node and all its children are added to the render layer.
        * If a tag in a render layers "Tags" property matches a tag in another render layers "Renderable tags" property, the former render layer is added to the latter render layer.
        * Each tag in the "Renderable tags" list has also an order index. 
          * All renderables in a render layer are rendered in the order given by the order index. 
          * If a renderable is added to a render layer more than once with different order indices, a scene warning is displayed.
        * The render layers also contain a property "Material Filter Tags" and a property "Incl./Excl. Material Filter". 
          * Those properties can be used to include or exclude mesh nodes based on the materials they use:
            * If "Incl./Excl. Material Filter" is set to "Include materials with any of the selected tags", the render layer renders only mesh nodes if they use a material whose "Tags" property contains a tag which matches any tag in the render layers "Material Filter Tags" property.
            * If "Incl./Excl. Material Filter" is set to "Exclude materials with any of the selected tags", the render layer renders only mesh nodes if they *do not* use a material whose "Tags" property contains a tag which matches any tag in the render layers "Material Filter Tags" property.
* Added error items for broken links ending on valid properties. Only one combined error item is created for each object.
* Added deselection in the UI - just click on an empty space in the Tree View
* Added dragging and dropping external objects as external reference - hold the Alt key while drag and dropping
* Limit number of entries in recent file menu to 10. Recent file menus longer than 10 entries will be truncated after loading a project.
* Added selective import of glTF Assets. Before importing a glTF file you will be able to select/deselect individual parts of the glTF scenegraph (right now only nodes & meshes).

### Changes
* Update from ramses-logic 0.7.0 to ramses-logic 0.9.1
* Update from ramses 27.0.105 to ramses 27.0.111
    * **!!!!!WARNING!!!!!** The new Ramses version has changed order of transformations (previously: first rotation, then scaling, then translation. Now first scaling, then rotation, then translation). This may affect nodes with non-zero rotations and non-uniform scaling.
* Improved copy-paste handling in the GUI.
    * The "Paste" context menu will now be disabled in the tree views depending on the types of objects about to be pasted.
    * Changed "Paste" context menu item in the Resource tree view to "Paste on Top Level".
    * Copy-pasting will prefer Scenegraph objects over Resources, e.g. a deep-copied MeshNode that references a Mesh will not be pasteable in the Resources tree view, vice versa with deep-copied PrefabInstances that reference a Prefab.
* The "U/V Origin" property for Textures has been replaced by a "Flip Vertically" flag.
    * Textures that had their origin set to "Top Left" in previous project versions will have the origin flip flag enabled.
* Importing meshes is now done by tinyGLTF instead of Assimp.
    * **!!!!!WARNING!!!!!** Textures that are based on CTM mesh UV coordinates may have to be explicitly flipped by hand.
    * **!!!!!WARNING!!!!!** Shaders that rely on UV coordinates may have to be recompensated due to Assimp flipping the V coordinate.
* Don't create engine objects for Prefabs contents. 
* Make the default resource subfolders configurable via the preferences.ini file and the preferences dialog.
* Removed DirectX fallback texture. All fallback textures will use the OpenGL-based image now.

### Fixes
* Paths in the recent file menu that are not accessible (either deleted file or no read access) will not be openable in the menu anymore.
* Project files that the user is not allowed to read will not be attempted to be loaded anymore.
* The "Copy" and "Paste" menu entry in the Edit menu is not enabled all the time anymore.
* Fixed "Paste" menu entry in the Edit menu not working anymore when the current layout contains no Tree Views.
* Prevent "paste as external reference" from creating duplicate links when pasting the same objects repeatedly.
* Prevent paste from sometimes dropping links incorrectly when pasting into existing object.
* Don't log error messages when trying to read empty files.
* "Create node" context menu item now generates only a single undo stack entry instead of separate "create" and "move" entries.
* Fix crash when pasting PrefabInstances containing LuaScripts which are based on external reference Prefabs. 
   * Caused by incorrect modification of the LuaScript URIs during paste.
   * Old files will contain incorrect URIs. These are repaired automatically during load and warnings will be logged to the console.
   * Projects which cause "Rewrite URI property..." warnings during load should be saved to avoid these warnings in the future.
* Fix external reference update to correctly handle simultaneous scenegraph move and node deletions in all situations.
* Fix prefab update to handle simultaneous scenegraph move and node deletion correctly.
* Reduce memory usage when creating undo stack entries in scenes with many links.

## [0.8.3]

### Changes
* Update from ramses-logic 0.6.2 to ramses-logic 0.7.0
* Locked Property Browsers whose item has been deleted, will become automatically unlocked.
* Dynamic LuaScript properties now follow the same alphabetical order as propagated by ramses-logic.
* Material uniforms are now sorted according to their chronological declaration appearance in the shader files.
* Added support for struct properties in the data model. Converted Camera viewport and frustum properties to structs. Allow the camera viewport and frustum structs to be linked as a whole.

### Fixes
* Deleting a Node will not affect locked Property Browsers anymore, except when the Property Browser was locked onto the now deleted Node.
* Closed docks do not get cached/saved anymore.
* The Property Browser does not get reset anymore when pressing the Delete key while a read-only object is selected.

## [0.8.2]

### Added
* Error View - a new dock window that lists all editable objects' and external reference update errors in the current scene, with easy go-to functionality (double click or context menu)
* The viewport background color can now be set from the project settings.
* The Linux build no longer requires a pre-installed Qt and can be started directly with shell scripts (`RamsesComposer.sh` and `RaCoHeadless.sh`).
* Added a default project subfolder structure.
    * Saving a project will now generate the following four subfolders in the project root folder: "images", "meshes", "scripts" and "shaders"
    * After loading a project, file browsers for new URIs will start at one of the previously mentioned subfolders, depending on what type the URI's root object has
    * The folder of the recently changed URI will be cached for next time an object of the same type needs an URI changed
    * When the URI file browser can't find the resource subfolder, it will start at the project root folder instead
    * The four default resource subfolders in the release folder have been moved from "resources" into "projects"
    * The file loader for saving/opening projects starts at the User Projects Directory, and will be cached at the folder of the last saved/opened project.
* Added support for shared materials which are now the default for newly created MeshNodes. The MeshNode now has a "Private Material" flag which can be used 
  to enable per-meshnode private materials reproducing the old behaviour. Loading of old projects will set the flag to private on import.

### Changes
* Update from ramses-logic 0.6.1 to ramses-logic 0.6.2
* Update from ramses 27.0.103 to ramses 27.0.105
* Improved performance of undo/redo, in particular when deleting large numbers of objects, and of prefab updates.
* Changed "Viewport" naming in Project Setting to "Display" naming.

### Fixes
* Changes to a geometry shader are now automatically applied even if the same material uses a "Defines URI".
* The "go to referenced object" button is now also enabled for read-only objects.
* Prevent MeshNodes from becoming invisible if they have children in the scenegraph.
* Resize of the preview no longer crashes in Linux (fix in Ramses 27.0.105)
* Identical Ramses Logic errors for an object do not get regenerated in the UI all the time anymore.
* Enabled portrait sizes for project display.
* Fixed runtime errors in scene not getting updated after deleting an object that contains a runtime error.


## [0.8.1]

### Added
* RamsesComposer can now be run on Ubuntu 18.0.4.

### Changes
* Update from ramses-logic 0.6.0 to ramses-logic 0.6.1
* Update from ramses 27.0.102 to ramses 27.0.103
* Removed RACO_CONVENTIONS.md - superseded with the Ramses Composer documentation repository: https://github.com/bmwcarit/ramses-composer-docs
* PrefabInstance interface scripts (LuaScripts on PrefabInstance top level) now get exported with the object name "<Prefab Instance name>.<LuaScript name>".
* The folder and library structure has been cleaned up.
* LuaScripts that have no parent are not displayed in the Resource tree anymore.
* LuaScripts can not be created via the Resource tree context menu anymore.
* Change classification of LuaScript objects to non-resource type object.
* Changed ramses node binding object name from _Binding to _NodeBinding suffix.
* Insert appearance and geometry binding objects as children of their MeshNode in the exported objects tree of the export dialog.
* CHANGELOG.MD can now be found in the root of the release folder, and the README.MD can now also be found there.
* The "Import Project" dialog now opens at the user projects directory (setting in "Preferences") instead of the last used path for URIs
* Added the C++ runtime libraries to the release/bin/RelWithDebInfo folder so the release version of RamsesComposer and RaCoHeadless can be run on a fresh system.
* Removed unneeded Qt libraries from the bin folder and added the LGPLv3 license.
* Visual Studio configurations have been reduced to Debug and RelWithDebInfo as those two are the only ones supported.

### Fixes
* Added camera bindings to list of exported objects shown in the export dialog.
* Fixed layouts not being restored when switching projects while layout.ini file is not present
* Shallow-copy-pasting a PrefabInstance without the Prefab will not contain children of the PrefabInstance anymore.

## [0.8.0]

### Added
* Added external reference feature allowing to use parts of other projects in the current project. The external references used are automatically updated when the origin project changes.
* Layout persistence - UI dock layout will be restored when restarting Ramses Composer or loading another project
* Custom layout saving - it is now possible to save, restore and delete the current UI dock layout via the menu bar -> "View" -> "Layouts"
* Support for links on frustrum and viewport camera properties (camera bindings).

### Changes
* Update from ramses-logic 0.5.2 to ramses-logic 0.6.0
* Update from ramses 27.0.100 to ramses 27.0.102
* Update from qt-Advanced-Docking-System commit ID f5433182 to 3.7.1
* Import of meshes with negative scales has been temporarily prohibited because of Assimp being too clever with glTF importing - see https://github.com/assimp/assimp/issues/3784
* LUA runtime error are now shown in the property browser of the script causing the error, not only in the console log.
  * All other LUA scripts show a warning in the property browser containing the name of the failing script.
* Change of RenderGroup approach: Entire scene is now one RenderGroup instead of having multiple one-to-one node RenderGroups; meshes are still rendered in Scene Graph order
* Disabled creating a Preview while another Preview is already open as a temporary crash prevention measure

### Fixes
* Fixed default mesh index, vertex data and effects still persisting in scene when no MeshNode is using them

### Known Bugs
* Undocking the Ramses Preview window, then inserting it in another dock may not properly dock the Ramses Preview window at first try - but at second try
* You may need to delete the "layout.ini" file in your "configfiles" folder to fully disable multiple preview window creation


## [0.7.3]

### Added
* Caching of links from/to outdated LuaScript and MeshNode uniform properties

### Changes
* Prefabs are now managed in their own prefab view rather than the scene graph

### Fixes
* Updates of nodes will not change around rendering order anymore
* Crash when removing a project file in the file browser while that project is loaded & expanded in the Project Browser has been fixed
* Links inside nested prefabs are no longer lost on reload or paste
* Removed unnecessary recalculation of rendering order
* Fixed Tree Views not properly updating when moving top-level nodes from one Tree View to another
* Added caching of values for struct members of outdated lua script properties.
* Fixed "<empty>" reference drop-down menu option not appearing in first place at all times, leading to false reference assignments
* Removing a link in a prefab will now correctly update the linked endpoint properties in the prefab instances.
* Fixed caching of lua script input properties inside prefab instances to work also when the corresponding prefab lua script breaks.


## [0.7.2]

### Added
* Prefab support.
 * Prefabs can be created in the Scenegraph
 * Top-level LUA scripts in Prefab act as interface to the outside 
 * Prefab instances (read-only) can be created, need to reference a Prefab
* Properties that reference another project object contain a new "select referenced object" button in the Property Browser
* added URI property for an optional file with shader defines (*.def) to Material
  * the file can contain the name of a compiler define in each line
  * the defines are injected into all three shaders before compiling them
  * empty lines and lines starting with "//" are allowed and ignored
* Path sanitation for URI properties (whitespaces get trimmed and slashes get normalized to forward slashes)
* Project Browser - It is now possible to load external Ramses Composer projects and show their scenegraph structure in the UI
* "Original fit" preview mode
* Linux support for headless version (experimental)
* Added support for tangent, bitangent and color attributes for meshes.
* Export dialog calls Ramses validation function and displays errors and warnings
* Empty references are highlighted in warning color
* On Dual GPU systems with Windows, the executable is now trying to force the High-End GPU (using NvOptimusEnablement / AmdPowerXpressRequestHighPerformance).

### Changes
* Update from ramses-logic 0.4.1 to ramses-logic 0.5.2
* Update from ramses 27.0.2 to ramses 27.0.100
* Update from Qt 5.15.0 to Qt 5.15.2
* Update to latest master of Assimp (for bugfix)
* Warning color in UI changed from yellow to orange to distinguish from selection
* Added icons to scene graph and resource view
* Read-only elements in scene graph are shown as disabled
* Comboboxes in property browser are no longer changed by the mousewheel
* Sliders show arrows on mouseover to decrement or increment their value

### Fixes
* Fixed crash when loading files with links when the lua files of the linked lua objects have been changed on disk since the last project save.
* All Ramses objects are assigned meaningful names
* Name changes are synced to Ramses immediately and can be exported.
* New scenegraph drag-drop exemption: dragging an object into itself or one of its children is now disallowed
* Semantic uniforms are no longer shown in property browser
* Edit Menu Undo/Redo action actually works now

### Known Bugs
* Ramses Composer UI version will not properly launch under Linux.


## [0.7.0]

### Added
* Support for glTF mesh selection & scenegraph import from gltf asset files.
* Propagation of mesh import error messages to Ramses Composer UI
* Assimp logger output in Ramses Composer logger output
* Textures with no valid URI assigned show a fallback test pattern
* Textures have adjustable origin for U/V coordinates
  * texture images are flipped vertically if required
  * default setting is compatible with blender default U/V handling
* CubeMaps without a valid set of images show a fallback test pattern
* Added error messages for CubeMap URIs
* RACO_CONVENTIONS.md - a Markdown file that explains Ramses Composer-internal conventions
* Unique naming mechanism for created & pasted objects
* Lexicographical sorting of Resource objects in Tree View and Property Browser

### Changes
* Added license headers for all source files and documents.

### Fixes
* Missing and invalid URIs for textures are marked in yellow and red.
* Reduced permanent output log spam when Ramses logic detects a runtime error.
* Fixed Shortcuts sometimes not working by setting Save, Undo and Redo shortcuts to application level context.
  * Widgets which have a local implementation will still use their own implementation instead of the application level behaviour. E.g. while a Line Edit is focused, undo and redo will only affect the local text changes inside the widget.
* Link editor dialog is always fully visible, never positioned partially off screen.
* Saving a new project will now correctly change relative URIs of Resources.
* Fixed crash in paste after deep copy for some complicated situations.


## [0.6.2]

### Added

* Using new Rotation Convention feature from ramses 27.0.2 with convention set to ramses::ERotationConvention::XYZ.
* In case of unhandled exceptions, a crash dump file (.dmp) is created in the executable's directory
* Lua arrays of structs are supported.
* New command line parameters for headless executable
  * -p for loading a project file
  * -e for exporting ramses / logic scene files
  * -c for compressing ramses scene on export
* Added standard parameters to both UI and headless executable
  * -h for usage help
  * -v for version
* CHANGELOG.md will be copied next to the executables on build.

### Changes

* Renamed ReleaseNotes.md to CHANGELOG.md (alignment with ramses convention)
* Update from ramses-logic 0.3 to 0.4.1
* Update from ramses 26.0.6 to ramses 27.0.2
* Enabled EditorObject's applying names to ramses::Resources
  * Exported ramses::Resources in scene's should now have matching names set in the Editor. 
  * This feature was previously disabled because of a bug in ramses 26.0.4.
  
### Fixes

* Defined a rendering order. As of this version the rendering order follows the order of scenegraph nodes.
* Indexing of Lua arrays starts from 1.
* Undo/redo of scenegraph moves is now correct.
* Linking for arrays of struct is now supported for both entire arrays and single elements.


## [0.6.1]

### Changes

* Adjustable viewport size for preview in settings
* Default project path is now relative to executable instead of in windows documents path
* Updated from Ramses 26.0.4 to Ramses 26.0.6.

### Fixes

* crashes if document folder or project folder were using network drives	
* crash in export preview for some scenes

## [0.6.0]

Initial release
