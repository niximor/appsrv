CXX := g++-4.9
CXXFLAGS := -std=c++14 -Wall -Wextra -pedantic-errors -g -ggdb3 -gdwarf-2 -O2 -fno-strict-aliasing -fPIC
CXXFLAGS += -I../include/
CXXFLAGS += $(CXXEF)
LDFLAGS := -pthread
LDFLAGS += -rdynamic
LDFLAGS += -ldl

SOURCES := $(shell find . -name '*.cc')
OBJS := $(SOURCES:.cc=.o)
APP_SOURCES := $(shell find . -name '*.cc' -a ! -path './handlers/*')
APP_OBJS := $(APP_SOURCES:.cc=.o)
DEPS := $(SOURCES:.cc=.d)

HANDLERS := $(shell find handlers/ -mindepth 1 -maxdepth 1 -type d)

APP := gcm-srv

all: build

build: $(OBJS) $(APP) $(HANDLERS)

$(APP): $(APP_OBJS)
	$(strip $(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@)

$(foreach file,$(DEPS),$(eval -include $(file)))

%.o: %.cc
	$(strip $(COMPILE.cpp) -MMD $< -o $@)

$(HANDLERS):
	make -w -C $@

clean:
	$(RM) -f $(OBJS) $(DEPS) $(APP)

.PHONY: all clean build $(HANDLERS)
.EXPORT_ALL_VARIABLES:
