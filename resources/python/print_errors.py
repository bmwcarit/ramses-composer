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

print(" %i Errors:\n\n" % len(raco.getErrors()))

for error in raco.getErrors():
    print("Category:", error.category())
    print("Level:", error.level())
    print("Message:")
    print("----------- start -----------")
    print(error.message())
    print("----------- end -----------")
    print("Handle:", error.handle())
    print("\n\n")

