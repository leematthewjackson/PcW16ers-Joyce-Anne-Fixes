# Standalone regression tests. Needs the SDL 1.2 headers only
# (Debian/Ubuntu: libsdl1.2-compat-dev or libsdl1.2-dev).

CXX      ?= g++
CXXFLAGS ?= -g -O0 -Wall
SDL_CFLAGS := $(shell sdl-config --cflags)

check: test_pckeymap
	./test_pckeymap

test_pckeymap: test_pckeymap.cxx ../bin/PcKeyMap.hxx
	$(CXX) $(CXXFLAGS) $(SDL_CFLAGS) -o $@ test_pckeymap.cxx

clean:
	rm -f test_pckeymap

.PHONY: check clean
