SCREEN_WIDTH       ?= 240
SCREEN_HEIGHT      ?= 160
VIEW_WINDOW_WIDTH  ?= $(SCREEN_WIDTH)
VIEW_WINDOW_HEIGHT ?= 128
DEFINES            ?=
WADS               ?= .

CC     ?= cc
CFLAGS ?= -O2

ENGINE := $(wildcard ../../src/*.c)

host: host.c $(ENGINE) $(wildcard ../../src/*.h)
	$(CC) -std=c11 -fno-short-enums $(CFLAGS) -I../../src \
		-DSCREEN_WIDTH=$(SCREEN_WIDTH) -DSCREEN_HEIGHT=$(SCREEN_HEIGHT) \
		-DVIEW_WINDOW_WIDTH=$(VIEW_WINDOW_WIDTH) -DVIEW_WINDOW_HEIGHT=$(VIEW_WINDOW_HEIGHT) \
		$(addprefix -D,$(DEFINES)) -o $@ host.c $(ENGINE)

check: host
	sh check.sh $(WADS)

clean:
	rm -f host host.exe

.PHONY: check clean
