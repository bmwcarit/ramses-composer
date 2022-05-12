<!--
SPDX-License-Identifier: MPL-2.0

This file is part of Ramses Composer
(see https://github.com/GENIVI/ramses-composer).

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
-->
# RaCoHeadless Python API Reference

The newly added "-r" commandline option of the RaCoHeadless application allows to run non-interactive python scripts with access to 
the RamsesComposer Python API using an embedded Python interpreter. An initial project may be specified with the "-p" commandline
option which will be loaded before the python script is started. The '--export' and '--compress' commandline options will be ignored 
if the '-r' commandline option is present.

The commandline options not recognized by RaCoHeadless are collected and passed on to the python script where they are accessible in `sys.argv`.
For example invoking `RaCoHeadless -r test.py abc -l 2 def` will run the script 'test.py', set the log level to 2 and  pass the list `['abc', 'def']`
to python as `sys.argv`.

The Python API is contained in the "raco" Python module which needs to be imported explicity. 

If an error occurs during loading of the initial project or the execution of the python script RaCoHeadless will exit with a non-zero exit code.
Similarly if exit is called in the Python script RaCoHeadless will exit with the specified exit code. The python API will report errors by 
throwing Python exceptions. 

All functions and types described below reside in the 'raco' module.

The python interface currently allows access only to the active project. The active project is implicit and doesn't need to specified in any of the operations. Load and reset operations will change the currently active project as in the RamsesComposer GUI application.

A few example scripts can be found in the 'python' folder in the installation zip file.

The Python environment used by RaCoHeadless is shipped with Ramses Composer, isolated from any Python installations on the system and can be found in the bin/python... folder. 
It is possible to use pip to install custom packages to that environment, for an example see the python/use_pip_to_install_module.py script.
Please be aware that virtualenv or venv are known to cause problems if used with the RaCoHeadless Python environment - particularly in Linux.

## General Functions 

> reset()
>> Create a new project which is empty except a newly created ProjectSettings object.

> load(path)
>> Load the project with the given `path` and replace the active project with it.

> save(path)
>> Save the active project under the given `path`.

> export(ramses_path, logic_path, compress)
>> Export the active project. The paths of the Ramses and RamsesLogic files need to be specified. Additionallly compression can be enabled using the `compress` flag.
	

## Active Project Access

> instances()
>> Returns a list of all objects in the active project.
	
> links()
>> Returns a list of all links in the active project.


## Objects

RamsesComposer objects can be accessed and used almost like normal Python objects.

The printed representation includes the type and name of the object, e.g. `<Node: 'my_node'>`.

Global functions:

> dir(object)
>> Returns a list of the names of all properties of the object.


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
	
Properties can be accessed like python class members using either the dot notation or the getattr/setattr functions.
To obtain a property `getattr(node, "visible")` or `node.visible` will return the 'visible' property of the 'node' object as a PropertyDescriptor. 
Changing properties is possible using the dot notation, e.g. `node.visible = True`, or using the setattr function, e.g. `setattr(node, 'visible', True)`.


## Properties

Properties are represented in the python api by PropertyDescriptor objects. These can be used to get or set the value, and will also be used in 
link-related operations.

PropertyDescriptors can also be used almost like normal Python objects. 

The printed representation includes the type and the full path of the property starting with the object name itself, e.g. 
`<Property[Bool]: 'lua.luaInputs.struct.in_bool'>`.

### Global Functions:

> dir(property)
>> Returns a list of the names of all nested properties if `property` has substructure. Returns an empty list if `property` has no substructure.
	
### Member Functions
	
> typeName()
>> Returns the name of the property type as a string. 
	
> propName()
>> The name of the property. This is the last part of the the full property path of the printed PropertyDescriptor representation.
	
> value()
>> Returns the value of the property for scalar properties (numbers, bool, string, references). Returns None for properties which have substructure.
	
Read and write access to substructure of complex properties is performed as for objects, i.e. with dot notation or the getattr/setattr functions.
`getattr(lua.luaInputs, 'bool')` or `lua.luaInputs.bool` will return a PropertyDescriptor for the nested property. Setting a property is possible with
`lua.luaInputs.bool = True` or `setattr(lua.luaInputs, 'bool', True)`.


## Links

Links are represented by LinkDescriptors. 

The printed representation includes the property paths including the object names of both the start end endpoints of the link as well as the link validity flag, e.g. `<Link: start='lua.luaOutputs.vec' end='my_node.rotation' valid='true'>`.

### Member variables

> start
>> The starting point as a PropertyDescriptor.

> end
>> The endpoint as a PropertyDescriptor.
	
> valid
>> 	The current link validity. Only valid links are created in the LogicEngine. Invalid links are kept for caching purposes and can become
	active if LuaScript properties change or if links on parent properties are changed. Link validity can not be set directly by the user.
	
The member variables of a LinkDescriptor can't be changed. Modification of links is only possible with the addLink and removeLink functions described below.


## Operations

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
 	
> addLink(start, end)
>> 	Creates a link between two properties given their PropertyDescriptors.
 	
> removeLink(end)
>> 	Removes a link given the PropertyDescriptor of the link endpoint.
 	
	





