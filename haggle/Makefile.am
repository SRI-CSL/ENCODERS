SUBDIRS :=
ACLOCAL_AMFLAGS =-I m4 -I /usr/share/aclocal
# Extra libraries should be compiled first
if BUNDLED_SQLITE
SUBDIRS += $(SQLITE_SUBDIR)
endif

if OS_IPHONEOS
SUBDIRS += $(OPENSSL_DIR)
endif

SUBDIRS += doc src android

EXTRA_DIST = \
	bin \
	lib \
	extlibs \
	winmobile \
	windows \
	iphone \
	macosx \
	resources \
	README \
	Android.mk

DIST_SUBDIRS = $(SUBDIRS) testsuite

dist_doc_DATA = README
docdir = doc

if DOCS

DOC_DIRS := \
	src/hagglekernel \
	src/libcpphaggle \
	src/libhaggle

docs: 
	@echo "Generating documentation"
	@for dir in ${DOC_DIRS}; do \
		pushd "$$dir"; \
		doxygen Doxyfile; \
		popd; \
	done
endif

tes%:
	$(MAKE) -C testsuite $@

clean-local:
	rm -f *~
	rm -f bin/*
	rm -f lib/*

realclean:
	hg status -i | grep "I " | awk '{print $$2}' | xargs rm -rf

dist-hook:
        # Remove all the .svn directories that may have made it into the distribution
	rm -rf `find $(distdir)/ -name .svn`
	rm -rf `find $(distdir)/extlibs -name .deps`
	rm -rf $(distdir)/bin/*
	rm -rf $(distdir)/lib/*

distcheck-hook:
	@echo "Distcheck hook! $(PWD)"
	mkdir -p $(distdir)/_build/bin
	mkdir -p $(distdir)/_build/lib
