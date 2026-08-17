# 
# Docs/conf.py
#

import os
import sys
sys.path.insert(0, os.path.abspath("./stubs"))
#sys.path.insert(1, os.path.abspath("../src/"))

project = "patricia26"
author = 'Gene C'
release = "1.1.0"
latex_engine = 'xelatex'

extensions = [
    'sphinx.ext.autodoc',
    'sphinx.ext.napoleon',
    'sphinx.ext.imgconverter',
    "hawkmoth",                  # Core Hawkmoth C-Autodoc engine
    "hawkmoth.ext.napoleon",     # Bridges Hawkmoth to the Napoleon engine
]

napoleon_google_docstring = True
napoleon_numpy_docstring = False

hawkmoth_cflags = [
    "-std=c23",
    "-DHAVE_IPV6"
]

latex_elements = {
    'papersize': 'letterpaper',
    'pointsize': '10pt',
    'preamble': r'''
        \usepackage{microtype}
        \usepackage{parskip}
    ''',
}

# Grouping the document tree into a single LaTeX document manual volume.
# Tuple structure: (source start file, target name, title, author, documentclass)
latex_documents = [
    (
        'index',
        'patricia26.tex',
        'patricia26 Developer Reference Documentation',
        'Gene',
        'manual'
    ),
]
