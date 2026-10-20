// Creado por Slam (2026)
#include "../include/game_modules.h"

static const multiboot_info_t *game_mb_info = 0;
static const multiboot_module_t *game_mb_modules = 0;

static int string_equal(const char *a, const char *b){
    if (a == 0 || b == 0) {
        return 0;
    }

    while (*a != '\0' && *b != '\0') {
        if (*a != *b) {
            return 0;
        }

        a++;
        b++;
    }

    return *a == '\0' && *b == '\0';
}

int game_modules_init(uint32_t magic, uint32_t info_addr){
    game_mb_info = 0;
    game_mb_modules = 0;

    if (magic != MULTIBOOT_BOOTLOADER_MAGIC ||
        info_addr == 0) {
        return 0;
    }

    const multiboot_info_t *info =
        (const multiboot_info_t *)(uintptr_t)info_addr;

    if (!(info->flags & MULTIBOOT_INFO_MODS) ||
        info->mods_count == 0 ||
        info->mods_addr == 0) {
        return 0;
    }

    const multiboot_module_t *modules =
        (const multiboot_module_t *)(uintptr_t)info->mods_addr;

    // Comprobar que todos los módulos tienen límites válidos.
    for (uint32_t i = 0; i < info->mods_count; i++) {
        if (modules[i].mod_end < modules[i].mod_start) {
            return 0;
        }
    }

    game_mb_info = info;
    game_mb_modules = modules;

    return 1;
}

const multiboot_module_t *game_module_find(const char *name){
    if (game_mb_info == 0 || game_mb_modules == 0 ||
        name == 0) {
        return 0;
    }

    for (uint32_t i = 0;
         i < game_mb_info->mods_count;
         i++) {

        const multiboot_module_t *module =
            &game_mb_modules[i];

        if (module->string == 0) {
            continue;
        }

        const char *module_name =
            (const char *)(uintptr_t)module->string;

        if (string_equal(module_name, name)) {
            return module;
        }
    }

    return 0;
}

uint32_t game_module_size(const multiboot_module_t *module){
    if (module == 0 ||
        module->mod_end < module->mod_start) {
        return 0;
    }

    return module->mod_end - module->mod_start;
}

const uint8_t *game_module_data(const multiboot_module_t *module){
    if (module == 0 ||
        module->mod_end < module->mod_start ||
        module->mod_start == 0) {
        return 0;
    }

    return (const uint8_t *)(uintptr_t)module->mod_start;
}

int game_module_read8(const multiboot_module_t *module,uint32_t offset){
    if (module == 0 ||
        module->mod_end < module->mod_start) {
        return -1;
    }

    uint32_t size = module->mod_end - module->mod_start;

    if (offset >= size || module->mod_start == 0) {
        return -1;
    }

    const uint8_t *data =
        (const uint8_t *)(uintptr_t)module->mod_start;

    return (int)data[offset];
}

int game_module_read_xor69(const multiboot_module_t *module,uint32_t offset) {
    int value = game_module_read8(module, offset);

    if (value < 0) {
        return -1;
    }

    return value ^ 0x69;
}

int game_module_read_xor69_be32(const multiboot_module_t *module,uint32_t offset,uint32_t *value) {
    if (module == 0 || value == 0) {
        return 0;
    }

    int b0 = game_module_read_xor69(module, offset);
    int b1 = game_module_read_xor69(module, offset + 1);
    int b2 = game_module_read_xor69(module, offset + 2);
    int b3 = game_module_read_xor69(module, offset + 3);

    if (b0 < 0 || b1 < 0 || b2 < 0 || b3 < 0) {
        return 0;
    }

    *value = ((uint32_t)b0 << 24)
           | ((uint32_t)b1 << 16)
           | ((uint32_t)b2 << 8)
           |  (uint32_t)b3;

    return 1;
}
