out := _out
libs := libparted-fs-resize libparted

CFLAGS := -g -Wall -std=c23 $(shell pkg-config --cflags $(libs))
LDFLAGS := $(shell pkg-config --libs $(libs))

deps := $(patsubst %.c, $(out)/%.o, $(wildcard *.c))

$(out)/vfat-resize: $(deps)
	$(CC) $^ $(LDFLAGS) -o $@

$(out)/%.o: %.c
	@mkdir -p $(dir $@)
	$(COMPILE.c) $< -o $@

test/20M:
	$(mkdir)
	-rm $@
	mkfs.vfat -C $@ $$(( 20 * 1024 ))

mkdir = @mkdir -p $(dir $@)
