OBJS = $(shell find . -iname '*.o')
MODULE := rtjs.so

$(MODULE): $(OBJS)
	$(strip $(LINK.cpp) -shared $^ -o $@)

clean:
	rm -f $(MODULE)

.PHONY: clean
