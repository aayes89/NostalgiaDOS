// Creado por Slam (2026)
#ifndef SCUMM_H
#define SCUMM_H

#include "../include/common.h"

#define SCUMM_XOR_KEY 0x69
#define SCUMM_MAX_WIDTH 640
#define SCUMM_MAX_HEIGHT 200
#define SCUMM_MAX_PIXELS (SCUMM_MAX_WIDTH * SCUMM_MAX_HEIGHT)

// Estructura universal para ubicar datos en los archivos .000 y .001 
typedef struct {
    uint32_t offset;
    uint32_t size;
    char id[5];
} ScummChunk;

// Ciclo de vida y control principal 
int scumm_init(void);
int scumm_start(void);
void scumm_update(void);
void scumm_stop(void);
int scumm_is_running(void);

// I/O del disco (Lectura de memoria cifrada) 
int scumm_resource_read8(uint8_t module_id, uint32_t offset, uint8_t *value);
uint32_t scumm_resource_size(uint8_t module_id);

// Buscador de scripts y lanzador en el planificador 
int scumm_start_script(uint16_t script_id);

// Interfaz Gráfica de Objetos (Solicitada por la VM) 
void scumm_set_object_state(uint16_t obj_id, uint8_t state);
void scumm_draw_frame(uint8_t *framebuffer, uint16_t dest_w, uint16_t dest_h);

// Manejo del Fondo de la habitación activa 
int scumm_decode_background(void);
const uint8_t *scumm_background_palette(void);
uint16_t scumm_background_width(void);
uint16_t scumm_background_height(void);

#endif
