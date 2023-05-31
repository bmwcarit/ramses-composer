#  -------------------------------------------------------------------------
#  Copyright (C) 2022 BMW AG
#  -------------------------------------------------------------------------
#  This Source Code Form is subject to the terms of the Mozilla Public
#  License, v. 2.0. If a copy of the MPL was not distributed with this
#  file, You can obtain one at https://mozilla.org/MPL/2.0/.
#  -------------------------------------------------------------------------

# Configuration file for the Sphinx documentation builder.
#
# This file only contains a selection of the most common options. For a full
# list see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

import subprocess, os

# -- Project information -----------------------------------------------------

project = 'ramses_composer'
copyright = '2018-2023, BMW AG, ParadoxCat'
author = 'BMW AG, ParadoxCat'

# -- General configuration ---------------------------------------------------

# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.
extensions = [
    "sphinx_rtd_theme",             # Read-the-docs html theme
    "sphinx.ext.autosectionlabel",  # Creates labels to sections in documents for easier :ref:-ing
    "myst_parser",                  # Can parse MyST markdown dialect
    "sphinx_lfs_content",           # Ensures git lfs support on readthedocs
]

# Add any paths that contain templates here, relative to this directory.
templates_path = ['_templates']

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
# This pattern also affects html_static_path and html_extra_path.
exclude_patterns = []


# -- Options for HTML output -------------------------------------------------

html_logo = "_static/logo.png"

html_theme_options = {
    'logo_only': True
}

html_theme = 'sphinx_rtd_theme'

# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
html_static_path = ['_static']

myst_heading_anchors = 2
autosectionlabel_prefix_document = True

# Allow to point repository links to a branch using an environment variable.
# This uses the substitutions syntax extension from the myst parser
myst_enable_extensions = [
    "substitution"
]

if "RACO_REPO" in os.environ:
    myst_substitutions = {
        "repo" : os.environ["RACO_REPO"]
    }
else:
    myst_substitutions = {
        "repo" : "https://github.com/bmwcarit/ramses-composer/tree/main"
    }