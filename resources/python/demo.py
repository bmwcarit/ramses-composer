#
# SPDX-License-Identifier: MPL-2.0
#
# This file is part of Ramses Composer
# (see https://github.com/bmwcarit/ramses-composer).
#
# This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
# If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
# 
import raco
import os
import sys

def scenegraphRoots():
    return [obj for obj in raco.instances() if obj.parent() == None]
    
def printProperties(handle, depth = 1):
    for name in handle.keys():
        childHandle = handle[name]
        if childHandle.hasSubstructure():
            print("    " * depth, name, "  ", childHandle)
            printProperties(childHandle, depth + 1)
        else:
            print("    " * depth, name, "  ", childHandle, "=", childHandle.value())

def printObject(obj):
    print(obj)
    print("    type = ", obj.typeName())
    print("    id = ", obj.objectID())
    print("    children = ", obj.children())
    print("    parent = ", obj.parent())
    print("\n    properties:")
    printProperties(obj, depth = 1)

print("\n ----- general info -----\n")
print(" argv = ", sys.argv)
print(" path = ", sys.path)
print(" cwd = ", os.getcwd())
print(" project = ", raco.projectPath())
print(" feature level = ", raco.projectFeatureLevel())

print("\n ----- project access -----\n")
print("Project instances: ", raco.instances())
print("Project links: ", raco.links())
print("\n\n")


print("\n ----- property access -----\n")

obj = raco.create("Node", "my_node")

print(" object", obj, " with properties ", obj.keys())
print("\n")

# getting properties

print("descriptor: ", obj.visibility)
print("descriptor: ", getattr(obj, "visibility"))

print("value = ", obj.visibility.value())
print("\n")

# setting properties

obj.visibility = False
setattr(obj, "visibility", False)

# nested properties 

print("descriptor: ", obj.translation.x)
obj.translation.x = 42.0
setattr(obj.scaling, "z", 5.5) 
print("\n")

printObject(obj)


# object creation 

prefab = raco.create("Prefab", "my_prefab")
node = raco.create("Node", "my_node")

tmp = raco.create("Node", "temp")
raco.delete(tmp)
# raco.delete([node, tmp])


# scenegraph modification

raco.moveScenegraph(node, prefab)



# create and sync luascript 

print("\n ----- LuaScript -----\n")

lua = raco.create("LuaScript", "lua")

print("Lua without uri:")
printObject(lua)
print("\n")

lua.uri = sys.path[0] + "/../scripts/types-scalar.lua"

print("Lua with uri:")
printObject(lua)
print("\n")



# create and remove links

print(" -- Create Link -- \n")

link = raco.addLink(lua.outputs.ovector3f, obj.translation)

print("created Link: ", link)
print("  link start = ", link.start)
print("  link end   = ", link.end)
print("  link valid = ", link.valid)
print("  link weak  = ", link.weak)
print("\n")

raco.addLink(lua.outputs.ovector3f, obj.rotation, True)

raco.removeLink(obj.translation)


# links queries

print(" -- Querying Links -- \n")
print("link for ", obj.translation, " = ", raco.getLink(obj.translation), "\n")
print("link for ", obj.rotation, " = ", raco.getLink(obj.rotation), "\n")
print("all links = ", raco.links())
print("\n\n")

