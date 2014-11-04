CXX := g++-4.9
CXXFLAGS := -std=c++14 -Wall -Wextra -pedantic-errors -g -ggdb3 -gdwarf-2 -O2
CXXFLAGS += -I../include/
CXXFLAGS += $(CXXEF)
LDFLAGS :=

SOURCES := $(shell find . -name '*.cc')
OBJS := $(SOURCES:.cc=.o)
DEPS := $(SOURCES:.cc=.d)

APP := gcm-srv

all: build

build: $(APP)

$(APP): $(OBJS)
	$(strip $(LINK.cpp) $^ -o $@)

$(foreach file,$(DEPS),$(eval -include $(file)))

%.o: %.cc
	$(strip $(COMPILE.cpp) -MMD $< -o $@)

clean:
	$(RM) -f $(OBJS) $(DEPS) $(APP)

.PHONY: all clean
