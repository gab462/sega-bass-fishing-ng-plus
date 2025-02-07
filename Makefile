CC := cc
CFLAGS := -std=c23 -pedantic -Wall -Wextra -I ./build/include -I . -I ./cglm/include -Wno-missing-braces -Wno-old-style-declaration
LDFLAGS := -L ./concord/lib -ldiscord -lcurl -lm

build/main: main.c build/lib/libdiscord.a
	$(CC) $(CFLAGS) main.c -o build/main $(LDFLAGS)

build/lib/libdiscord.a:
	CFLAGS="-DCCORD_SIGINTCATCH" make -C concord -j16
	make -C concord PREFIX=$(PWD)/build SHAREDIR=$(PWD)/build install
