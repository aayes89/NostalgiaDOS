// Creado por Slam (2026 - V0.0.10)
#include "../include/common.h"
#include "../include/scumm.h"
#include "../include/game_modules.h"
#include "../include/scumm_scheduler.h"
#include "../include/serial.h"

// VARIABLES GLOBALES DEL MOTOR
static const multiboot_module_t *game000;
static const multiboot_module_t *game001;

static int engine_initialized = 0;
static int game_loaded = 0;
static int scumm_running = 0;

static ScummChunk rooms[SCUMM_MAX_ROOMS];
static int room_count = 0;
static ScummChunk first_room;
static uint8_t scumm_object_states[2048];

// Variables gráficas restauradas 
static uint8_t background[SCUMM_MAX_PIXELS];
static uint8_t palette[768];
static uint16_t background_width = 0;
static uint16_t background_height = 0;
static int background_decoded = 0;

// LECTURA FÍSICA Y DESCIFRADO
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

static int read_le32(const multiboot_module_t *module, uint32_t offset, uint32_t *value) {
    int a = read_byte(module, offset);
    int b = read_byte(module, offset + 1);
    int c = read_byte(module, offset + 2);
    int d = read_byte(module, offset + 3);
    if (a < 0 || b < 0 || c < 0 || d < 0) return 0;
    *value = (uint32_t)a | ((uint32_t)b << 8) | ((uint32_t)c << 16) | ((uint32_t)d << 24);
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

// ÍNDICES Y LANZAMIENTO DE SCRIPTS   
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

// CICLO DE VIDA
int scumm_init(void) {
    for (int i = 0; i < 2048; i++) scumm_object_states[i] = 0;
    game000 = game_module_find("MONKEY2.000");
    game001 = game_module_find("MONKEY2.001");
    if (!game000 || !game001 || game_module_size(game000) == 0 || game_module_size(game001) < 8) return 0;
    engine_initialized = 1;
    return 1;
}

int scumm_load_game(void) {
    game_loaded = 0;
    if (!engine_initialized || !scumm_index_rooms()) return 0;
    first_room = rooms[0]; // Arranque real en la Habitación 0 Global 
    game_loaded = 1;
    return 1;
}

int scumm_start(void) {
    scumm_running = 0;
    if (!engine_initialized && !scumm_init()) return 0;
    if (!game_loaded && !scumm_load_game()) return 0;

    background_decoded = 0;
    scumm_scheduler_init();
    scumm_running = 1;
    
    serial_write("=> INICIANDO BOOT ORIGINAL (ROOM 0)\n");
    
    if (!scumm_start_entry_script()) {
        serial_write("ERROR CRÍTICO: No se encontró el ENCD de Room 0.\n");
    }
    return 1;
}

void scumm_update(void) {
	if (scumm_running){
		scumm_scheduler_update(); 
	}
}
void scumm_stop(void) { 
	scumm_running = 0; 
	scumm_scheduler_stop_all();
}
int scumm_is_running(void) { 
	return scumm_running; 
}

// SERVICIOS PARA LA VM
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

// SUBSISTEMA GRÁFICO Y DECODIFICADOR 
typedef struct {
    const multiboot_module_t *module;
    uint32_t position;
    uint32_t end;
    uint8_t current;
    uint8_t available;
} ScummBitReader;

static int read_bit(ScummBitReader *reader) {
    if (reader->available == 0) {
        if (reader->position >= reader->end) return -1;
        int value = read_byte(reader->module, reader->position++);
        if (value < 0) return -1;
        reader->current = (uint8_t)value;
        reader->available = 8;
    }
    int bit = reader->current & 1;
    reader->current >>= 1;
    reader->available--;
    return bit;
}

static int read_bits(ScummBitReader *reader, unsigned count, uint32_t *value) {
    uint32_t result = 0;
    for (unsigned i = 0; i < count; i++) {
        int bit = read_bit(reader);
        if (bit < 0) return 0;
        result |= (uint32_t)bit << i;
    }
    *value = result;
    return 1;
}

static void put_strip_pixel(uint8_t *dest, uint16_t dest_w, uint16_t dest_h, uint32_t strip, uint32_t index, uint8_t color, int vertical, int base_x, int base_y, uint16_t item_h, int is_object) {
    int x, y;
    if (vertical) {
        x = base_x + strip * 8 + (index / item_h);
        y = base_y + (index % item_h);
    } else {
        x = base_x + strip * 8 + (index % 8);
        y = base_y + (index / 8);
    }
    if (x >= 0 && x < dest_w && y >= 0 && y < dest_h) {
        if (!is_object || color != 0) dest[y * dest_w + x] = color;
    }
}

static int decode_method1(const multiboot_module_t *module, uint32_t start, uint32_t end, uint32_t strip, int vertical, unsigned palette_bits, uint8_t *dest, uint16_t dest_w, uint16_t dest_h, int base_x, int base_y, uint16_t item_h, int is_object) {
    int initial = read_byte(module, start);
    if (initial < 0) return 0;

    uint32_t total = (uint32_t)item_h * 8;
    uint32_t output = 0;
    uint8_t color = (uint8_t)initial;
    int direction = 1;

    put_strip_pixel(dest, dest_w, dest_h, strip, output++, color, vertical, base_x, base_y, item_h, is_object);
    ScummBitReader reader = { module, start + 1, end, 0, 0};

    while (output < total) {
        if (read_bit(&reader) == 1) {
            if (read_bit(&reader) == 0) {
                uint32_t val;
                if (!read_bits(&reader, palette_bits, &val)) return 0;
                color = (uint8_t)val;
                direction = 1;
            } else {
                if (read_bit(&reader) == 0) color -= direction;
                else { direction = -direction; color -= direction; }
            }
        }
        put_strip_pixel(dest, dest_w, dest_h, strip, output++, color, vertical, base_x, base_y, item_h, is_object);
    }
    return 1;
}

static int decode_method2(const multiboot_module_t *module, uint32_t start, uint32_t end, uint32_t strip, unsigned palette_bits, uint8_t *dest, uint16_t dest_w, uint16_t dest_h, int base_x, int base_y, uint16_t item_h, int is_object) {
    int initial = read_byte(module, start);
    if (initial < 0) return 0;

    uint32_t total = (uint32_t)item_h * 8;
    uint32_t output = 0;
    uint8_t color = (uint8_t)initial;

    put_strip_pixel(dest, dest_w, dest_h, strip, output++, color, 0, base_x, base_y, item_h, is_object);
    ScummBitReader reader = { module, start + 1, end, 0, 0 };

    while (output < total) {
        if (read_bit(&reader) == 1) {
            if (read_bit(&reader) == 0) {
                uint32_t val;
                if (!read_bits(&reader, palette_bits, &val)) return 0;
                color = (uint8_t)val;
            } else {
                uint32_t delta, repeat = 0;
                if (!read_bits(&reader, 3, &delta)) return 0;
                if (delta == 4) {
                    if (!read_bits(&reader, 8, &repeat)) return 0;
                } else color = (uint8_t)(color + (int)delta - 4);
                
                while (repeat-- > 0 && output < total) {
                    put_strip_pixel(dest, dest_w, dest_h, strip, output++, color, 0, base_x, base_y, item_h, is_object);
                }
                continue;
            }
        }
        put_strip_pixel(dest, dest_w, dest_h, strip, output++, color, 0, base_x, base_y, item_h, is_object);
    }
    return 1;
}

static int decode_strip(const multiboot_module_t *module, uint32_t start, uint32_t end, uint32_t strip, uint8_t *dest, uint16_t dest_w, uint16_t dest_h, int base_x, int base_y, uint16_t item_h, int is_object) {
    int code = read_byte(module, start);
    if (code < 0 || start + 2 > end) return 0;
    uint32_t total = (uint32_t)item_h * 8;

    if (code == 1) {
        if (end - (start + 2) < total) return 0;
        for (uint32_t i = 0; i < total; i++) {
            put_strip_pixel(dest, dest_w, dest_h, strip, i, (uint8_t)read_byte(module, start + 2 + i), 0, base_x, base_y, item_h, is_object);
        }
        return 1;
    }

    int vertical = 0, method = 0;
    unsigned p_bits = 0;
    if (code >= 0x0E && code <= 0x12) { method = 1; vertical = 1; p_bits = code - 0x0A; }
    else if (code >= 0x18 && code <= 0x1C) { method = 1; p_bits = code - 0x14; }
    else if (code >= 0x22 && code <= 0x26) { method = 1; vertical = 1; p_bits = code - 0x1E; }
    else if (code >= 0x2C && code <= 0x30) { method = 1; p_bits = code - 0x28; }
    else if (code >= 0x40 && code <= 0x44) { method = 2; p_bits = code - 0x3C; }
    else if (code >= 0x54 && code <= 0x58) { method = 2; p_bits = code - 0x50; }
    else if (code >= 0x68 && code <= 0x6C) { method = 2; p_bits = code - 0x64; }
    else if (code >= 0x7C && code <= 0x80) { method = 2; p_bits = code - 0x78; }
    else return 0;

    if (method == 1) return decode_method1(module, start + 1, end, strip, vertical, p_bits, dest, dest_w, dest_h, base_x, base_y, item_h, is_object);
    return decode_method2(module, start + 1, end, strip, p_bits, dest, dest_w, dest_h, base_x, base_y, item_h, is_object);
}

int scumm_decode_background(void) {
    ScummChunk rmhd, clut, rmim, im00, smap;
    background_decoded = 0;

    if (!game_loaded || !game001) return 0;
    if (!find_child(game001, &first_room, "RMHD", &rmhd) || !find_child(game001, &first_room, "CLUT", &clut) || !find_child(game001, &first_room, "RMIM", &rmim)) return 0;

    if (!read_le16(game001, rmhd.offset + 8, &background_width) || !read_le16(game001, rmhd.offset + 10, &background_height)) return 0;
    if (background_width == 0 || background_height == 0 || background_width > SCUMM_MAX_WIDTH || background_height > SCUMM_MAX_HEIGHT) return 0;

    for (uint32_t i = 0; i < sizeof(palette); i++) palette[i] = (uint8_t)read_byte(game001, clut.offset + 8 + i);

    if (!find_child(game001, &rmim, "IM00", &im00) || !find_child(game001, &im00, "SMAP", &smap)) return 0;

    uint32_t strips = background_width / 8;
    for (uint32_t strip = 0; strip < strips; strip++) {
        uint32_t rel_start, rel_end;
        if (!read_le32(game001, smap.offset + 8 + strip * 4, &rel_start)) return 0;
        if (strip + 1 < strips) { if (!read_le32(game001, smap.offset + 8 + (strip + 1) * 4, &rel_end)) return 0; } 
        else rel_end = smap.size;
        decode_strip(game001, smap.offset + rel_start, smap.offset + rel_end, strip, background, background_width, background_height, 0, 0, background_height, 0);
    }
    background_decoded = 1;
    return 1;
}

void scumm_draw_frame(uint8_t *framebuffer, uint16_t dest_w, uint16_t dest_h) {
    if (!background_decoded || !framebuffer) return;

    for (uint32_t i = 0; i < (uint32_t)background_width * background_height; i++) framebuffer[i] = background[i];

    ScummChunk obcd, obim, im_state, smap, cdhd, imhd;
    uint32_t offset = first_room.offset + 8;
    uint32_t end = first_room.offset + first_room.size;

    while (offset + 8 <= end) {
        if (read_chunk(game001, offset, end, &obcd) && chunk_is(&obcd, "OBCD")) {
            if (find_child(game001, &obcd, "CDHD", &cdhd)) {
                uint16_t obj_id;
                if (read_le16(game001, cdhd.offset + 8, &obj_id)) {
                    uint8_t state = scumm_object_states[obj_id];
                    if (state > 0) {
                        char im_name[5] = {'I', 'M', '0' + (state / 10), '0' + (state % 10), '\0'};
                        if (find_child(game001, &obcd, "OBIM", &obim) && find_child(game001, &obim, "IMHD", &imhd) &&
                            find_child(game001, &obim, im_name, &im_state) && find_child(game001, &im_state, "SMAP", &smap)) {
                            uint16_t obj_x, obj_y, img_w, img_h, rx_off, ry_off;
                            read_le16(game001, cdhd.offset + 10, &obj_x); read_le16(game001, cdhd.offset + 12, &obj_y);
                            read_le16(game001, imhd.offset + 10, &img_w); read_le16(game001, imhd.offset + 12, &img_h);
                            read_le16(game001, imhd.offset + 14, &rx_off); read_le16(game001, imhd.offset + 16, &ry_off);
                            
                            int f_x = obj_x + (int16_t)rx_off; int f_y = obj_y + (int16_t)ry_off;
                            uint32_t strips = img_w / 8;
                            for (uint32_t strip = 0; strip < strips; strip++) {
                                uint32_t r_start, r_end;
                                if (read_le32(game001, smap.offset + 8 + strip * 4, &r_start)) {
                                    if (strip + 1 < strips) read_le32(game001, smap.offset + 8 + (strip + 1) * 4, &r_end);
                                    else r_end = smap.size;
                                    decode_strip(game001, smap.offset + r_start, smap.offset + r_end, strip, framebuffer, dest_w, dest_h, f_x, f_y, img_h, 1);
                                }
                            }
                        }
                    }
                }
            }
        }
        offset += obcd.size;
    }
}

const uint8_t *scumm_background_palette(void) { 
	return palette; 
}
uint16_t scumm_background_width(void) { 
	return background_width; 
}
uint16_t scumm_background_height(void) { 
	return background_height; 
}
