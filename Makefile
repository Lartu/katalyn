# LSPL C++ build
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
TARGET := $(BUILD_DIR)/lspl
RUNTIME_TEST := $(BUILD_DIR)/runtime-isolation-test
TIGER_TARGET := $(BUILD_DIR)/lspl-tiger-ppc
TIGER_CXX ?= ppc-ld64-g++
TIGER_CXXFLAGS ?= -std=c++17 -O2

SOURCES := \
	main.cpp \
	compiler.cpp \
	narivm.cpp \
	lib/tiny-process-library/process.cpp \
	lib/tiny-process-library/process_unix.cpp
OBJECTS := $(SOURCES:%.cpp=$(OBJ_DIR)/%.o)
DEPFILES := $(OBJECTS:.o=.d)

CPPFLAGS += -I. -I$(GEN_DIR) -Ilib/tiny-process-library -MMD -MP
OPTFLAGS ?= -O2 -DNDEBUG
LTOFLAGS ?= -flto
CXXFLAGS ?= $(OPTFLAGS)
CXXFLAGS += $(LTOFLAGS) -std=c++17 -Wall -Wextra -Wpedantic
LDFLAGS += $(LTOFLAGS)
LDLIBS += -pthread

.PHONY: all test benchmark tiger-ppc install clean run

all: $(TARGET)

# Cross-compile for 32-bit PowerPC Mac OS X 10.4. This target must be run on
# x86_64 Linux with the PPC Tiger GCC/ld64 toolchain in PATH.
tiger-ppc: $(TIGER_TARGET)

$(TIGER_TARGET): $(SOURCES) $(GEN_HEADER)
	@mkdir -p $(dir $@)
	$(TIGER_CXX) $(TIGER_CXXFLAGS) -I$(GEN_DIR) -Ilib/tiny-process-library \
		$(SOURCES) -pthread -o $@

$(TARGET): $(OBJECTS)
	@mkdir -p $(dir $@)
	$(CXX) $(LDFLAGS) $(OBJECTS) $(LDLIBS) -o $@

$(OBJ_DIR)/%.o: %.cpp $(GEN_HEADER)
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(GEN_HEADER): stdlib.lspl Makefile
	@mkdir -p $(dir $@)
	@{ \
		printf '%s\n' '#pragma once' '' 'namespace lspl {'; \
		printf '%s' 'inline constexpr const char standard_library[] = R"LSPL_STDLIB('; \
		cat $<; \
		printf '%s\n' ')LSPL_STDLIB";' '}'; \
	} > $@.tmp
	@mv $@.tmp $@

$(RUNTIME_TEST): tests/runtime_isolation.cpp $(OBJ_DIR)/narivm.o \
	$(OBJ_DIR)/lib/tiny-process-library/process.o \
	$(OBJ_DIR)/lib/tiny-process-library/process_unix.o
	$(CXX) $(LDFLAGS) $(CPPFLAGS) $(CXXFLAGS) $^ $(LDLIBS) -o $@

test: $(TARGET) $(RUNTIME_TEST)
	@echo "Running LSPL compatibility tests"
	@$(RUNTIME_TEST) >/dev/null 2>&1
	@$(TARGET) --help | grep -q '^Usage: lspl '
	@$(TARGET) --version | grep -q 'Programming Language'
	@$(TARGET) -v | grep -q 'Programming Language'
	@$(TARGET) --version | grep -Eq '^Built on .+ at [0-9]{2}:[0-9]{2}:[0-9]{2}\.$$'
	@! $(TARGET) --version | grep -Eq '^Built on [A-Z][a-z]{2}  [0-9]'
	@$(TARGET) --version | grep -q '^This is LSPL version .*, running on the NariVM\.$$'
	@$(TARGET) -n -a 'print("hello", 2 + 3);' | grep -qx 'hello5'
	@$(TARGET) -n tests/core.lspl | grep -qx 'core-ok'
	@$(TARGET) -n tests/nil_truth.lspl | grep -qx 'nil-truth-ok'
	@$(TARGET) -n tests/short_circuit.lspl | grep -qx 'short-circuit-ok'
	@$(TARGET) -n tests/precedence.lspl | grep -qx 'precedence-ok'
	@$(TARGET) -n tests/operators_values.lspl | grep -qx 'operators-values-ok'
	@$(TARGET) tests/unicode.lspl | grep -qx 'unicode-ok'
	@$(TARGET) -n tests/tables.lspl | grep -qx 'tables-ok'
	@$(TARGET) -n tests/control_flow.lspl | grep -qx 'control-flow-ok'
	@$(TARGET) tests/errors.lspl | grep -qx 'errors-ok'
	@$(TARGET) tests/bytes.lspl | grep -qx 'bytes-ok'
	@$(TARGET) tests/filesystem.lspl | grep -qx 'filesystem-ok'
	@$(TARGET) tests/datetime.lspl | grep -qx 'datetime-ok'
	@$(TARGET) tests/concurrency.lspl | grep -qx 'concurrency-ok'
	@$(TARGET) -n -a '$$t: datetime(); print(len($$t) = 10);' | grep -qx '1'
	@$(TARGET) -n tests/functions_scope.lspl | grep -qx 'functions-scope-ok'
	@$(TARGET) tests/stdlib.lspl | grep -qx 'stdlib-ok'
	@$(TARGET) -a '$$j: parse_json("{\"x\":1}"); print($$j{x});' | grep -qx '1'
	@$(TARGET) -n tests/json.lspl | grep -qx 'json-ok'
	@printf 'abcdef' | $(TARGET) -n tests/stdin.lspl | grep -qx 'abc|def'
	@printf 'color=purple&message=Hello+CGI' | env \
		LSPL_CGI_TEST=present REQUEST_METHOD=POST \
		QUERY_STRING='page=2&search=LSPL+CGI' \
		CONTENT_TYPE='application/x-www-form-urlencoded; charset=UTF-8' \
		CONTENT_LENGTH=30 HTTP_ACCEPT=application/json PATH_INFO=/demo \
		$(TARGET) -n tests/cgi.lspl | grep -qx 'cgi-ok'
	@$(TARGET) -n tests/cgi_response.lspl | tr -d '\r' | \
		diff -u tests/expected/cgi_response.txt -
	@! $(TARGET) -n -a 'json_decode("[1,");' >/dev/null 2>&1
	@! printf '"\377"' | $(TARGET) -n -a 'json_decode(read_stdin());' >/dev/null 2>&1
	@! printf '\377' | $(TARGET) -n -a 'json_encode(read_stdin());' >/dev/null 2>&1
	@! printf 'short' | env REQUEST_METHOD=POST CONTENT_LENGTH=6 \
		$(TARGET) -n -a 'cgi_request(10);' >/dev/null 2>&1
	@! printf '12345' | env REQUEST_METHOD=POST CONTENT_LENGTH=5 \
		$(TARGET) -n -a 'cgi_request(4);' >/dev/null 2>&1
	@$(TARGET) tests/print_arr.lspl | diff -u tests/expected/print_arr.txt -
	@$(TARGET) -n tests/io_import.lspl alpha beta | grep -qx 'io-import-ok'
	@$(TARGET) -n tests/magic_paths.lspl "$(CURDIR)" "$(CURDIR)/tests/magic_paths.lspl" \
		"$(CURDIR)/tests" | \
		grep -qx 'magic-paths-ok'
	@$(TARGET) -n -a 'print(!is($$_scriptpath) && !is($$_scriptdir) && len($$_wdir) > 0);' | grep -qx '1'
	@printf 'print(!is($$_scriptpath) && !is($$_scriptdir) && len($$_wdir) > 0);' | \
		$(TARGET) -n -s | grep -qx '1'
	@$(RM) $(BUILD_DIR)/lspl-test-output.txt
	@$(TARGET) -a 'print(ceil(2.2));' | grep -qx '3'
	@echo "All tests passed."

benchmark: $(TARGET)
	@echo "Arithmetic and variable access"
	@/usr/bin/time -p $(TARGET) -n benchmarks/arithmetic.lspl >/dev/null
	@echo "Function calls"
	@/usr/bin/time -p $(TARGET) -n benchmarks/functions.lspl >/dev/null
	@echo "Table reads and writes"
	@/usr/bin/time -p $(TARGET) -n benchmarks/tables.lspl >/dev/null
	@echo "Actor message round trips"
	@/usr/bin/time -p $(TARGET) -n benchmarks/messages.lspl >/dev/null

install: $(TARGET)
	install -d "$(DESTDIR)$(PREFIX)/bin"
	install -m 755 $(TARGET) "$(DESTDIR)$(PREFIX)/bin/lspl"
	@echo "Installed $(DESTDIR)$(PREFIX)/bin/lspl"

run: $(TARGET)
	@if [ -z "$(SCRIPT)" ]; then \
		echo 'Usage: make run SCRIPT=path/to/script.lspl'; \
		exit 1; \
	fi
	@$(TARGET) "$(SCRIPT)"

clean:
	rm -rf $(BUILD_DIR)

-include $(DEPFILES)
