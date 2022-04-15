#include "tokenizer.h"
#include "memory.h"
#include "tables.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char memory[0x10000];
static int loadat;

// TODO: CLEAN THINGS UP

// TODO: use formatted strings to return error

static int inc_loadat() {
	int p  = loadat;
	loadat = (loadat + 1) & 0xFFFF;
	return p;
}

const char *load_instruction_opcode(lenstring n, tokenizer *tk) {
	int64_t i = (int64_t)instruction_table_get(n);

	if (i != -1)
		memory[inc_loadat()] = i;
	else
		return "bad key";

	token a = tokenizer_get_next(tk);
	if (a.type != TOKEN_EOI && a.type != TOKEN_CMT)
		return "expected end of instruction";
	return NULL;
}

const char *load_instruction(lenstring n, tokenizer *tk) {
	int64_t c = (int64_t)partial_instruction_table_get(n);
	return (c != -1) ?
		((loader)c)(n, tk) :
		load_instruction_opcode(n, tk);
}

/* "MOV" */
const char *load_instruction_mov(lenstring n, tokenizer *tk) {
	char k[10] = "MOV ";

	token a = tokenizer_get_next(tk);
	if (a.type != TOKEN_SYM)
		return "expected one of 'A', 'B', 'C', 'D', 'E', 'H', 'L' or 'M'";
	if (a.len != 1)
		return "expected one of 'A', 'B', 'C', 'D', 'E', 'H', 'L' or 'M'";

	char p = TO_UCASE(a.sym[0]);
	if (p != 'A' && p != 'B' && p != 'C' && p != 'D' &&
		p != 'E' && p != 'H' && p != 'L' && p != 'M')
		return "expected one of 'A', 'B', 'C', 'D', 'E', 'H', 'L' or 'M'";
	k[4] = p;

	a = tokenizer_get_next(tk);
	if (a.type != TOKEN_COM)
		return "expected ','";
	k[5] = ',';

	a = tokenizer_get_next(tk);
	if (a.type != TOKEN_SYM)
		return (p == 'M') ?
			"expected one of 'A', 'B', 'C', 'D', 'E', 'H' or 'L'" :
			"expected one of 'A', 'B', 'C', 'D', 'E', 'H', 'L' or 'M'";
	if (a.len != 1)
		return (p == 'M') ?
			"expected one of 'A', 'B', 'C', 'D', 'E', 'H' or 'L'" :
			"expected one of 'A', 'B', 'C', 'D', 'E', 'H', 'L' or 'M'";

	char l = TO_UCASE(a.sym[0]);
	if ((l == 'A') || (l == 'B') || (l == 'C') ||
		(l == 'D') || (l == 'E') || (l == 'H') ||
		(l == 'L') || (l == 'M' && p != 'M'))
		k[6] = l;
	else
		return (l == 'M' && p == 'M') ?
			"MOV M,M is not a valid instruction":
			"expected one of 'A', 'B', 'C', 'D', 'E', 'H', 'L' or 'M'";

	lenstring key;
	key.data = k;
	key.len  = 7;
	return load_instruction_opcode(key, tk);
}

/* "MVI" */
const char *load_instruction_mvi(lenstring n, tokenizer *tk) {
	char k[10] = "MVI ";

	token a = tokenizer_get_next(tk);
	if (a.type != TOKEN_SYM)
		return "expected one of 'A', 'B', 'C', 'D', 'E', 'H', 'L' or 'M'";
	if (a.len != 1)
		return "expected one of 'A', 'B', 'C', 'D', 'E', 'H', 'L' or 'M'";

	char p = TO_UCASE(a.sym[0]);
	if (p != 'A' && p != 'B' && p != 'C' && p != 'D' &&
		p != 'E' && p != 'H' && p != 'L' && p != 'M')
		return "expected one of 'A', 'B', 'C', 'D', 'E', 'H', 'L' or 'M'";
	k[4] = p;

	a = tokenizer_get_next(tk);
	if (a.type != TOKEN_COM)
		return "expected ','";

	a = tokenizer_get_next(tk);
	if (a.type != TOKEN_NUM)
		return "expected 8-bit number (00h - 0xFFh)";
	if (a.num  > 0xFF)
		return "expected 8-bit number (00h - 0xFFh)";

	lenstring key;
	key.data = k;
	key.len  = strlen(k);
	const char *msg = load_instruction_opcode(key, tk);
	if (msg) return msg;

	memory[inc_loadat()] = a.num;

	return NULL;
}

/* "LXI" */
const char *load_instruction_lxi(lenstring n, tokenizer *tk) {
	char k[10] = "LXI ";

	token a = tokenizer_get_next(tk);
	if (a.type != TOKEN_SYM)
		return "expected one of 'B', 'D', 'H' or 'SP'";

	if (a.len == 1) {
		char p = TO_UCASE(a.sym[0]);
		if (p != 'B' && p != 'D' && p != 'H')
			return "expected one of 'B', 'D', 'H' or 'SP'";
		k[4] = p;
	} else if (a.len == 2) {
		char p0 = TO_UCASE(a.sym[0]);
		char p1 = TO_UCASE(a.sym[1]);
		if (p0 != 'S' || p1 != 'P')
			return "expected one of 'B', 'D', 'H' or 'SP'";
		k[4] = p0;
		k[5] = p1;
	} else {
		return "expected one of 'B', 'D', 'H' or 'SP'";
	}

	a = tokenizer_get_next(tk);
	if (a.type != TOKEN_COM)
		return "expected ','";

	a = tokenizer_get_next(tk);
	if (a.type != TOKEN_NUM) // TODO: labels are also allowed here
		return "expected 16-bit number (0000h - FFFFh)";
	if (a.num  > 0xFFFF)
		return "expected 16-bit number (0000h - FFFFh)";

	lenstring key;
	key.data = k;
	key.len  = strlen(k);
	const char *msg = load_instruction_opcode(key, tk);
	if (msg) return msg;

	memory[inc_loadat()] = (a.num >> 0) & 0xFF;
	memory[inc_loadat()] = (a.num >> 8) & 0xFF;

	return NULL;
}

/* "LDAX" */ /* "STAX" */
const char *load_instruction_ldax_stax(lenstring n, tokenizer *tk) {
	char k[10] = {0};

	int i = 0;
	for (; i < n.len; i ++)
		k[i] = n.data[i];
	k[i++] = ' ';

	token a = tokenizer_get_next(tk);
	if (a.type != TOKEN_SYM)
		return "expected 'B' or 'D'";
	if (a.len != 1)
		return "expected 'B' or 'D'";

	char p = TO_UCASE(a.sym[0]);
	if (p != 'B' && p != 'D')
		return "expected 'B' or 'D'";
	k[i++] = p;

	lenstring key;
	key.data = k;
	key.len  = strlen(k);
	return load_instruction_opcode(key, tk);
}

/* "PUSH" */ /* "POP"  */
const char *load_instruction_push_pop(lenstring n, tokenizer *tk) {
	char k[10] = {0};

	int i = 0;
	for (; i < n.len; i ++)
		k[i] = n.data[i];
	k[i++] = ' ';

	token a = tokenizer_get_next(tk);
	if (a.type != TOKEN_SYM)
		return "expected one of 'B', 'D', 'H' or 'PSW'";

	if (a.len == 1) {
		char p = TO_UCASE(a.sym[0]);
		if (p != 'B' && p != 'D' && p != 'H')
			return "expected one of 'B', 'D', 'H' or 'PSW'";
		k[i++] = p;
	} else if (a.len == 3) {
		char p0 = TO_UCASE(a.sym[0]);
		char p1 = TO_UCASE(a.sym[1]);
		char p2 = TO_UCASE(a.sym[2]);
		if (p0 != 'P' || p1 != 'S' || p2 != 'W')
			return "expected one of 'B', 'D', 'H' or 'PSW'";
		k[i++] = p0;
		k[i++] = p1;
		k[i++] = p2;
	} else {
		return "expected one of 'B', 'D', 'H' or 'PSW'";
	}

	lenstring key;
	key.data = k;
	key.len  = strlen(k);
	return load_instruction_opcode(key, tk);
}

/* "ADD" */ /* "ADC" */ /* "SUB" */ /* "SBB" */ /* "INR" */
/* "DCR" */ /* "ANA" */ /* "XRA" */ /* "ORA" */ /* "CMP" */
const char *load_instruction_arith_reg(lenstring n, tokenizer *tk) {
	char k[10] = {0};

	int i = 0;
	for (; i < n.len; i ++)
		k[i] = n.data[i];
	k[i++] = ' ';

	token a = tokenizer_get_next(tk);
	if (a.type != TOKEN_SYM)
		return "expected one of 'A', 'B', 'C', 'D', 'E', 'H', 'L' or 'M'";
	if (a.len != 1)
		return "expected one of 'A', 'B', 'C', 'D', 'E', 'H', 'L' or 'M'";

	char p = TO_UCASE(a.sym[0]);
	if (p != 'A' && p != 'B' && p != 'C' && p != 'D' &&
		p != 'E' && p != 'H' && p != 'L' && p != 'M')
		return "expected one of 'A', 'B', 'C', 'D', 'E', 'H', 'L' or 'M'";
	k[i++] = p;

	lenstring key;
	key.data = k;
	key.len  = strlen(k);
	return load_instruction_opcode(key, tk);
}

/* "DAD" */ /* "INX" */ /* "DCX" */
const char *load_instruction_arith_rp(lenstring n, tokenizer *tk) {
	char k[10] = {0};

	int i = 0;
	for (; i < n.len; i ++)
		k[i] = n.data[i];
	k[i++] = ' ';

	token a = tokenizer_get_next(tk);
	if (a.type != TOKEN_SYM)
		return "expected 'B', 'D' or 'H'";
	if (a.len != 1)
		return "expected 'B', 'D' or 'H'";

	char p = TO_UCASE(a.sym[0]);
	if (p != 'B' && p != 'D' && p != 'H')
		return "expected 'B', 'D' or 'H'";
	k[i++] = p;

	lenstring key;
	key.data = k;
	key.len  = strlen(k);
	return load_instruction_opcode(key, tk);
}

/* "ADI" */ /* "ACI" */ /* "SUI" */ /* "SBI" */
/* "ANI" */ /* "XRI" */ /* "ORI" */ /* "CPI" */
/* "IN"  */ /* "OUT" */
const char *load_instruction_imm_w(lenstring n, tokenizer *tk) {
	char k[10] = {0};
	for (int i = 0; i < n.len; i ++)
		k[i] = n.data[i];

	token a = tokenizer_get_next(tk);
	if (a.type != TOKEN_NUM)
		return "expected 8-bit number (00h - 0xFFh)";
	if (a.num  > 0xFF)
		return "expected 8-bit number (00h - 0xFFh)";

	lenstring key;
	key.data = k;
	key.len  = strlen(k);
	const char *msg = load_instruction_opcode(key, tk);
	if (msg) return msg;

	memory[inc_loadat()] = a.num;

	return NULL;
}

/* "LHLD" */ /* "SHLD" */ /* "LDA"  */ /* "STA" */
/* "JMP"  */
/* "JNZ"  */ /* "JZ"   */ /* "JNC"  */ /* "JC"  */
/* "JPO"  */ /* "JPE"  */ /* "JP"   */ /* "JM"  */
/* "CALL" */
/* "CNZ"  */ /* "CZ"   */ /* "CNC"  */ /* "CC"  */
/* "CPO"  */ /* "CPE"  */ /* "CP"   */ /* "CM"  */
/* "RET"  */
/* "RNZ"  */ /* "RZ"   */ /* "RNC"  */ /* "RC"  */
/* "RPO"  */ /* "RPE"  */ /* "RP"   */ /* "RM"  */
const char *load_instruction_imm_dw(lenstring n, tokenizer *tk) {
	char k[10] = {0};
	for (int i = 0; i < n.len; i ++)
		k[i] = n.data[i];

	token a = tokenizer_get_next(tk);
	if (a.type != TOKEN_NUM) // TODO: labels are also allowed here
		return "expected 16-bit number (0000h - FFFFh)";
	if (a.num  > 0xFFFF)
		return "expected 16-bit number (0000h - FFFFh)";

	lenstring key;
	key.data = k;
	key.len  = strlen(k);
	load_instruction_opcode(key, tk);

	memory[inc_loadat()] = (a.num >> 0) & 0xFF;
	memory[inc_loadat()] = (a.num >> 8) & 0xFF;

	return NULL;
}

/* "RST" */
const char *load_instruction_rst(lenstring n, tokenizer *tk) {
	char k[10] = "RST ";

	token a = tokenizer_get_next(tk);
	if (a.type != TOKEN_NUM)
		return "expected number number in range [0 - 7]";
	if (a.num  > 7)
		return "expected number number in range [0 - 7]";
	k[4] = a.num + '0';

	lenstring key;
	key.data = k;
	key.len  = strlen(k);
	return load_instruction_opcode(key, tk);
}
