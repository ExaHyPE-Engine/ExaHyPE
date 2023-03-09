OUT      := guidebook
MAINFILE := documentation

all:
	mkdir -p $(OUT)
	echo '*' > $(OUT)/.gitignore
	make pdflatex
	make pdflatex

pdf:
	# Also see .latexmkrc
	mkdir -p $(OUT)
	echo '*' > $(OUT)/.gitignore
	latexmk -outdir=$(OUT) -auxdir=$(OUT) -shell-escape -pdf $(MAINFILE)

pdfa:
	gs -dPDFA -dBATCH -dNOPAUSE -sProcessColorModel=DeviceCMYK -sDEVICE=pdfwrite -dPDFACompatibilityPolicy=1 -sOutputFile=$(OUT)/$(MAINFILE)-pdfa.pdf $(OUT)/$(MAINFILE).pdf

pdflatex:
	pdflatex $(MAINFILE).tex
	bibtex $(MAINFILE).aux

clean:
	rm -rf $(filter-out $(OUT)/$(MAINFILE).pdf, $(wildcard $(OUT)/*))
	rm -f $(MAINFILE).acn
	rm -f $(MAINFILE).acr $(MAINFILE).alg $(MAINFILE).aux $(MAINFILE).bbl $(MAINFILE).blg $(MAINFILE).glg
	rm -f $(MAINFILE).glo $(MAINFILE).gls $(MAINFILE).ist $(MAINFILE).log $(MAINFILE).out $(MAINFILE).pdesyg
	rm -f $(MAINFILE).pdesyi $(MAINFILE).sierpinskisyg $(MAINFILE).slg $(MAINFILE).syg $(MAINFILE).syi $(MAINFILE).toc
	rm -f $(MAINFILE).pdf
	rm -f $(MAINFILE).ilg
	rm -f $(MAINFILE).run.xml
	rm -f $(MAINFILE).synctex.gz
	rm -f $(MAINFILE)-blx.bib
	rm -f texput.log

purge:
	rm -rf $(OUT)

.PHONY: all pdf clean purge pdfa
