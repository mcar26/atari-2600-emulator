#include <map>

#define CARRY_FLAG (flags && 0x01)
#define ZERO_FLAG (flags && 0x02)
#define DECIMAL_FLAG (flags && 0x08)
#define OVERFLOW_FLAG (flags && 0x40)
#define SIGN_FLAG (flags && 0x80)

#define TO_BCD(n) ((n & 0x0f) + ((n & 0xf0) >> 4)*10)
#define CHARS_TO_SHORT(n1, n2) (n1 + (((short)n2) << 8))

extern char* memory;
extern char flags, a, x, y, sp;
extern short pc;

extern const std::map<char, void(*)()> ops_noargs;
extern const std::map<char, void(*)(char)> ops_onearg;
extern const std::map<char, void(*)(char, char)> ops_twoargs;

void nop();

void clc();

void sec();

void clv();

void sed();

void cld();

void cli();

void sei();

void tax();

void txa();

void txs();

void tsx();

void dex();

void inx();

void tay();

void tya();

void dey();

void iny();

void lda_im(char argument);

void lda_z(char argument);

void lda_zx(char argument);

void lda_a(char arg1, char arg2);

void lda_ax(char arg1, char arg2);

void lda_ay(char arg1, char arg2);

void lda_inx(char argument);

void lda_iny(char argument);

void ldx_im(char argument);

void ldx_z(char argument);

void ldx_zy(char argument);

void ldx_a(char arg1, char arg2);

void ldx_ay(char arg1, char arg2);

void ldy_im(char argument);

void ldy_z(char argument);

void ldy_zx(char argument);

void ldy_a(char arg1, char arg2);

void ldy_ax(char arg1, char arg2);

void sta_z(char argument);

void sta_zx(char argument);

void sta_a(char arg1, char arg2);

void sta_ax(char arg1, char arg2);

void sta_ay(char arg1, char arg2);

void sta_inx(char argument);

void sta_iny(char argument);

void stx_z(char argument);

void stx_zy(char argument);

void stx_a(char arg1, char arg2);

void sty_z(char argument);

void sty_zx(char argument);

void sty_a(char arg1, char arg2);

void pha();

void php();

void pla();

void plp();

void jmp_ain(char arg1, char arg2);

void jmp_a(char arg1, char arg2);

void call(char arg1, char arg2);

void reti();

void ret();

void jns(char argument);

void js(char argument);

void jno(char argument);

void jo(char argument);

void jnc(char argument);

void jc(char argument);

void jnz(char argument);

void jz(char argument);

void brk();

void adc_im(char argument);

void adc_z(char argument);

void adc_zx(char argument);

void adc_a(char arg1, char arg2);

void adc_ax(char arg1, char arg2);

void adc_ay(char arg1, char arg2);

void adc_inx(char argument);

void adc_iny(char argument);

void sbc_im(char argument);

void sbc_z(char argument);

void sbc_zx(char argument);

void sbc_a(char arg1, char arg2);

void sbc_ax(char arg1, char arg2);

void sbc_ay(char arg1, char arg2);

void sbc_inx(char argument);

void sbc_iny(char argument);

void and_im(char argument);

void and_z(char argument);

void and_zx(char argument);

void and_a(char arg1, char arg2);

void and_ax(char arg1, char arg2);

void and_ay(char arg1, char arg2);

void and_inx(char argument);

void and_iny(char argument);

void eor_im(char argument);

void eor_z(char argument);

void eor_zx(char argument);

void eor_a(char arg1, char arg2);

void eor_ax(char arg1, char arg2);

void eor_ay(char arg1, char arg2);

void eor_inx(char argument);

void eor_iny(char argument);

void ora_im(char argument);

void ora_z(char argument);

void ora_zx(char argument);

void ora_a(char arg1, char arg2);

void ora_ax(char arg1, char arg2);

void ora_ay(char arg1, char arg2);

void ora_inx(char argument);

void ora_iny(char argument);

void inc_z(char argument);

void inc_zx(char argument);

void inc_a(char arg1, char arg2);

void inc_ax(char arg1, char arg2);

void dec_z(char argument);

void dec_zx(char argument);

void dec_a(char arg1, char arg2);

void dec_ax(char arg1, char arg2);

void cmp_im(char argument);

void cmp_z(char argument);

void cmp_zx(char argument);

void cmp_a(char arg1, char arg2);

void cmp_ax(char arg1, char arg2);

void cmp_ay(char arg1, char arg2);

void cmp_inx(char argument);

void cmp_iny(char argument);

void cpx_im(char argument);

void cpx_z(char argument);

void cpx_a(char arg1, char arg2);

void cpy_im(char argument);

void cpy_z(char argument);

void cpy_a(char arg1, char arg2);

void bit_im(char argument);

void bit_a(char arg1, char arg2);

void asl();

void asl_z(char argument);

void asl_zx(char argument);

void asl_a(char arg1, char arg2);

void asl_ax(char arg1, char arg2);

void lsr();

void lsr_z(char argument);

void lsr_zx(char argument);

void lsr_a(char arg1, char arg2);

void lsr_ax(char arg1, char arg2);

void rol();

void rol_z(char argument);

void rol_zx(char argument);

void rol_a(char arg1, char arg2);

void rol_ax(char arg1, char arg2);

void ror();

void ror_z(char argument);

void ror_zx(char argument);

void ror_a(char arg1, char arg2);

void ror_ax(char arg1, char arg2);