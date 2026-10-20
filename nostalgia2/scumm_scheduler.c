// Creado por Slam (2026)
// Planificador
#include "../include/scumm_scheduler.h"
#include "../include/scumm.h"

typedef struct {
    ScummVM vm;
    uint8_t active;
} ScummScriptSlot;

static ScummScriptSlot slots[SCUMM_SCHEDULER_SLOTS];

void scumm_scheduler_init(void) {
    for (int i = 0; i < SCUMM_SCHEDULER_SLOTS; i++) slots[i].active = 0;
}

int scumm_scheduler_start_chunk(uint8_t module_id, const ScummChunk *chunk, uint8_t *slot_out) {
    if (!chunk || chunk->size == 0) return 0;
    
    for (int i = 0; i < SCUMM_SCHEDULER_SLOTS; i++) {
        if (!slots[i].active) {
            if (scumm_vm_init_resource(&slots[i].vm, module_id, chunk->offset, chunk->size)) {
                slots[i].active = 1;
                *slot_out = (uint8_t)i;
                return 1;
            }
            return 0;
        }
    }
    return 0; // Planificador lleno 
}

void scumm_scheduler_stop_all(void) {
    for (int i = 0; i < SCUMM_SCHEDULER_SLOTS; i++) {
        scumm_vm_stop(&slots[i].vm);
        slots[i].active = 0;
    }
}

void scumm_scheduler_update(void) {
    for (int i = 0; i < SCUMM_SCHEDULER_SLOTS; i++) {
        if (slots[i].active) {
            scumm_vm_run(&slots[i].vm, SCUMM_SCHEDULER_STEPS);
            if (!slots[i].vm.running || slots[i].vm.error) {
                scumm_vm_stop(&slots[i].vm);
                slots[i].active = 0;
            }
        }
    }
}
