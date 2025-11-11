from __future__ import annotations

import os
import sys
from datetime import datetime

# Add project src to path for autodoc
PROJECT_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
PYTHON_SRC = os.path.join(PROJECT_ROOT, "python")
if PYTHON_SRC not in sys.path:
    sys.path.insert(0, PYTHON_SRC)

project = "Hybrid Logic Minimizer"
author = "Hybrid Logic Maintainers"
current_year = datetime.utcnow().year
copyright = f"{current_year}, {author}"
release = "0.1"
version = release

extensions = [
    "myst_nb",
    "sphinx.ext.autodoc",
    "sphinx.ext.autosummary",
    "sphinx.ext.napoleon",
    "sphinx.ext.intersphinx",
    "sphinx.ext.viewcode",
    "sphinx_copybutton",
    "sphinx_design",
    "sphinxcontrib.mermaid",
]

autosummary_generate = True
napoleon_google_docstring = True
napoleon_numpy_docstring = False
napoleon_include_private_with_doc = False
napoleon_use_rtype = False

intersphinx_mapping = {
    "python": ("https://docs.python.org/3", {}),
    "numpy": ("https://numpy.org/doc/stable", {}),
}

myst_enable_extensions = [
    "colon_fence",
    "deflist",
    "fieldlist",
    "html_admonition",
    "html_image",
    "substitution",
]

myst_heading_anchors = 3

nb_execution_mode = "off"
nb_execution_timeout = 120

# Mock heavy optional dependencies to keep RTD builds lean
autodoc_mock_imports = [
    "numpy",
    "scipy",
    "matplotlib",
    "networkx",
    "pygraphviz",
    "graphviz",
    "pandas",
]

templates_path = ["_templates"]
exclude_patterns = ["_build", "Thumbs.db", ".DS_Store"]

html_theme = "furo"
html_static_path = ["_static"]
html_title = "Hybrid Logic Minimizer"
html_logo = None
html_favicon = None

html_theme_options = {
    "light_css_variables": {
        "color-brand-primary": "#C4470F",
        "color-brand-content": "#B13B00",
    },
    "dark_css_variables": {
        "color-brand-primary": "#FF9555",
        "color-brand-content": "#FFA673",
    },
}

pygments_style = "sphinx"
pygments_dark_style = "monokai"

def setup(app):
    app.add_css_file("custom.css")
