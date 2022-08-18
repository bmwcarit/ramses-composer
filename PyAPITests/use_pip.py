#
# SPDX-License-Identifier: MPL-2.0
#
# This file is part of Ramses Composer
# (see https://github.com/bmwcarit/ramses-composer).
#
# This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
# If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
# 
import unittest
import sys
import runpy
import os
import platform
import shutil
import subprocess

class UsePip(unittest.TestCase):
    def setUp(self):
        self._install_script = '../resources/python/use_pip_to_install_module.py'
        self._uninstall_script = '../resources/python/use_pip_to_uninstall_module.py'
        if platform.system() == "Windows":
            self._tested_module_path = '../build/release/bin/python-3.8.10-with-pip/Lib/site-packages/simplejson/__init__.py'
        elif platform.system() == "Linux":
            self._tested_module_path = '../build/release/bin/python-3.8.0-with-pip/python3.8/lib/python3.8/site-packages/simplejson/__init__.py'
        else:
            raise RuntimeError('Unknown platform')
            
        # Make sure the module we test with is not already installed.
        if os.path.exists(self._tested_module_path):
            raise RuntimeError('The simplejson module used for testing is already installed in the environment')

    def tearDown(self):
        # Remove simplejson again
        if os.path.exists(self._tested_module_path):
            subprocess.call([sys.executable, '-r', self._uninstall_script])

    def test_install_simplejson(self):
        subprocess.check_call([sys.executable, '-r', self._install_script])
        # Make sure the module was imported to the correct path (it is possible for the pip install to complete
        # but pip having used the wrong path).
        self.assertTrue(os.path.exists(self._tested_module_path), msg="'" + self._tested_module_path + "' does not exist!")

