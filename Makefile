out := _out
libs := libparted-fs-resize libparted

CFLAGS := -g -Wall -std=c23 $(shell pkg-config --cflags $(libs))
LDFLAGS := $(shell pkg-config --libs $(libs))

deps := $(patsubst %.c, $(out)/%.o, $(wildcard *.c))

$(out)/fat32-resize: $(deps)
	$(CC) $^ $(LDFLAGS) -o $@

$(out)/%.o: %.c
	@mkdir -p $(dir $@)
	$(COMPILE.c) $< -o $@

$(out)/600M:
	$(mkdir)
	-rm $@
	mkfs.vfat -C $@ $$(( 520 * 1024 ))
	truncate -s $(notdir $@) $@
	mcopy -i $@ /usr/share/mythes/th_en_US_v2.dat ::

$(out)/1G:
	$(mkdir)
	sh test/1G.sh $@

mkdir = @mkdir -p $(dir $@)
.DELETE_ON_ERROR:
