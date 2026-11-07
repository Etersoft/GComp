# Tests are built as Windows PE with winegcc and run under Wine.
WINEGXX ?= wineg++
WINE ?= wine
SDK ?=
CXXFLAGS = -b i686-windows -Iinclude -Wno-non-pod-varargs
LDFLAGS = $(if $(SDK),-L$(SDK))

check: build/basic.exe
	$(WINE) build/basic.exe

build/basic.exe: tests/basic.cpp include/*.h
	mkdir -p build
	$(WINEGXX) $(CXXFLAGS) tests/basic.cpp $(LDFLAGS) -o $@

clean:
	rm -rf build
