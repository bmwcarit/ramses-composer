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
* **File version number has changed. Files saved with RaCo X.Y.Z cannot be opened by previous versions.**
* **Export file format has changed. Scenes exported with RaCo X.Y.Z / ramses-logic A.B.C cannot be opened by previous ramses-logic versions.**

### Known Issues

### Added

### Changes

### Fixes

-->

## [1.0.0] ramses-logic 1.x, Python API, timer, new animations
* **File version number has changed. Files saved with RaCo 1.0.0 cannot be opened by previous versions.**
* **Export file format has changed. Scenes exported with RaCo 1.0.0 / ramses-logic 1.0.2 cannot be opened by ramses-logic versions 0.x.**
* **Breaking Change: ramses-logic 1.0.2 uses a different lua syntax for the type definitions and the interface() and run() function definitions.**
    * See the ramses-logic [CHANGELOG.md](https://github.com/bmwcarit/ramses-logic/blob/master/CHANGELOG.md) for details. 
    * A [migration script provided by ramses-logic](https://github.com/bmwcarit/ramses-logic/tree/master/tools/migrate) that converts the lua syntax is included as migrate_to_v1.0.py in the root folder of the RaCo package.
        * The migration script needs to be run twice - once for adding the  IN,OUT parameter to the interface()/run() functions, and once to update the name of the types.
        * Example for running the migration script in your root project folder on Windows (adjust paths as required):
            * `cd MyProjectFolder`
            * `python \RaCoInstallationFolder\migrate_to_v1.0.py inout .`
            * `python \RaCoInstallationFolder\migrate_to_v1.0.py types .`

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
    * This is due to https://github.com/COVESA/ramses/issues/52
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
