# This test suite requires bandit testing framework.
# You can get it here: https://github.com/joakimkarlsson/bandit/

CXX := g++-4.9
CXXFLAGS := -std=c++14 -Wall -Werror -Wextra -pedantic-errors -g -ggdb3 -gdwarf-2
CXXFLAGS += -I../include/
CXXFLAGS += -fdiagnostics-color=always
CXXFLAGS += $(CXXEF)

LDFLAGS := -pthread
SOURCES := $(shell find . -iname '*.cc')
APP_OBJS := $(SOURCES:.cc=.o)

HEADER_PATH := ../include/gcm/
GEN_DIR := ./target/

APP := tester
HEADERS := $(subst $(HEADER_PATH),,$(shell find $(HEADER_PATH) -iname '*.h'))
HEAD_GEN_SOURCE := $(addprefix $(GEN_DIR),$(HEADERS:.h=.gen.cc))
HEAD_GEN_SOURCE2 := $(addprefix $(GEN_DIR),$(HEADERS:.h=.gen2.cc))
HEAD_OBJS := $(HEAD_GEN_SOURCE:.cc=.o) $(HEAD_GEN_SOURCE2:.cc=.o)
HEAD_DIRS := $(addsuffix .,$(dir $(HEAD_OBJS)))

OBJS := $(APP_OBJS) $(HEAD_OBJS)
DEPS := $(OBJS:.o=.d)
GEN_SOURCES := $(HEAD_GEN_SOURCE) $(HEAD_GEN_SOURCE2)

# Execute whole test suite.
all: build test

# Build all that should be built.
build: $(HEAD_DIRS) $(HEAD_GEN_SOURCE) $(HEAD_GEN_SOURCE2) $(APP)

# What to test
test: test-heads test-code

# Runs binary tests
test-code: $(APP)
	./$(APP)

# Link test app that tests written unit tests.
$(APP): $(APP_OBJS)
	$(strip $(LINK.cpp) $^ -o $@)

# Test whether headers are correctly written by trying to compile
# simple .cc files which includes only single header.
test-heads: $(HEAD_OBJS)

# Include dependencies
$(foreach file,$(DEPS),$(eval -include $(file)))

# Create directories
%/.:
	mkdir -p $@

# Compile .cc to .o
%.o: %.cc
	$(strip $(COMPILE.cpp) -MMD $< -o $@)

# Compile test header object, but do not print the g++ command.
# Instead, print pretty message of what is being done.
%.gen.o: %.gen.cc
	@echo "Testing header $<"
	@$(strip $(COMPILE.cpp) -MMD $< -o $@)

%.gen2.o: %.gen2.cc
	@echo "Testing header $< for multiple inclusion."
	@$(strip $(COMPILE.cpp) -MMD $< -o $@)

# Generate source for header test with single inclusion
# Tests whether header includes all it's requirements.
$(GEN_DIR)%.gen.cc: $(HEADER_PATH)%.h
	@echo '#include "$<"\n' > $@

# Generate source for header test with double inclusion
# Tests whether header has protection against multiple includes.
$(GEN_DIR)%.gen2.cc: $(HEADER_PATH)%.h
	@echo '#include "$<"\n#include "$<"\n' > $@

# Do the cleanup
clean:
	rm -f $(OBJS) $(DEPS) $(APP) $(GEN_SOURCES)

.PHONY: all build test test-code test-heads clean