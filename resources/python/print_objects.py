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
    printProperties(obj, depth = 1)
		
for obj in raco.instances():
    printObject(obj)
    print("\n\n")