CCB_PYTHON_FILE=../python/ccb.py

PY4A := ../py4a
PY4A_INC := $(PY4A)/Include
PY4A_LIB := $(PY4A)

PYCFLAGS_LINUX=$(filter-out -Wstrict-prototypes,$(shell echo `python2.7-config --cflags`))
PYCFLAGS_ANDROID=-I$(PY4A) -I$(PY4A_INC) -fno-strict-aliasing -g -O2 -DNDEBUG -g -fwrapv -O2 -Wall

PYLDFLAGS_LINUX=$(shell echo `python2.7-config --ldflags`)
PYLDFLAGS_ANDROID=-ldl -lm -lpython2.7 -Xlinker -export-dynamic -Wl,-O1 -Wl,-Bsymbolic-functions

LIBS=-lpbc -lgmp

CC_ANDROID=arm-linux-androideabi-g++
CFLAGS_ANDROID=-c -g -Wall -O3 -fwrapv $(PYCFLAGS_ANDROID) -I../../haggle/src/libcpphaggle/include -DOS_ANDROID -DCCBTEST
LDFLAGS_ANDROID=-g -lstdc++ $(LIBS) -L$(PY4A_LIB) -L$(PY4A_LIB)/Lib $(PYLDFLAGS_ANDROID)

CC_LINUX=g++
CFLAGS_LINUX=-c -g -Wall -O3 -fwrapv $(PYCFLAGS_LINUX) -I../../haggle/src/libcpphaggle/include  -DCCBTEST
LDFLAGS_LINUX=-g -lstdc++ $(LIBS) $(PYLDFLAGS_LINUX)

SOURCES=String.cpp CharmCryptoBridge.cpp test.cpp
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=test

linux:
	$(CC_LINUX) $(CFLAGS_LINUX) String.cpp -o String.o
	$(CC_LINUX) $(CFLAGS_LINUX) CharmCryptoBridge.cpp -o CharmCryptoBridge.o
	$(CC_LINUX) $(CFLAGS_LINUX) test.cpp -o test.o
	$(CC_LINUX) String.o CharmCryptoBridge.o test.o $(LDFLAGS_LINUX) -Wl,--format=binary -Wl,$(CCB_PYTHON_FILE) -Wl,--format=default -o test

android:
	$(CC_ANDROID) $(CFLAGS_ANDROID) String.cpp -o String.o
	$(CC_ANDROID) $(CFLAGS_ANDROID) CharmCryptoBridge.cpp -o CharmCryptoBridge.o
	$(CC_ANDROID) $(CFLAGS_ANDROID) test.cpp -o test.o
	$(CC_ANDROID) String.o CharmCryptoBridge.o test.o $(LDFLAGS_ANDROID) -Wl,--format=binary -Wl,$(CCB_PYTHON_FILE) -Wl,--format=default -o test

clean:
	rm -rf *.o *.pyc $(EXECUTABLE) __pycache__ charm_state.pickle

run: $(EXECUTABLE)
	./$(EXECUTABLE)
