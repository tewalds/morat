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


all: morat-gomoku morat-havannah morat-hex morat-pentago morat-rex morat-y

test: \
		lib/test.o \
		lib/fileio.o \
		lib/move_test.o \
		lib/outcome.o \
		lib/outcome_test.o \
		lib/sgf_test.o \
		lib/string.o \
		lib/string_test.o \
		lib/timecontrol_test.o \
		lib/zobrist.o \
		gomoku/agentmcts.o \
		gomoku/agentmctsthread.o \
		gomoku/agentmcts_test.o \
		gomoku/agentpns.o \
		gomoku/agentpns_test.o \
		gomoku/board.o \
		gomoku/board_test.o \
		havannah/agentmcts.o \
		havannah/agentmctsthread.o \
		havannah/agentmcts_test.o \
		havannah/agentpns.o \
		havannah/agentpns_test.o \
		havannah/board.o \
		havannah/board_test.o \
		havannah/lbdist_test.o \
		hex/agentmcts.o \
		hex/agentmctsthread.o \
		hex/agentmcts_test.o \
		hex/agentpns.o \
		hex/agentpns_test.o \
		hex/board.o \
		hex/board_test.o \
		pentago/agentmcts.o \
		pentago/agentmctsthread.o \
		pentago/agentmcts_test.o \
		pentago/agentpns.o \
		pentago/agentpns_test.o \
		pentago/board.o \
		rex/agentmcts.o \
		rex/agentmctsthread.o \
		rex/agentmcts_test.o \
		rex/agentpns.o \
		rex/agentpns_test.o \
		rex/board.o \
		rex/board_test.o \
		y/agentmcts.o \
		y/agentmctsthread.o \
		y/agentmcts_test.o \
		y/agentpns.o \
		y/agentpns_test.o \
		y/board.o \
		y/board_test.o \
		$(ALARM)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LOADLIBES) $(LDLIBS)
	./test

morat-gomoku: \
		gomoku/main.o \
		gomoku/agentmcts.o \
		gomoku/agentmctsthread.o \
		gomoku/agentpns.o \
		gomoku/board.o \
		gomoku/gtpgeneral.o \
		gomoku/gtpagent.o \
		lib/fileio.o \
		lib/gtpcommon.o \
		lib/outcome.o \
		lib/string.o \
		lib/zobrist.o \
		$(ALARM)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LOADLIBES) $(LDLIBS)

morat-havannah: \
		havannah/main.o \
		havannah/agentmcts.o \
		havannah/agentmctsthread.o \
		havannah/agentpns.o \
		havannah/board.o \
		havannah/gtpgeneral.o \
		havannah/gtpagent.o \
		lib/fileio.o \
		lib/gtpcommon.o \
		lib/outcome.o \
		lib/string.o \
		lib/zobrist.o \
		$(ALARM)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LOADLIBES) $(LDLIBS)

morat-pentago: \
		pentago/main.o \
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

morat-y: \
		y/main.o \
		y/agentmcts.o \
		y/agentmctsthread.o \
		y/agentpns.o \
		y/board.o \
		y/gtpagent.o \
		y/gtpgeneral.o \
		lib/fileio.o \
		lib/gtpcommon.o \
		lib/outcome.o \
		lib/string.o \
		lib/zobrist.o \
		$(ALARM)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LOADLIBES) $(LDLIBS)

morat-hex: \
		hex/main.o \
		hex/agentmcts.o \
		hex/agentmctsthread.o \
		hex/agentpns.o \
		hex/board.o \
		hex/gtpagent.o \
		hex/gtpgeneral.o \
		lib/fileio.o \
		lib/gtpcommon.o \
		lib/outcome.o \
		lib/string.o \
		lib/zobrist.o \
		$(ALARM)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LOADLIBES) $(LDLIBS)

morat-rex: \
		rex/main.o \
		rex/agentmcts.o \
		rex/agentmctsthread.o \
		rex/agentpns.o \
		rex/board.o \
		rex/gtpagent.o \
		rex/gtpgeneral.o \
		lib/fileio.o \
		lib/gtpcommon.o \
		lib/outcome.o \
		lib/string.o \
		lib/zobrist.o \
		$(ALARM)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LOADLIBES) $(LDLIBS)

clean:
	rm -f */*.o test morat-havannah morat-hex morat-pentago morat-rex morat-y .Makefile

fresh: clean all

profile:
	valgrind --tool=callgrind

gendeps: .Makefile

.Makefile: # contains the actual dependencies for all the .o files above
	./gendeps.sh > .Makefile

include .Makefile
