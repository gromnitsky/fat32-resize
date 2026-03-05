out := _out
libs := libparted-fs-resize libparted

CFLAGS := -g -Wall -std=c23 $(shell pkg-config --cflags $(libs))
LDFLAGS := $(shell pkg-config --libs $(libs))

deps := $(patsubst %.c, $(out)/%.o, $(wildcard *.c))

all: $(addprefix $(out)/, fat32-resize.8 fat32-resize)

$(out)/fat32-resize: $(deps)
	$(CC) $^ $(LDFLAGS) -o $@

$(out)/%.o: %.c
	$(mkdir)
	$(COMPILE.c) $< -o $@

$(out)/%.8: %.8.md
	$(mkdir)
	pandoc $< -s -t man -o $@


test: all $(out)/junk/600M $(out)/junk/1G
	bats test/test_*.sh --verbose-run $(o)

.PHONY: $(out)/junk/*

$(out)/junk/600M:
	$(mkdir)
	-rm $@
	mkfs.vfat -C $@ $$(( 520 * 1024 ))
	truncate -s $(notdir $@) $@

$(out)/junk/1G:
	$(mkdir)
	sh test/1G.sh > /dev/null $@

mkdir = @mkdir -p $(dir $@)
.DELETE_ON_ERROR:
