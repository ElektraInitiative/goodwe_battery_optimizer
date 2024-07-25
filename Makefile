SHELL ?= /bin/sh
CC ?= gcc

%: %.c
	$(CC) $^ -o $@ $(shell pkg-config --cflags --libs libmodbus)

TARGETS = read_reg  set_battery_current  write_reg

all: $(TARGETS)

clean:
	rm $(TARGETS)

