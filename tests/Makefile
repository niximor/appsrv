# This test suite requires bandit testing framework.
# You can get it here: https://github.com/joakimkarlsson/bandit/

CXX := g++-4.9
CXXFLAGS := -std=c++14 -Wall -Werror -Wextra -pedantic-errors -g -ggdb3 -gdwarf-2
CXXFLAGS += -I../include/
CXXFLAGS += $(CXXEF)

LDFLAGS := -pthread
SOURCES := $(shell find . -iname '*.cc')
OBJS := $(SOURCES:.cc=.o)
DEPS := $(SOURCES:.cc=.d)
APP := tester

all: build test

build: $(APP)

test: $(APP)
	./$(APP)

$(APP): $(OBJS)
	$(strip $(LINK.cpp) $^ -o $@)

$(foreach file,$(DEPS),$(eval -include $(file)))

%.o: %.cc
	$(strip $(COMPILE.cpp) -MMD $< -o $@)

clean:
	rm -f $(OBJS) $(DEPS) $(APP)

.PHONY: build test all clean