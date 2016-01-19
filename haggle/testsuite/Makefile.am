DIST_SUBDIRS = \
	test_thread \
	test_mutex \
	test_condition \
	test_Queue \
	test_metadata \
	test_libcpphaggle \
	test_utils \
	test_dObj \
	test_kernel \
	test_security \
	test_nc \
	test_fragment


.PHONY: test $(DIST_SUBDIRS)

EXTRA_DIST = \
	hagglemain.h \
	testhlp.h

noinst_LIBRARIES = libtesthlp.a libhagglemain.a
libtesthlp_a_SOURCES = testhlp.c

AM_CFLAGS = -std=gnu99 -I../src/hagglekernel
AM_CPPFLAGS = \
	-DTRACE_ENABLE_PRINTF \
	-I$(top_srcdir)/src/hagglekernel \
	-I$(top_srcdir)/src/utils \
	-I$(top_srcdir)/src/libcpphaggle/include \
	-I$(top_srcdir)/src/libhaggle/include \
	$(XML_CPPFLAGS) $(PYCFLAGS) 

libhagglemain_a_SOURCES = hagglemain.cpp

all-local:

clean-local:
	rm -f *~ libtesthlp.a libhagglemain.a *.o

test:
	@$(MAKE) -C ..
	@$(MAKE)
	@$(MAKE) -C test_thread
	@$(MAKE) -C test_mutex
	@$(MAKE) -C test_condition
	@$(MAKE) -C test_metadata
	@$(MAKE) -C test_libcpphaggle
	@$(MAKE) -C test_Queue
	@$(MAKE) -C test_utils
	@$(MAKE) -C test_dObj
	@$(MAKE) -C test_kernel
	@$(MAKE) -C test_security
	@echo "------ Thread test suite             ------"
	@$(MAKE) test -C test_thread --no-print-directory
	@echo "------ Mutex test suite              ------"
	@$(MAKE) test -C test_mutex --no-print-directory
	@echo "------ Condition variable test suite ------"
	@$(MAKE) test -C test_condition --no-print-directory
	@echo "------ Metadata test suite           ------"
	@$(MAKE) test -C test_metadata --no-print-directory
	@echo "------ Libcpphaggle test suite       ------"
	@$(MAKE) test -C test_libcpphaggle --no-print-directory
	@echo "------ Queue test suite              ------"
	@$(MAKE) test -C test_Queue --no-print-directory
	@echo "------ Utilities test suite          ------"
	@$(MAKE) test -C test_utils --no-print-directory
	@echo "------ Data object test suite        ------"
	@$(MAKE) test -C test_dObj --no-print-directory
#	@echo "------ Haggle kernel test suite      ------"
#	@$(MAKE) test -C test_kernel --no-print-directory
	@echo "------ Haggle security test suite      ------"
	@$(MAKE) test -C test_security --no-print-directory

test_thread:
	@$(MAKE) -C ..
	@$(MAKE)
	@$(MAKE) -C test_thread
	@echo "------ Thread test suite             ------"
	@$(MAKE) test -C test_thread --no-print-directory

test_mutex:
	@$(MAKE) -C ..
	@$(MAKE)
	@$(MAKE) -C test_mutex
	@echo "------ Mutex test suite              ------"
	@$(MAKE) test -C test_mutex --no-print-directory

test_condition:
	@$(MAKE) -C ..
	@$(MAKE)
	@$(MAKE) -C test_condition
	@echo "------ Condition variable test suite ------"
	@$(MAKE) test -C test_condition --no-print-directory

test_metadata:
	@$(MAKE) -C ..
	@$(MAKE)
	@$(MAKE) -C test_metadata
	@echo "------ Metadata test suite           ------"
	@$(MAKE) test -C test_metadata --no-print-directory

test_libcpphaggle:
	@$(MAKE) -C ..
	@$(MAKE)
	@$(MAKE) -C test_libcpphaggle
	@echo "------ Libcpphaggle test suite       ------"
	@$(MAKE) test -C test_libcpphaggle --no-print-directory

test_Queue:
	@$(MAKE) -C ..
	@$(MAKE)
	@$(MAKE) -C test_Queue
	@echo "------ Queue test suite              ------"
	@$(MAKE) test -C test_Queue --no-print-directory

test_utils:
	@$(MAKE) -C ..
	@$(MAKE)
	@$(MAKE) -C test_utils
	@echo "------ Utilities test suite          ------"
	@$(MAKE) test -C test_utils --no-print-directory

test_dObj:
	@$(MAKE) -C ..
	@$(MAKE)
	@$(MAKE) -C test_dObj
	@echo "------ Data object test suite        ------"
	@$(MAKE) test -C test_dObj --no-print-directory

test_kernel:
	@$(MAKE) -C ..
	@$(MAKE)
	@$(MAKE) -C test_kernel
	@echo "------ Haggle kernel test suite      ------"
	@$(MAKE) test -C test_kernel --no-print-directory

test_security:
	@$(MAKE) -C ..
	@$(MAKE)
	@$(MAKE) -C test_security
	@echo "------ Haggle security test suite      ------"
	@$(MAKE) test -C test_security --no-print-directory
