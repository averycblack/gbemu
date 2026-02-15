#include <gb/gb.h>
#include <tchar.h>
#include <stdio.h>
#include <Windows.h>
#include "prefixOpcodes.hpp"

void halt(gbCpu* cpu) {
    cpu->state = CpuState::Halt;
}

void stop(gbCpu* cpu) {
    debugPrint("Stop!\n");
    cpu->state = CpuState::Stop;
}

void fatal(gbCpu* cpu) {
    debugPrint("Unknown Op Code, exiting!\n");
    exit(-1);
}

#define IS_ADD_HALF_CARRY(a, b) !!((( a & 0x0f) + ( b & 0x0f)) > 0x0F)
#define IS_ADC_HALF_CARRY(a, b, c) ((( a & 0x0f) + ( b & 0x0f) + ( c & 0x0f )) > 0x0F)

#define LD_R16_IM16(high, low)                                                      \
void ld_r##low##_im16_low(gbCpu* cpu) { cpu->low = g_gb->readByte(cpu->PC - 2); }   \
void ld_r##high##_im16_high(gbCpu* cpu) { cpu->high = g_gb->readByte(cpu->PC - 1); }\

LD_R16_IM16(B, C)
LD_R16_IM16(D, E)
LD_R16_IM16(H, L)
LD_R16_IM16(SP_h, SP_l)

void ld_rSP_rHL(gbCpu* cpu) {
    cpu->SP = cpu->HL;
}

#define LD_A16_R8(regA, regB)               \
void ld_a##regA##_r##regB##(gbCpu* cpu) {   \
    g_gb->writeByte(cpu->regA, cpu->regB);  \
}

LD_A16_R8(BC, A)
LD_A16_R8(DE, A)

LD_A16_R8(HL, A)
LD_A16_R8(HL, B)
LD_A16_R8(HL, C)
LD_A16_R8(HL, D)
LD_A16_R8(HL, E)
LD_A16_R8(HL, H)
LD_A16_R8(HL, L)

#define INC_R16(regA)               \
void inc_r##regA##(gbCpu* cpu) {    \
    cpu->regA++;                    \
}

INC_R16(BC)
INC_R16(DE)
INC_R16(HL)
INC_R16(SP)

#define DEC_R16(regA)           \
void dec_r##regA##(gbCpu* cpu) {\
    cpu->regA--;                \
}

DEC_R16(BC)
DEC_R16(DE)
DEC_R16(HL)
DEC_R16(SP)

#define INC_R8(reg_a)                               \
void inc_r##reg_a##(gbCpu* cpu) {                   \
    UINT8 val = cpu->reg_a;                         \
    cpu->half_carry = IS_ADD_HALF_CARRY(val, 1);    \
    cpu->reg_a++;                                   \
    cpu->subtract = 0;                              \
    cpu->zero = cpu->reg_a == 0;                    \
}

INC_R8(A)
INC_R8(B)
INC_R8(C)
INC_R8(D)
INC_R8(E)
INC_R8(H)
INC_R8(L)

#define DEC_R8(reg_a)                               \
void dec_r##reg_a##(gbCpu* cpu) {                   \
    cpu->half_carry = ((cpu->reg_a & 0x0F) == 0); \
    cpu->reg_a--;                                  \
    cpu->subtract = 1;                             \
    cpu->zero = cpu->reg_a == 0;                  \
}

DEC_R8(A)
DEC_R8(B)
DEC_R8(C)
DEC_R8(D)
DEC_R8(E)
DEC_R8(H)
DEC_R8(L)

// Increment value loaded from HL
void inc_aHL_write(gbCpu* cpu) {
    UINT8 val = cpu->low;
    cpu->half_carry = IS_ADD_HALF_CARRY(val, 1);
    val++;
    g_gb->writeByte(cpu->HL, val);
    cpu->subtract = 0;
    cpu->zero = val == 0;
}

// Decrement value loaded from HL
void dec_aHL_write(gbCpu* cpu) {
    UINT8 val = cpu->low;
    cpu->half_carry = ((val & 0x0F) == 0);
    val--;
    g_gb->writeByte(cpu->HL, val);
    cpu->subtract = 1;
    cpu->zero = val == 0;
}

#define LD_R8_IM8(reg_a)                        \
void ld_r##reg_a##_im8(gbCpu* cpu) {            \
    cpu->reg_a = g_gb->readByte(cpu->PC - 1);   \
}

LD_R8_IM8(A)
LD_R8_IM8(B)
LD_R8_IM8(C)
LD_R8_IM8(D)
LD_R8_IM8(E)
LD_R8_IM8(H)
LD_R8_IM8(L)

#define ADD_HL_R16(reg_b)                                               \
void add_rHL_r##reg_b##(gbCpu* cpu) {                                   \
    UINT32 res = cpu->HL + cpu->reg_b;                                  \
    UINT16 mask = 0x0FFF;                                               \
    cpu->half_carry = (cpu->HL & mask) + (cpu->reg_b & mask) > mask;    \
    cpu->subtract = 0;                                                  \
    cpu->carry = (res >> 16) & 0x1;                                     \
    cpu->HL = res;                                                      \
}

ADD_HL_R16(BC)
ADD_HL_R16(DE)
ADD_HL_R16(HL)
ADD_HL_R16(SP)

void add_rSP_im8(gbCpu* cpu) {
    cpu->carry = ((cpu->SP & 0xFF) + cpu->low) >> 8 == 1;
    cpu->half_carry = IS_ADD_HALF_CARRY(cpu->SP, cpu->low);
    cpu->SP += (INT8) cpu->low;
    cpu->subtract = 0;
    cpu->zero = 0;
}

void ld_rHL_rSP_im8(gbCpu* cpu) {
    cpu->carry = ((cpu->SP & 0xFF) + cpu->low) >> 8 == 1;
    cpu->half_carry = IS_ADD_HALF_CARRY(cpu->SP, cpu->low);
    cpu->HL = cpu->SP + (INT8) cpu->low;
    cpu->subtract = 0;
    cpu->zero = 0;
}

#define LD_R8_A16(reg_a, reg_b)                 \
void ld_r##reg_a##_a##reg_b##(gbCpu* cpu) {     \
    cpu->reg_a = g_gb->readByte(cpu->reg_b);    \
}

LD_R8_A16(A, BC)
LD_R8_A16(A, DE)

LD_R8_A16(A, HL)
LD_R8_A16(B, HL)
LD_R8_A16(C, HL)
LD_R8_A16(D, HL)
LD_R8_A16(E, HL)
LD_R8_A16(H, HL)
LD_R8_A16(L, HL)

void rlca(gbCpu* cpu) {
    int carry = (cpu->A & 0x80) >> 7;
    UINT16 res = cpu->A << 1;
    cpu->carry = carry;
    cpu->A = res | carry;
    cpu->subtract = 0;
    cpu->half_carry = 0;
    cpu->zero = 0;
}

void rrca(gbCpu* cpu) {
    int carry = cpu->A & 0x1;
    UINT16 res = cpu->A >> 1;
    cpu->carry = carry;
    cpu->A = res | (carry << 7);
    cpu->subtract = 0;
    cpu->half_carry = 0;
    cpu->zero = 0;
}

void rla(gbCpu* cpu) {
    int new_carry = (cpu->A & 0x80) >> 7;
    UINT16 res = cpu->A << 1;
    cpu->A = res | cpu->carry;
    cpu->carry = new_carry;
    cpu->subtract = 0;
    cpu->half_carry = 0;
    cpu->zero = 0;
}

void rra(gbCpu* cpu) {
    int new_carry = cpu->A & 0x1;
    UINT16 res = cpu->A >> 1;
    cpu->A = res | (cpu->carry << 7);
    cpu->carry = new_carry;
    cpu->subtract = 0;
    cpu->half_carry = 0;
    cpu->zero = 0;
}

void im8_read(gbCpu* cpu) {
    cpu->low = g_gb->readByte(cpu->PC - 1);
}

#define IM8_READ_COND(cond)             \
void im8_read_##cond##(gbCpu* cpu) {    \
    im8_read(cpu);                      \
    if (!cpu->cond) {                   \
        cpu->state = CpuState::Fetch;   \
    }                                   \
}

#define IM8_READ_NOT_COND(cond)         \
void im8_read_not_##cond##(gbCpu* cpu) {\
    im8_read(cpu);                      \
    if (cpu->cond) {                    \
        cpu->state = CpuState::Fetch;   \
    }                                   \
}

IM8_READ_COND(carry)
IM8_READ_COND(zero)
IM8_READ_NOT_COND(carry)
IM8_READ_NOT_COND(zero)

void im16_low_read(gbCpu* cpu) {
    cpu->low = g_gb->readByte(cpu->PC - 2);
}

void im16_high_read(gbCpu* cpu) {
    cpu->high = g_gb->readByte(cpu->PC - 1);
}

#define IM16_HIGH_READ_COND(cond)           \
void im16_high_read_##cond##(gbCpu* cpu) {  \
    im16_high_read(cpu);                    \
    if (!cpu->cond) {                       \
        cpu->state = CpuState::Fetch;       \
    }                                       \
}

#define IM16_HIGH_READ_NOT_COND(cond)           \
void im16_high_read_not_##cond##(gbCpu* cpu) {  \
    im16_high_read(cpu);                        \
    if (cpu->cond) {                            \
        cpu->state = CpuState::Fetch;           \
    }                                           \
}

IM16_HIGH_READ_COND(carry)
IM16_HIGH_READ_COND(zero)
IM16_HIGH_READ_NOT_COND(carry)
IM16_HIGH_READ_NOT_COND(zero)

void im16_rSP_high(gbCpu* cpu) {
    g_gb->writeByte(((cpu->high << 8) | cpu->low) + 1, cpu->SP_h);
}

void im16_rSP_low(gbCpu* cpu) {
    g_gb->writeByte((cpu->high << 8) | cpu->low, cpu->SP_l);
}

// Read/write at immediate address

void ld_im16_rA(gbCpu* cpu) {
    UINT16 addr = (cpu->high << 8) | cpu->low;
    g_gb->writeByte(addr, cpu->A);
}

void ld_rA_im16(gbCpu* cpu) {
    UINT16 addr = (cpu->high << 8) | cpu->low;
    cpu->A = g_gb->readByte(addr);
}

// Relative 8-bit jumps

void jr_im8(gbCpu* cpu) {
    cpu->PC += (INT8) cpu->low;
}

// Absolute 16-bit jumps

void jp_im16(gbCpu* cpu) {
    cpu->PC = (cpu->high << 8) | cpu->low;
}

void jp_rHL(gbCpu* cpu) {
    cpu->PC = cpu->HL;
}

// Returns

void ret(gbCpu* cpu) {
    cpu->PC = (cpu->high << 8) | cpu->low;
}

void reti(gbCpu* cpu) {
    ret(cpu);
    cpu->interruptMaster = InterruptEnable::Enabled;
}

void pop_16_low(gbCpu* cpu) {
    cpu->low = g_gb->popStackByte();
}

void pop_16_high(gbCpu* cpu) {
    cpu->high = g_gb->popStackByte();
}

#define RET_COND(cond)                  \
void ret_##cond##(gbCpu* cpu) {         \
    if (!cpu->cond) {                   \
        cpu->state = CpuState::Fetch;   \
    }                                   \
}

#define RET_NOT_COND(cond)              \
void ret_not_##cond##(gbCpu* cpu) {     \
    if (cpu->cond) {                    \
        cpu->state = CpuState::Fetch;   \
    }                                   \
}

RET_COND(carry)
RET_COND(zero)
RET_NOT_COND(carry)
RET_NOT_COND(zero)

// Call subroutines

void call_im16_write_low(gbCpu* cpu) {
    g_gb->pushStackByte(cpu->PC_l);
    cpu->PC_l = cpu->low;
}

void call_im16_write_high(gbCpu* cpu) {
    g_gb->pushStackByte(cpu->PC_h);
    cpu->PC_h = cpu->high;
}

// Set interrupts

void disable_interrupts(gbCpu* cpu) {
    //debugPrint("Interrupt disable\n");
    cpu->interruptMaster = InterruptEnable::Disabled;
}

void enable_interrupts(gbCpu* cpu) {
    //debugPrint("Interrupt enable\n");
    cpu->interruptMaster = InterruptEnable::EnableDelay;
}

// *HL = A, then increment HL
void ld_aHL_inc_rA(gbCpu* cpu) {
    g_gb->writeByte(cpu->HL, cpu->A);
    cpu->HL++;
}

// *HL = A, then decrement HL
void ld_aHL_dec_rA(gbCpu* cpu) {
    g_gb->writeByte(cpu->HL, cpu->A);
    cpu->HL--;
}

// A = *HL, then increment HL
void ld_rA_aHL_inc(gbCpu* cpu) {
    cpu->A = g_gb->readByte(cpu->HL);
    cpu->HL++;
}

// A = *HL, then decrement HL
void ld_rA_aHL_dec(gbCpu* cpu) {
    cpu->A = g_gb->readByte(cpu->HL);
    cpu->HL--;
}

// (HL) = im8
void ld_aHL_im8(gbCpu* cpu) {
    g_gb->writeByte(cpu->HL, cpu->low);
}

// Stack Push/Pop

void pop_rA(gbCpu* cpu) { cpu->A = g_gb->popStackByte(); }
void pop_rF(gbCpu* cpu) { cpu->F = g_gb->popStackByte() & 0xF0; }

#define POP_R16(high, low)                                          \
void pop_r##low##(gbCpu* cpu) { cpu->low = g_gb->popStackByte(); }  \
void pop_r##high##(gbCpu* cpu) { cpu->high = g_gb->popStackByte(); }

POP_R16(B, C)
POP_R16(D, E)
POP_R16(H, L)

void push_rA(gbCpu* cpu) { g_gb->pushStackByte(cpu->A); }
void push_rF(gbCpu* cpu) { g_gb->pushStackByte(cpu->F & 0xF0); }

#define PUSH_R16(high, low)                                         \
void push_r##low##(gbCpu* cpu) { g_gb->pushStackByte(cpu->low); }   \
void push_r##high##(gbCpu* cpu) { g_gb->pushStackByte(cpu->high); }

PUSH_R16(B, C)
PUSH_R16(D, E)
PUSH_R16(H, L)

// Reset to Vector

void rst_high(gbCpu* cpu) {
    g_gb->pushStackByte(cpu->PC_h);
    cpu->PC_h = 0;
}

#define RST_R8(vec)                 \
void rst_##vec##h_low(gbCpu* cpu) { \
    g_gb->pushStackByte(cpu->PC_l); \
    cpu->PC_l = 0x##vec;            \
}

// Given in hex
RST_R8(00)
RST_R8(08)
RST_R8(10)
RST_R8(18)
RST_R8(20)
RST_R8(28)
RST_R8(30)
RST_R8(38)

// Decimal Adjust A (Used for BCD arithmatic)
// https://ehaskins.com/2018-01-30%20Z80%20DAA/
void daa(gbCpu* cpu) {
    UINT8 correction = 0;
    bool carry = false;
    
    if (cpu->carry || (!cpu->subtract && cpu->A > 0x99)) {
        correction |= 0x60;
        carry = true;
    }

    if (cpu->half_carry || (!cpu->subtract && (cpu->A & 0xF) > 0x9)) {
        correction |= 0x06;
    }

    cpu->A += cpu->subtract ? -correction : correction;

    cpu->zero = cpu->A == 0;
    cpu->half_carry = 0;
    cpu->carry = carry;
}


// Invert the A register
void cpl(gbCpu* cpu) {
    cpu->A ^= 0xFF;
    cpu->half_carry = 1;
    cpu->subtract = 1;
}

// Set carry flag
void scf(gbCpu* cpu) {
    cpu->carry = 1;
    cpu->subtract = 0;
    cpu->half_carry = 0;
}

// Compliment carry flag
void ccf(gbCpu* cpu) {
    cpu->carry = ~cpu->carry;
    cpu->subtract = 0;
    cpu->half_carry = 0;
}

void prefix(gbCpu* cpu) {
    cpu->state = CpuState::Prefix;
}

#define LD_R8_R8(reg_a, reg_b)              \
void ld_r##reg_a##_r##reg_b##(gbCpu* cpu) { \
    cpu->reg_a = cpu->reg_b;                \
}

#define LD_R8_ALL(reg_a)    \
LD_R8_R8(reg_a, A)          \
LD_R8_R8(reg_a, B)          \
LD_R8_R8(reg_a, C)          \
LD_R8_R8(reg_a, D)          \
LD_R8_R8(reg_a, E)          \
LD_R8_R8(reg_a, H)          \
LD_R8_R8(reg_a, L)

LD_R8_ALL(A)
LD_R8_ALL(B)
LD_R8_ALL(C)
LD_R8_ALL(D)
LD_R8_ALL(E)
LD_R8_ALL(H)
LD_R8_ALL(L)

void ld_io_im8_rA(gbCpu* cpu) {
    g_gb->writeByte(cpu->low + 0xFF00, cpu->A);
}

void ld_io_rA_im8(gbCpu* cpu) {
    cpu->A = g_gb->readByte(cpu->low + 0xFF00);
}

void ld_io_aC_rA(gbCpu* cpu) {
    g_gb->writeByte(cpu->C + 0xFF00, cpu->A);
}

void ld_io_rA_aC(gbCpu* cpu) {
    cpu->A = g_gb->readByte(cpu->C + 0xFF00);
}

void add(gbCpu* cpu, UINT8 val) {
    cpu->half_carry = IS_ADD_HALF_CARRY(cpu->A, val);
    cpu->subtract = 0;
    UINT16 res = cpu->A + val;
    cpu->carry = (res >> 8) & 0x1;
    cpu->A = (UINT8)res;
    cpu->zero = cpu->A == 0;
}

void add_rA_im8(gbCpu* cpu) {
    im8_read(cpu);
    add(cpu, cpu->low);
}

void adc(gbCpu* cpu, UINT8 val) {
    cpu->half_carry = IS_ADC_HALF_CARRY(cpu->A, val, cpu->carry);
    cpu->subtract = 0;
    UINT16 res = cpu->A + val + cpu->carry;
    cpu->carry = (res >> 8) & 0x1;
    cpu->A = (UINT8)res;
    cpu->zero = cpu->A == 0;
}

void adc_rA_im8(gbCpu* cpu) {
    im8_read(cpu);
    adc(cpu, cpu->low);
}

void sub(gbCpu* cpu, UINT8 val) {
    cpu->half_carry = (val & 0xf) > (cpu->A & 0xf);
    cpu->carry = val > cpu->A;
    cpu->subtract = 1;
    INT8 res = cpu->A - val;
    cpu->zero = res == 0;
    cpu->A = res;
}

void sub_rA_im8(gbCpu* cpu) {
    im8_read(cpu);
    sub(cpu, cpu->low);
}

// https://github.com/rvaccarim/FrozenBoy/blob/master/FrozenBoyCore/Processor/Opcode/OpcodeHandler.cs#L672
void sbc(gbCpu* cpu, UINT8 val) {
    int res = cpu->A - val - cpu->carry;
    cpu->half_carry = !!(( cpu->A ^ val ^ (res & 0xFF)) & 0x10);
    cpu->carry = res < 0;
    cpu->subtract = 1;
    cpu->zero = (res & 0xFF) == 0;
    cpu->A = (UINT8) res;
}

void sbc_rA_im8(gbCpu* cpu) {
    im8_read(cpu);
    sbc(cpu, cpu->low);
}

void _and(gbCpu* cpu, UINT8 val) {
    cpu->half_carry = 1;
    cpu->subtract = 0;
    cpu->A &= val;
    cpu->carry = 0;
    cpu->zero = cpu->A == 0;
}

void and_rA_im8(gbCpu* cpu) {
    im8_read(cpu);
    _and(cpu, cpu->low);
}

void _xor(gbCpu* cpu, UINT8 val) {
    cpu->half_carry = 0;
    cpu->subtract = 0;
    cpu->A ^= val;
    cpu->carry = 0;
    cpu->zero = cpu->A == 0;
}

void xor_rA_im8(gbCpu* cpu) {
    im8_read(cpu);
    _xor(cpu, cpu->low);
}

void _or(gbCpu* cpu, UINT8 val) {
    cpu->half_carry = 0;
    cpu->subtract = 0;
    cpu->A |= val;
    cpu->carry = 0;
    cpu->zero = cpu->A == 0;
}

void or_rA_im8(gbCpu* cpu) {
    im8_read(cpu);
    _or(cpu, cpu->low);
}

// Compare A and val
void cp(gbCpu* cpu, UINT8 val) {
    cpu->half_carry = (val & 0xf) > (cpu->A & 0xf);
    cpu->carry = val > cpu->A;
    cpu->subtract = 1;
    cpu->zero = cpu->A == val;
}

void cp_rA_im8(gbCpu* cpu) {
    im8_read(cpu);
    cp(cpu, cpu->low);
}

#define ALU_A_R8(alu_op, reg_b)     \
void alu_op##_rA_r##reg_b##(gbCpu* cpu) {   \
    alu_op(cpu, cpu->reg_b);                     \
}

#define ALU_A_AHL(alu_op)               \
void alu_op##_rA_aHL(gbCpu* cpu) {      \
    alu_op(cpu, g_gb->readByte(cpu->HL));    \
}

#define ALU_A_ALL(alu_op)   \
ALU_A_R8(alu_op, B)         \
ALU_A_R8(alu_op, C)         \
ALU_A_R8(alu_op, D)         \
ALU_A_R8(alu_op, E)         \
ALU_A_R8(alu_op, H)         \
ALU_A_R8(alu_op, L)         \
ALU_A_AHL(alu_op)           \
ALU_A_R8(alu_op, A)

ALU_A_ALL(add)
ALU_A_ALL(adc)
ALU_A_ALL(sub)
ALU_A_ALL(sbc)
ALU_A_ALL(_and)
ALU_A_ALL(_xor)
ALU_A_ALL(_or)
ALU_A_ALL(cp)

#define INSTR_ALU_A_ALL(alu_op_upper, alu_op_lower)                     \
    { alu_op_upper " A, B", 1, 1, { ##alu_op_lower##_rA_rB }},          \
    { alu_op_upper " A, C", 1, 1, { ##alu_op_lower##_rA_rC }},          \
    { alu_op_upper " A, D", 1, 1, { ##alu_op_lower##_rA_rD }},          \
    { alu_op_upper " A, E", 1, 1, { ##alu_op_lower##_rA_rE }},          \
    { alu_op_upper " A, H", 1, 1, { ##alu_op_lower##_rA_rH }},          \
    { alu_op_upper " A, L", 1, 1, { ##alu_op_lower##_rA_rL }},          \
    { alu_op_upper " A, (HL)", 1, 2, { nop, ##alu_op_lower##_rA_aHL }}, \
    { alu_op_upper " A, A", 1, 1, { ##alu_op_lower##_rA_rA }}

#define INSTR_LD_R8_ALL(reg_a)                                  \
    { "LD " #reg_a", B", 1, 1, { ld_r##reg_a##_rB }},           \
    { "LD " #reg_a", C", 1, 1, { ld_r##reg_a##_rC }},           \
    { "LD " #reg_a", D", 1, 1, { ld_r##reg_a##_rD }},           \
    { "LD " #reg_a", E", 1, 1, { ld_r##reg_a##_rE }},           \
    { "LD " #reg_a", H", 1, 1, { ld_r##reg_a##_rH }},           \
    { "LD " #reg_a", L", 1, 1, { ld_r##reg_a##_rL }},           \
    { "LD " #reg_a", (HL)", 1, 2, { nop, ld_r##reg_a##_aHL }},  \
    { "LD " #reg_a", A", 1, 1, { ld_r##reg_a##_rA }}

#define INSTR_UNKNOWN \
    { "UNKOWN", 1, 1, { fatal }}

/* name, bytes, cycles, steps[] */
static const Instruction instructions[] {
    /* 0x00 */
    { "NOP",            1, 1, { nop }},
    { "LD BC, d16",     3, 3, { nop, ld_rC_im16_low, ld_rB_im16_high }},
    { "LD (BC), A",     1, 2, { nop, ld_aBC_rA }},
    { "INC BC",         1, 2, { nop, inc_rBC }},
    { "INC B",          1, 1, { inc_rB }},
    { "DEC B",          1, 1, { dec_rB }},
    { "LD B, d8",       2, 2, { nop, ld_rB_im8 }},
    { "RLCA",           1, 1, { rlca } },
    { "LD (a16), SP",   3, 5, { nop,
                                im16_low_read, im16_high_read,
                                im16_rSP_low, im16_rSP_high }},
    { "ADD HL, BC",     1, 2, { nop, add_rHL_rBC }},
    { "LD A, (BC)",     1, 2, { nop, ld_rA_aBC }},
    { "DEC BC",         1, 2, { nop, dec_rBC }},
    { "INC C",          1, 1, { inc_rC }},
    { "DEC C",          1, 1, { dec_rC }},
    { "LD C, d8",       2, 2, { nop, ld_rC_im8 }},
    { "RRCA",           1, 1, { rrca }},
    /* 0x10 */
    { "STOP",           1, 1, { stop }},
    { "LD DE, d16",     3, 3, { nop, ld_rE_im16_low, ld_rD_im16_high }},
    { "LD (DE), A",     1, 2, { nop, ld_aDE_rA }},
    { "INC DE",         1, 2, { nop, inc_rDE }},
    { "INC D",          1, 1, { inc_rD }},
    { "DEC D",          1, 1, { dec_rD }},
    { "LD D, d8",       2, 2, { nop, ld_rD_im8 }},
    { "RLA",            1, 1, { rla }},
    { "JR r8",          2, 3, { nop, im8_read, jr_im8 }},
    { "ADD HL, DE",     1, 2, { nop, add_rHL_rDE }},
    { "LD A, (DE)",     1, 2, { nop, ld_rA_aDE }},
    { "DEC DE",         1, 2, { nop, dec_rDE }},
    { "INC E",          1, 1, { inc_rE }},
    { "DEC E",          1, 1, { dec_rE }},
    { "LD E, d8",       2, 2, { nop, ld_rE_im8 }},
    { "RRA",            1, 1, { rra }},
    /* 0x20 */
    { "JR NZ, r8",      2, 3, { nop, im8_read_not_zero, jr_im8 }},
    { "LD HL, d16",     3, 3, { nop, ld_rL_im16_low, ld_rH_im16_high }},
    { "LD (HL+), A",    1, 2, { nop, ld_aHL_inc_rA }},
    { "INC HL",         1, 2, { nop, inc_rHL }},
    { "INC H",          1, 1, { inc_rH }},
    { "DEC H",          1, 1, { dec_rH }},
    { "LD H, d8",       2, 2, { nop, ld_rH_im8 }},
    { "DAA",            1, 1, { daa }},
    { "JR Z, r8",       2, 3, { nop, im8_read_zero, jr_im8 }},
    { "ADD HL, HL",     1, 2, { nop, add_rHL_rHL }},
    { "LD A, (HL+)",    1, 2, { nop, ld_rA_aHL_inc }},
    { "DEC HL",         1, 2, { nop, dec_rHL }},
    { "INC L",          1, 1, { inc_rL }},
    { "DEC L",          1, 1, { dec_rL }},
    { "LD L, d8",       2, 2, { nop, ld_rL_im8 }},
    { "CPL",            1, 1, { cpl }},
    /* 0x30 */
    { "JR NC, r8",      2, 3, { nop, im8_read_not_carry, jr_im8 }},
    { "LD SP, d16",     3, 3, { nop, ld_rSP_l_im16_low, ld_rSP_h_im16_high }},
    { "LD (HL-), A",    1, 2, { nop, ld_aHL_dec_rA }},
    { "INC SP",         1, 2, { nop, inc_rSP }},
    { "INC (HL)",       1, 3, { nop, aHL_read, inc_aHL_write }},
    { "DEC (HL)",       1, 3, { nop, aHL_read, dec_aHL_write }},
    { "LD (HL), d8",    2, 3, { nop, im8_read, ld_aHL_im8 }},
    { "SCF",            1, 1, { scf }},
    { "JR C, r8",       2, 3, { nop, im8_read_carry, jr_im8 }},
    { "ADD HL, SP",     1, 2, { nop, add_rHL_rSP }},
    { "LD A, (HL-)",    1, 2, { nop, ld_rA_aHL_dec }},
    { "DEC SP",         1, 2, { nop, dec_rSP} },
    { "INC A",          1, 1, { inc_rA }},
    { "DEC A",          1, 1, { dec_rA }},
    { "LD A, d8",       2, 2, { nop, ld_rA_im8 }},
    { "CCF",            1, 1, { ccf }},
    /* 0x40 */
    INSTR_LD_R8_ALL(B),
    INSTR_LD_R8_ALL(C),
    /* 0x50 */
    INSTR_LD_R8_ALL(D),
    INSTR_LD_R8_ALL(E),
    /* 0x60 */
    INSTR_LD_R8_ALL(H),
    INSTR_LD_R8_ALL(L),
    /* 0x70 */
    { "LD (HL), B",     1, 2, { nop, ld_aHL_rB }},
    { "LD (HL), C",     1, 2, { nop, ld_aHL_rC }},
    { "LD (HL), D",     1, 2, { nop, ld_aHL_rD }},
    { "LD (HL), E",     1, 2, { nop, ld_aHL_rE }},
    { "LD (HL), H",     1, 2, { nop, ld_aHL_rH }},
    { "LD (HL), L",     1, 2, { nop, ld_aHL_rL }},
    { "HALT",           1, 1, { halt }},
    { "LD (HL), A",     1, 2, { nop, ld_aHL_rA }},
    INSTR_LD_R8_ALL(A),
    /* 0x80 */
    INSTR_ALU_A_ALL("ADD", add),
    INSTR_ALU_A_ALL("ADC", adc),
    /* 0x90 */
    INSTR_ALU_A_ALL("SUB", sub),
    INSTR_ALU_A_ALL("SBC", sbc),
    /* 0xA0 */
    INSTR_ALU_A_ALL("AND", _and),
    INSTR_ALU_A_ALL("XOR", _xor),
    /* 0xB0 */
    INSTR_ALU_A_ALL("OR", _or),
    INSTR_ALU_A_ALL("CP", cp),
    /* 0xC0 */
    { "RET NZ",         1, 5, { nop, ret_not_zero,
                                pop_16_low, pop_16_high,
                                ret }},
    { "POP BC",         1, 3, { nop, pop_rC, pop_rB }},
    { "JP NZ, a16",     3, 4, { nop,
                                im16_low_read, im16_high_read_not_zero,
                                jp_im16 }},
    { "JP a16",         3, 4, { nop,
                                im16_low_read, im16_high_read,
                                jp_im16 }},
    { "CALL NZ, a16",   3, 6, { nop,
                                im16_low_read, im16_high_read_not_zero,
                                nop,
                                call_im16_write_high, call_im16_write_low }},
    { "PUSH BC",        1, 4, { nop, nop, push_rB, push_rC }},
    { "ADD A, d8",      2, 2, { nop, add_rA_im8 } },
    { "RST 00H",        1, 4, { nop, nop, rst_high, rst_00h_low }},
    { "RET Z",          1, 5, { nop, ret_zero,
                                pop_16_low, pop_16_high,
                                ret }},
    { "RET",            1, 4, { nop,
                                pop_16_low, pop_16_high,
                                ret }},
    { "JP Z, a16",      3, 4, { nop,
                                im16_low_read, im16_high_read_zero,
                                jp_im16 }},
    { "PREFIX",         1, 1, { prefix }},
    { "CALL Z, a16",    3, 6, { nop,
                                im16_low_read, im16_high_read_zero,
                                nop,
                                call_im16_write_high, call_im16_write_low }},
    { "CALL a16",       3, 6, { nop,
                                im16_low_read, im16_high_read,
                                nop,
                                call_im16_write_high, call_im16_write_low }},
    { "ADC A, d8",      2, 2, { nop, adc_rA_im8 }},
    { "RST 08H",        1, 4, { nop, nop, rst_high, rst_08h_low }},
    /* 0xD0 */
    { "RET NC",         1, 5, { nop, ret_not_carry,
                                pop_16_low, pop_16_high,
                                ret }},
    { "POP DE",         1, 3, { nop, pop_rE, pop_rD }},
    { "JP NC, a16",     3, 4, { nop,
                                im16_low_read, im16_high_read_not_carry,
                                jp_im16 }},
    INSTR_UNKNOWN,
    { "CALL NC, a16",   3, 6, { nop,
                                im16_low_read, im16_high_read_not_carry,
                                nop,
                                call_im16_write_high, call_im16_write_low }},
    { "PUSH DE",        1, 4, { nop, nop, push_rD, push_rE }},
    { "SUB d8",         2, 2, { nop, sub_rA_im8 }},
    { "RST 10h",        1, 4, { nop, nop, rst_high, rst_10h_low }},
    { "RET C",          1, 5, { nop, ret_carry,
                                pop_16_low, pop_16_high,
                                ret }},
    { "RETI",           1, 4, { nop,
                                pop_16_low, pop_16_high,
                                reti }},
    { "JP C, a16",      3, 4, { nop,
                                im16_low_read, im16_high_read_carry,
                                jp_im16 }},
    INSTR_UNKNOWN,
    { "CALL C, a16",    3, 6, { nop,
                                im16_low_read, im16_high_read_carry,
                                nop,
                                call_im16_write_high, call_im16_write_low }},
    INSTR_UNKNOWN,
    { "SBC A, d8",      2, 2, { nop, sbc_rA_im8 }},
    { "RST 18H",        1, 4, { nop, nop, rst_high, rst_18h_low }},
    /* 0xE0 */
    { "LDH (a8), A",    2, 3, { nop, im8_read, ld_io_im8_rA }},
    { "POP HL",         1, 3, { nop, pop_rL, pop_rH }},
    { "LD (C), A",      1, 2, { nop, ld_io_aC_rA }},
    INSTR_UNKNOWN,
    INSTR_UNKNOWN,
    { "PUSH HL",        1, 4, { nop, nop, push_rH, push_rL }},
    { "AND d8",         2, 2, { nop, and_rA_im8 }},
    { "RST 20H",        1, 4, { nop, nop, rst_high, rst_20h_low }},
    { "ADD SP, i8",     2, 4, { nop, im8_read, nop, add_rSP_im8 }},
    { "JP HL",          1, 1, { jp_rHL }},
    { "LD (a16), A",    3, 4, { nop, im16_low_read, im16_high_read, ld_im16_rA }},
    INSTR_UNKNOWN,
    INSTR_UNKNOWN,
    INSTR_UNKNOWN,
    { "XOR d8",         2, 2, { nop, xor_rA_im8 }},
    { "RST 28H",        1, 4, { nop, nop, rst_high, rst_28h_low }},
    /* 0xF0 */
    { "LDH A, (a8)",    2, 3, { nop, im8_read, ld_io_rA_im8 }},
    { "POP AF",         1, 3, { nop, pop_rF, pop_rA }},
    { "LD A, (C)",      1, 2, { nop, ld_io_rA_aC }},
    { "DI",             1, 1, { disable_interrupts }},
    INSTR_UNKNOWN,
    { "PUSH AF",        1, 4, { nop, nop, push_rA, push_rF, }},
    { "OR d8",          2, 2, { nop, or_rA_im8 }},
    { "RST 30H",        1, 4, { nop, nop, rst_high, rst_30h_low }},
    { "LD HL, SP + r8", 2, 3, { nop, im8_read, ld_rHL_rSP_im8 }},
    { "LD SP, HL",      1, 2, { nop, ld_rSP_rHL }},
    { "LD A, (a16)",    3, 4, { nop, im16_low_read, im16_high_read, ld_rA_im16 }},
    { "EI",             1, 1, { enable_interrupts }},
    INSTR_UNKNOWN,
    INSTR_UNKNOWN,
    { "CP d8",          2, 2, { nop, cp_rA_im8 }},
    { "RST 38H",        1, 4, { nop, nop, rst_high, rst_38h_low }}
};

static_assert(sizeof(instructions) == 0x100 * sizeof(Instruction));

// Interrupts

void push_rPC_l(gbCpu* cpu) { g_gb->pushStackByte(cpu->PC_l); }
void push_rPC_h(gbCpu* cpu) { g_gb->pushStackByte(cpu->PC_h); }

// Set PC to interrupt vector
void int_rPC_40h(gbCpu *cpu) { cpu->PC = 0x0040; }
void int_rPC_48h(gbCpu* cpu) { cpu->PC = 0x0048; }
void int_rPC_50h(gbCpu* cpu) { cpu->PC = 0x0050; }
void int_rPC_58h(gbCpu* cpu) { cpu->PC = 0x0058; }
void int_rPC_60h(gbCpu* cpu) { cpu->PC = 0x0060; }

static const Instruction interruptInstructions[] {
    { "INT VBlank",     0, 5, { disable_interrupts, nop, push_rPC_h, push_rPC_l, int_rPC_40h }},
    { "INT LCD Stat",   0, 5, { disable_interrupts, nop, push_rPC_h, push_rPC_l, int_rPC_48h }},
    { "INT Timer",      0, 5, { disable_interrupts, nop, push_rPC_h, push_rPC_l, int_rPC_50h }},
    { "INT Serial",     0, 5, { disable_interrupts, nop, push_rPC_h, push_rPC_l, int_rPC_58h }},
    { "INT Joypad",     0, 5, { disable_interrupts, nop, push_rPC_h, push_rPC_l, int_rPC_60h }},
};

const Instruction* gbCpu::getIntInstruction() {
    UINT8 enabled = interruptFlags & interruptsEnabled;
    for (int i = 0; i < 5; i++) {
        if ((enabled & 0x1) == 1) {
            interruptFlags &= ~(1 << i);
            return &interruptInstructions[i];
        }
        enabled >>= 1;
    }

    return nullptr;
}

void gbCpu::printOp(UINT8 opByte) {
    if (currentOp->bytes == 1) {
        debugPrint("PC: 0x%4x | OP: (0x%2x) %-15s\n",
            PC, opByte,
            currentOp->name
        );
    } else {
        debugPrint("PC: 0x%4x | OP: (0x%2x) %-15s | 0x%x\n",
            PC, opByte,
            currentOp->name,
            currentOp->bytes == 2 ?
                g_gb->readByte(PC + 1) : g_gb->readShort(PC + 1)
        );
    }
}

void gbCpu::step()
{
    UINT8 opByte = 0x00;
    bool eiHaltBug, intRequest;

    switch (state) {
    case CpuState::Halt:
        // Wait until interrupt is requested
        if (requestedIntr() == 0) break;

        [[fallthrough]];
    case CpuState::Fetch:
        opStep = 0;

        opByte = g_gb->readByte(PC);
        
        // If EI then Halt is executed, then interrupt will be serviced before halt
        eiHaltBug = interruptMaster == InterruptEnable::EnableDelay
            && opByte == 0x76 /* HALT */ && requestedIntr() != 0;

        intRequest = interruptMaster == InterruptEnable::Enabled
            && requestedIntr() != 0;

        if (eiHaltBug || intRequest) {
            currentOp = getIntInstruction();
        } else {
            currentOp = &instructions[opByte];
        }

        // EI interrupt enable has a 1 instruction delay
        if (interruptMaster == InterruptEnable::EnableDelay)
            interruptMaster = InterruptEnable::Enabled;

#ifdef _DEBUG
        //printOp(opByte);
#endif

        PC += currentOp->bytes;
        state = CpuState::Working;
        [[fallthrough]];
    case CpuState::Working:
        currentOp->steps[opStep](this);
        opStep++;
        if (opStep == currentOp->num_steps &&
            state == CpuState::Working) {
            state = CpuState::Fetch;

            // If Halt was called with IME=0 and pending interrupts, current byte is read again
            if (halt_bug) {
                halt_bug = false;
                PC -= currentOp->bytes;
            }
        }

        break;
    case CpuState::Prefix:
        opByte = g_gb->readByte(PC);
        currentOp = &prefixInstructions[opByte];
        opStep = 0;

        PC += 1;
        state = CpuState::PrefixWork;

        [[fallthrough]];
    case CpuState::PrefixWork:
        currentOp->steps[opStep](this);
        opStep++;

        if (opStep == currentOp->num_steps &&
            state == CpuState::PrefixWork) {
            state = CpuState::Fetch;

            // If Halt was called with IME=0 and pending interrupts, current byte is read again
            if (halt_bug) {
                halt_bug = false;
                PC -= currentOp->bytes;
            }
        }
        break;
    }
}

int gbCpu::writeByte(UINT16 addr, UINT8 val) {
    switch (addr) {
    case 0xFF0F: interruptFlags = val; break;
    case 0xFFFF: interruptsEnabled = val; break;
    default: return -1;
    }

    return 0;
}

int gbCpu::readByte(UINT16 addr) {
    switch (addr) {
    case 0xFF0F: return interruptFlags;
    case 0xFFFF: return interruptsEnabled;
    default: return -1;
    }
}