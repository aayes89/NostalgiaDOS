// Creado por Slam (2026)
#ifndef GAME_MODULES_H
#define GAME_MODULES_H

#include "common.h"
#include "multiboot.h"

// Inicializa el acceso a los módulos cargados por GRUB. 
int game_modules_init(uint32_t magic, uint32_t info_addr);

// Busca un módulo por su nombre, por ejemplo MONKEY2.000. 
const multiboot_module_t *game_module_find(const char *name);

// Devuelve el tamaño del módulo en bytes. 
uint32_t game_module_size(const multiboot_module_t *module);

// Devuelve un puntero a los datos del módulo. 
const uint8_t *game_module_data(const multiboot_module_t *module);

// Lee un byte; devuelve -1 si el desplazamiento no es válido. 
int game_module_read8(const multiboot_module_t *module, uint32_t offset);

int game_module_read_xor69(const multiboot_module_t *module, uint32_t offset);

int game_module_read_xor69_be32(const multiboot_module_t *module, uint32_t offset, uint32_t *value);

#endif
