#
# patricia26
#
PKG = patricia26

SPHINXOPTS    ?=
SPHINXBUILD   ?= sphinx-build
SOURCEDIR     = .
BUILDDIR      = _build

SOURCES = conf.py ../version.txt \
		  c-api.rst \
		  index.rst \
		  python-api.rst \
		  pytricia-migrate.rst \
		  stubs/patricia26.py \
		  lib/patricia.c \
		  lib/patricia.h

.PHONY: help Makefile clean html latexpdf

html: $(BUILDDIR)/html/index.html
latexpdf: $(PKG).pdf

help:
	@$(SPHINXBUILD) -M help "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)

$(BUILDDIR)/html/index.html: $(SOURCES)
	@$(SPHINXBUILD) -b html "$(SOURCEDIR)" "$(BUILDDIR)/html" $(SPHINXOPTS) $(O)
	@$(SPHINXBUILD) -b html "$(SOURCEDIR)" "$(BUILDDIR)/html" $(SPHINXOPTS) $(O)
	@rsync -av _build/html .
	@echo "HTML doc done. $@"

$(PKG).pdf: $(SOURCES)
	@$(SPHINXBUILD) -b latex "$(SOURCEDIR)" "$(BUILDDIR)/latex" $(SPHINXOPTS) $(O)
	@echo "Running xelatex"
	$(MAKE) -C "$(BUILDDIR)/latex" all-pdf LATEXOPTS="-interaction=nonstopmode" PDFLATEX=xelatex
	$(MAKE) -C "$(BUILDDIR)/latex" all-pdf LATEXOPTS="-interaction=nonstopmode" PDFLATEX=xelatex
	@rsync -av _build/latex/$(PKG).pdf .
	@echo "PDF doc build done: Path: $@"

clean:
	#@rm -rf $(BUILDDIR)/*
	@rm -rf $(PKG).pdf

