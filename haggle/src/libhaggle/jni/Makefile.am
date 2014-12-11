SUBDIRS = java/org/haggle native .
JARFILE = org.haggle.jar
MANIFEST = Manifest.txt
haggledir = $(datadir)/java
haggle_DATA = $(JARFILE)
EXTRA_DIST = $(MANIFEST) README Android.mk
CLASSFILES = $(shell find java -name *.class -print)

classes.list: $(SUBDIRS) $(CLASSFILES)
	@echo $(CLASSFILES)
	(cd java; find . -name "*.class" -print > ../$@)

sources.list: $(SUBDIRS) $(SOURCEFILES)
	@echo $(SOURCEFILES)
	(cd java; find . -name "*.java" -print > ../$@)

$(JARFILE): classes.list sources.list $(MANIFEST)
	@echo "Creating Jar file"
	(cd java; jar cfm ../$@ ../$(MANIFEST) @../classes.list @../sources.list)
	cp -f $@ $(top_srcdir)/lib/

clean-local:
	rm -f *~
	rm -f $(JARFILE)
	rm -f classes.list
