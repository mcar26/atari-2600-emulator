#include <map>
#include "ops.h"

#define CARRY_FLAG (flags && 0x01)
#define ZERO_FLAG (flags && 0x02)
#define DECIMAL_FLAG (flags && 0x08)
#define OVERFLOW_FLAG (flags && 0x40)
#define SIGN_FLAG (flags && 0x80)

#define TO_BCD(n) ((n & 0x0f) + ((n & 0xf0) >> 4)*10)
#define CHARS_TO_SHORT(n1, n2) (n1 + (((short)n2) << 8))

char* memory;
char flags, a, x, y, sp;
short pc;

void update_flags(char value, char inc) {
    if (value == 0) {
        flags |= 0x02; // zero flag
    } else {
        flags &= 0xfd;
        if (value < 0) {
            flags |= 0x80; // negative flag
        } else {
            flags &= 0x7f;
        }
    }

    if (inc != 0) {
        if ((short)value - (short)inc > 127 || (short)value - (short)inc < -128) {
            flags |= 0x40; // overflow flag
        } else {
            flags &= 0xbf;
        }
        if (value + inc > 255) {
            flags |= 0x01; // carry flag
        } else {
            flags &= 0xfe;
        }
    }
}

// opcode -> function maps
const std::map<char, void(*)()> ops_noargs = {
    {0xa8, tay}, {0xaa, tax}, {0xba, tsx},
    {0x98, tya}, {0x8a, txa}, {0x9a, txs},
    {0x48, pha}, {0x08, php}, {0x68, pla},
    {0x28, plp}, {0xe8, inx}, {0xc8, iny}, 
    {0xca, dex}, {0x88, dey}, {0x0a, asl},
    {0x4a, lsr}, {0x2a, rol}, {0x6a, ror},
    {0x40, reti}, {0x60, ret}, {0xea, nop},
    {0x18, clc}, {0x38, sec}, {0x58, cli},
    {0x78, sei}, {0xb8, clv}, {0xd8, cld},
    {0xf8, sed}
};

const std::map<char, void(*)(char)> ops_onearg = {
    {0x69, adc_im}, {0x65, adc_z}, {0x75, adc_zx}, 
    {0x61, adc_inx}, {0x71, adc_iny}, {0x29, and_im}, 
    {0x25, and_z}, {0x35, and_zx}, {0x21, and_inx}, 
    {0x31, and_iny}, {0x06, asl_z}, {0x16, asl_zx},
    {0x24, bit_im}, {0xc9, cmp_im}, {0xc5, cmp_z},
    {0xd5, cmp_zx}, {0xc1, cmp_inx}, {0xd1, cmp_iny},
    {0xe0, cpx_im}, {0xe4, cpx_z}, {0xc0, cpy_im},
    {0xc4, cpy_im}, {0xc6, dec_z}, {0xd6, dec_zx},
    {0x49, eor_im}, {0x45, eor_z}, {0x55, eor_zx},
    {0x41, eor_inx}, {0x51, eor_iny}, {0xe6, inc_z},
    {0xf6, inc_zx}, {0xa9, lda_im}, {0xa5, lda_z},
    {0xb5, lda_zx}, {0xa1, lda_inx}, {0xb1, lda_iny},
    {0xa2, ldx_im}, {0xa6, ldx_z}, {0xb6, ldx_zy},
    {0xa0, ldy_im}, {0xa4, ldy_z}, {0xb4, ldy_zx},
    {0x46, lsr_z}, {0x56, lsr_zx}, {0x09, ora_im},
    {0x05, ora_z}, {0x15, ora_zx}, {0x01, ora_inx},
    {0x11, ora_iny}, {0x26, rol_z}, {0x36, rol_zx},
    {0x66, ror_z}, {0x76, ror_zx}, {0xe9, sbc_im},
    {0xe5, sbc_z}, {0xf5, sbc_zx}, {0xe1, sbc_inx},
    {0xf1, sbc_iny}, {0x85, sta_z}, {0x95, sta_zx},
    {0x81, sta_inx}, {0x91, sta_iny}, {0x86, stx_z},
    {0x96, stx_zy}, {0x84, sty_z}, {0x94, sty_zx}
};

const std::map<char, void(*)(char, char)> ops_twoargs = {
    {0x6d, adc_a}, {0x7d, adc_ax}, {0x79, adc_ay},
    {0x2d, and_a}, {0x3d, and_ax}, {0x39, and_ay},
    {0x0e, asl_a}, {0x1e, asl_ax}, {0x2c, bit_a},
    {0xcd, cmp_a}, {0xdd, cmp_ax}, {0xd9, cmp_ay},
    {0xec, cpx_a}, {0xcc, cpy_a}, {0xce, dec_a},
    {0xde, dec_ax}, {0x4d, eor_a}, {0x5d, eor_ax},
    {0x59, eor_ay}, {0xee, inc_a}, {0xfe, inc_ax},
    {0x4c, jmp_a}, {0x6c, jmp_ain}, {0x20, call},
    {0xad, lda_a}, {0xbd, lda_ax}, {0xb9, lda_ay},
    {0xae, ldx_a}, {0xbe, ldx_ay}, {0xac, ldy_a},
    {0xbc, ldy_ax}, {0x4e, lsr_a}, {0x5e, lsr_ax},
    {0x0d, ora_a}, {0x1d, ora_ax}, {0x19, ora_ay},
    {0x2e, rol_a}, {0x3e, rol_ax}, {0x6e, ror_a},
    {0x7e, ror_ax}, {0xed, sbc_a}, {0xfd, sbc_ax},
    {0xf9, sbc_ay}, {0x8d, sta_a}, {0x9d, sta_ax},
    {0x99, sta_ay}, {0x8e, stx_a}, {0x8c, sty_a}
};

// uncategorized
void nop() {
    return;
}

// flag instructions
void clc() {
    flags &= 0xfe;
}

void sec() {
    flags |= 0x01;
}

void clv() {
    flags &= 0xbf;
}

void sed() {
    flags |= 0x08;
}

void cld() {
    flags &= 0xf7;
}

void cli() {
    flags &= 0xfb;
}

void sei() {
    flags |= 0x04;
}

// register instructions
void tax() {
    x = a;
    update_flags(x, 0);
}

void txa() {
    a = x;
    update_flags(a, 0);
}

void txs() {
    sp = x;
}

void tsx() {
    x = sp;
}

void dex() {
    x--;
    update_flags(x, -1);
}

void inx() {
    x++;
    update_flags(x, 1);
}

void tay() {
    y = a;
    update_flags(y, 0);
}

void tya() {
    a = y;
    update_flags(a, 0);
}
void dey() {
    y--;
    update_flags(y, -1);
}

void iny() {
    y++;
    update_flags(y, 1);
}

void lda_im(char argument) {
    a = argument;
    update_flags(a, 0);
}

void lda_z(char argument) {
    a = memory[argument];
    update_flags(a, 0);
}

void lda_zx(char argument) {
    a = memory[argument+x];
    update_flags(a, 0);
}

void lda_a(char arg1, char arg2) {
    short argument = CHARS_TO_SHORT(arg1, arg2);
    a = memory[argument];
    update_flags(a, 0);
}

void lda_ax(char arg1, char arg2) {
    short argument = CHARS_TO_SHORT(arg1, arg2);
    a = memory[argument+x];
    update_flags(a, 0);
}

void lda_ay(char arg1, char arg2) {
    short argument = CHARS_TO_SHORT(arg1, arg2);
    a = memory[argument+y];
    update_flags(a, 0);
}

void lda_inx(char argument) {
    short address = (((short)memory[argument + x + 1]) << 4) + memory[argument];
    a = memory[address];
    update_flags(a, 0);
}

void lda_iny(char argument) {
    short address = (short)memory[argument] + (short)y;
    a = memory[address];
    update_flags(a, 0);
}

void ldx_im(char argument) {
    x = argument;
    update_flags(x, 0);
}

void ldx_z(char argument) {
    x = memory[argument];
    update_flags(x, 0);
}

void ldx_zy(char argument) {
    x = memory[argument+y];
    update_flags(x, 0);
}

void ldx_a(char arg1, char arg2) {
    short argument = CHARS_TO_SHORT(arg1, arg2);
    x = memory[argument];
    update_flags(x, 0);
}

void ldx_ay(char arg1, char arg2) {
    short argument = CHARS_TO_SHORT(arg1, arg2);
    x = memory[argument+y];
    update_flags(x, 0);
}

void ldy_im(char argument) {
    y = argument;
    update_flags(y, 0);
}

void ldy_z(char argument) {
    y = memory[argument];
    update_flags(y, 0);
}

void ldy_zx(char argument) {
    y = memory[argument+x];
    update_flags(y, 0);
}

void ldy_a(char arg1, char arg2) {
    short argument = CHARS_TO_SHORT(arg1, arg2);
    y = memory[argument];
    update_flags(y, 0);
}

void ldy_ax(char arg1, char arg2) {
    short argument = CHARS_TO_SHORT(arg1, arg2);
    y = memory[argument+x];
    update_flags(y, 0);
}

void sta_z(char argument) {
    memory[argument] = a;
}

void sta_zx(char argument) {
    memory[argument+x] = a;
}

void sta_a(char arg1, char arg2) {
    short argument = CHARS_TO_SHORT(arg1, arg2);
    memory[argument] = a;
}

void sta_ax(char arg1, char arg2) {
    short argument = CHARS_TO_SHORT(arg1, arg2);
    memory[argument+x] = a;
}

void sta_ay(char arg1, char arg2) {
    short argument = CHARS_TO_SHORT(arg1, arg2);
    memory[argument+y] = a;
}

void sta_inx(char argument) {
    short address = (((short)memory[argument + x + 1]) << 4) + memory[argument];
    memory[address] = a;
}

void sta_iny(char argument) {
    short address = (short)memory[argument] + (short)y;
    memory[address] = a;
}

void stx_z(char argument) {
    memory[argument] = x;
}

void stx_zy(char argument) {
    memory[argument+y] = x;
}

void stx_a(char arg1, char arg2) {
    short argument = CHARS_TO_SHORT(arg1, arg2);
    memory[argument] = x;
}

void sty_z(char argument) {
    memory[argument] = y;
}

void sty_zx(char argument) {
    memory[argument+x] = y;
}

void sty_a(char arg1, char arg2) {
    short argument = CHARS_TO_SHORT(arg1, arg2);
    memory[argument] = y;
}

// stack operations
void pha() {
    memory[sp] = a;
    sp--;
}

void php() {
    memory[sp] = flags;
    sp--;
}

void pla() {
    sp++;
    a = memory[sp];
    update_flags(a,0);
}

void plp() {
    sp++;
    flags = memory[sp];
}

// unconditional jumps and function operations
void jmp_ain(char arg1, char arg2) {
    short argument = CHARS_TO_SHORT(arg1, arg2);
    pc = argument;
}

void jmp_a(char arg1, char arg2) {
    short argument = CHARS_TO_SHORT(arg1, arg2);
    pc = CHARS_TO_SHORT(memory[argument], memory[argument+1]);
}

void call(char arg1, char arg2) {
    short argument = CHARS_TO_SHORT(arg1, arg2);
    memory[sp] = pc + 2;
    sp--;
    pc = argument;
}

void reti() {
    flags = memory[sp];
    sp++;
    pc = CHARS_TO_SHORT(memory[sp], memory[sp-1]);
    sp+=2;
}

void ret() {
    pc = CHARS_TO_SHORT(memory[sp], memory[sp-1]) + 1;
    sp+=2;
}

// conditional jumps
void jns(char argument) {
    if (!(SIGN_FLAG)) {
        pc += argument;
    }
}

void js(char argument) {
    if ((SIGN_FLAG)) {
        pc += argument;
    }
}

void jno(char argument) {
    if (!(OVERFLOW_FLAG)) {
        pc += argument;
    }
}

void jo(char argument) {
    if ((OVERFLOW_FLAG)) {
        pc += argument;
    }
}

void jnc(char argument) {
    if (!(CARRY_FLAG)) {
        pc += argument;
    }
}

void jc(char argument) {
    if ((CARRY_FLAG)) {
        pc += argument;
    }
}

void jnz(char argument) {
    if (!(ZERO_FLAG)) {
        pc += argument;
    }
}

void jz(char argument) {
    if ((ZERO_FLAG)) {
        pc += argument;
    }
}

// breaks
void brk() {
    flags |= 0x10; // set break flag
    memory[sp] = (char)(pc & 0x00ff); // push pc little byte
    sp--;
    memory[sp] = (char)((pc & 0xff00) >> 8) + 1; // push pc big byte + 1
    sp--;
    memory[sp] = flags;
    sp--;
    flags |= 0x04; // set interrupt flag
    pc = memory[0xfffe];

}

// math operations
void adc_im(char argument) {
    if (DECIMAL_FLAG) {
        unsigned char bcd_a = TO_BCD(a) + CARRY_FLAG + TO_BCD(argument);
        a = (bcd_a % 10) + ((bcd_a / 10) << 4);
    } else {
        a += CARRY_FLAG + argument;
    }
    update_flags(a, CARRY_FLAG+argument);
}

void adc_z(char argument) {
    if (DECIMAL_FLAG) {
        unsigned char bcd_a = TO_BCD(a) + CARRY_FLAG + TO_BCD(memory[argument]);
        a = (bcd_a % 10) + ((bcd_a / 10) << 4);
    } else {
        a += CARRY_FLAG + memory[argument];
    }
    update_flags(a, CARRY_FLAG+memory[argument]);
}

void adc_zx(char argument) {
    if (DECIMAL_FLAG) {
        unsigned char bcd_a = TO_BCD(a) + CARRY_FLAG + TO_BCD(memory[argument + x]);
        a = (bcd_a % 10) + ((bcd_a / 10) << 4);
    } else {
        a += CARRY_FLAG + memory[argument + x];
    }
    update_flags(a, CARRY_FLAG+memory[argument + x]);
}

void adc_a(char arg1, char arg2) {
    short argument = CHARS_TO_SHORT(arg1, arg2);
    if (DECIMAL_FLAG) {
        unsigned char bcd_a = TO_BCD(a) + CARRY_FLAG + TO_BCD(memory[argument]);
        a = (bcd_a % 10) + ((bcd_a / 10) << 4);
    } else {
        a += CARRY_FLAG + memory[argument];
    }
    update_flags(a, CARRY_FLAG+memory[argument]);
}

void adc_ax(char arg1, char arg2) {
    short argument = CHARS_TO_SHORT(arg1, arg2);
    if (DECIMAL_FLAG) {
        unsigned char bcd_a = TO_BCD(a) + CARRY_FLAG + TO_BCD(memory[argument + x]);
        a = (bcd_a % 10) + ((bcd_a / 10) << 4);
    } else {
        a += CARRY_FLAG + memory[argument + x];
    }
    update_flags(a, CARRY_FLAG+memory[argument + x]);
}

void adc_ay(char arg1, char arg2) {
    short argument = CHARS_TO_SHORT(arg1, arg2);
    if (DECIMAL_FLAG) {
        unsigned char bcd_a = TO_BCD(a) + CARRY_FLAG + TO_BCD(memory[argument + y]);
        a = (bcd_a % 10) + ((bcd_a / 10) << 4);
    } else {
        a += CARRY_FLAG + memory[argument + y];
    }
    update_flags(a, CARRY_FLAG+memory[argument + y]);
}

void adc_inx(char argument) {
    short address = (((short)memory[argument + x + 1]) << 4) + memory[argument];
    if (DECIMAL_FLAG) {
        unsigned char bcd_a = TO_BCD(a) + CARRY_FLAG + TO_BCD(memory[address]);
    } else {
        a += CARRY_FLAG + memory[address];
    }
    update_flags(a, CARRY_FLAG+memory[address]);
}

void adc_iny(char argument) {
    short address = (short)memory[argument] + (short)y;
    if (DECIMAL_FLAG) {
        unsigned char bcd_a = TO_BCD(a) + CARRY_FLAG + TO_BCD(memory[address]);
    } else {
        a += CARRY_FLAG + memory[address];
    }
    update_flags(a, CARRY_FLAG+memory[address]);
}

void sbc_im(char argument) {
    if (DECIMAL_FLAG) {
        unsigned char bcd_a = TO_BCD(a) + CARRY_FLAG - 1 - TO_BCD(argument);
        a = (bcd_a % 10) + ((bcd_a / 10) << 4);
    } else {
        a -= (-CARRY_FLAG + 1 + argument);
    }
    update_flags(a, -(-CARRY_FLAG+1+argument));
}

void sbc_z(char argument) {
    if (DECIMAL_FLAG) {
        unsigned char bcd_a = TO_BCD(a) + CARRY_FLAG -1 - TO_BCD(memory[argument]);
        a = (bcd_a % 10) + ((bcd_a / 10) << 4);
    } else {
        a -= (-CARRY_FLAG + 1 + memory[argument]);
    }
    update_flags(a, -(-CARRY_FLAG+1+memory[argument]));
}

void sbc_zx(char argument) {
    if (DECIMAL_FLAG) {
        unsigned char bcd_a = TO_BCD(a) + CARRY_FLAG - 1 - TO_BCD(memory[argument + x]);
        a = (bcd_a % 10) + ((bcd_a / 10) << 4);
    } else {
        a -= -(-CARRY_FLAG + 1 + memory[argument + x]);
    }
    update_flags(a, -(-CARRY_FLAG+1+memory[argument + x]));
}

void sbc_a(char arg1, char arg2) {
    short argument = CHARS_TO_SHORT(arg1, arg2);
    if (DECIMAL_FLAG) {
        unsigned char bcd_a = TO_BCD(a) + CARRY_FLAG - 1 - TO_BCD(memory[argument]);
        a = (bcd_a % 10) + ((bcd_a / 10) << 4);
    } else {
        a -= -(-CARRY_FLAG + 1 + memory[argument]);
    }
    update_flags(a, -(-CARRY_FLAG+1+memory[argument]));
}

void sbc_ax(char arg1, char arg2) {
    short argument = CHARS_TO_SHORT(arg1, arg2);
    if (DECIMAL_FLAG) {
        unsigned char bcd_a = TO_BCD(a) + CARRY_FLAG - 1 - TO_BCD(memory[argument + x]);
        a = (bcd_a % 10) + ((bcd_a / 10) << 4);
    } else {
        a -= -(-CARRY_FLAG + 1 + memory[argument + x]);
    }
    update_flags(a, -(-CARRY_FLAG+1+memory[argument + x]));
}

void sbc_ay(char arg1, char arg2) {
    short argument = CHARS_TO_SHORT(arg1, arg2);
    if (DECIMAL_FLAG) {
        unsigned char bcd_a = TO_BCD(a) + CARRY_FLAG - 1 - TO_BCD(memory[argument + y]);
        a = (bcd_a % 10) + ((bcd_a / 10) << 4);
    } else {
        a -= -(-CARRY_FLAG + 1 + memory[argument + y]);
    }
    update_flags(a, -(-CARRY_FLAG+1+memory[argument + y]));
}

void sbc_inx(char argument) {
    short address = (((short)memory[argument + x + 1]) << 4) + memory[argument];
    if (DECIMAL_FLAG) {
        unsigned char bcd_a = TO_BCD(a) + CARRY_FLAG - 1 - TO_BCD(memory[address]);
        a = (bcd_a % 10) + ((bcd_a / 10) << 4);
    } else {
        a -= -(-CARRY_FLAG + 1 + memory[address]);
    }
    update_flags(a, -(-CARRY_FLAG+1+memory[address]));
}

void sbc_iny(char argument) {
    short address = (short)memory[argument] + (short)y;
    if (DECIMAL_FLAG) {
        unsigned char bcd_a = TO_BCD(a) + CARRY_FLAG - 1 - TO_BCD(memory[address]);
        a = (bcd_a % 10) + ((bcd_a / 10) << 4);
    } else {
        a -= -(CARRY_FLAG + 1 + memory[address]);
    }
    update_flags(a, -(-CARRY_FLAG+1+memory[address]));
}

// logical operations
void and_im(char argument) {
    a &= argument;
    update_flags(a, 0);
}

void and_z(char argument) {
    a &= memory[argument];
    update_flags(a, 0);
}

void and_zx(char argument) {
    a &= memory[argument+x];
    update_flags(a, 0);
}

void and_a(char arg1, char arg2) {
    short argument = ((short)arg1 + (((short)arg2) << 4));
    a &= memory[argument];
    update_flags(a, 0);
}

void and_ax(char arg1, char arg2) {
    short argument = ((short)arg1 + (((short)arg2) << 4));
    a &= memory[argument + x];
    update_flags(a, 0);
}

void and_ay(char arg1, char arg2) {
    short argument = ((short)arg1 + (((short)arg2) << 4));
    a &= memory[argument + y];
    update_flags(a, 0);
}

void and_inx(char argument) {
    short address = (((short)memory[argument + x + 1]) << 4) + memory[argument];
    a &= memory[address];
    update_flags(a,0);
}

void and_iny(char argument) {
    short address = (short)memory[argument] + (short)y;
    a &= memory[address];
    update_flags(a, 0);
}

void eor_im(char argument) {
    a ^= argument;
    update_flags(a, 0);
}

void eor_z(char argument) {
    a ^= memory[argument];
    update_flags(a, 0);
}

void eor_zx(char argument) {
    a ^= memory[argument+x];
    update_flags(a, 0);
}

void eor_a(char arg1, char arg2) {
    short argument = ((short)arg1 + (((short)arg2) << 4));
    a ^= memory[argument];
    update_flags(a, 0);
}

void eor_ax(char arg1, char arg2) {
    short argument = ((short)arg1 + (((short)arg2) << 4));
    a ^= memory[argument + x];
    update_flags(a, 0);
}

void eor_ay(char arg1, char arg2) {
    short argument = ((short)arg1 + (((short)arg2) << 4));
    a ^= memory[argument + y];
    update_flags(a, 0);
}

void eor_inx(char argument) {
    short address = (((short)memory[argument + x + 1]) << 4) + memory[argument];
    a ^= memory[address];
    update_flags(a,0);
}

void eor_iny(char argument) {
    short address = (short)memory[argument] + (short)y;
    a ^= memory[address];
    update_flags(a, 0);
}

void ora_im(char argument) {
    a |= argument;
    update_flags(a, 0);
}

void ora_z(char argument) {
    a |= memory[argument];
    update_flags(a, 0);
}

void ora_zx(char argument) {
    a |= memory[argument+x];
    update_flags(a, 0);
}

void ora_a(char arg1, char arg2) {
    short argument = ((short)arg1 + (((short)arg2) << 4));
    a |= memory[argument];
    update_flags(a, 0);
}

void ora_ax(char arg1, char arg2) {
    short argument = ((short)arg1 + (((short)arg2) << 4));
    a |= memory[argument + x];
    update_flags(a, 0);
}

void ora_ay(char arg1, char arg2) {
    short argument = ((short)arg1 + (((short)arg2) << 4));
    a |= memory[argument + y];
    update_flags(a, 0);
}

void ora_inx(char argument) {
    short address = (((short)memory[argument + x + 1]) << 4) + memory[argument];
    a |= memory[address];
    update_flags(a,0);
}

void ora_iny(char argument) {
    short address = (short)memory[argument] + (short)y;
    a |= memory[address];
    update_flags(a, 0);
}

// memory inc/dec operations
void inc_z(char argument) {
    memory[argument]++;
    update_flags(memory[argument], 1);
}

void inc_zx(char argument) {
    memory[argument+x]++;
    update_flags(memory[argument+x], 1);
}

void inc_a(char arg1, char arg2) {
    short argument = ((short)arg1 + (((short)arg2) << 4));
    memory[argument]++;
    update_flags(memory[argument], 1);
}

void inc_ax(char arg1, char arg2) {
    short argument = ((short)arg1 + (((short)arg2) << 4));
    memory[argument+x]++;
    update_flags(memory[argument+x], 1);
}

void dec_z(char argument) {
    memory[argument]--;
    update_flags(memory[argument], -1);
}

void dec_zx(char argument) {
    memory[argument+x]--;
    update_flags(memory[argument+x], -1);
}

void dec_a(char arg1, char arg2) {
    short argument = ((short)arg1 + (((short)arg2) << 4));
    memory[argument]--;
    update_flags(memory[argument], -1);
}

void dec_ax(char arg1, char arg2) {
    short argument = ((short)arg1 + (((short)arg2) << 4));
    memory[argument+x]--;
    update_flags(memory[argument+x], -1);
}

// cmp and bit test operations
void cmp_im(char argument) {
    update_flags(a-argument, -argument);
}

void cmp_z(char argument) {
    update_flags(a-memory[argument], -memory[argument]);
}

void cmp_zx(char argument) {
    update_flags(a-memory[argument+x], -memory[argument+x]);
}

void cmp_a(char arg1, char arg2) {
    short argument = ((short)arg1 + (((short)arg2) << 4));
    update_flags(a-memory[argument], -memory[argument]);
}

void cmp_ax(char arg1, char arg2) {
    short argument = ((short)arg1 + (((short)arg2) << 4));
    update_flags(a-memory[argument+x], -memory[argument+x]);
}

void cmp_ay(char arg1, char arg2) {
    short argument = ((short)arg1 + (((short)arg2) << 4));
    update_flags(a-memory[argument+y], -memory[argument+y]);
}

void cmp_inx(char argument) {
    short address = (((short)memory[argument + x + 1]) << 4) + memory[argument];
    update_flags(a-memory[address], -memory[address]);
}

void cmp_iny(char argument) {
    short address = (short)memory[argument] + (short)y;
    update_flags(a-memory[address], -memory[address]);
}

void cpx_im(char argument) {
    update_flags(x-argument, -argument);
}

void cpx_z(char argument) {
    update_flags(x-memory[argument], -memory[argument]);
}

void cpx_a(char arg1, char arg2) {
    short argument = ((short)arg1 + (((short)arg2) << 4));
    update_flags(x-memory[argument], -memory[argument]);
}

void cpy_im(char argument) {
    update_flags(y-argument, -argument);
}

void cpy_z(char argument) {
    update_flags(y-memory[argument], -memory[argument]);
}

void cpy_a(char arg1, char arg2) {
    short argument = ((short)arg1 + (((short)arg2) << 4));
    update_flags(y-memory[argument], -memory[argument]);
}

void bit_im(char argument) {
    if (a & memory[argument] == 0) {
        flags |= 0x02; // zero flag
    }
    if (memory[argument] & 0x80) {
        flags |= 0x80; // sign flag
    } else {
        flags &= 0xef;
    }
    if (memory[argument] & 0x40) {
        flags |= 0x40; // overflow flag
    } else {
        flags &= 0xbf;
    }
}

void bit_a(char arg1, char arg2) {
    short argument = ((short)arg1 + (((short)arg2) << 4));
    if (a & memory[argument] == 0) {
        flags |= 0x02; // zero flag
    }
    if (memory[argument] & 0x80) {
        flags |= 0x80; // sign flag
    } else {
        flags &= 0xef;
    }
    if (memory[argument] & 0x40) {
        flags |= 0x40; // overflow flag
    } else {
        flags &= 0xbf;
    }
}

// shift operations
void asl() {
    flags |= (a & 0x80); // set carry flag to bit 7
    a = a << 1;
    update_flags(a,0);
}

void asl_z(char argument) {
    flags |= (memory[argument] & 0x80); // set carry flag to bit 7
    memory[argument] = memory[argument] << 1;
    update_flags(memory[argument],0);
}

void asl_zx(char argument) {
    flags |= (memory[argument+x] & 0x80); // set carry flag to bit 7
    memory[argument+x] = memory[argument+x] << 1;
    update_flags(memory[argument+x],0);
}

void asl_a(char arg1, char arg2) {
    short argument = ((short)arg1 + (((short)arg2) << 4));
    flags |= (memory[argument] & 0x80); // set carry flag to bit 7
    memory[argument] = memory[argument] << 1;
    update_flags(memory[argument],0);
}

void asl_ax(char arg1, char arg2) {
    short argument = ((short)arg1 + (((short)arg2) << 4));
    flags |= (memory[argument+x] & 0x80); // set carry flag to bit 7
    memory[argument+x] = memory[argument+x] << 1;
    update_flags(memory[argument+x],0);
}

void lsr() {
    flags |= (a & 0x01); // set carry flag to bit 7
    a = (a >> 1) & 0x7f; // shift and set bit 7 to 0
    update_flags(a, 0);
}

void lsr_z(char argument) {
    flags |= (memory[argument] & 0x80); // set carry flag to bit 7
    memory[argument] = (memory[argument] >> 1) & 0x7f; // shift and set bit 7 to 0
    update_flags(memory[argument],0);
}

void lsr_zx(char argument) {
    flags |= (memory[argument+x] & 0x80); // set carry flag to bit 7
    memory[argument+x] = (memory[argument+x] >> 1) & 0x7f; // shift and set bit 7 to 0
    update_flags(memory[argument+x],0);
}

void lsr_a(char arg1, char arg2) {
    short argument = ((short)arg1 + (((short)arg2) << 4));
    flags |= (memory[argument] & 0x80); // set carry flag to bit 7
    memory[argument] = (memory[argument] >> 1) & 0x7f; // shift and set bit 7 to 0
    update_flags(memory[argument],0);
}

void lsr_ax(char arg1, char arg2) {
    short argument = ((short)arg1 + (((short)arg2) << 4));
    flags |= (memory[argument+x] & 0x80); // set carry flag to bit 7
    memory[argument+x] = (memory[argument+x] >> 1) & 0x7f; // shift and set bit 7 to 0
    update_flags(memory[argument+x],0);
}

void rol() {
    char old_carry = CARRY_FLAG;
    flags |= (a & 0x80); // set carry flag to bit 7
    a = a << 1;
    a |= old_carry; // set bit 0 to old carry flag
    update_flags(a,0);
}

void rol_z(char argument) {
    char old_carry = CARRY_FLAG;
    flags |= (memory[argument] & 0x80); // set carry flag to bit 7
    memory[argument] = memory[argument] << 1;
    memory[argument] |= old_carry; // set bit 0 to old carry flag
    update_flags(memory[argument],0);
}

void rol_zx(char argument) {
    char old_carry = CARRY_FLAG;
    flags |= (memory[argument+x] & 0x80); // set carry flag to bit 7
    memory[argument+x] = memory[argument+x] << 1;
    memory[argument+x] |= old_carry; // set bit 0 to old carry flag
    update_flags(memory[argument+x],0);
}

void rol_a(char arg1, char arg2) {
    char old_carry = CARRY_FLAG;
    short argument = ((short)arg1 + (((short)arg2) << 4));
    flags |= (memory[argument] & 0x80); // set carry flag to bit 7
    memory[argument] = memory[argument] << 1;
    memory[argument] |= old_carry; // set bit 0 to old carry flag
    update_flags(memory[argument],0);
}

void rol_ax(char arg1, char arg2) {
    char old_carry = CARRY_FLAG;
    short argument = ((short)arg1 + (((short)arg2) << 4));
    flags |= (memory[argument+x] & 0x80); // set carry flag to bit 7
    memory[argument+x] = memory[argument+x] << 1;
    memory[argument+x] |= old_carry; // set bit 0 to old carry flag
    update_flags(memory[argument+x],0);
}

void ror() {
    char old_carry = CARRY_FLAG;
    flags |= (a & 0x01); // set carry flag to bit 0
    a = a >> 1;
    a |= (old_carry << 7); // set bit 7 to old carry flag
    update_flags(a,0);
}

void ror_z(char argument) {
    char old_carry = CARRY_FLAG;
    flags |= (memory[argument] & 0x01); // set carry flag to bit 0
    memory[argument] = memory[argument] >> 1;
    memory[argument] |= (old_carry << 7); // set bit 7 to old carry flag
    update_flags(memory[argument],0);
}

void ror_zx(char argument) {
    char old_carry = CARRY_FLAG;
    flags |= (memory[argument+x] & 0x01); // set carry flag to bit 0
    memory[argument+x] = memory[argument+x] >> 1;
    memory[argument+x] |= (old_carry << 7); // set bit 7 to old carry flag
    update_flags(memory[argument+x],0);
}

void ror_a(char arg1, char arg2) {
    char old_carry = CARRY_FLAG;
    short argument = ((short)arg1 + (((short)arg2) << 4));
    flags |= (memory[argument] & 0x01); // set carry flag to bit 0
    memory[argument] = memory[argument] >> 1;
    memory[argument] |= (old_carry << 7); // set bit 7 to old carry flag
    update_flags(memory[argument],0);
}

void ror_ax(char arg1, char arg2) {
    char old_carry = CARRY_FLAG;
    short argument = ((short)arg1 + (((short)arg2) << 4));
    flags |= (memory[argument+x] & 0x01); // set carry flag to bit 0
    memory[argument+x] = memory[argument+x] >> 1;
    memory[argument+x] |= (old_carry << 7); // set bit 7 to old carry flag
    update_flags(memory[argument+x],0);
}
