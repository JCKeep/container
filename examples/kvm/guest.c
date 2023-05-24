#include <stddef.h>
#include <stdint.h>

#define IN_PORT   0x80
#define OUT_PORT  0xE9

static inline void hlt() {
	asm("hlt" : /* empty */ : "a" (42) : "memory");
}

static inline void outb(uint16_t port, uint8_t value) {
	asm("outb %0,%1" : /* empty */ : "a" (value), "Nd" (port) : "memory");
}

static inline uint8_t inb(uint16_t port) {
    uint8_t data;
    asm volatile("inb %1, %0" : "=a" (data) : "Nd" (port));
    return data;
}

static inline void echo(void) {
	for (;;) {
		char c = inb(IN_PORT);
		outb(OUT_PORT, c);
		if (c == '\n')
			break;
	}
}

void
__attribute__((noreturn))
__attribute__((section(".start")))
_start(void) {
	const char *p = "Hello, world!\n";

	echo();
	for (; *p; p++)
		outb(OUT_PORT, *p);

	*(long *)0x400 = 42;

	for (;;)
		hlt();
}