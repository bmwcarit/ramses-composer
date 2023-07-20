#
# SPDX-License-Identifier: MPL-2.0
#
# This file is part of Ramses Composer
# (see https://github.com/bmwcarit/ramses-composer).
#
# This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
# If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
# 
import sys

def usage():
    print("""
Usage: RaCoHeadless.exe -r migrate_recursive.py -p <project>.rca

Perform upgrade of the given project and all directly and indirectly used external projects 
to the current RamsesComposer version but keeping the feature level.

Use only for projects saved with RamsesComposer versions >= v1.1.0, i.e. *after* the introduction of LuaInterfaces.
""")

try:
    import raco
except:
    usage()
    sys.exit(1)

def update_external_projects(project_path, processed = set()):
    if project_path in processed:
        return
    print("'%s': loading..." % project_path)
    raco.load(project_path)
    
    if raco.externalProjects() != []:
        print("'%s': processing external projects..." % project_path)
        for ext_path in raco.externalProjects():
            update_external_projects(ext_path)
        
        print("'%s': finished processing external projects. Reloading scene..." % project_path)
        raco.load(project_path)

    print("'%s': saving scene..." % project_path)
    raco.save(project_path)
    print("'%s': done!" % project_path)
    processed.add(project_path)

if not raco.projectPath():
    usage()
    sys.exit(1)
    
update_external_projects(raco.projectPath())
