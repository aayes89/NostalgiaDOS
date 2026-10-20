// Creado por Slam (2026)
#include "../include/scumm.h"
#include "../include/game_modules.h"
#include "../include/scumm_scheduler.h"
#include "../include/serial.h"

static const multiboot_module_t *game000;
static const multiboot_module_t *game001;

static int engine_initialized = 0;
static int game_loaded = 0;
static int scumm_running = 0;

static ScummChunk rooms[SCUMM_MAX_ROOMS];
static int room_count = 0;
static ScummChunk first_room;
static uint8_t scumm_object_states[2048];

// Lectura cruda de archivos (descifrado XOR)
static int read_byte(const multiboot_module_t *module, uint32_t offset) {
    int value = game_module_read8(module, offset);
    return (value < 0) ? -1 : (value ^ SCUMM_XOR_KEY);
}

static int read_be32(const multiboot_module_t *module, uint32_t offset, uint32_t *value) {
    int b0 = read_byte(module, offset);
    int b1 = read_byte(module, offset + 1);
    int b2 = read_byte(module, offset + 2);
    int b3 = read_byte(module, offset + 3);
    if (b0 < 0 || b1 < 0 || b2 < 0 || b3 < 0) return 0;
    *value = ((uint32_t)b0 << 24) | ((uint32_t)b1 << 16) | ((uint32_t)b2 << 8) | (uint32_t)b3;
    return 1;
}

static int read_le16(const multiboot_module_t *module, uint32_t offset, uint16_t *value) {
    int a = read_byte(module, offset);
    int b = read_byte(module, offset + 1);
    if (a < 0 || b < 0) return 0;
    *value = (uint16_t)((uint16_t)a | ((uint16_t)b << 8));
    return 1;
}

static int read_chunk(const multiboot_module_t *module, uint32_t offset, uint32_t limit, ScummChunk *chunk) {
    if (!module || !chunk || offset > limit || limit - offset < 8) return 0;
    for (uint32_t i = 0; i < 4; i++) {
        int c = read_byte(module, offset + i);
        if (c < 0) return 0;
        chunk->id[i] = (char)c;
    }
    chunk->id[4] = '\0';
    if (!read_be32(module, offset + 4, &chunk->size) || chunk->size < 8 || chunk->size > limit - offset) return 0;
    chunk->offset = offset;
    return 1;
}

static int chunk_is(const ScummChunk *chunk, const char *id) {
    return chunk->id[0] == id[0] && chunk->id[1] == id[1] && 
           chunk->id[2] == id[2] && chunk->id[3] == id[3];
}

static int find_child(const multiboot_module_t *module, const ScummChunk *parent, const char *id, ScummChunk *result) {
    uint32_t offset = parent->offset + 8;
    uint32_t end = parent->offset + parent->size;
    while (offset + 8 <= end) {
        ScummChunk child;
        if (!read_chunk(module, offset, end, &child)) return 0;
        if (chunk_is(&child, id)) {
            *result = child;
            return 1;
        }
        offset += child.size;
    }
    return 0;
}

// Búsqueda de recursos
static int scumm_index_rooms(void) {
    ScummChunk container, loff, bundle, room;
    room_count = 0;

    if (!read_chunk(game001, 0, game_module_size(game001), &container) || !chunk_is(&container, "LECF")) return 0;
    if (!find_child(game001, &container, "LOFF", &loff)) return 0;

    uint32_t offset = loff.offset + loff.size;
    uint32_t end = container.offset + container.size;

    while (offset + 8 <= end) {
        if (!read_chunk(game001, offset, end, &bundle)) break;
        if (chunk_is(&bundle, "LFLF") && find_child(game001, &bundle, "ROOM", &room)) {
            if (room_count < SCUMM_MAX_ROOMS) rooms[room_count++] = room;
        }
        offset += bundle.size;
    }
    return room_count > 0;
}

static int scumm_start_entry_script(void) {
    ScummChunk script;
    uint8_t slot;
    if (!game_loaded) return 0;

    if (find_child(game001, &first_room, "ENCD", &script)) {
        return scumm_scheduler_start_chunk(1, &script, &slot);
    }
    return 0;
}

int scumm_start_script(uint16_t script_id) {
    ScummChunk chunk;
    uint32_t offset = 0;
    uint32_t end = game_module_size(game000);

    if (!game_loaded || !game000 || !game001) return 0;

    // Buscar posición rápida en Directorio DSCR (.000) 
    while (offset + 8 <= end) {
        if (!read_chunk(game000, offset, end, &chunk)) break;
        if (chunk_is(&chunk, "DSCR")) {
            uint32_t entry_offset = chunk.offset + 8 + (script_id * 5);
            if (entry_offset + 5 <= chunk.offset + chunk.size) {
                uint32_t script_offset = (uint32_t)read_byte(game000, entry_offset + 1) |
                                         ((uint32_t)read_byte(game000, entry_offset + 2) << 8) |
                                         ((uint32_t)read_byte(game000, entry_offset + 3) << 16) |
                                         ((uint32_t)read_byte(game000, entry_offset + 4) << 24);

                if (script_offset > 0 && script_offset < game_module_size(game001)) {
                    ScummChunk script_chunk;
                    if (read_chunk(game001, script_offset, game_module_size(game001), &script_chunk)) {
                        uint8_t slot;
                        script_chunk.offset += 8; // Saltar cabecera 
                        script_chunk.size -= 8;
                        return scumm_scheduler_start_chunk(1, &script_chunk, &slot);
                    }
                }
            }
        }
        offset += chunk.size;
    }

    // Si no está, buscar en Local Scripts LSCR (.001) 
    if (first_room.size > 0) {
        offset = first_room.offset + 8;
        end = first_room.offset + first_room.size;
        while (offset + 8 <= end) {
            if (!read_chunk(game001, offset, end, &chunk)) break;
            if (chunk_is(&chunk, "LSCR")) {
                if (read_byte(game001, chunk.offset + 8) == script_id) {
                    uint8_t slot;
                    chunk.offset += 9; // Saltar cabecera y 1 byte de ID 
                    chunk.size -= 9;
                    return scumm_scheduler_start_chunk(1, &chunk, &slot);
                }
            }
            offset += chunk.size;
        }
    }
    serial_write("ERROR: Script ID no encontrado.\n");
    return 0;
}

static int scan_for_script(const multiboot_module_t *module, uint16_t target_id, uint8_t module_id) {
    uint32_t offset = 0;
    uint32_t end = game_module_size(module);
    ScummChunk chunk;

    while (offset + 8 <= end) {
        if (!read_chunk(module, offset, end, &chunk)) break;

        // Entrar transparentemente en contenedores conocidos 
        if (chunk_is(&chunk, "LECF") || chunk_is(&chunk, "LFLF") || chunk_is(&chunk, "ROOM")) {
            offset += 8;
            continue;
        }

        // Comprobar si es un script con el ID correcto 
        if (chunk_is(&chunk, "SCRP") || chunk_is(&chunk, "LSCR")) {
            uint16_t current_id;
            if (read_le16(module, chunk.offset + 8, &current_id) && current_id == target_id) {
                uint8_t slot;
                ScummChunk exec = chunk;
                exec.offset += 2; // Saltar ID incrustado 
                exec.size -= 2;
                return scumm_scheduler_start_chunk(module_id, &exec, &slot);
            }
        }
        offset += chunk.size; // Saltar datos irrelevantes 
    }
    return 0;
}


// Ciclo de vida del motor
int scumm_init(void) {
    for (int i = 0; i < 2048; i++) scumm_object_states[i] = 0;
    
    game000 = game_module_find("MONKEY2.000");
    game001 = game_module_find("MONKEY2.001");

    if (!game000 || !game001 || game_module_size(game000) == 0 || game_module_size(game001) < 8) {
        return 0;
    }
    engine_initialized = 1;
    return 1;
}

int scumm_start(void) {
    scumm_running = 0;
    if (!engine_initialized && !scumm_init()) return 0;
    if (!game_loaded && !scumm_load_game()) return 0;
    
    scumm_scheduler_init();
    scumm_running = 1;

    serial_write("\n==================================\n");
    serial_write("=> INICIANDO BOOT ORIGINAL (ROOM 0)\n");
    serial_write("==================================\n");
    
    if (!scumm_start_entry_script()) {
        serial_write("ERROR CRÍTICO: No se encontró el ENCD de Room 0.\n");
    }

    return 1;
}

void scumm_update(void) {
    if (scumm_running) scumm_scheduler_update();
}

void scumm_stop(void) {
    scumm_running = 0;
    scumm_scheduler_stop_all();
}

int scumm_is_running(void) {
    return scumm_running;
}

int scumm_load_game(void) {
    game_loaded = 0;
    if (!engine_initialized || !scumm_index_rooms()) return 0;
    
    // La Habitación 0 contiene el verdadero Boot Script en su ENCD 
    first_room = rooms[0];
    game_loaded = 1;
    return 1;
}


// API para la VM
int scumm_resource_read8(uint8_t module_id, uint32_t offset, uint8_t *value) {
    const multiboot_module_t *m = (module_id == 0) ? game000 : game001;
    if (!m || offset >= game_module_size(m)) return 0;
    
    int result = read_byte(m, offset);
    if (result < 0) return 0;
    
    *value = (uint8_t)result;
    return 1;
}

uint32_t scumm_resource_size(uint8_t module_id) {
    const multiboot_module_t *m = (module_id == 0) ? game000 : game001;
    return m ? game_module_size(m) : 0;
}

void scumm_set_object_state(uint16_t obj_id, uint8_t state) {
    if (obj_id < 2048) scumm_object_states[obj_id] = state;
}

// Funciones gráficas inactivas temporalmente hasta implementar el opcode loadRoom 
void scumm_draw_frame(uint8_t *framebuffer, uint16_t dest_w, uint16_t dest_h) { (void)framebuffer; (void)dest_w; (void)dest_h; }
int scumm_decode_background(void) { return 0; }
const uint8_t *scumm_background_palette(void) { return palette; }
uint16_t scumm_background_width(void) { return background_width; }
uint16_t scumm_background_height(void) { return background_height; }
