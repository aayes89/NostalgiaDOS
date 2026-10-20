// Creado por Slam (2026)
#ifndef COMMON_H
#define COMMON_H

// Tipos
typedef unsigned char      uint8_t;
typedef signed char        int8_t;

typedef unsigned short     uint16_t;
typedef signed short       int16_t;

typedef unsigned int       uint32_t;
typedef signed int         int32_t;

typedef unsigned long long uint64_t;
typedef signed long long   int64_t;

// Tipos para direcciones y tamaños en x86 de 32 bits
typedef unsigned int       uintptr_t;
typedef signed int         intptr_t;
typedef unsigned int       size_t;

// Booleanos para kernel bare-metal 

typedef unsigned char bool;

#ifndef true
#define true  1
#endif

#ifndef false
#define false 0
#endif

#ifndef TRUE
#define TRUE  1
#endif

#ifndef FALSE
#define FALSE 0
#endif

// Colores VGA
typedef enum {
    COLOR_BLACK         = 0x00,
    COLOR_BLUE          = 0x01,
    COLOR_GREEN         = 0x02,
    COLOR_CYAN          = 0x03,
    COLOR_RED           = 0x04,
    COLOR_MAGENTA       = 0x05,
    COLOR_BROWN         = 0x06,
    COLOR_LIGHT_GRAY    = 0x07,
    COLOR_DARK_GRAY     = 0x08,
    COLOR_LIGHT_BLUE    = 0x09,
    COLOR_LIGHT_GREEN   = 0x0A,
    COLOR_LIGHT_CYAN    = 0x0B,
    COLOR_LIGHT_RED     = 0x0C,
    COLOR_LIGHT_MAGENTA = 0x0D,
    COLOR_YELLOW        = 0x0E,
    COLOR_WHITE         = 0x0F
} vga_color_t;

// Modo VGA 13h
#define VGA_MODE13_WIDTH       320
#define VGA_MODE13_HEIGHT      200
#define VGA_MODE13_BPP         8
#define VGA_MODE13_COLORS      256

#define VGA_MODE13_FRAMEBUFFER_SIZE \
    (VGA_MODE13_WIDTH * VGA_MODE13_HEIGHT)

// Memoria de vídeo convencional en modo real / identidad física. 
#define VGA_MODE13_FRAMEBUFFER ((volatile uint8_t*)0xA0000)

// VGA: Puertos de hardware

#define VGA_MISC_OUTPUT_READ   0x3CC
#define VGA_MISC_OUTPUT_WRITE  0x3C2

#define VGA_CRTC_INDEX_COLOR   0x3D4
#define VGA_CRTC_DATA_COLOR    0x3D5

#define VGA_CRTC_INDEX_MONO    0x3B4
#define VGA_CRTC_DATA_MONO     0x3B5

#define VGA_STATUS_COLOR       0x3DA
#define VGA_STATUS_MONO        0x3BA

// Constantes para modo texto

#define VGA_TEXT_COLUMNS       80
#define VGA_TEXT_ROWS          25

// MACROS 
#define ARRAY_SIZE(array) \
    (sizeof(array) / sizeof((array)[0]))

#define UNUSED(value) ((void)(value))

#endif 
