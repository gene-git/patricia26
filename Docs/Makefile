# docs/Makefile
SPHINXOPTS    ?=
SPHINXBUILD   ?= sphinx-build
SOURCEDIR     = .
BUILDDIR      = _build

.PHONY: help Makefile html latexpdf clean

help:
	@$(SPHINXBUILD) -M help "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)

html:
	@$(SPHINXBUILD) -b html "$(SOURCEDIR)" "$(BUILDDIR)/html" $(SPHINXOPTS) $(O)
	@echo "HTML documentation build complete. Path: $(BUILDDIR)/html/index.html"

latexpdf:
	@$(SPHINXBUILD) -b latex "$(SOURCEDIR)" "$(BUILDDIR)/latex" $(SPHINXOPTS) $(O)
	@echo "Running xelatex compiler framework..."
	$(MAKE) -C "$(BUILDDIR)/latex" all-pdf LATEXOPTS="-interaction=nonstopmode"
	@echo "PDF documentation compilation complete. Path: $(BUILDDIR)/latex/patricia26.pdf"
	@rsync -av _build/latex/patricia26.pdf .

clean:
	@rm -rf $(BUILDDIR)/*
	@echo "Documentation build caches cleared."

# Catch-all target for standard Sphinx fallback routing
%: Makefile
	@$(SPHINXBUILD) -M $@ "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)

