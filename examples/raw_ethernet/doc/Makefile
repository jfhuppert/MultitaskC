VERSION=	5.2.2

DOCFILES+=	userman devman diskless freebsd sshterminal lilo+etherboot
HTMLDOC=	$(patsubst %,html/%/t1.html,$(DOCFILES))
TEXTDOC=	$(patsubst %,text/%.txt,$(DOCFILES))
DVIDOC=		$(patsubst %,dvi/%.dvi,$(DOCFILES))
PSDOC=		$(patsubst %,ps/%.ps,$(DOCFILES))

all:	$(HTMLDOC) $(TEXTDOC) $(DVIDOC) $(PSDOC)

sgml/userman.sgml:	sgml/userman.xsgml ../src/Config ../src/../Copyrights
	@[ -L ../src -a -e ../src/Config -a -e ../src/../Copyrights ] || echo 'No ../src link' || /bin/false
	m4 -P sgml/userman.xsgml > sgml/userman.sgml

html/%/t1.html:	sgml/%.sgml
	cd sgml; sgmltools -b html $*.sgml; rm -fr ../html/$*; mv $* ../html

text/%.txt:	sgml/%.sgml	
	cd sgml; sgmltools -b txt $*.sgml; mv $*.txt ../text

dvi/%.dvi:	sgml/%.sgml
	cd sgml; sgmltools -b dvi $*.sgml; mv $*.dvi ../dvi

# Make PS via DVI so we don't have to do JadeTeX twice
ps/%.ps:	dvi/%.dvi
	cd dvi; dvips -o $*.ps $*.dvi; mv -f $*.ps ../ps

tarball:
	(cd ..; tar cf /tmp/etherboot-doc-$(VERSION).tar doc)
	bzip2 -9 < /tmp/etherboot-doc-$(VERSION).tar > /tmp/etherboot-doc-$(VERSION).tar.bz2
	gzip -9 < /tmp/etherboot-doc-$(VERSION).tar > /tmp/etherboot-doc-$(VERSION).tar.gz

clean:
	rm -fr html/* text/*.txt dvi/*.dvi ps/*.ps
