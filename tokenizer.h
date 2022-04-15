typedef struct lenstring lenstring;
typedef struct tokenizer tokenizer;
typedef struct token     token;

const char *token_name(int type);
token tokenizer_get_next (tokenizer *tk);
void token_print(token tok);
void token_println(token tok);

enum {TOKEN_NONE, TOKEN_SYM, TOKEN_NUM, TOKEN_ERR, TOKEN_COL, TOKEN_COM, TOKEN_EOI, TOKEN_CMT};

struct token {
	int type;
	union {
		const char *sym;
		const char *err;
		const char *com;
		unsigned int num;
	};
	int row, col, len;
};

struct tokenizer {
	char *data;
	int row, col;
};