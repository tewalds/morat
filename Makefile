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


all: castro moy pentagod

castro: \
		havannah/castro.o \
		havannah/gtpgeneral.o \
		havannah/gtpplayer.o \
		havannah/gtpsolver.o \
		havannah/solverab.o \
		havannah/solverpns.o \
		havannah/solverpns2.o \
		havannah/solverpns_tt.o \
		havannah/player.o \
		havannah/playeruct.o \
		lib/fileio.o \
		lib/gtpcommon.o \
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
		pentago/move.o \
		pentago/moveiterator.o \
		lib/fileio.o \
		lib/gtpcommon.o \
		lib/string.o \
		$(ALARM)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LOADLIBES) $(LDLIBS)

moy: \
		y/moy.o \
		y/gtpgeneral.o \
		y/gtpplayer.o \
		y/gtpsolver.o \
		y/solverab.o \
		y/solverpns.o \
		y/solverpns2.o \
		y/solverpns_tt.o \
		y/player.o \
		y/playeruct.o \
		lib/fileio.o \
		lib/gtpcommon.o \
		lib/string.o \
		lib/zobrist.o \
		$(ALARM)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LOADLIBES) $(LDLIBS)


clean:
	rm -f */*.o castro moy pentagod .Makefile

fresh: clean all

profile:
	valgrind --tool=callgrind

gendeps: .Makefile

.Makefile: # contains the actual dependencies for all the .o files above
	./gendeps.sh > .Makefile

include .Makefile
