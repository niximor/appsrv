OBJS = $(shell find . -iname '*.o')
HANDLER := http-json-rpc.so

$(HANDLER): $(OBJS)
	$(strip $(LINK.cpp) -shared $^ -o $@)

clean:
	rm -f $(HANDLER)

.PHONY: clean

