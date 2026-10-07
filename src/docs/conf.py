# 
# src/docs/conf.py
#

import os
import sys

if os.environ.get('READTHEDOCS') == 'True':
    from hawkmoth.util import readthedocs
    readthedocs.clang_setup()


# -----------------------------------------------------------
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

# -----------------------------------------------------------
# proj
#
project = "patricia26"
author = 'Gene C'

release = read_version()

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

# -----------------------------------------------------------
# latex
#pygments_style = 'sphinx'
latex_engine = 'xelatex'
latex_use_xindy = True

latex_elements = {
    'papersize': 'letterpaper',
    'pointsize': '11pt',

    'fvset': r'\fvset{fontsize=\scriptsize}',

    'fontpkg': r'''
        \usepackage{fontspec}

        \setmainfont{Source Sans 3}[Ligatures=TeX]
        \setsansfont{Source Sans 3}[Ligatures=TeX]
        \setmonofont{Source Code Pro}
    ''',

    'preamble': r'''
        \usepackage{parskip}

        %
        % Fix the 11pt headheight layout warnings
        %
        \setlength{\headheight}{14pt}
        \addtolength{\topmargin}{-2pt}

        \usepackage{enumitem}
        \setlist[itemize]{
            noitemsep,
            topsep=6pt,
            parsep=0pt,
            partopsep=0pt,
            after=\vspace{0pt}
        }
        \setlist[enumerate]{
            noitemsep,
            topsep=6pt,
            parsep=0pt,
            partopsep=0pt,
            after=\vspace{0pt}
        }

        \usepackage{newunicodechar}
        \newunicodechar{␣}{\textvisiblespace}
        \tracinglostchars=0

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

html_theme = 'sphinx_rtd_theme'
html_static_path = ['_static']
html_css_files = [ 'custom.css',]

