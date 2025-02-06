CC := cc
CFLAGS := -std=c23 -pedantic -Wall -Wextra -Wshadow -I ./build/include -I ./cut
LDFLAGS := -L ./concord/lib/ -ldiscord -lcurl

build/main: main.c build/lib/libdiscord.a
	$(CC) $(CFLAGS) main.c -o build/main $(LDFLAGS)

build/lib/libdiscord.a:
	make -C concord -j16
	make -C concord PREFIX=$(PWD)/build SHAREDIR=$(PWD)/build install
