CC := gcc

SOURCES := $(wildcard *.c)
TARGETS := $(patsubst %.c,%,$(SOURCES))


.PHONY: all
all: $(TARGETS)

$(TARGETS): %: %.c
	$(CC) $< -o $@


.PHONY: clean
clean:
	rm -f $(TARGETS)