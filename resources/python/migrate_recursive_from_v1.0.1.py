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
Usage: RaCoHeadless.exe -r migrate_recursive_v1.0.1.py -p <project>.rca

Perform upgrade of the given project and all directly and indirectly used external projects 
to the current RamsesComposer version but keeping the feature level.

Use only for projects saved with RamsesComposer version <= v1.0.1, i.e. *before* the introduction of LuaInterfaces.
""")


try:
    import raco
except:
    usage()
    sys.exit(1)

def check_broken_links():
    status = True
    for link in raco.links():
        if link.start.object().typeName() == "LuaInterface" and link.end.object().typeName() == "LuaScript" and not link.valid:
            status = False
            print("WARNING: properties in generated LuaInterface '%s' don't match LuaScript '%s': generated interface invalid!" % (link.start.object().objectName.value(), link.end.object().objectName.value()))
    return status

def update_external_projects(project_path, processed = set()):
    if project_path in processed:
        return True
    print("'%s': loading..." % project_path)
    raco.load(project_path)
    
    print("'%s': processing external projects..." % project_path)
    for ext_path in raco.externalProjects():
        if not update_external_projects(ext_path):
            return False
        
    print("'%s': finished processing external projects. Reloading scene..." % project_path)
    raco.load(project_path)

    # migration check: check for broken links
    # if the link between the generated LuaInterface and the source LuaScript is broken this indicates 
    # that the properties don't match and the migrated interface is invalid
    status = check_broken_links()
    if not status:
        print("'%s': Mismatch in interface properties detected -> Aborting migration!\nPotential causes include errors in the script or the script file having been changed since the scene was saved.\nCheck LuaScripts for errors and save scene using RamsesCompoer V1.0.\nThen attempt migration again." % project_path)
        return False
    
    print("'%s': saving scene..." % project_path)
    raco.save(project_path)
    print("'%s': done!" % project_path)
    processed.add(project_path)
    return status

if not update_external_projects(raco.projectPath()):
    sys.exit(1)
    