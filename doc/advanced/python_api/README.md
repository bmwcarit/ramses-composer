<!--
SPDX-License-Identifier: MPL-2.0

This file is part of Ramses Composer
(see https://github.com/bmwcarit/ramses-composer-docs).

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
-->
# Python API Reference
*You can find the example project {{ '[here]({}/doc/advanced/python_api)'.format(repo) }}.*

This chapter introduces you to the Python API functionality of Ramses Composer, including a Python API reference at the end.

## Running Python Scripts
The python interface currently allows access only to the active project. The active project is implicit and doesn't need to specified in any of the operations. Load and reset operations will change the currently active project as in the RamsesComposer GUI application.

A few example scripts can be found in the `python` folder in the installation zip file.

Any python output will be logged.

The Python environment used by Ramses Composer is shipped with the application, isolated from any Python installations on the system and can be found in the `bin/python...` folder.
It is possible to use pip to install custom packages to that environment, for an example see the `python/use_pip_to_install_module.py` script.

Please be aware that virtualenv or venv are known to cause problems if used with the RaCo Python environment - particularly in Linux.

### Running Scripts in RaCoHeadless

**Running scripts with `-r`**

The `-r` commandline option of the RaCoHeadless application allows running non-interactive python scripts with access to
the RamsesComposer Python API using an embedded Python interpreter. An initial project may be specified with the `-p` commandline
option which will be loaded before the python script is started. The `--export` and `--compress` commandline options will be ignored
if the `-r` commandline option is present.

The commandline options not recognized by RaCoHeadless are collected and passed on to the python script where they are accessible in `sys.argv`.
The first element of `sys.argv` is the python script path. For example invoking `RaCoHeadless -r test.py abc -l 2 def` will run the script 'test.py',
set the log level to 2 and  pass the list `['test.py', 'abc', 'def']` to python as `sys.argv`.

#### Adding paths with `-y`
By using the `-y` command line option you can set additional python module search paths. Multiple `-y` options may be specified to add multiple directories. The additional paths are added before the default search path.

#### Error handling
If an error occurs during loading of the initial project or the execution of the python script RaCoHeadless will exit with a non-zero exit code.
Similarly, if exit is called in the Python script RaCoHeadless will exit with the specified exit code. The python API will report errors by throwing Python exceptions.

### Running Scripts in RaCoEditor
While the editor window is open, a python script can be run on the current project.

The menu bar option `View` -> `New Python Runner` will add a Python Runner dock to the editor, where the path to the script as well as command line arguments can be specified.

Scripts with functions that load or reset the current project are currently disabled , due to UI incompatibilities.

**Running scripts with `-r`**
Just like in RaCoHeadless, the `-r` commandline option of the RaCoEditor application also allows to run non-interactive python scripts for the UI version of Ramses Composer. The script will be run before the editor window appears.
To specify position arguments for the Python script, use `--` so filenames (before `--`) can be separated from python arguments (after `--`).

Examples:
* `RamsesComposer.exe -r script.py test.rca` will load test.rca, pass no parameters to python
* `RamsesComposer.exe -r script.py -- a b c` will load no project, pass 3 parameters to python
* `RamsesComposer.exe -r script.py test.rca -- a b c` will load test.rca, pass 3 parameters to python

Aside from that, all information from the RaCoHeadless Python API Reference also applies here, including the `-y` command line option.

## Writing Python Scripts for Ramses Composer
The Python API is contained within the `raco` and `raco_gui` Python modules which need to be imported explicitly. `raco_gui` is only available in the RaCoEditor.

All functions and types described below reside in either of these modules.

### Simple Example: Purging Invalid Links

A simple example that illuminates how Python-based automated workflows look like is purging invalid links.

This chapter contains an attached project `broken_link.rca` which consists of a `LuaScript`, a node `Node_validLink` with a valid link and 100 copies of the node `Node_invalidLink` with a broken link.
Think of a gigantic project with a few Lua interface nodes that are linked to a huge amount of nodes and where, due to a late change in the Lua properties, a large amount of links have now been invalidated.
Manually removing these broken links in the GUI by hand is tedious and can be already automated using a Python script.

The `python` subfolder of this chapter contains a script `purge_invalid_links.py`.
When you open this file in a text editor you can see what this script is doing: Using the `raco` module to access project data, we iterate through all links and remove the invalid ones while keeping count of how many we removed. In the final step, the modified project gets saved to a new path, as specified by a Python parameter.

Let's launch this script. You can do that by starting RaCoHeadless with the following parameters:

`<path to RaCoHeadless executable> -p <path to broken_link.rca> -r <path to purge_invalid_links.py> <new path for fixed project file>`

If every path has been correctly specified you will encounter a lot of log messages in your console window - but by looking through you will also see the Python user output messages that were specified in `purge_invalid_links.py`:

![](docs/python_output.png)

Upon opening the newly created project specified at `<new path for fixed project file>` you will discover that the entire project has been cleaned of invalid links while keeping the valid link intact.

## `raco` module reference
The `raco` module is available in both RaCoHeadless and RaCoEditor. You'll need to add an explicit import statement for the module in order to call the methods mentioned below.

### General Functions

> reset([featureLevel])
>> Create a new project which is empty except a newly created ProjectSettings object.
>> This function is currently disabled when using the Python Runner in RaCoEditor.
>> The optional featureLevel parameter allows to set the feature level of the new project. If no feature level is given the largest supported feature level is used by default.

> load(path[, featureLevel])
>> Load the project with the given `path` and replace the active project with it.
>> This function is currently disabled when using the Python Runner in RaCoEditor.
>> If the optional featureLevel parameter is used loading will attempt to upgrade the project to the given feature level. Since feature level downgrades are not allowed, the featureLevel parameter must not be smaller than the project feature level.

> save(path[, setNewIDs: bool])
>> Save the active project under the given `path`.
>> If the optional `setNewIDs` parameter is set to true, all project's object IDs are regenerated in order to allow reuse of its contents as external reference without conflicts (copies of the same project could not be used as source of external references more than once otherwise).

> projectPath()
>> Get the path of the active project. Returns an empty string if there is no active project loaded.

> isRunningInUi()
>> Use this to figure out if the composer is running in Headless mode or with the GUI.

> projectFeatureLevel()
>> Returns the feature level of the current project. This is a convenience function which reads the feature level from the `featureLevel` property of the `ProjectSettings` object.

> minFeatureLevel()
>> Returns the current minimum feature level supported by the engine.

> maxFeatureLevel()
>> Returns the current maximum feature level supported by the engine.

> externalProjects()
>> Get a list of the absolute paths of all externally referenced projects.

> export(ramses_path, logic_path, compress)
>> Export the active project. The paths of the Ramses and RamsesLogic files need to be specified. Additionally, compression can be enabled using the `compress` flag.

> getErrors()
>> Returns a list of active `ErrorItems`

> importGLTF(path[, parent])
>> Import complete contents of a gltf file into the current scene. Inserts the new nodes below `parent` in the scenegraph when the optional argument is given.

> saveScreenshot(path)
>> Save a screenshot of the preview to the given `path` as a `.png` image file.


### Active Project Access

> instances()
>> Returns a list of all objects in the active project.

> getInstanceById(id)
>> Returns the object with the specified id or None.

> links()
>> Returns a list of all links in the active project.


### Operations

> create(typename, object_name)
>> 	Creates a new object of the given type and sets the name.

> delete(object)
>> Deletes a single object.

> delete([object, ...])
>> Deletes a list of objects.

> moveScenegraph(object, new_parent)
>> 	Moves an object in the scenegraph and adds it at the end of the scenegraph children of the new parent object. Makes the object a top-level scenegraph object if `new_parent` is `None`.

> moveScenegraph(object, new_parent, insert_before_index)
>> 	Moves an object in the scenegraph and inserts it into the scenegraph children of `new_parent` before the given index.

> getLink(property)
>> 	Given a PropertyDescriptor this will return a LinkDescriptor if the property has a link ending on it or `None` if there is no link.

> addLink(start, end[, isWeak])
>> 	Creates a link between two properties given their PropertyDescriptors. Weak links can be created using an optional boolean flag. By default strong link are created.

> removeLink(end)
>> 	Removes a link given the PropertyDescriptor of the link endpoint.

> addExternalProject(path)
>> Adds the project at `path` as external reference. Path can be either absolute or relative to the current project directory.

> addExternalReferences(path, type)
>> Adds all objects of a certain `type` (or a list of `types`) from an external project at `path` as external references. Returns a list containing the added objects.

> resolveUriPropertyToAbsolutePath(property)
>> Obtain the absolute path from the value of a uri property given by a `PropertyDescriptor`. This function will resolve relative to absolute paths using the appropriate project path as a base directory.

### Objects

RamsesComposer objects can be accessed and used almost like normal Python objects.

The printed representation includes the type and name of the object, e.g. `<Node: 'my_node'>`.

Global functions:

> dir(object)
>> Returns a list of the names of all properties of the object in alphabetical order.

Member functions:

> typeName()
>> 	Returns the type of the object as a string.

> children()
>> 	Returns a list of the scenegraph children of the object. For resource-type objects an empty list is returned.
>> 	The children can't be modified directly. Instead the moveScenegraph function can be used to modify the scenegraph structure.

> parent()
>> 	Returns the scenegraph parent of the object or None. As the children() this can't be modified directly.
>> 	Instead the moveScenegraph function can be used to modify the scenegraph structure.

> objectID()
>> 	Returns the internal object ID of the object as a string. The object ID is automatically generated and can't be changed.

> isReadOnly()
>> Returns true if the object cannot be edited.

> isExternalReference()
>> Returns true if the object is part of an external reference

> isResource()
>> Returns true if the object is part of the project's resources.

> getPrefab()
>> If the object is in a Prefab, this will return the Prefab. Otherwise, this will return None.

> getPrefabInstance()
>> If the object is in a PrefabInstance, this will return the PrefabInstance. Otherwise, this will return None.

> getOuterContainingPrefabInstance()
>> In case of nested prefabs, this will return the outermost prefab instance, or None if the object is not part of a PrefabInstance.

> keys()
>> Returns a list of the names of all properties in internal data model order.

> metadata()
>> If the object is a mesh this will return gltf `extras` metadata as a dictionary. Only string values in the gltf `extras` are supported.

> getUserTags()
>> Return the `userTags` property of an object as list of strings.

> setUserTags(tags)
>> Set the `userTags` property of an object from a list of strings.

> getTags()
>> Return the `tags` property of a `Node`, `Material`, or `RenderLayer` object as list of strings.

> setTags(tags)
>> Set the `tags` property of a `Node`, `Material`, or `RenderLayer` object from a list of strings.

> getMaterialFilterTags()
>> Retun the `materialFilterTags` property of a `RenderLayer` object as list of strings.

> setMaterialFilterTags(tags)
>> Set the `materialFilterTags` property of a `RenderLayer` object from a list of strings.

> getRenderableTags()
>> Return the `renderableTags` property of `RenderLayer` object as a list of (tag, priority) tuples.

> setRenderableTags(renderableTags)
>> Set the `renderableTags` property of `RenderLayer` object from a list of (tag, priority) tuples.


### Properties

Properties are represented in the python api by PropertyDescriptor objects. These can be used to get or set the value, and will also be used in
link-related operations.

PropertyDescriptors can also be used almost like normal Python objects.

The printed representation includes the type and the full path of the property starting with the object name itself, e.g.
`<Property[Bool]: 'lua.inputs.struct.in_bool'>`.

#### Global Functions:

> dir(property)
>> Returns a list of the names of all nested properties if `property` has substructure. Returns an empty list if `property` has no substructure.

#### Member Functions

> object()
>> Return the object the property is contained in.

> typeName()
>> Returns the name of the property type as a string.

> propName()
>> The name of the property. This is the last part of the the full property path of the printed PropertyDescriptor representation.

> value()
>> Returns the value of the property for scalar properties (numbers, bool, string, references). Throws exception if the property type has substructure.

> isReadOnly()
>> Returns true if the property value cannot be edited.

> isValidLinkStart()
>> Returns true if a link can start from this property.

> isValidLinkEnd()
>> Returns true if a link can end at this property.

> hasSubstructure()
>> Returns True if the type of the property can have substructure. Returns False for scalar properties. This will return True for empty container properties, e.g. the `inputs` property of a LuaScript which has no uri set.

> keys()
>> Returns a list of the names of all nested properties if `property` has substructure. Returns an empty list if `property` has no substructure.


### Child property access

Child properties may be accessed by attribute notation or using the indexing operator. Both will return the PropertyDescriptor of the child property. To obtain the value of scalar properties the value() member function has to be used. Child properties of both objects and PropertyDescriptors can be accessed in the same way.

#### Attribute-based access

The PropertyDescriptor for a child property of a complex property or an object can be obtained using the dot notation for attribute access, e.g.
```
node.translation
```
This works recursively, e.g. `node.translation.x` returns a PropertyDescriptor for the `x` component of the translation of `node`.

Setting a property is also possible using the attribute notation:
```
node.translation.x = 2.0
```
Only scalar properties may be set directly in this way. Complex properties must be set component by component.

Alternatively the getattr and setattr functions may be used for attribute access. These take an object or PropertyDescriptor and the property name as their first two arguments. The setattr function takes the new value as last argument:
```
getattr(node, 'translation')
setattr(node.translation, 'x', 2.0)
```

While attribute access allows a convenient notation the property names may not start with numbers or conflict with the member functions listed above. For these cases use the dictionary-like access.

#### Dictionary-like access

Another way to obtain the PropertyDescriptor of a child property is via the indexing operator `[]` allowing dictionary-like access to properties. The operator requires a string as an argument, e.g.
```
node['translation']
```

The indexing operator can also be used to set properties
```
node['visibility'] = 2.0
```

The indexing operator allows access to all property names, including names conflicting with member function names or property names starting with numbers. To access the elements of a Lua array property use
```
lua.inputs.array['1']
lua.inputs.array['1'] = 3.0
```

Note that all property access variations described above can be mixed subject to the restrictions on property names mentioned,  e.g.
```
lua.inputs.vec.x
getattr(lua.inputs.vec, 'x')
getattr(lua.inputs, 'vec').x
lua["inputs"].vec.x
```
will all return the same PropertyDescriptor.


### Links

Links are represented by LinkDescriptors.

The printed representation includes the property paths including the object names of both the start end endpoints of the link as well as the link validity and weak flags, e.g. `<Link: start='lua.outputs.vec' end='my_node.rotation' valid='true' weak='false'>`.

**Member variables**

> start
>> The starting point as a PropertyDescriptor.

> end
>> The endpoint as a PropertyDescriptor.

> valid
>> 	The current link validity. Only valid links are created in the LogicEngine. Invalid links are kept for caching purposes and can become
	active if LuaScript properties change or if links on parent properties are changed. Link validity can not be set directly by the user.

> weak
>> Flag indicating a weak link.

The member variables of a LinkDescriptor can't be changed. Modification of links is only possible with the addLink and removeLink functions described below.

### ErrorItems

**Member variables**

> category
>> The category this ErrorItem belongs to.

> level
>> The severity level of this ErrorItem.

> message
>> The error message of this ErrorItem.

> handle
>> In case of object errors, this will return the EditorObject causing the error.
>> If the error refers to a property, this will return the PropertyDescriptor instead.

## `raco_gui` module reference
When running python scripts inside RaCoEditor, `raco_gui` is available to interact with editor specific things. You'll need to add an explicit import statement for the module in order to call the methods mentioned below.

If you need to ensure that your scripts run both in headless and in gui, you can wrap the import and usages of these methods inside a `raco.isRunningInUi()` check.

> getSelection()
>> Returns a list of the currently selected editor objects.
