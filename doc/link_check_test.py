#
# SPDX-License-Identifier: MPL-2.0
#
# This file is part of Ramses Composer
# (see https://github.com/bmwcarit/ramses-composer).
#
# This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
# If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
# 

from link_check import LinkChecker
import unittest

class LinkCheckTest(unittest.TestCase):
    def test_repo_links(self):
        self.assertEqual(LinkChecker.get_all_links(r"Text {{ '[here]({}/doc/basics/monkey_broken)'.format(repo) }}.")[0], "doc/basics/monkey_broken")
        self.assertEqual(LinkChecker.get_all_links(r"{{ '[fragment shader]({}/doc/basics/monkey/shaders/phong.frag#L8)'.format(repo) }}.")[0], "doc/basics/monkey/shaders/phong.frag#L8")

    def test_normal_links_ignored(self):
        self.assertEqual(len(LinkChecker.get_all_links(r"[flat Phong](https://en.wikipedia.org/wiki/Phong_shading) shader.")), 0)
        self.assertEqual(len(LinkChecker.get_all_links(r"![](./docs/private_material.png)")), 0)
        self.assertEqual(len(LinkChecker.get_all_links(r"Looking at the [vertex shader](./shaders/phong.vert)")), 0)

    def test_validate_folder_link(self):
        lc = LinkChecker()
        self.assertTrue(lc.is_link_valid("doc"))
        self.assertFalse(lc.is_link_valid("missing_folder"))

    def test_validate_file_link(self):
        lc = LinkChecker()
        self.assertTrue(lc.is_link_valid("doc/link_check_test.py"))
        self.assertTrue(lc.is_link_valid("doc/link_check_test.py#123"))
        self.assertFalse(lc.is_link_valid("doc/missing"))

if __name__ == "__main__":
    test_prog = unittest.main(exit=False, verbosity=2)
    success = test_prog.result.wasSuccessful()
    exit(not success)
 