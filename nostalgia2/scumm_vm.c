// Creado por Slam (2026)
#include "../include/scumm_vm.h"
#include "../include/scumm.h"
#include "../include/serial.h"

static void vm_fail(ScummVM *vm) {
    if (vm) { vm->running = 0; vm->error = 1; }
}

static void push(ScummVM *vm, uint16_t value) {
    if (vm->sp >= 255) vm_fail(vm);
    else vm->stack[++vm->sp] = value;
}

static uint16_t pop(ScummVM *vm) {
    if (vm->sp == 0) { vm_fail(vm); return 0; }
    return vm->stack[vm->sp--];
}

static int fetch8(ScummVM *vm, uint8_t *value) {
    if (!vm || !vm->running || vm->pc >= vm->size || 
        !scumm_resource_read8(vm->module_id, vm->resource_offset + vm->pc, value)) {
        vm_fail(vm); return 0;
    }
    vm->pc++;
    return 1;
}

static int fetch16(ScummVM *vm, uint16_t *value) {
    uint8_t lo, hi;
    if (!fetch8(vm, &lo) || !fetch8(vm, &hi)) return 0;
    *value = (uint16_t)lo | ((uint16_t)hi << 8);
    return 1;
}

static int fetch_param(ScummVM *vm, uint8_t opcode, uint8_t mask, uint16_t *value) {
    uint16_t raw;
    if (!fetch16(vm, &raw)) return 0;
    if ((opcode & mask) != 0) {
        uint16_t idx = raw & 0x0FFF;
        if (idx >= SCUMM_VM_VARIABLES) { vm_fail(vm); return 0; }
        *value = vm->variables[idx];
    } else {
        *value = raw;
    }
    return 1;
}

static int fetch_result_pos(ScummVM *vm, uint16_t *index) {
    uint16_t base, offset;
    if (!fetch16(vm, &base)) return 0;
    uint32_t eff = base;

    if (base & 0x2000) {
        if (!fetch16(vm, &offset)) return 0;
        base &= ~0x2000;
        eff = base + ((offset & 0x2000) ? vm->variables[offset & ~0x2000] : (offset & 0x0FFF));
    }
    if (eff >= SCUMM_VM_VARIABLES) { vm_fail(vm); return 0; }
    *index = (uint16_t)eff;
    return 1;
}

int scumm_vm_init_resource(ScummVM *vm, uint8_t module_id, uint32_t offset, uint32_t size) {
    if (!vm || size == 0 || offset > scumm_resource_size(module_id)) return 0;
    vm->module_id = module_id;
    vm->resource_offset = offset;
    vm->size = size;
    vm->pc = 0;
    vm->sp = 0;
    vm->running = 1;
    vm->error = 0;
    for (int i = 0; i < SCUMM_VM_VARIABLES; i++) vm->variables[i] = 0;
    return 1;
}

void scumm_vm_stop(ScummVM *vm) { if (vm) vm->running = 0; }

int scumm_vm_run(ScummVM *vm, uint32_t max_steps) {
    uint32_t steps = 0;

    while (vm->running && steps < max_steps) {
        uint8_t opcode;
        uint32_t saved_pc = vm->pc;

        if (!fetch8(vm, &opcode)) return 0;
        steps++;

        switch (opcode) {
            case 0x00: case 0x80: /* stopObjectCode */
                vm->running = 0; 
                return 1;

            case 0x19: case 0x99: /* breakHere */
                return 1;

            case 0x16: case 0x96: { /* startScript */
                uint16_t script_id;
                if (!fetch_param(vm, opcode, 0x80, &script_id)) return 0;
                scumm_start_script(script_id);
                break;
            }

            case 0x01: case 0x81: { /* pushWord */
                uint16_t op;
                if (!fetch_param(vm, opcode, 0x80, &op)) return 0;
                push(vm, op);
                break;
            }

            case 0x0E: case 0x8E: case 0x0F: case 0x8F: /* add / sub */
            case 0x14: case 0x94: case 0x15: case 0x95: /* mul / div */
            case 0x1A: case 0x9A: /* move */
            {
                uint16_t dest, op;
                if (!fetch_result_pos(vm, &dest) || !fetch_param(vm, opcode, 0x80, &op)) return 0;
                uint16_t cur = vm->variables[dest];
                switch (opcode & 0x7F) {
                    case 0x0E: vm->variables[dest] = cur + op; break;
                    case 0x0F: vm->variables[dest] = cur - op; break;
                    case 0x14: vm->variables[dest] = cur * op; break;
                    case 0x15: if(op != 0) vm->variables[dest] = cur / op; break;
                    case 0x1A: vm->variables[dest] = op; break;
                }
                break;
            }

            case 0x1B: case 0x9B: case 0x1C: case 0x9C: { /* inc / dec */
                uint16_t dest;
                if (!fetch_result_pos(vm, &dest)) return 0;
                if ((opcode & 0x7F) == 0x1B) vm->variables[dest]++; else vm->variables[dest]--;
                break;
            }

            case 0x18: case 0x98: { /* jumpRelative */
                uint16_t offset;
                if (!fetch16(vm, &offset)) return 0;
                vm->pc = (uint32_t)((int32_t)saved_pc + (int16_t)offset);
                break;
            }

            case 0x04: case 0x84: case 0x08: case 0x88: /* isEqual / NotEqual */
            case 0x09: case 0x89: case 0x0A: case 0x8A: /* GTEqual / Less */
            case 0x1D: case 0x9D: case 0x1E: case 0x9E: /* Greater / LTEqual */
            {
                uint16_t op, offset;
                if (!fetch_param(vm, opcode, 0x80, &op) || !fetch16(vm, &offset)) return 0;
                uint16_t a = pop(vm);
                int cond = 0;
                switch (opcode & 0x7F) {
                    case 0x04: cond = (a == op); break;
                    case 0x08: cond = (a != op); break;
                    case 0x09: cond = ((int16_t)a >= (int16_t)op); break;
                    case 0x0A: cond = ((int16_t)a < (int16_t)op); break;
                    case 0x1D: cond = ((int16_t)a > (int16_t)op); break;
                    case 0x1E: cond = ((int16_t)a <= (int16_t)op); break;
                }
                if (cond) vm->pc = (uint32_t)((int32_t)saved_pc + (int16_t)offset);
                break;
            }

            case 0x05: case 0x45: case 0x85: case 0xC5: { /* drawObject */
                uint16_t obj, state;
                if (!fetch_param(vm, opcode, 0x80, &obj) || !fetch_param(vm, opcode, 0x40, &state)) return 0;
                scumm_set_object_state(obj, (uint8_t)state);
                break;
            }

            default:
                serial_write("VM OP DESCONOCIDO 0x");
                serial_write_hex(opcode);
                serial_write(" en PC ");
                serial_write_dec(saved_pc);
                serial_write("\n");
                vm_fail(vm);
                return 0;
        }
    }
    return vm->running ? 1 : 0;
}
