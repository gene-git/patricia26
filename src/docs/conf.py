# 
# src/docs/conf.py
#

import os
import sys

# 
# version
#
def read_version() -> str:
    """
    Get package version from version.txt file
    """
    file = '../version.txt'
    if os.path.exists(file):
        with open(file, 'r') as fob:
            proj_vers = fob.readlines()[0]
    else:
        proj_vers = '0.1.0-unknown'
    return proj_vers

docs_root = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.abspath("./stubs"))

#
# proj
#
project = "patricia26"
author = 'Gene C'
release = read_version()
latex_engine = 'xelatex'

extensions = [
    'sphinx.ext.autodoc',
    'sphinx.ext.imgconverter',
    "hawkmoth",
]

primary_domain = 'c'
hawkmoth_clang = [
    "-std=c23",
    "-Ilib",
    "-UPATRICIA_EXPORT",
        ]

pygments_style = 'sphinx'

latex_elements = {
    'papersize': 'letterpaper',
    'pointsize': '10pt',
    'preamble': r'''
        \usepackage{microtype}
        \usepackage{parskip}
        \usepackage{needspace}
        \usepackage{fontspec}

        \usepackage{newunicodechar}
        \newunicodechar{␣}{\textvisiblespace}
        \tracinglostchars=0

        \makeatletter
        \renewcommand{\subsection}[1]{\par\bigskip\needspace{14\baselineskip}\textbf{#1}}
        %\renewcommand{\subsection}{\par\bigskip\needspace{14\baselineskip}}
        \makeatother

    ''',
}

#
# Group doc tree into a single doc.
# Tuple structure: (source start file, target name, title, author, documentclass)
#
latex_documents = [
    (
        'index',
        'patricia26.tex',
        'Patricia26 API Reference',
        'Gene C',
        'manual'
    ),
]
