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

def printScenegraph(obj, depth = 1):
	print("    " * depth, obj)
	for child in obj.children():
		printScenegraph(child, depth+1)
		
for obj in scenegraphRoots():
	printScenegraph(obj)