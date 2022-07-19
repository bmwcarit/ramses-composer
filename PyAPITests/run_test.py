#
# SPDX-License-Identifier: MPL-2.0
#
# This file is part of Ramses Composer
# (see https://github.com/GENIVI/ramses-composer).
#
# This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
# If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
# 
import sys
import unittest
  
if __name__ == "__main__":
    test_prog = unittest.main(module=sys.argv[1], argv=sys.argv[1:], exit=False, verbosity=2)
    success = test_prog.result.wasSuccessful()
    exit(not success)
    