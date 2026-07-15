# Configuration file for the Sphinx documentation builder.
#
# dx2 is a C++ library, so Sphinx is used purely as the presentation
# layer over Doxygen. The pipeline is:
#
#     Doxygen (parse comments -> XML)
#       -> Breathe (expose the XML to Sphinx)
#       -> Exhale (auto-generate a page per class/file/namespace)
#       -> Sphinx (render HTML)
#
# Exhale runs Doxygen for us (exhaleExecutesDoxygen = True) with the
# Doxygen configuration inlined below, so there is no separate Doxyfile
# to keep in sync.

import textwrap

# Project information

project = "dx2"
copyright = "Diamond Light Source"
# Contributors from the GitHub repository, ordered by contribution
# count. Update from `gh api repos/dials/dx2/contributors` as needed.
author = (
    "Dimitri Vlachos, James Beilsten-Edmands, Nicholas Devenish, "
    "Yash Karan"
)


def _cmake_version():
    """Read the project() version from the top-level CMakeLists.txt so
    the docs stay in step with the real project version instead of
    duplicating it here."""
    import pathlib
    import re

    cmake = pathlib.Path(__file__).parent.parent / "CMakeLists.txt"
    match = re.search(
        r"project\([^)]*VERSION\s+([0-9]+(?:\.[0-9]+)*)", cmake.read_text()
    )
    return match.group(1) if match else "0.0.0"


version = _cmake_version()
release = version

# General configuration

extensions = [
    "breathe",
    "exhale",
    "myst_parser",
]

# Allow the README (Markdown) to be pulled into the toctree.
source_suffix = {
    ".rst": "restructuredtext",
    ".md": "markdown",
}

exclude_patterns = ["_build", "Thumbs.db", ".DS_Store"]

# HTML output

html_theme = "furo"
html_title = "dx2 documentation"

# Breathe configuration

# Breathe reads the XML from the Doxygen run that Exhale triggers below.
breathe_projects = {"dx2": "./doxyoutput/xml"}
breathe_default_project = "dx2"

# Exhale configuration

exhale_args = {
    # Where Exhale writes the generated .rst API tree (git-ignored).
    "containmentFolder": "./api",
    "rootFileName": "library_root.rst",
    "rootFileTitle": "API Reference",
    "doxygenStripFromPath": "..",
    # Let Exhale invoke Doxygen; the config below is fed to it on stdin.
    "createTreeView": True,
    "exhaleExecutesDoxygen": True,
    "exhaleDoxygenStdin": textwrap.dedent(
        """\
        INPUT            = ../include ../dx2
        RECURSIVE        = YES
        FILE_PATTERNS    = *.hpp *.cxx
        # dx2 is header-heavy; document everything the parser sees.
        EXTRACT_ALL      = YES
        EXTRACT_STATIC   = YES
        HIDE_UNDOC_MEMBERS = NO
        # C++20 / mdspan can trip the parser; these shims keep it quiet
        # without changing the documented signatures.
        MACRO_EXPANSION  = YES
        EXPAND_ONLY_PREDEF = YES
        PREDEFINED       = __cplusplus=202002L
        """
    ),
}

# The language Exhale/Breathe should assume for the generated tree.
primary_domain = "cpp"
highlight_language = "cpp"
