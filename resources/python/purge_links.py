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
import raco

print("\nInitial number of links: ", len(raco.links()))

count = 0
count_ro = 0
for link in raco.links():
    if not link.valid:
        if not link.end.object().isReadOnly():
            count += 1
            raco.removeLink(link.end)
            #print(link)
        else:
            count_ro += 1
		
print("\nPurging invalid links:")        
print("Number of removed links: ", count)
print("Number of invalid read-only links (not removed): ", count_ro)
print("New number of links: ", len(raco.links()))

if len(sys.argv) > 1:
    filename = sys.argv[1]
    print("Save to ", filename)
    raco.save(filename)

