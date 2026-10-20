// Creado por Slam (2026)
#include "../include/common.h"
#include "../include/serial.h"

#define COM1 0x3F8

static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

void serial_init(void) {
    outb(COM1 + 1, 0x00); // Desactivar interrupciones 
    outb(COM1 + 3, 0x80); // Habilitar divisor 
    outb(COM1 + 0, 0x03); // 38400 baudios 
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x03); // 8 bits, sin paridad, 1 stop 
    outb(COM1 + 2, 0xC7); // FIFO 
    outb(COM1 + 4, 0x0B);
}

void serial_putc(char c) {
    // Esperar hasta que el transmisor esté listo 
    unsigned int timeout = 1000000;

    while ((inb(COM1 + 5) & 0x20) == 0 && --timeout) {
        __asm__ volatile ("pause");
    }

    if (timeout != 0) {
        outb(COM1, (uint8_t)c);
    }
}

void serial_write(const char *s) {
    if (!s) return;

    while (*s) {
        if (*s == '\n') serial_putc('\r');
        serial_putc(*s++);
    }
}

void serial_write_dec(unsigned int value) {
    char digits[10];
    unsigned int n = 0;

    do {
        digits[n++] = (char)('0' + value % 10);
        value /= 10;
    } while (value && n < sizeof(digits));

    while (n) serial_putc(digits[--n]);
}

void serial_write_hex(unsigned int value) {
    static const char hex[] = "0123456789ABCDEF";

    serial_write("0x");

    for (int shift = 28; shift >= 0; shift -= 4) {
        serial_putc(hex[(value >> shift) & 0xF]);
    }
}
