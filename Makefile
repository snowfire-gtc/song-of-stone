# Song of Stone - Root Makefile
# This is a wrapper for CI/CD and convenience

.PHONY: all build clean test help

all: build

build:
	$(MAKE) -C src release

debug:
	$(MAKE) -C src debug

clean:
	$(MAKE) -C src clean
	rm -f ctf_server network_server.o network_client.o

test: build
	@echo "Build successful! Binary located at build/song-of-stone"
	@ls -lh build/song-of-stone

# Network server build
server: network_server.c network.h
	gcc -std=c11 -Wall -Wextra -O2 -DSTANDALONE_SERVER -D_POSIX_C_SOURCE=200809L -o ctf_server network_server.c -lpthread

# Client requires full integration with main game
# client: network_client.c network.h
# 	gcc -std=c11 -Wall -Wextra -O2 -D_POSIX_C_SOURCE=200809L -DSTANDALONE_CLIENT -I./libs -o ctf_client network_client.c -L./libs -lraylib -lm -ldl -lpthread -lX11

help:
	@echo "Song of Stone Build System"
	@echo "  make        - build release version (default)"
	@echo "  make debug  - build debug version"
	@echo "  make clean  - remove build artifacts"
	@echo "  make test   - build and verify"
	@echo "  make server - build standalone network server"
	@echo "  make client - build standalone network client"
	@echo "  make run    - run the game (requires display)"
