OBJS := $(patsubst src/%.cpp, build/%_cxx.o, $(wildcard src/*.cpp)) $(patsubst src/%.S, build/%_asm.o, $(wildcard src/*.S))
TESTS := $(patsubst test/%.cpp, bin/test/%, $(wildcard test/*.cpp))
INCLUDES := $(wildcard include/suika/*.hpp)
AS := as
ASFLAGS :=
CC := gcc
CXX := g++
CFLAGS := -Wall -Wextra -Wno-unused-function
CXXFLAGS := $(CFLAGS) -std=c++1z

all: release

.PHONY: debug release clean

debug: CFLAGS += -g
debug: CXXFLAGS += -g
debug: bin/libsuika.a

release: CFLAGS += -Ofast -flto -march=native -D_NDEBUG
release: CXXFLAGS += -Ofast -flto -march=native -D_NDEBUG
release: bin/libsuika.a

clean:
	-rm -f build/*

bin/libsuika.a: $(OBJS) | bin
	ar rcs bin/libsuika.a $(OBJS)

test: $(TESTS)

bin/test: bin
	-mkdir -p bin/test

bin:
	-mkdir -p bin

build:
	-mkdir -p build

bin/test/%: test/%.cpp bin/libsuika.a | bin/test
	$(CXX) -o $@ $< $(CXXFLAGS) -g -Iinclude/ -Lbin/ -lsuika -lpthread

build/%_cxx.o: src/%.cpp $(INCLUDES) | build
	$(CXX) -o $@ $(CXXFLAGS) -Iinclude/ -c $<

build/%_asm.o: src/%.S | build
	$(AS) -o $@ $(ASFLAGS) $<
