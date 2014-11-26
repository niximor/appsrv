OBJS = $(shell find . -iname '*.o')

rtjs.so: $(OBJS)
	$(strip $(LINK.cpp) -shared $^ -o $@)