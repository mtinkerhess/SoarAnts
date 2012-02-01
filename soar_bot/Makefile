CC=g++
CFLAGS=-O3 -funroll-loops -c
INCLUDEFLAGS=-I $$SOAR_HOME/include/
LDFLAGS=-O2 -lm
SOARFLAGS=-L$$SOAR_HOME/lib/ -lClientSML -lElementXML -lConnectionSML
SOURCES=Bot.cc MyBot.cc State.cc
OBJECTS=$(addsuffix .o, $(basename ${SOURCES}))
EXECUTABLE=SoarBot

#Uncomment the following to enable debugging
#CFLAGS+=-g -DDEBUG

all: $(OBJECTS) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(SOARFLAGS) $(OBJECTS) -o $@

.cc.o: *.h
	$(CC) $(INCLUDEFLAGS) $(CFLAGS) $< -o $@

clean: 
	-rm -f ${EXECUTABLE} ${OBJECTS} *.d
	-rm -f debug.txt

.PHONY: all clean

