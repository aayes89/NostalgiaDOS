// Creado por Slam (2026)
#ifndef SCUMM_SCHEDULER_H
#define SCUMM_SCHEDULER_H
#include "scumm_vm.h"
#include "scumm.h"

#define SCUMM_SCHEDULER_SLOTS 16
#define SCUMM_SCHEDULER_STEPS 256

void scumm_scheduler_init(void);
int scumm_scheduler_start_chunk(uint8_t module_id, const ScummChunk *chunk, uint8_t *slot_out);
void scumm_scheduler_stop_all(void);
void scumm_scheduler_update(void);

#endif
