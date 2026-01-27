CC ?= gcc
LD ?= ld.bfd
CFLAGS = -O2 -march=native -std=c99 -fuse-ld=$(LD)

.PHONY: main clean

main:
	mkdir -p build
	$(CC) ${CFLAGS} src/mediacntrl.c -o build/mediacntrl $(shell pkg-config --cflags --libs libsystemd)

clean:
	rm -rf build