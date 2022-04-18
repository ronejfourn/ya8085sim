#include "tokenizer.h"
#include "memory.h"
#include "tables.h"
#include "fmtstr.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

uint8_t memory[0x10000];
static uint16_t loadat;
static uint16_t loadcpy;

struct sqdata{
	token t;
	int i;
};

struct {
	int count, cap;
	sqdata *data;
} sq;

void reset_load() {loadcpy = loadat;}
uint16_t get_loadat() {return loadat;}
uint16_t get_load_pos() {return loadcpy;}
void set_loadat(uint16_t v) {
	loadat = v;
	loadcpy = v;
}

void symbol_queue_init() {
	sq.data  = (sqdata *)malloc(sizeof(*sq.data) * 8);
	sq.cap   = 8;
	sq.count = 0;
}

void symbol_queue_end() {
	free(sq.data);
	sq.data = nullptr;
	sq.cap   = 0;
	sq.count = 0;
}

void symbol_queue(token t) {
	if (sq.count == sq.cap) {
		sq.cap ++;
		sq.data = (sqdata *)realloc(sq.data, sq.cap * sizeof(*sq.data));
	}
	sq.data[sq.count].t = t;
	sq.data[sq.count].i = loadcpy;
	sq.count ++;
}

token *symbol_resolve() {
	for (int i = 0; i < sq.count; i++) {
		lenstring key = {
			sq.data[i].t.sym,
			(size_t)sq.data[i].t.len
		};
		int64_t p = (int64_t)symbol_table_get(key);
		if (p != -1) {
			memory[sq.data[i].i    ] = (p >> 0) & 0xFF;
			memory[sq.data[i].i + 1] = (p >> 8) & 0xFF;
		} else {
			return &sq.data[i].t;
		}
	}
	return NULL;
}

char *load_instruction(lenstring n, token *tk, int *ti) {
	tk[*ti].type = TOKEN_INS;
	int64_t c = (int64_t)partial_instruction_table_get(n);
	return (c != -1) ?
		((loader)c)(n, tk, ti):
		load_opcode(n, tk, ti);
}

char *load_opcode(lenstring n, token *tk, int *ti) {
	int64_t i = (int64_t)instruction_table_get(n);

	if (i != -1)
		memory[loadcpy++] = i & 0xFF;
	else
		return fmtstr("unknown instruction, got %.*s", n.len, n.data);

	token a = tk[++(*ti)];
	if (a.type != TOKEN_EOI && a.type != TOKEN_CMT) {
		char *v = token_val_str(a);
		char *m = fmtstr("expected %s or %s, got (%s) %s",
				token_name(TOKEN_EOI), token_name(TOKEN_CMT), token_name(a.type), v);
		free(v);
		return m;
	}
	return NULL;
}

/* "MOV" */
char *load_mov(lenstring n, token *tk, int *ti) {
	char k[10] = "MOV ";

	token a = tk[++(*ti)];
	if (a.type != TOKEN_SYM) {
		char *v = token_val_str(a);
		char *m = fmtstr("expected one of 'A', 'B', 'C', 'D', 'E', 'H', 'L' or 'M', got (%s) %s", token_name(a.type), v);
		free(v);
		return m;
	}
	if (a.len != 1)
		return fmtstr("expected one of 'A', 'B', 'C', 'D', 'E', 'H', 'L' or 'M', got %.*s", a.len, a.sym);

	char p = TO_UCASE(a.sym[0]);
	if (p != 'A' && p != 'B' && p != 'C' && p != 'D' &&
		p != 'E' && p != 'H' && p != 'L' && p != 'M')
		return fmtstr("expected one of 'A', 'B', 'C', 'D', 'E', 'H', 'L' or 'M', got %.*s", a.len, a.sym);
	tk[*ti].type = TOKEN_REG;
	k[4] = p;

	a = tk[++(*ti)];
	if (a.type != TOKEN_COM) {
		char *v = token_val_str(a);
		char *m = fmtstr("expected ',', got (%s) %s", token_name(a.type), v);
		free(v);
		return m;
	}
	k[5] = ',';

	a = tk[++(*ti)];
	if (a.type != TOKEN_SYM)
		return (p == 'M') ?
			fmtstr("expected one of 'A', 'B', 'C', 'D', 'E', 'H' or 'L', got %.*s", a.len, a.sym):
			fmtstr("expected one of 'A', 'B', 'C', 'D', 'E', 'H', 'L' or 'M', got %.*s", a.len, a.sym);
	if (a.len != 1)
		return (p == 'M') ?
			fmtstr("expected one of 'A', 'B', 'C', 'D', 'E', 'H' or 'L', got %.*s", a.len, a.sym):
			fmtstr("expected one of 'A', 'B', 'C', 'D', 'E', 'H', 'L' or 'M', got %.*s", a.len, a.sym);

	char l = TO_UCASE(a.sym[0]);
	if ((l == 'A') || (l == 'B') || (l == 'C') ||
		(l == 'D') || (l == 'E') || (l == 'H') ||
		(l == 'L') || (l == 'M' && p != 'M'))
		k[6] = l;
	else
		return (l == 'M' && p == 'M') ?
			fmtstr("MOV M,M is not a valid instruction"):
			fmtstr("expected one of 'A', 'B', 'C', 'D', 'E', 'H', 'L' or 'M', got %.*s", a.len, a.sym);
	tk[*ti].type = TOKEN_REG;
	return load_opcode(ls_from_cstr(k), tk, ti);
}

/* "MVI" */
char *load_mvi(lenstring n, token *tk, int *ti) {
	char k[10] = "MVI ";

	token a = tk[++(*ti)];
	if (a.type != TOKEN_SYM) {
		char *v = token_val_str(a);
		char *m = fmtstr("expected one of 'A', 'B', 'C', 'D', 'E', 'H', 'L' or 'M', got (%s) %s", token_name(a.type), v);
		free(v);
		return m;
	}
	if (a.len != 1)
		return fmtstr("expected one of 'A', 'B', 'C', 'D', 'E', 'H', 'L' or 'M', got %.*s", a.len, a.sym);

	char p = TO_UCASE(a.sym[0]);
	if (p != 'A' && p != 'B' && p != 'C' && p != 'D' &&
		p != 'E' && p != 'H' && p != 'L' && p != 'M')
		return fmtstr("expected one of 'A', 'B', 'C', 'D', 'E', 'H', 'L' or 'M', got %.*s", a.len, a.sym);
	k[4] = p;
	tk[*ti].type = TOKEN_REG;

	a = tk[++(*ti)];
	if (a.type != TOKEN_COM) {
		char *v = token_val_str(a);
		char *m = fmtstr("expected ',', got (%s) %s", token_name(a.type), v);
		free(v);
		return m;
	}

	a = tk[++(*ti)];
	if (a.type != TOKEN_NUM) {
		char *v = token_val_str(a);
		char *m = fmtstr("expected 8-bit number (00h - 0ffh), got (%s) %s", token_name(a.type), v);
		free(v);
		return m;
	}
	if (a.num  > 0xFF)
		return fmtstr("expected 8-bit number (00h - 0ffh), got %0xh", a.num);
	tk[*ti].type = TOKEN_WRD;

	char *msg = load_opcode(ls_from_cstr(k), tk, ti);
	if (msg) return msg;

	memory[loadcpy++] = a.num & 0xFF;

	return NULL;
}

/* "LXI" */
char *load_lxi(lenstring n, token *tk, int *ti) {
	char k[10] = "LXI ";

	token a = tk[++(*ti)];
	if (a.type != TOKEN_SYM) {
		char *v = token_val_str(a);
		char *m = fmtstr("expected one of 'B', 'D', 'H', or 'SP', got (%s) %s", token_name(a.type), v);
		free(v);
		return m;
	}

	if (a.len == 1) {
		char p = TO_UCASE(a.sym[0]);
		if (p != 'B' && p != 'D' && p != 'H')
			return fmtstr("expected one of 'B', 'D', 'H', or 'SP', got %.*s", a.len, a.sym);
		k[4] = p;
	} else if (a.len == 2) {
		char p0 = TO_UCASE(a.sym[0]);
		char p1 = TO_UCASE(a.sym[1]);
		if (p0 != 'S' || p1 != 'P')
			return fmtstr("expected one of 'B', 'D', 'H', or 'SP', got %.*s", a.len, a.sym);
		k[4] = p0;
		k[5] = p1;
	} else {
		return fmtstr("expected one of 'B', 'D', 'H', or 'SP', got %.*s", a.len, a.sym);
	}
	tk[*ti].type = TOKEN_REP;

	a = tk[++(*ti)];
	if (a.type != TOKEN_COM) {
		char *v = token_val_str(a);
		char *m = fmtstr("expected ',', got (%s) %s", token_name(a.type), v);
		free(v);
		return m;
	}

	a = tk[++(*ti)];
	if (a.type == TOKEN_SYM) {
		char *msg = load_opcode(ls_from_cstr(k), tk, ti);
		if (msg) return msg;

		int64_t p = (int64_t)symbol_table_get((lenstring) {a.sym, (size_t)a.len});
		if (p != -1) {
			memory[loadcpy++] = (p >> 0) & 0xFF;
			memory[loadcpy++] = (p >> 8) & 0xFF;
		} else {
			symbol_queue(a);
			loadcpy++;
			loadcpy++;
		}
	} else if (a.type == TOKEN_NUM) {
		tk[*ti].type = TOKEN_DWD;
		char *msg = load_opcode(ls_from_cstr(k), tk, ti);
		if (msg) return msg;

		if (a.num  > 0xFFFF)
			return fmtstr("expected 16-bit number (0000h - 0ffffh), got %0xh", a.num);

		memory[loadcpy++] = (a.num >> 0) & 0xFF;
		memory[loadcpy++] = (a.num >> 8) & 0xFF;
	} else {
		char *v = token_val_str(a);
		char *m = fmtstr("expected 16-bit number (0000h - 0ffffh) or label, got (%s) %s", token_name(a.type), v);
		free(v);
		return m;
	}

	return NULL;
}

/* "LDAX" */ /* "STAX" */
char *load_ldax_stax(lenstring n, token *tk, int *ti) {
	char k[10] = {0};

	int i = 0;
	for (; i < n.len; i ++)
		k[i] = n.data[i];
	k[i++] = ' ';

	token a = tk[++(*ti)];
	if (a.type != TOKEN_SYM) {
		char *v = token_val_str(a);
		char *m = fmtstr("expected one of 'B' or 'D', got (%s) %s", token_name(a.type), v);
		free(v);
		return m;
	}
	if (a.len != 1)
		return fmtstr("expected one of 'B' or 'D', got %.*s", a.len, a.sym);

	char p = TO_UCASE(a.sym[0]);
	if (p != 'B' && p != 'D')
		return fmtstr("expected one of 'B' or 'D', got %.*s", a.len, a.sym);
	k[i++] = p;
	tk[*ti].type = TOKEN_REP;

	return load_opcode(ls_from_cstr(k), tk, ti);
}

/* "PUSH" */ /* "POP"  */
char *load_push_pop(lenstring n, token *tk, int *ti) {
	char k[10] = {0};

	int i = 0;
	for (; i < n.len; i ++)
		k[i] = n.data[i];
	k[i++] = ' ';

	token a = tk[++(*ti)];
	if (a.type != TOKEN_SYM) {
		char *v = token_val_str(a);
		char *m = fmtstr("expected one of 'B', 'D', 'H' or 'PSW', got (%s) %s", token_name(a.type), v);
		free(v);
		return m;
	}

	if (a.len == 1) {
		char p = TO_UCASE(a.sym[0]);
		if (p != 'B' && p != 'D' && p != 'H')
			return fmtstr("expected one of 'B', 'D', 'H' or 'PSW', got %.*s", a.len, a.sym);
		k[i++] = p;
	} else if (a.len == 3) {
		char p0 = TO_UCASE(a.sym[0]);
		char p1 = TO_UCASE(a.sym[1]);
		char p2 = TO_UCASE(a.sym[2]);
		if (p0 != 'P' || p1 != 'S' || p2 != 'W')
			return fmtstr("expected one of 'B', 'D', 'H' or 'PSW', got %.*s", a.len, a.sym);
		k[i++] = p0;
		k[i++] = p1;
		k[i++] = p2;
	} else {
		return fmtstr("expected one of 'B', 'D', 'H' or 'PSW', got %.*s", a.len, a.sym);
	}
	tk[*ti].type = TOKEN_REP;

	return load_opcode(ls_from_cstr(k), tk, ti);
}

/* "ADD" */ /* "ADC" */ /* "SUB" */ /* "SBB" */ /* "INR" */
/* "DCR" */ /* "ANA" */ /* "XRA" */ /* "ORA" */ /* "CMP" */
char *load_arith_reg(lenstring n, token *tk, int *ti) {
	char k[10] = {0};

	int i = 0;
	for (; i < n.len; i ++)
		k[i] = n.data[i];
	k[i++] = ' ';

	token a = tk[++(*ti)];
	if (a.type != TOKEN_SYM) {
		char *v = token_val_str(a);
		char *m = fmtstr("expected one of 'A', 'B', 'C', 'D', 'E', 'H', 'L' or 'M', got (%s) %s", token_name(a.type), v);
		free(v);
		return m;
	}
	if (a.len != 1)
		return fmtstr("expected one of 'A', 'B', 'C', 'D', 'E', 'H', 'L' or 'M', got %.*s", a.len, a.sym);

	char p = TO_UCASE(a.sym[0]);
	if (p != 'A' && p != 'B' && p != 'C' && p != 'D' &&
		p != 'E' && p != 'H' && p != 'L' && p != 'M')
		return fmtstr("expected one of 'A', 'B', 'C', 'D', 'E', 'H', 'L' or 'M', got %.*s", a.len, a.sym);
	k[i++] = p;
	tk[*ti].type = TOKEN_REG;

	return load_opcode(ls_from_cstr(k), tk, ti);
}

/* "DAD" */ /* "INX" */ /* "DCX" */
char *load_arith_rp(lenstring n, token *tk, int *ti) {
	char k[10] = {0};

	int i = 0;
	for (; i < n.len; i ++)
		k[i] = n.data[i];
	k[i++] = ' ';

	token a = tk[++(*ti)];
	if (a.type != TOKEN_SYM) {
		char *v = token_val_str(a);
		char *m = fmtstr("expected one of 'B', 'D', 'H', or 'SP', got (%s) %s", token_name(a.type), v);
		free(v);
		return m;
	}

	if (a.len == 1) {
		char p = TO_UCASE(a.sym[0]);
		if (p != 'B' && p != 'D' && p != 'H')
			return fmtstr("expected one of 'B', 'D', 'H', or 'SP', got %.*s", a.len, a.sym);
		k[i++] = p;
	} else if (a.len == 2) {
		char p0 = TO_UCASE(a.sym[0]);
		char p1 = TO_UCASE(a.sym[1]);
		if (p0 != 'S' || p1 != 'P')
			return fmtstr("expected one of 'B', 'D', 'H', or 'SP', got %.*s", a.len, a.sym);
		k[i++] = p0;
		k[i++] = p1;
	} else {
		return fmtstr("expected one of 'B', 'D', 'H', or 'SP', got %.*s", a.len, a.sym);
	}
	tk[*ti].type = TOKEN_REP;

	return load_opcode(ls_from_cstr(k), tk, ti);
}

/* "ADI" */ /* "ACI" */ /* "SUI" */ /* "SBI" */
/* "ANI" */ /* "XRI" */ /* "ORI" */ /* "CPI" */
/* "IN"  */ /* "OUT" */
char *load_imm_w(lenstring n, token *tk, int *ti) {
	char k[10] = {0};
	for (int i = 0; i < n.len; i ++)
		k[i] = n.data[i];

	token a = tk[++(*ti)];
	if (a.type != TOKEN_NUM) {
		char *v = token_val_str(a);
		char *m = fmtstr("expected 8-bit number (00h - 0ffh), got (%s) %s", token_name(a.type), v);
		free(v);
		return m;
	}
	if (a.num  > 0xFF)
		return fmtstr("expected 8-bit number (00h - 0ffh), got %0xh", a.num);
	tk[*ti].type = TOKEN_WRD;

	char *msg = load_opcode(ls_from_cstr(k), tk, ti);
	if (msg) return msg;

	memory[loadcpy++] = a.num & 0xFF;

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
char *load_imm_dw(lenstring n, token *tk, int *ti) {
	char k[10] = {0};
	for (int i = 0; i < n.len; i ++)
		k[i] = n.data[i];

	token a = tk[++(*ti)];
	if (a.type == TOKEN_SYM) {
		char *msg = load_opcode(ls_from_cstr(k), tk, ti);
		if (msg) return msg;

		int64_t p = (int64_t)symbol_table_get((lenstring) {a.sym, (size_t)a.len});
		if (p != -1) {
			memory[loadcpy++] = (p >> 0) & 0xFF;
			memory[loadcpy++] = (p >> 8) & 0xFF;
		} else {
			symbol_queue(a);
			loadcpy++;
			loadcpy++;
		}
	} else if (a.type == TOKEN_NUM) {
		tk[*ti].type = TOKEN_DWD;
		char *msg = load_opcode(ls_from_cstr(k), tk, ti);
		if (msg) return msg;

		if (a.num  > 0xFFFF)
			return fmtstr("expected 16-bit number (0000h - 0ffffh), got %0xh", a.num);

		memory[loadcpy++] = (a.num >> 0) & 0xFF;
		memory[loadcpy++] = (a.num >> 8) & 0xFF;
	} else {
		char *v = token_val_str(a);
		char *m = fmtstr("expected 16-bit number (0000h - 0ffffh) or label, got (%s) %s", token_name(a.type), v);
		free(v);
		return m;
	}

	return NULL;
}

/* "RST" */
char *load_rst(lenstring n, token *tk, int *ti) {
	char k[10] = "RST ";

	tk[*ti].type = TOKEN_INS;
	token a = tk[++(*ti)];
	if (a.type != TOKEN_NUM) {
		char *v = token_val_str(a);
		char *m = fmtstr("expected number in range [0 - 7], got (%s) %s", token_name(a.type), v);
		free(v);
		return m;
	}
	if (a.num  > 7)
		return fmtstr("expected number in range [0 - 7], got %0x", a.num);
	k[4] = a.num + '0';

	return load_opcode(ls_from_cstr(k), tk, ti);
}
