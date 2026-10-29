DEFINES ?=
WADS    ?= .

CC     ?= cc
CFLAGS ?= -O2

SIZES  := SCREEN_WIDTH SCREEN_HEIGHT VIEW_WINDOW_WIDTH VIEW_WINDOW_HEIGHT
ENGINE := $(wildcard ../../src/*.c)

host: host.c $(ENGINE) $(wildcard ../../src/*.h)
	$(CC) -std=c11 -fno-short-enums $(CFLAGS) -I../../src \
		$(foreach v,$(SIZES),$(if $($(v)),-D$(v)=$($(v)))) \
		$(addprefix -D,$(DEFINES)) -o $@ host.c $(ENGINE)

check: host
	sh check.sh $(WADS)

clean:
	rm -f host host.exe

.PHONY: check clean
