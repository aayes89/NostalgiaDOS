// Creado por Slam (2026)
#ifndef SCUMM_VM_H
#define SCUMM_VM_H

#include "../include/common.h"

#define SCUMM_VM_VARIABLES 4096

typedef struct {
    uint32_t pc;
    uint32_t size;
    
    uint8_t module_id;
    uint32_t resource_offset;

    uint16_t variables[SCUMM_VM_VARIABLES];
    uint16_t stack[256];
    uint8_t sp;

    int running;
    int error;
} ScummVM;

int scumm_vm_init_resource(ScummVM *vm, uint8_t module_id, uint32_t offset, uint32_t size);
int scumm_vm_run(ScummVM *vm, uint32_t max_steps);
void scumm_vm_stop(ScummVM *vm);

#endif
