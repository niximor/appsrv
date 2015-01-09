CXX := g++-4.9
CXXFLAGS := -std=c++14 -Wall -Wextra -pedantic-errors -g -ggdb3 -gdwarf-2 -fno-strict-aliasing -fPIC
CXXFLAGS += -I../include/
CXXFLAGS += -fdiagnostics-color=always
CXXFLAGS += $(CXXEF)
CXXFLAGS += -fdiagnostics-color=always
LDFLAGS := -pthread
LDFLAGS += -rdynamic
LDFLAGS += -ldl

SOURCES := $(shell find . -name '*.cc')
OBJS := $(SOURCES:.cc=.o)
APP_SOURCES := $(shell find . -name '*.cc' -a ! -path './handlers/*' -a ! -path './modules/*')
APP_OBJS := $(APP_SOURCES:.cc=.o)
DEPS := $(SOURCES:.cc=.d)

HANDLERS := $(shell find handlers/ -mindepth 1 -maxdepth 1 -type d)
MODULES := $(shell find modules/ -mindepth 1 -maxdepth 1 -type d)

APP := gcm-srv

all: build

build: $(OBJS) $(APP) $(HANDLERS) $(MODULES)

$(APP): $(APP_OBJS)
	$(strip $(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@)

$(foreach file,$(DEPS),$(eval -include $(file)))

%.o: %.cc
	$(strip $(COMPILE.cpp) -MMD $< -o $@)

$(HANDLERS):
	$(MAKE) -w -C $@

$(MODULES):
	$(MAKE) -w -C $@

clean:
	$(RM) -f $(OBJS) $(DEPS) $(APP)
	$(foreach mod,$(MODULES),$(MAKE) -w -C $(mod) clean;)
	$(foreach handler,$(HANDLERS),$(MAKE) -w -C $(handler) clean;)

.PHONY: all clean build $(HANDLERS) $(MODULES)
.EXPORT_ALL_VARIABLES:
