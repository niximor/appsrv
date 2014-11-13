OBJS = $(shell find . -iname '*.o')

http-json-rpc.so: $(OBJS)
	$(strip $(LINK.cpp) -shared $^ -o $@)