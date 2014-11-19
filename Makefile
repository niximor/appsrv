all: build test

build:
	$(MAKE) -w -C src/

test:
	$(MAKE) -w -C tests/

clean:
	$(MAKE) -w -C tests/ clean
	$(MAKE) -w -C src/ clean

.PHONY: all build test clean