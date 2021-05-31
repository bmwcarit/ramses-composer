<!--
SPDX-License-Identifier: MPL-2.0

This file is part of Ramses Composer
(see https://github.com/GENIVI/ramses-composer).

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
-->
# Ramses Composer - Change Log

<!--- Template for next release section
## [unreleased]
### Added

### Changes

### Fixes

### Known Bugs
-->

## [0.8.1]

### Added
* RamsesComposer can now be run on Ubuntu 18.0.4.

### Changes
* Update from ramses-logic 0.6.0 to ramses-logic 0.6.1
* Update from ramses 27.0.102 to ramses 27.0.103
* Removed RACO_CONVENTIONS.md - superseded with the Ramses Composer documentation repository: https://github.com/GENIVI/ramses-composer-docs
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
* Added LGPL 2.1 license file.
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
