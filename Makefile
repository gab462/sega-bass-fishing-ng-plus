CC := cc
CFLAGS := -std=c23 -pedantic -Wall -Wextra -Ibuild/include -Ilib -Ilib/cglm/include -Wno-missing-braces -Wno-old-style-declaration
LDFLAGS := -Lbuild/lib -ldiscord -lcurl -lm

build/main: src/main.c build/lib/libdiscord.a
	$(CC) $(CFLAGS) src/main.c -o build/main $(LDFLAGS)

build/lib/libdiscord.a:
	CFLAGS="-DCCORD_SIGINTCATCH" make -C lib/concord -j16
	make -C lib/concord PREFIX=$(PWD)/build SHAREDIR=$(PWD)/build install
