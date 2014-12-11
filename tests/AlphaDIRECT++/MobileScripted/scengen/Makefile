CC = g++
CC_FLAGS = -g -Wall

LIBS = -lm
INCLUDES = 

MODELS =  waypoint.o fixedwp.o brownian.o pursue.o column.o gaussmarkov.o

OBJS = $(MODELS) \
       scengen.o util.o random.o \
       spec.o modelspec.o scenspec.o \
       model.o nodegroup.o nodeman.o

TEST_OBJS = test.o util.o random.o \
       spec.o modelspec.o scenspec.o \
       model.o nodegroup.o nodeman.o \
       $(MODELS)

RELEASE = Makefile *.cc *.h README scengen

AD_HOCKEY = /usr/local/ad-hockey/ad-hockey-li

all : scengen

release :
	cp -f $(RELEASE) release

run : scengen
	./scengen

s : scengen scen-spec model-spec
	./scengen 1>s

cs : 
	rm -f s

a : 
	$(AD_HOCKEY) s

clean : 
	rm -f $(OBJS) core

realclean : 
	rm -f $(OBJS) core scengen s

wc :
	wc -l *

scengen : $(OBJS)
	$(CC) $(CC_FLAGS) $(LIBS) -o scengen $(OBJS)

test : $(TEST_OBJS)
	$(CC) $(CC_FLAGS) $(LIBS) -o test $(OBJS)

test.o : test.cc
	$(CC) $(CC_FLAGS) -c test.cc

scengen.o : scengen.cc scengen.h
	$(CC) $(CC_FLAGS) -c scengen.cc

nodeman.o : nodeman.cc nodeman.h
	$(CC) $(CC_FLAGS) -c nodeman.cc

util.o : util.cc util.h
	$(CC) $(CC_FLAGS) -c util.cc

random.o : random.cc random.h
	$(CC) $(CC_FLAGS) -c random.cc

model.o : model.cc model.h
	$(CC) $(CC_FLAGS) -c model.cc

nodegroup.o : nodegroup.cc nodegroup.h
	$(CC) $(CC_FLAGS) -c nodegroup.cc

spec.o : spec.cc spec.h
	$(CC) $(CC_FLAGS) -c spec.cc

modelspec.o : modelspec.cc modelspec.h
	$(CC) $(CC_FLAGS) -c modelspec.cc

scenspec.o : scenspec.cc scenspec.h
	$(CC) $(CC_FLAGS) -c scenspec.cc

waypoint.o : waypoint.cc waypoint.h
	$(CC) $(CC_FLAGS) -c waypoint.cc

fixedwp.o : fixedwp.cc fixedwp.h
	$(CC) $(CC_FLAGS) -c fixedwp.cc

brownian.o : brownian.cc brownian.h
	$(CC) $(CC_FLAGS) -c brownian.cc

pursue.o : pursue.cc pursue.h
	$(CC) $(CC_FLAGS) -c pursue.cc

column.o : column.cc column.h
	$(CC) $(CC_FLAGS) -c column.cc

gaussmarkov.o : gaussmarkov.cc gaussmarkov.h
	$(CC) $(CC_FLAGS) -c gaussmarkov.cc
