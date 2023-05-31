#
# SPDX-License-Identifier: MPL-2.0
#
# This file is part of Ramses Composer
# (see https://github.com/bmwcarit/ramses-composer).
#
# This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
# If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
# 
import os
import re

# Match links with format string  “replacement fields” {}:
# [text]({}/doc/basics/monkey) - directory
# [fragment shader]({}/doc/basics/monkey/shaders/phong.frag#L8) - file with line #
# Ignore complete links without format string.
LINK_RE = re.compile(r'\[[^\[\]]*\]\(\{\}/*(.*?)\)')

class LinkChecker:
    @staticmethod
    def get_all_links(text):
        return LINK_RE.findall(text)


    def __init__(self):
        # Base directory for repo links is repo root.
        self.baseDir = os.path.abspath(os.path.join(os.getcwd(), os.pardir))

        # Documents root directory to search Markdown files.
        self.docRoot = os.path.join(os.getcwd())


    def find_files_recursive(self, directory):
        with os.scandir(directory) as it:
            for entry in it:
                if entry.is_file() and entry.name.endswith('.md'):
                    yield entry.path
                elif entry.is_dir(follow_symlinks=False):
                    yield from self.find_files_recursive(entry.path)


    def find_files(self):
        yield from self.find_files_recursive(self.docRoot)


    def find_links(self, file_name):
        text = str()
        try:
            # Expect input files are encoded in UTF-8.
            with open(file_name, encoding="utf-8") as file:
                text = file.read()
        except Exception:
            print(f"Exception while reading file: {file_name}")
            raise
        yield from LinkChecker.get_all_links(text)


    def is_link_valid(self, link):
        # Split out optional line number: #L123
        link = link.split('#')[0]

        # Link path is relative to base dir and must exist
        absPath = os.path.join(self.baseDir, link)
        return os.path.exists(absPath) and (os.path.isdir(absPath) or os.path.isfile(absPath))


    def find_broken_links(self):
        yield from ((file, filter(lambda link: not self.is_link_valid(link), self.find_links(file))) for file in self.find_files())


if __name__ == "__main__":
    print("Link Check started in: " + os.getcwd())

    link_checker = LinkChecker()
    broken_links = link_checker.find_broken_links()

    broken_link_found = False
    for (file, links) in broken_links:
        links_list = list(links)
        if len(links_list):
            broken_link_found = True
            print(f"\tBroken links in file: {file}")
            for link in links_list:
                print(f"\t\t{link}")

    print("Link Check done")
    exit(1 if broken_link_found else 0)
