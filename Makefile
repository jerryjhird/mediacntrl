CC ?= gcc
CFLAGS = -O2 -march=native -std=c99

# -DHTTP_SUPPORT for serving a http interface for commands when --http is passed
CFLAGS_ARGS = -DHTTP_SUPPORT

.PHONY: main clean

main:
	mkdir -p build
	$(CC) ${CFLAGS} $(CFLAGS_ARGS) src/mediacntrl.c -o build/mediacntrl $(shell pkgconf --cflags --libs libsystemd)

clean:
	rm -rf build