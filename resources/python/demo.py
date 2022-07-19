#
# SPDX-License-Identifier: MPL-2.0
#
# This file is part of Ramses Composer
# (see https://github.com/GENIVI/ramses-composer).
#
# This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
# If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
# 
import raco

def scenegraphRoots():
    return [obj for obj in raco.instances() if obj.parent() == None]
    
def printProperties(handle, depth = 1):
    for name in dir(handle):
        childHandle = getattr(handle, name)
        print("    " * depth, name, "  ", childHandle, "  ", childHandle.value())
        printProperties(childHandle, depth + 1)

def printObject(obj):
    print(obj)
    print("    type = ", obj.typeName())
    print("    id = ", obj.objectID())
    print("    children = ", obj.children())
    print("    parent = ", obj.parent())
    print("\n    properties:")
    printProperties(obj, depth = 1)


print("\n ----- project access -----\n")
print("Project instances: ", raco.instances())
print("Project links: ", raco.links())
print("\n\n")


print("\n ----- property access -----\n")

obj = raco.instances()[0]
print(" object", obj, " with properties ", dir(obj))
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

print("Lua withut uri:")
printObject(lua)
print("\n")

lua.uri = R"C:\Users\MarcusWeber\OneDrive - Paradox Cat GmbH\Documents\RamsesComposer\scripts\through.lua"

print("Lua with uri:")
printObject(lua)
print("\n")



# create and remove links

obj = raco.instances()[0]

print(" -- Create Link -- \n")

link = raco.addLink(lua.outputs.vec, obj.translation)

print("created Link: ", link)
print("  link start = ", link.start)
print("  link end   = ", link.end)
print("  link valid = ", link.valid)
print("\n")

raco.addLink(lua.outputs.vec, obj.rotation)

raco.removeLink(obj.translation)


# links queries

print(" -- Querying Links -- \n")
print("link for ", obj.translation, " = ", raco.getLink(obj.translation), "\n")
print("link for ", obj.rotation, " = ", raco.getLink(obj.rotation), "\n")
print("all links = ", raco.links())
print("\n\n")

