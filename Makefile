# Katalyn C++ build
#
# Common commands:
#   make
#   make test
#   make install
#   make clean

CXX ?= c++
PREFIX ?= /usr/local
DESTDIR ?=

BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj
GEN_DIR := $(BUILD_DIR)/generated
GEN_HEADER := $(GEN_DIR)/stdlib.hpp
TARGET := $(BUILD_DIR)/katalyn

SOURCES := \
	main.cpp \
	compiler.cpp \
	narivm.cpp \
	lib/tiny-process-library/process.cpp \
	lib/tiny-process-library/process_unix.cpp
OBJECTS := $(SOURCES:%.cpp=$(OBJ_DIR)/%.o)
DEPFILES := $(OBJECTS:.o=.d)

CPPFLAGS += -I$(GEN_DIR) -Ilib/tiny-process-library -MMD -MP
CXXFLAGS ?= -O2
CXXFLAGS += -std=c++17 -Wall -Wextra -Wpedantic
LDLIBS += -pthread

.PHONY: all test install clean run

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@mkdir -p $(dir $@)
	$(CXX) $(LDFLAGS) $(OBJECTS) $(LDLIBS) -o $@

$(OBJ_DIR)/%.o: %.cpp $(GEN_HEADER)
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(GEN_HEADER): stdlib.kat Makefile
	@mkdir -p $(dir $@)
	@{ \
		printf '%s\n' '#pragma once' '' 'namespace katalyn {'; \
		printf '%s' 'inline constexpr const char standard_library[] = R"KATALYN_STDLIB('; \
		cat $<; \
		printf '%s\n' ')KATALYN_STDLIB";' '}'; \
	} > $@.tmp
	@mv $@.tmp $@

test: $(TARGET)
	@echo "Running Katalyn compatibility tests"
	@$(TARGET) -n -a 'print("hello", 2 + 3);' | grep -qx 'hello5'
	@$(TARGET) -n tests/core.kat | grep -qx 'core-ok'
	@$(TARGET) -n tests/nil_truth.kat | grep -qx 'nil-truth-ok'
	@$(TARGET) -n tests/short_circuit.kat | grep -qx 'short-circuit-ok'
	@$(TARGET) -n tests/precedence.kat | grep -qx 'precedence-ok'
	@$(TARGET) -n tests/operators_values.kat | grep -qx 'operators-values-ok'
	@$(TARGET) tests/unicode.kat | grep -qx 'unicode-ok'
	@$(TARGET) -n tests/tables.kat | grep -qx 'tables-ok'
	@$(TARGET) -n tests/control_flow.kat | grep -qx 'control-flow-ok'
	@$(TARGET) -n tests/functions_scope.kat | grep -qx 'functions-scope-ok'
	@$(TARGET) tests/stdlib.kat | grep -qx 'stdlib-ok'
	@$(TARGET) tests/print_arr.kat | diff -u tests/expected/print_arr.txt -
	@$(TARGET) -n tests/io_import.kat alpha beta | grep -qx 'io-import-ok'
	@$(RM) $(BUILD_DIR)/katalyn-test-output.txt
	@$(TARGET) -a 'print(ceil(2.2));' | grep -qx '3'
	@echo "All tests passed."

install: $(TARGET)
	install -d "$(DESTDIR)$(PREFIX)/bin"
	install -m 755 $(TARGET) "$(DESTDIR)$(PREFIX)/bin/katalyn"
	@echo "Installed $(DESTDIR)$(PREFIX)/bin/katalyn"

run: $(TARGET)
	@if [ -z "$(SCRIPT)" ]; then \
		echo 'Usage: make run SCRIPT=path/to/script.kat'; \
		exit 1; \
	fi
	@$(TARGET) "$(SCRIPT)"

clean:
	rm -rf $(BUILD_DIR)

-include $(DEPFILES)
