EXTRA_DIST = manual.rst \
             manual.css \
             html4css1.css

doc_DATA =

if BUILD_RST
doc_DATA += manual.html

EXTRA_DIST += manual.html

manual.html: manual.rst manual.css
	$(AM_V_GEN) $(RST2HTML) -d --strict --stylesheet-path manual.css $< $@
endif BUILD_RST
