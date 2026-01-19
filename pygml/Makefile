# Linux-only: build a shared library that embeds Python via pybind11
# Usage:
#   make                  (uses python3.8 by default)
#   make PYTHON=python3.8
#   make clean

NAME   := pygml
TARGET := lib$(NAME).so

PYTHON ?= python3.8
CXX    ?= g++

SRC := pygml.cpp
OBJ := $(SRC:.cpp=.o)

# Python compile/link flags (3.8+ supports --embed; fallback if not)
PY_CFLAGS  := $(shell $(PYTHON)-config --includes)
PY_LDFLAGS := $(shell $(PYTHON)-config --ldflags --embed 2>/dev/null || $(PYTHON)-config --ldflags)

# pybind11 include flags (installed via pip)
PYBIND11_CFLAGS := $(shell $(PYTHON) -m pybind11 --includes 2>/dev/null)

CXXFLAGS ?= -O2 -fPIC -std=c++17
LDFLAGS  ?= -shared

# If you want the .so to find bundled libs next to itself, keep this:
# (harmless if you don't bundle anything)
RPATH_ORIGIN := -Wl,-rpath,'$$ORIGIN'

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(LDFLAGS) -o $@ $^ $(PY_LDFLAGS) $(RPATH_ORIGIN)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(PY_CFLAGS) $(PYBIND11_CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)
