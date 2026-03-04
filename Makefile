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

gentest: $(out)/600M $(out)/1G

$(out)/600M:
	$(mkdir)
	-rm $@
	mkfs.vfat -C $@ $$(( 520 * 1024 ))
	truncate -s $(notdir $@) $@

$(out)/1G:
	$(mkdir)
	sh test/1G.sh > /dev/null $@

$(out)/%.8: %.8.md
	$(mkdir)
	pandoc $< -s -t man -o $@

mkdir = @mkdir -p $(dir $@)
.DELETE_ON_ERROR:
