CC := cc
CFLAGS := -std=c23 -pedantic -Wall -Wextra -I ./build/include -I ./cut -I ./msf_gif/ -I ./olive.c/ -I ./cglm/include/cglm/ -Wno-missing-braces -Wno-old-style-declaration
LDFLAGS := -L ./concord/lib/ -ldiscord -lcurl -lm

build/main: main.c build/lib/libdiscord.a
	$(CC) $(CFLAGS) main.c -o build/main $(LDFLAGS)

build/lib/libdiscord.a:
	make -C concord -j16
	make -C concord PREFIX=$(PWD)/build SHAREDIR=$(PWD)/build install
