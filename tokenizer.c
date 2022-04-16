#include "tokenizer.h"
#include "lenstring.h"
#include "fmtstr.h"
#include <stdio.h>

token tokenizer_get_next(tokenizer *tk){
	token tok;

	while (*tk->data && IS_SPACE(*tk->data)) {
		tk->data ++;
		tk->col ++;
	}

	tok.row = tk->row;
	tok.col = tk->col;
	tok.len = 0;

	if (IS_ALPHA(*tk->data)) {
		tok.type = TOKEN_SYM;
		tok.sym = tk->data;
		while (*tk->data && IS_ALNUM(*tk->data)) {
			tk->data ++;
			tk->col ++;
			tok.len ++;
		}
		return tok;
	}

	if (IS_NUM(*tk->data)) {
		tok.type = TOKEN_NUM;
		tok.num  = 0;
		unsigned int hex = 0;
		unsigned char is_decimal = 1;
		while (*tk->data && (IS_NUM(*tk->data)    ||
				(*tk->data >= 'A' && *tk->data <= 'F') ||
				(*tk->data >= 'a' && *tk->data <= 'f'))) {

			if (*tk->data >= 'A' && *tk->data <= 'F') {
				is_decimal = 0;
				hex = hex << 4 | (*tk->data - 'A');
			} else if (*tk->data >= 'a' && *tk->data <= 'f') {
				is_decimal = 0;
				hex = hex << 4 | (*tk->data - 'a');
			} else if (IS_NUM(*tk->data)) {
				hex = hex << 4 | (*tk->data - '0');
				tok.num = tok.num * 10 + *tk->data - '0';
			}

			tk->data ++;
			tk->col ++;
			tok.len ++;
		}
		if (*tk->data == 'h' || *tk->data == 'H') {
			tok.num = hex;
			tk->data ++;
			tk->col ++;
			tok.len ++;
		} else if (!is_decimal) {
			tok.type = TOKEN_ERR;
			tok.err  = "expected decimal number";
			tok.len  = strlen(tok.err);
		}
		if (tok.num > 0xFFFF) {
			tok.type = TOKEN_ERR;
			tok.err  = "number out of range";
			tok.len  = strlen(tok.err);
		}
		return tok;
	}

	if (*tk->data == ':') {
		tok.type = TOKEN_COL;
		tk->data ++;
		tk->col ++;
		tok.len ++;
		return tok;
	}

	if (*tk->data == ',') {
		tok.type = TOKEN_COM;
		tk->data ++;
		tok.col ++;
		tok.len ++;
		return tok;
	}

	if (*tk->data == ';') {
		tok.type = TOKEN_CMT;
		tok.cmt  = tk->data + 1;
		while (!IS_NEWLINE(*tk->data) && *tk->data) {
			tk->data ++;
			tok.len ++;
		}
		tok.len --;
		tk->data += (*tk->data != 0);
		tk->data += (*tk->data != 0) && (*tk->data == '\n');
		tk->col = 0;
		tk->row ++;
		return tok;
	}

	if (IS_NEWLINE(*tk->data) || !*tk->data) {
		tok.type = TOKEN_EOI;
		tk->data += (*tk->data != 0);
		tok.len  += (*tk->data != 0);
		tk->data += (*tk->data != 0) && (*tk->data == '\n');
		tok.len  += (*tk->data != 0) && (*tk->data == '\n');
		tk->col = 0; tk->row ++;
		return tok;
	}

	tok.type = TOKEN_ERR;
	tok.err  = "bad character";
	tok.len  = strlen(tok.err);
	return tok;
}

token tokenizer_peek_next(tokenizer *tk){
	int r    = tk->row;
	int c    = tk->col;
	char *d  = tk->data;
	token t  = tokenizer_get_next(tk);
	tk->row  = r;
	tk->col  = c;
	tk->data = d;
	return t;
}

lenstring tokenizer_get_line(tokenizer *tk){
	lenstring ret;
	ret.data = tk->data;
	ret.len = 0;
	while (!IS_NEWLINE(*tk->data) && *tk->data) {
		ret.len ++;
		tk->data ++;
	}
	tk->data += (*tk->data != 0);
	tk->data += (*tk->data != 0) && (*tk->data == '\n');
	return ret;
}

lenstring tokenizer_peek_line(tokenizer *tk){
	int r    = tk->row;
	int c    = tk->col;
	char *d  = tk->data;
	lenstring t = tokenizer_get_line(tk);
	tk->row  = r;
	tk->col  = c;
	tk->data = d;
	return t;
}

char *token_name(int type) {
	switch (type) {
		case TOKEN_INS: return "instruction";
		case TOKEN_REG: return "register";
		case TOKEN_REP: return "register pair";
		case TOKEN_SYM: return "symbol";
		case TOKEN_NUM: return "number";
		case TOKEN_WRD: return "word";
		case TOKEN_DWD: return "double word";
		case TOKEN_ERR: return "error";
		case TOKEN_EOI: return "end of instruction";
		case TOKEN_COL: return "colon";
		case TOKEN_COM: return "comma";
		case TOKEN_CMT: return "comment";
		default: return "unknown";
	}
}

char *token_val_str(token t) {
	switch (t.type) {
		case TOKEN_INS: return fmtstr("%.*s", t.len, t.sym);
		case TOKEN_REG: return fmtstr("%.*s", t.len, t.sym);
		case TOKEN_REP: return fmtstr("%.*s", t.len, t.sym);
		case TOKEN_SYM: return fmtstr("%.*s", t.len, t.sym);
		case TOKEN_NUM: return fmtstr("%XH", t.num);
		case TOKEN_WRD: return fmtstr("%.2XH", t.num);
		case TOKEN_DWD: return fmtstr("%.4XH", t.num);
		case TOKEN_ERR: return fmtstr("%.*s", t.len, t.err);
		case TOKEN_EOI: return fmtstr("\\n");
		case TOKEN_COL: return fmtstr(":");
		case TOKEN_COM: return fmtstr(",");
		case TOKEN_CMT: return fmtstr("%.*s", t.len, t.cmt);
		default: return fmtstr("unknown");
	}
}

void token_print(token tok) {
	char *val = token_val_str(tok);
	printf ("%4i [%i-%i] %-20s %s", tok.row, tok.col, tok.col + tok.len, token_name(tok.type), val);
	free(val);
}

void token_println(token tok) { token_print(tok); putchar('\n'); }
