.PHONY: clean fresh run gendeps

#CXX=clang++
CPPFLAGS += -std=c++0x -Wall
LDFLAGS  += -pthread

SYS := $(shell $(CXX) -dumpmachine)
ifneq (, $(findstring linux, $(SYS)))
	ALARM = lib/alarm.o
else
	ALARM = lib/alarm-timer.o
endif

ifdef DEBUG
	CPPFLAGS += -g3
else
	CPPFLAGS += -O3 -funroll-loops

	ifneq (, $(findstring darwin, $(SYS)))
		CPPFLAGS += -m64
		LDFLAGS += -m64
	else
		CPPFLAGS += -march=native
	endif
endif


all: castro chex moy trex pentagod

castro: \
		havannah/castro.o \
		havannah/agentmcts.o \
		havannah/agentmctsthread.o \
		havannah/agentpns.o \
		havannah/gtpgeneral.o \
		havannah/gtpagent.o \
		lib/fileio.o \
		lib/gtpcommon.o \
		lib/outcome.o \
		lib/string.o \
		lib/zobrist.o \
		$(ALARM)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LOADLIBES) $(LDLIBS)

pentagod: \
		pentago/pentagod.o \
		pentago/agentab.o \
		pentago/agentmcts.o \
		pentago/agentmctsthread.o \
		pentago/agentpns.o \
		pentago/board.o \
		pentago/gtpgeneral.o \
		pentago/gtpagent.o \
		pentago/moveiterator.o \
		lib/fileio.o \
		lib/gtpcommon.o \
		lib/outcome.o \
		lib/string.o \
		$(ALARM)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LOADLIBES) $(LDLIBS)

moy: \
		y/moy.o \
		y/agentmcts.o \
		y/agentmctsthread.o \
		y/agentpns.o \
		y/gtpagent.o \
		y/gtpgeneral.o \
		lib/fileio.o \
		lib/gtpcommon.o \
		lib/outcome.o \
		lib/string.o \
		lib/zobrist.o \
		$(ALARM)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LOADLIBES) $(LDLIBS)

chex: \
		hex/chex.o \
		hex/agentmcts.o \
		hex/agentmctsthread.o \
		hex/agentpns.o \
		hex/gtpagent.o \
		hex/gtpgeneral.o \
		lib/fileio.o \
		lib/gtpcommon.o \
		lib/outcome.o \
		lib/string.o \
		lib/zobrist.o \
		$(ALARM)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LOADLIBES) $(LDLIBS)

trex: \
		rex/trex.o \
		rex/agentmcts.o \
		rex/agentmctsthread.o \
		rex/agentpns.o \
		rex/gtpagent.o \
		rex/gtpgeneral.o \
		lib/fileio.o \
		lib/gtpcommon.o \
		lib/string.o \
		lib/zobrist.o \
		$(ALARM)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LOADLIBES) $(LDLIBS)

clean:
	rm -f */*.o castro moy pentagod chex trex .Makefile

fresh: clean all

profile:
	valgrind --tool=callgrind

gendeps: .Makefile

.Makefile: # contains the actual dependencies for all the .o files above
	./gendeps.sh > .Makefile

include .Makefile
