#include "tokenizer.h"
#include <stdio.h>

// TODO: use formatted strings to for error

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
		}
		if (tok.num > 0xFFFF) {
			tok.type = TOKEN_ERR;
			tok.err  = "number out of range";
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
		tok.com  = tk->data + 1;
		while (!IS_NEWLINE(*tk->data) && *tk->data) {
			tk->data ++;
			tok.len ++;
		}
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
	return tok;
}

const char *token_name(int type) {
	switch (type) {
		case TOKEN_NONE: return "none";
		case TOKEN_SYM : return "symbol";
		case TOKEN_NUM : return "number";
		case TOKEN_ERR : return "error";
		case TOKEN_EOI : return "EOI";
		case TOKEN_COL : return "colon";
		case TOKEN_COM : return "comma";
		case TOKEN_CMT : return "comment";
		default: return "unknown";
	}
}

void token_print(token tok) {
	printf ("%4i [%4i:%-4i] %-10s", tok.row, tok.col, tok.col + tok.len, token_name(tok.type));
	switch (tok.type) {
		case TOKEN_SYM:
			printf(": %.*s", tok.len, tok.sym);
			break;
		case TOKEN_NUM:
			printf(": 0x%0x", tok.num);
			break;
		case TOKEN_ERR:
			printf(": %s", tok.err);
			break;
		case TOKEN_CMT:
			printf(": %.*s", tok.len - 1, tok.com);
			break;
	}
}

void token_println(token tok) { token_print(tok); putchar('\n'); }
