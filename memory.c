#include "tokenizer.h"
#include "memory.h"
#include "tables.h"
#include "fmtstr.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char memory[0x10000];
static int loadat;

static int inc_loadat() {
	int p  = loadat;
	loadat = (loadat + 1) & 0xFFFF;
	return p;
}

char *load_instruction_opcode(lenstring n, tokenizer *tk) {
	int64_t i = (int64_t)instruction_table_get(n);

	if (i != -1)
		memory[inc_loadat()] = i;
	else
		return fmtstr("at [%i:%i] bad token, got %.*s",
				tk->row, tk->col, n.len, n.data);

	token a = tokenizer_get_next(tk);
	if (a.type != TOKEN_EOI && a.type != TOKEN_CMT) {
		char *v = token_val_str(a);
		char *m = fmtstr("at [%i:%i] expected %s or %s, got (%s) %s", a.row, a.col,
				token_name(TOKEN_EOI), token_name(TOKEN_CMT), token_name(a.type), v);
		free(v);
		return m;
	}
	return NULL;
}

char *load_instruction(lenstring n, tokenizer *tk) {
	int64_t c = (int64_t)partial_instruction_table_get(n);
	return (c != -1) ?
		((loader)c)(n, tk) :
		load_instruction_opcode(n, tk);
}

/* "MOV" */
char *load_instruction_mov(lenstring n, tokenizer *tk) {
	char k[10] = "MOV ";

	token a = tokenizer_get_next(tk);
	if (a.type != TOKEN_SYM) {
		char *v = token_val_str(a);
		char *m = fmtstr("at [%i:%i] expected one of 'A', 'B', 'C', 'D', 'E', 'H', 'L' or 'M', got (%s) %s",
				a.row, a.col, token_name(a.type), v);
		free(v);
		return m;
	}
	if (a.len != 1)
		return fmtstr("at [%i:%i] expected one of 'A', 'B', 'C', 'D', 'E', 'H', 'L' or 'M', got %.*s",
				a.row, a.col, a.len, a.sym);

	char p = TO_UCASE(a.sym[0]);
	if (p != 'A' && p != 'B' && p != 'C' && p != 'D' &&
		p != 'E' && p != 'H' && p != 'L' && p != 'M')
		return fmtstr("at [%i:%i] expected one of 'A', 'B', 'C', 'D', 'E', 'H', 'L' or 'M', got %.*s",
				a.row, a.col, a.len, a.sym);
	k[4] = p;

	a = tokenizer_get_next(tk);
	if (a.type != TOKEN_COM) {
		char *v = token_val_str(a);
		char *m = fmtstr("at [%i:%i] expected ',', got (%s) %s",
				a.row, a.col, token_name(a.type), v);
		free(v);
		return m;
	}
	k[5] = ',';

	a = tokenizer_get_next(tk);
	if (a.type != TOKEN_SYM)
		return (p == 'M') ?
			fmtstr("at [%i:%i] expected one of 'A', 'B', 'C', 'D', 'E', 'H' or 'L', got %.*s",
							a.row, a.col, a.len, a.sym):
			fmtstr("at [%i:%i] expected one of 'A', 'B', 'C', 'D', 'E', 'H', 'L' or 'M', got %.*s",
							a.row, a.col, a.len, a.sym);
	if (a.len != 1)
		return (p == 'M') ?
			fmtstr("at [%i:%i] expected one of 'A', 'B', 'C', 'D', 'E', 'H' or 'L', got %.*s",
							a.row, a.col, a.len, a.sym):
			fmtstr("at [%i:%i] expected one of 'A', 'B', 'C', 'D', 'E', 'H', 'L' or 'M', got %.*s",
							a.row, a.col, a.len, a.sym);

	char l = TO_UCASE(a.sym[0]);
	if ((l == 'A') || (l == 'B') || (l == 'C') ||
		(l == 'D') || (l == 'E') || (l == 'H') ||
		(l == 'L') || (l == 'M' && p != 'M'))
		k[6] = l;
	else
		return (l == 'M' && p == 'M') ?
			fmtstr("at [%i:%i] MOV M,M is not a valid instruction",
							a.row, a.col):
			fmtstr("at [%i:%i] expected one of 'A', 'B', 'C', 'D', 'E', 'H', 'L' or 'M', got %.*s",
							a.row, a.col, a.len, a.sym);

	lenstring key;
	key.data = k;
	key.len  = 7;
	return load_instruction_opcode(key, tk);
}

/* "MVI" */
char *load_instruction_mvi(lenstring n, tokenizer *tk) {
	char k[10] = "MVI ";

	token a = tokenizer_get_next(tk);
	if (a.type != TOKEN_SYM) {
		char *v = token_val_str(a);
		char *m = fmtstr("at [%i:%i] expected one of 'A', 'B', 'C', 'D', 'E', 'H', 'L' or 'M', got (%s) %s",
				a.row, a.col, token_name(a.type), v);
		free(v);
		return m;
	}
	if (a.len != 1)
		return fmtstr("at [%i:%i] expected one of 'A', 'B', 'C', 'D', 'E', 'H', 'L' or 'M', got %.*s",
				a.row, a.col, a.len, a.sym);

	char p = TO_UCASE(a.sym[0]);
	if (p != 'A' && p != 'B' && p != 'C' && p != 'D' &&
		p != 'E' && p != 'H' && p != 'L' && p != 'M')
		return fmtstr("at [%i:%i] expected one of 'A', 'B', 'C', 'D', 'E', 'H', 'L' or 'M', got %.*s",
				a.row, a.col, a.len, a.sym);
	k[4] = p;

	a = tokenizer_get_next(tk);
	if (a.type != TOKEN_COM) {
		char *v = token_val_str(a);
		char *m = fmtstr("at [%i:%i] expected ',', got (%s) %s",
				a.row, a.col, token_name(a.type), v);
		free(v);
		return m;
	}

	a = tokenizer_get_next(tk);
	if (a.type != TOKEN_NUM) {
		char *v = token_val_str(a);
		char *m = fmtstr("at [%i:%i] expected 8-bit number (00h - 0ffh), got (%s) %s",
				a.row, a.col, token_name(a.type), v);
		free(v);
		return m;
	}
	if (a.num  > 0xFF)
		return fmtstr("at [%i:%i] expected 8-bit number (00h - 0ffh), got %0xh",
				a.row, a.col, a.num);

	lenstring key;
	key.data = k;
	key.len  = strlen(k);
	char *msg = load_instruction_opcode(key, tk);
	if (msg) return msg;

	memory[inc_loadat()] = a.num;

	return NULL;
}

/* "LXI" */
char *load_instruction_lxi(lenstring n, tokenizer *tk) {
	char k[10] = "LXI ";

	token a = tokenizer_get_next(tk);
	if (a.type != TOKEN_SYM) {
		char *v = token_val_str(a);
		char *m = fmtstr("at [%i:%i] expected one of 'B', 'D', 'H', or 'SP', got (%s) %s",
				a.row, a.col, token_name(a.type), v);
		free(v);
		return m;
	}

	if (a.len == 1) {
		char p = TO_UCASE(a.sym[0]);
		if (p != 'B' && p != 'D' && p != 'H')
			return fmtstr("at [%i:%i] expected one of 'B', 'D', 'H', or 'SP', got %.*s",
					a.row, a.col, a.len, a.sym);
		k[4] = p;
	} else if (a.len == 2) {
		char p0 = TO_UCASE(a.sym[0]);
		char p1 = TO_UCASE(a.sym[1]);
		if (p0 != 'S' || p1 != 'P')
			return fmtstr("at [%i:%i] expected one of 'B', 'D', 'H', or 'SP', got %.*s",
					a.row, a.col, a.len, a.sym);
		k[4] = p0;
		k[5] = p1;
	} else {
		return fmtstr("at [%i:%i] expected one of 'B', 'D', 'H', or 'SP', got %.*s",
				a.row, a.col, a.len, a.sym);
	}

	a = tokenizer_get_next(tk);
	if (a.type != TOKEN_COM)
		return "expected ','";

	a = tokenizer_get_next(tk);
	if (a.type != TOKEN_NUM) {  // TODO: labels are also allowed here
		char *v = token_val_str(a);
		char *m = fmtstr("at [%i:%i] expected 16-bit number (0000h - 0ffffh), got (%s) %s",
				a.row, a.col, token_name(a.type), v);
		free(v);
		return m;
	}
	if (a.num  > 0xFFFF)
		return fmtstr("at [%i:%i] expected 16-bit number (0000h - 0ffffh), got %0xh",
				a.row, a.col, a.num);

	lenstring key;
	key.data = k;
	key.len  = strlen(k);
	char *msg = load_instruction_opcode(key, tk);
	if (msg) return msg;

	memory[inc_loadat()] = (a.num >> 0) & 0xFF;
	memory[inc_loadat()] = (a.num >> 8) & 0xFF;

	return NULL;
}

/* "LDAX" */ /* "STAX" */
char *load_instruction_ldax_stax(lenstring n, tokenizer *tk) {
	char k[10] = {0};

	int i = 0;
	for (; i < n.len; i ++)
		k[i] = n.data[i];
	k[i++] = ' ';

	token a = tokenizer_get_next(tk);
	if (a.type != TOKEN_SYM) {
		char *v = token_val_str(a);
		char *m = fmtstr("at [%i:%i] expected one of 'B' or 'D', got (%s) %s",
				a.row, a.col, token_name(a.type), v);
		free(v);
		return m;
	}
	if (a.len != 1)
		return fmtstr("at [%i:%i] expected one of 'B' or 'D', got %.*s",
				a.row, a.col, a.len, a.sym);

	char p = TO_UCASE(a.sym[0]);
	if (p != 'B' && p != 'D')
		return fmtstr("at [%i:%i] expected one of 'B' or 'D', got %.*s",
				a.row, a.col, a.len, a.sym);
	k[i++] = p;

	lenstring key;
	key.data = k;
	key.len  = strlen(k);
	return load_instruction_opcode(key, tk);
}

/* "PUSH" */ /* "POP"  */
char *load_instruction_push_pop(lenstring n, tokenizer *tk) {
	char k[10] = {0};

	int i = 0;
	for (; i < n.len; i ++)
		k[i] = n.data[i];
	k[i++] = ' ';

	token a = tokenizer_get_next(tk);
	if (a.type != TOKEN_SYM) {
		char *v = token_val_str(a);
		char *m = fmtstr("at [%i:%i] expected one of 'B', 'D', 'H' or 'PSW', got (%s) %s",
				a.row, a.col, token_name(a.type), v);
		free(v);
		return m;
	}

	if (a.len == 1) {
		char p = TO_UCASE(a.sym[0]);
		if (p != 'B' && p != 'D' && p != 'H')
			return fmtstr("at [%i:%i] expected one of 'B', 'D', 'H' or 'PSW', got %.*s",
					a.row, a.col, a.len, a.sym);
		k[i++] = p;
	} else if (a.len == 3) {
		char p0 = TO_UCASE(a.sym[0]);
		char p1 = TO_UCASE(a.sym[1]);
		char p2 = TO_UCASE(a.sym[2]);
		if (p0 != 'P' || p1 != 'S' || p2 != 'W')
			return fmtstr("at [%i:%i] expected one of 'B', 'D', 'H' or 'PSW', got %.*s",
					a.row, a.col, a.len, a.sym);
		k[i++] = p0;
		k[i++] = p1;
		k[i++] = p2;
	} else {
		return fmtstr("at [%i:%i] expected one of 'B', 'D', 'H' or 'PSW', got %.*s",
				a.row, a.col, a.len, a.sym);
	}

	lenstring key;
	key.data = k;
	key.len  = strlen(k);
	return load_instruction_opcode(key, tk);
}

/* "ADD" */ /* "ADC" */ /* "SUB" */ /* "SBB" */ /* "INR" */
/* "DCR" */ /* "ANA" */ /* "XRA" */ /* "ORA" */ /* "CMP" */
char *load_instruction_arith_reg(lenstring n, tokenizer *tk) {
	char k[10] = {0};

	int i = 0;
	for (; i < n.len; i ++)
		k[i] = n.data[i];
	k[i++] = ' ';

	token a = tokenizer_get_next(tk);
	if (a.type != TOKEN_SYM) {
		char *v = token_val_str(a);
		char *m = fmtstr("at [%i:%i] expected one of 'A', 'B', 'C', 'D', 'E', 'H', 'L' or 'M', got (%s) %s",
				a.row, a.col, token_name(a.type), v);
		free(v);
		return m;
	}
	if (a.len != 1)
		return fmtstr("at [%i:%i] expected one of 'A', 'B', 'C', 'D', 'E', 'H', 'L' or 'M', got %.*s",
				a.row, a.col, a.len, a.sym);

	char p = TO_UCASE(a.sym[0]);
	if (p != 'A' && p != 'B' && p != 'C' && p != 'D' &&
		p != 'E' && p != 'H' && p != 'L' && p != 'M')
		return fmtstr("at [%i:%i] expected one of 'A', 'B', 'C', 'D', 'E', 'H', 'L' or 'M', got %.*s",
				a.row, a.col, a.len, a.sym);
	k[i++] = p;

	lenstring key;
	key.data = k;
	key.len  = strlen(k);
	return load_instruction_opcode(key, tk);
}

/* "DAD" */ /* "INX" */ /* "DCX" */
char *load_instruction_arith_rp(lenstring n, tokenizer *tk) {
	char k[10] = {0};

	int i = 0;
	for (; i < n.len; i ++)
		k[i] = n.data[i];
	k[i++] = ' ';

	token a = tokenizer_get_next(tk);
	if (a.type != TOKEN_SYM) {
		char *v = token_val_str(a);
		char *m = fmtstr("at [%i:%i] expected one of 'B', 'D' or 'H', got (%s) %s",
				a.row, a.col, token_name(a.type), v);
		free(v);
		return m;
	}
	if (a.len != 1)
		return fmtstr("at [%i:%i] expected one of 'B', 'D' or 'H', got %.*s",
				a.row, a.col, a.len, a.sym);

	char p = TO_UCASE(a.sym[0]);
	if (p != 'B' && p != 'D' && p != 'H')
		return fmtstr("at [%i:%i] expected one of 'B', 'D' or 'H', got %.*s",
				a.row, a.col, a.len, a.sym);
	k[i++] = p;

	lenstring key;
	key.data = k;
	key.len  = strlen(k);
	return load_instruction_opcode(key, tk);
}

/* "ADI" */ /* "ACI" */ /* "SUI" */ /* "SBI" */
/* "ANI" */ /* "XRI" */ /* "ORI" */ /* "CPI" */
/* "IN"  */ /* "OUT" */
char *load_instruction_imm_w(lenstring n, tokenizer *tk) {
	char k[10] = {0};
	for (int i = 0; i < n.len; i ++)
		k[i] = n.data[i];

	token a = tokenizer_get_next(tk);
	if (a.type != TOKEN_NUM) {
		char *v = token_val_str(a);
		char *m = fmtstr("at [%i:%i] expected 8-bit number (00h - 0ffh), got (%s) %s",
				a.row, a.col, token_name(a.type), v);
		free(v);
		return m;
	}
	if (a.num  > 0xFF)
		return fmtstr("at [%i:%i] expected 8-bit number (00h - 0ffh), got %0xh",
				a.row, a.col, a.num);

	lenstring key;
	key.data = k;
	key.len  = strlen(k);
	char *msg = load_instruction_opcode(key, tk);
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
char *load_instruction_imm_dw(lenstring n, tokenizer *tk) {
	char k[10] = {0};
	for (int i = 0; i < n.len; i ++)
		k[i] = n.data[i];

	token a = tokenizer_get_next(tk);
	if (a.type != TOKEN_NUM) { // TODO: labels are also allowed here
		char *v = token_val_str(a);
		char *m = fmtstr("at [%i:%i] expected 16-bit number (0000h - 0ffffh), got (%s) %s",
				a.row, a.col, token_name(a.type), v);
		free(v);
		return m;
	}
	if (a.num  > 0xFFFF)
		return fmtstr("at [%i:%i] expected 16-bit number (0000h - 0ffffh), got %0xh",
				a.row, a.col, a.num);

	lenstring key;
	key.data = k;
	key.len  = strlen(k);
	load_instruction_opcode(key, tk);

	memory[inc_loadat()] = (a.num >> 0) & 0xFF;
	memory[inc_loadat()] = (a.num >> 8) & 0xFF;

	return NULL;
}

/* "RST" */
char *load_instruction_rst(lenstring n, tokenizer *tk) {
	char k[10] = "RST ";

	token a = tokenizer_get_next(tk);
	if (a.type != TOKEN_NUM) {
		char *v = token_val_str(a);
		char *m = fmtstr("at [%i:%i] expected number in range [0 - 7], got (%s) %s",
				a.row, a.col, token_name(a.type), v);
		free(v);
		return m;
	}
	if (a.num  > 7)
		return fmtstr("at [%i:%i] expected number in range [0 - 7], got %0x",
				a.row, a.col, a.num);
	k[4] = a.num + '0';

	lenstring key;
	key.data = k;
	key.len  = strlen(k);
	return load_instruction_opcode(key, tk);
}
