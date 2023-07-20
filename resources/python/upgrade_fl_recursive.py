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
Usage: RaCoHeadless.exe -r upgrade_fl_recursive.py -p <project>.rca -- <fl>

Perform upgrade of the given project and all directly and indirectly used external projects
to the current RamsesComposer version while also upgrading the feature level to the given level.
""")

try:
    import raco
except:
    usage()
    sys.exit(1)

def update_external_projects(project_path, feature_level, processed = set()):
    if project_path in processed:
        return
    print("'%s': loading..." % project_path)
    raco.load(project_path, feature_level)
    
    if raco.externalProjects() != []:
        print("'%s': processing external projects..." % project_path)
        for ext_path in raco.externalProjects():
            update_external_projects(ext_path, feature_level)
        
        print("'%s': finished processing external projects. Reloading scene..." % project_path)
        raco.load(project_path, feature_level)

    print("'%s': saving scene..." % project_path)
    raco.save(project_path)
    print("'%s': done!" % project_path)
    processed.add(project_path)

if not raco.projectPath():
    usage()
    sys.exit(1)
    
fl = int(sys.argv[1])

print("Upgrading %s to feature level %i\n" % (raco.projectPath(), fl))

update_external_projects(raco.projectPath(), fl)
