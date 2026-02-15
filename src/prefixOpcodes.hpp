#pragma once

#include <gb/gb.h>
#include <gb/gbcpu.h>

void nop(gbCpu* cpu) {}

// Load value pointed to by HL
void aHL_read(gbCpu* cpu) {
    cpu->low = g_gb->readByte(cpu->HL);
}

// Rotate left/right no carry
UINT8 rlc(gbCpu* cpu, UINT8 val) {
    int carry = (val >> 7) & 0x1;
    cpu->carry = carry;
    cpu->zero = val == 0;
    cpu->subtract = 0;
    cpu->half_carry = 0;
    return (val << 1) | carry;
}

UINT8 rrc(gbCpu* cpu, UINT8 val) {
    int carry = val & 0x1;
    cpu->carry = carry;
    cpu->zero = val == 0;
    cpu->subtract = 0;
    cpu->half_carry = 0;
    return (val >> 1) | (carry << 7);
}

// Rotate left/right through carry
UINT8 rl(gbCpu* cpu, UINT8 val) {
    int new_carry = (val >> 7) & 0x1;
    val = (val << 1) | cpu->carry;
    cpu->carry = new_carry;
    cpu->zero = val == 0;
    cpu->subtract = 0;
    cpu->half_carry = 0;
    return val;
}

UINT8 rr(gbCpu* cpu, UINT8 val) {
    int new_carry = val & 0x1;
    val = (val >> 1) | (cpu->carry << 7);
    cpu->carry = new_carry;
    cpu->zero = val == 0;
    cpu->subtract = 0;
    cpu->half_carry = 0;
    return val;
}

// Arithmatic Left/Right Shift
UINT8 sla(gbCpu* cpu, UINT8 val) {
    cpu->carry = (val >> 7) & 0x1;
    val <<= 1;
    cpu->zero = val == 0;
    cpu->subtract = 0;
    cpu->half_carry = 0;
    return val;
}

UINT8 sra(gbCpu* cpu, UINT8 val) {
    cpu->carry = val & 0x1;
    val = (val >> 1) | (val & 0x80);
    cpu->zero = val == 0;
    cpu->subtract = 0;
    cpu->half_carry = 0;
    return val;
}

// Logic Right Shift
UINT8 srl(gbCpu* cpu, UINT8 val) {
    cpu->carry = val & 0x1;
    val = val >> 1;
    cpu->zero = val == 0;
    cpu->subtract = 0;
    cpu->half_carry = 0;
    return val;
}

// Swap low/high nibbles
UINT8 swap(gbCpu* cpu, UINT8 val) {
    cpu->zero = val == 0;
    cpu->carry = 0;
    cpu->half_carry = 0;
    cpu->subtract = 0;
    return ((val & 0x0F) << 4) | ((val & 0xF0) >> 4);
}

#define PREFIX_FUNC(op, reg)		            \
void prefix_##op##_r##reg##(gbCpu* cpu) {		\
    cpu->reg = op(cpu, cpu->reg);		        \
}

#define PREFIX_FUNC_HL(op)                          \
void prefix_##op##_rHL(gbCpu* cpu) {                \
    g_gb->writeByte(cpu->HL, op(cpu, cpu->low));    \
}

#define PREFIX_FUNC_ALL(op)	\
PREFIX_FUNC(op, B)			\
PREFIX_FUNC(op, C)			\
PREFIX_FUNC(op, D)			\
PREFIX_FUNC(op, E)			\
PREFIX_FUNC(op, H)			\
PREFIX_FUNC(op, L)			\
PREFIX_FUNC_HL(op)			\
PREFIX_FUNC(op, A)

PREFIX_FUNC_ALL(rlc)
PREFIX_FUNC_ALL(rrc)
PREFIX_FUNC_ALL(rl)
PREFIX_FUNC_ALL(rr)
PREFIX_FUNC_ALL(sla)
PREFIX_FUNC_ALL(sra)
PREFIX_FUNC_ALL(swap)
PREFIX_FUNC_ALL(srl)

#define PREFIX_INSTR_ALL(op_upper, op_lower)									\
    { #op_upper " B",		1, 1, { prefix_##op_lower##_rB }},					\
    { #op_upper " C",		1, 1, { prefix_##op_lower##_rC }},					\
    { #op_upper " D",		1, 1, { prefix_##op_lower##_rD }},					\
    { #op_upper " E",		1, 1, { prefix_##op_lower##_rE }},					\
    { #op_upper " H",		1, 1, { prefix_##op_lower##_rH }},					\
    { #op_upper " L",		1, 1, { prefix_##op_lower##_rL }},					\
    { #op_upper " (HL)",	1, 3, { nop, aHL_read, prefix_##op_lower##_rHL }},	\
    { #op_upper " L",		1, 1, { prefix_##op_lower##_rA }}

// Test bit

void _bit(gbCpu* cpu, UINT8 val, int bit) {
    cpu->subtract = 0;
    cpu->half_carry = 1;
    cpu->zero = ((val >> bit) & 0x1) == 0;
}

void bit_rHL(gbCpu* cpu,int bit) {
    aHL_read(cpu);
    _bit(cpu, cpu->low, bit);
}

#define PREFIX_BIT_TEST(reg, bit)               \
void prefix_bit_r##reg##_b##bit##(gbCpu* cpu) { \
    _bit(cpu, cpu->reg, bit);                   \
}

#define PREFIX_BIT_TEST_HL(bit)             \
void prefix_bit_aHL_b##bit##(gbCpu* cpu) {  \
    aHL_read(cpu);                          \
    _bit(cpu, cpu->low, bit);               \
}

#define PREFIX_BIT_TEST_ALL(bit)    \
PREFIX_BIT_TEST(B, bit)             \
PREFIX_BIT_TEST(C, bit)             \
PREFIX_BIT_TEST(D, bit)             \
PREFIX_BIT_TEST(E, bit)             \
PREFIX_BIT_TEST(H, bit)             \
PREFIX_BIT_TEST(L, bit)             \
PREFIX_BIT_TEST_HL(bit)             \
PREFIX_BIT_TEST(A, bit)             \

PREFIX_BIT_TEST_ALL(0)
PREFIX_BIT_TEST_ALL(1)
PREFIX_BIT_TEST_ALL(2)
PREFIX_BIT_TEST_ALL(3)
PREFIX_BIT_TEST_ALL(4)
PREFIX_BIT_TEST_ALL(5)
PREFIX_BIT_TEST_ALL(6)
PREFIX_BIT_TEST_ALL(7)

#define PREFIX_INSTR_BIT_TEST(bit)								        \
    { "BIT " #bit ", B",    1, 1, { prefix_bit_rB_b##bit## }},          \
    { "BIT " #bit ", C",    1, 1, { prefix_bit_rC_b##bit## }},          \
    { "BIT " #bit ", D",    1, 1, { prefix_bit_rD_b##bit## }},          \
    { "BIT " #bit ", E",    1, 1, { prefix_bit_rE_b##bit## }},          \
    { "BIT " #bit ", H",    1, 1, { prefix_bit_rH_b##bit## }},          \
    { "BIT " #bit ", L",    1, 1, { prefix_bit_rL_b##bit## }},          \
    { "BIT " #bit ", (HL)",	1, 2, { nop, prefix_bit_aHL_b##bit## }},    \
    { "BIT " #bit ", A",    1, 1, { prefix_bit_rA_b##bit## }}


// Set/Reset bits

UINT8 set(gbCpu* cpu, UINT8 val, int bit) {
    return val | (1 << bit);
}

UINT8 res(gbCpu* cpu, UINT8 val, int bit) {
    return val & ~(1 << bit);
}

#define PREFIX_BIT_MANIP(op, reg, bit)              \
void prefix_##op##_r##reg##_b##bit##(gbCpu* cpu) {  \
    cpu->reg = op(cpu, cpu->reg, bit);              \
}

#define PREFIX_BIT_MANIP_HL(op, bit)                    \
void prefix_##op##_aHL_b##bit##(gbCpu* cpu) {           \
    g_gb->writeByte(cpu->HL, op(cpu, cpu->low, bit));   \
}

#define PREFIX_BIT_MANIP_ALL(op, bit)   \
PREFIX_BIT_MANIP(op, B, bit)            \
PREFIX_BIT_MANIP(op, C, bit)            \
PREFIX_BIT_MANIP(op, D, bit)            \
PREFIX_BIT_MANIP(op, E, bit)            \
PREFIX_BIT_MANIP(op, H, bit)            \
PREFIX_BIT_MANIP(op, L, bit)            \
PREFIX_BIT_MANIP_HL(op, bit)            \
PREFIX_BIT_MANIP(op, A, bit)            \

PREFIX_BIT_MANIP_ALL(set, 0)
PREFIX_BIT_MANIP_ALL(set, 1)
PREFIX_BIT_MANIP_ALL(set, 2)
PREFIX_BIT_MANIP_ALL(set, 3)
PREFIX_BIT_MANIP_ALL(set, 4)
PREFIX_BIT_MANIP_ALL(set, 5)
PREFIX_BIT_MANIP_ALL(set, 6)
PREFIX_BIT_MANIP_ALL(set, 7)

PREFIX_BIT_MANIP_ALL(res, 0)
PREFIX_BIT_MANIP_ALL(res, 1)
PREFIX_BIT_MANIP_ALL(res, 2)
PREFIX_BIT_MANIP_ALL(res, 3)
PREFIX_BIT_MANIP_ALL(res, 4)
PREFIX_BIT_MANIP_ALL(res, 5)
PREFIX_BIT_MANIP_ALL(res, 6)
PREFIX_BIT_MANIP_ALL(res, 7)

#define PREFIX_INSTR_BIT_MANIP(op_upper, op_lower, bit)								            \
    { #op_upper " " #bit ", B",    1, 1, { prefix_##op_lower##_rB_b##bit## }},                  \
    { #op_upper " " #bit ", C",    1, 1, { prefix_##op_lower##_rC_b##bit## }},                  \
    { #op_upper " " #bit ", D",    1, 1, { prefix_##op_lower##_rD_b##bit## }},                  \
    { #op_upper " " #bit ", E",    1, 1, { prefix_##op_lower##_rE_b##bit## }},                  \
    { #op_upper " " #bit ", H",    1, 1, { prefix_##op_lower##_rH_b##bit## }},                  \
    { #op_upper " " #bit ", L",    1, 1, { prefix_##op_lower##_rL_b##bit## }},                  \
    { #op_upper " " #bit ", (HL)", 1, 3, { nop, aHL_read, prefix_##op_lower##_aHL_b##bit## }},  \
    { #op_upper " " #bit ", A",    1, 1, { prefix_##op_lower##_rA_b##bit## }}

static const Instruction prefixInstructions[] = {
    /* 0x00 */
    PREFIX_INSTR_ALL(RLC, rlc),
    PREFIX_INSTR_ALL(RRC, rrc),
    /* 0x10 */
    PREFIX_INSTR_ALL(RL, rl),
    PREFIX_INSTR_ALL(RR, rr),
    /* 0x20 */
    PREFIX_INSTR_ALL(SLA, sla),
    PREFIX_INSTR_ALL(SRA, sra),
    /* 0x30 */
    PREFIX_INSTR_ALL(SWAP, swap),
    PREFIX_INSTR_ALL(SRL, srl),
    /* 0x40 */
    PREFIX_INSTR_BIT_TEST(0),
    PREFIX_INSTR_BIT_TEST(1),
    /* 0x50 */
    PREFIX_INSTR_BIT_TEST(2),
    PREFIX_INSTR_BIT_TEST(3),
    /* 0x60 */
    PREFIX_INSTR_BIT_TEST(4),
    PREFIX_INSTR_BIT_TEST(5),
    /* 0x70 */
    PREFIX_INSTR_BIT_TEST(6),
    PREFIX_INSTR_BIT_TEST(7),
    /* 0x80 */
    PREFIX_INSTR_BIT_MANIP(RES, res, 0),
    PREFIX_INSTR_BIT_MANIP(RES, res, 1),
    /* 0x90 */
    PREFIX_INSTR_BIT_MANIP(RES, res, 2),
    PREFIX_INSTR_BIT_MANIP(RES, res, 3),
    /* 0xA0 */
    PREFIX_INSTR_BIT_MANIP(RES, res, 4),
    PREFIX_INSTR_BIT_MANIP(RES, res, 5),
    /* 0xB0 */
    PREFIX_INSTR_BIT_MANIP(RES, res, 6),
    PREFIX_INSTR_BIT_MANIP(RES, res, 7),
    /* 0xC0 */
    PREFIX_INSTR_BIT_MANIP(SET, set, 0),
    PREFIX_INSTR_BIT_MANIP(SET, set, 1),
    /* 0xD0 */
    PREFIX_INSTR_BIT_MANIP(SET, set, 2),
    PREFIX_INSTR_BIT_MANIP(SET, set, 3),
    /* 0xE0 */
    PREFIX_INSTR_BIT_MANIP(SET, set, 4),
    PREFIX_INSTR_BIT_MANIP(SET, set, 5),
    /* 0xF0 */
    PREFIX_INSTR_BIT_MANIP(SET, set, 6),
    PREFIX_INSTR_BIT_MANIP(SET, set, 7)
};

static_assert(sizeof(prefixInstructions) == 0x100 * sizeof(Instruction));

