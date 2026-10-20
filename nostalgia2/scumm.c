// Creado por Slam (2026)
#include "../include/common.h"
#include "../include/scumm.h"
#include "../include/game_modules.h"
#include "../include/scumm_scheduler.h"
#include "../include/serial.h"

static const multiboot_module_t *game000;
static const multiboot_module_t *game001;
static int engine_initialized = 0;
static int scumm_running = 0;

static ScummChunk current_room;
static uint8_t background[SCUMM_MAX_PIXELS];
static uint8_t palette[768];
static uint16_t background_width = 0;
static uint16_t background_height = 0;
static int background_decoded = 0;

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

// Búsqueda de recursos
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

int scumm_start_script(uint16_t script_id) {
    if (!engine_initialized) return 0;
    if (game000 && scan_for_script(game000, script_id, 0)) return 1;
    if (game001 && scan_for_script(game001, script_id, 1)) return 1;
    
    serial_write("ERROR CRITICO: Script ID ");
    serial_write_dec(script_id);
    serial_write(" no encontrado.\n");
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
    if (!engine_initialized && !scumm_init()) return 0;

    scumm_scheduler_init();
    scumm_running = 1;
    background_decoded = 0;
    current_room.size = 0; // Vacío hasta que la VM pida cargar una habitación 

    serial_write("\n=> ARRANCANDO SCUMM VM (SCRIPT 1)\n");
    scumm_start_script(1); // Arranque original de DOS 
    
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
