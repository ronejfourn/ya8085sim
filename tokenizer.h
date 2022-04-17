typedef struct lenstring lenstring;
typedef struct tokenizer tokenizer;
typedef struct token     token;

const char *token_name(int type);
char *token_val_str(token t);
token tokenizer_get_next (tokenizer *tk);
token tokenizer_peek_next (tokenizer *tk);
lenstring tokenizer_get_line(tokenizer *tk);
lenstring tokenizer_peek_line(tokenizer *tk);
void token_print(token tok);
void token_println(token tok);

enum {
	TOKEN_SYM, TOKEN_NUM, TOKEN_ERR, TOKEN_COL, TOKEN_COM, TOKEN_EOI,
	TOKEN_CMT, TOKEN_INS, TOKEN_REG, TOKEN_REP, TOKEN_WRD, TOKEN_DWD
};

struct token {
	int type;
	union {
		const char *sym;
		const char *err;
		const char *cmt;
		unsigned int num;
	};
	int row, col, len;
};

struct tokenizer {
	char *data;
	int row, col;
};
