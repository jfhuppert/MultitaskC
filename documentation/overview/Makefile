EPSTOPDF	= epstopdf
PDFLATEX	= pdflatex
GNUPLOT		= gnuplot
FIG2DEV		= fig2dev
INKSCAPE	= inkscape

DEST		= 
PWD		:= $(shell basename `pwd`)

GOAL		:= report

.SUFFIXES:

.SUFFIXES: .eps .dvi .ps .tex .bbl .gp .eps .pdf

.PHONY: all clean open dvi eps ps pdf touch tar edits install xdvi rights

PLOTFILES	:= $(wildcard *.plot)
DATAFILES	:= $(wildcard *.data)
BIBFILES	:= $(GOAL).bib
FIGFILES	:= $(wildcard figures/*.fig)
SVGFILES	:= $(wildcard figures/*.svg)
GIFFILES	:= $(wildcard *.gif)
STYFILES	:= $(wildcard *.sty)

TEXMAINFILES	:= $(GOAL).tex 
TEXSUBFILES	:= 
TEXFILES	:= $(TEXMAINFILES) $(TEXSUBFILES)

BBLFILES	:= $(BIBFILES:%.bib=%.bbl )

AUXFILES	:= $(TEXMAINFILES:%.tex=%.aux)
DVIFILES	:= $(TEXMAINFILES:%.tex=%.dvi)
PSFILES		:= $(TEXMAINFILES:%.tex=%.ps)
PDFFILES	:= $(TEXMAINFILES:%.tex=%.pdf)

EPSFIGFILES	:= $(FIGFILES:%.fig=%.eps)
EPSSVGFILES	:= $(SVGFILES:%.svg=%.eps)
PNGFIGFILES	:= $(FIGFILES:%.fig=%.png)
PDFFIGFILES	:= $(FIGFILES:%.fig=%.pdf)
EPSPLOTFILES	:= $(PLOTFILES:%.plot=%.eps)
EPSGIFFILES	:= $(GIFFILES:%.gif=%.eps)
JPGGIFFILES	:= $(GIFFILES:%.gif=%.jpg)

DVIIMAGEFILES	:= $(EPSFIGFILES) $(EPSSVGFILES) $(EPSPLOTFILES) $(EPSGIFFILES)
PDFIMAGEFILES	:= $(PNGFIGFILES) $(EPSSVGFILES) $(JPGGIFFILES) $(PDFFIGFILES)


all: dvi rights pdf

$(GOAL).dvi $(GOAL).pdf: $(STYFILES) 
$(GOAL).aux: $(STYFILES)

$(GOAL).aux $(GOAL).dvi: $(DVIIMAGEFILES) 

#################################################################

$(EPSGIFFILES): %.eps: %.gif
	convert $< $@

$(EPSPLOTFILES): %.eps: %.data %.plot
	$(GNUPLOT) $*.plot	

$(EPSFIGFILES): %.eps: %.fig
	$(FIG2DEV) -L eps $*.fig > $*.eps

$(EPSSVGFILES): %.eps: %.svg
	$(INKSCAPE) -z $*.svg --export-eps=$*.eps --export-text-to-path

eps: $(EPSFIGFILES) $(EPSSVGFILES) $(EPSPLOTFILES) $(EPSGIFFILES)

$(PNGFIGFILES): %.png: %.fig
	$(FIG2DEV) -L png $*.fig > $*.png

png: $(PNGFIGFILES)

$(JPGGIFFILES): %.jpg: %.gif
	convert $< $@

jpg: $(JPGGIFFILES)

$(PDFFIGFILES): %.pdf: %.eps %.fig
	$(FIG2DEV) -L pdf $*.fig > $*.pdf

#################################################################

$(AUXFILES): %.aux: %.tex 
	latex $*

$(BBLFILES): %.bbl: %.bib %.tex
	$(MAKE) $*.aux
	bibtex $*

bbl: $(BBLFILES)

$(DVIFILES): %.dvi: %.tex $(BBLFILES)
	latex $*
	@while grep 'Rerun to get cross-references right' $*.log > /dev/null; \
		do latex $*; done
	chmod -R a+rX $*.*

dvi: $(DVIFILES)

$(PSFILES): %.ps: %.dvi
	dvips $* -o
	chmod -R a+rX $*.*

ps: $(PSFILES)

$(PDFFILES): %.pdf: %.ps $(PDFIMAGEFILES)
	ps2pdf $(PSFILES) $(PDFFILES)
	chmod -R a+rX $*.*

pdf: $(PDFFILES)

clean: open
	-rm  -f *.bbl *.aux *.log *.toc *.cb *.ps *.png \
		*.blg *~ *.dvi *.tgz $(EPSFIGFILES) $(EPSSVGFILES) $(EPSPLOTFILES) \
		$(EPSGIFFILES) $(JPGGIFFILES) $(PNGFIGFILES) $(PDFFIGFILES)

open:
	-chmod -R a+rX . ./*

touch:
	touch *.plot *.fig *.tex *.bib

tar:
	cd ..; tar zcfv $(PWD).tgz $(PWD); chmod a+rX *; ls -la $(PWD).tgz

install: ps
	cp Makefile $(TEXMAINFILES) $(PSFILES) $(PDFFILES) \
		$(DEST) $(FIGFILES)
	chmod -R a+rX $(DEST)

xdvi: $(GOAL).dvi
	xdvi -s 0 -paper a4 $< &

rights:
	chmod a-rwx,u+rw,og+r `find . -type f`
