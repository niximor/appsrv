OBJS=$(shell find . -iname '*.o')

http.so: $(OBJS)
	$(strip $(LINK.cpp) -shared $^ -o $@)