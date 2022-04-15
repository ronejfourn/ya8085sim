#include "tokenizer.h"
#include "memory.h"
#include "tables.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
	if (argc != 2) {
		fprintf(stdout, "Usage: ./bruhsm [filename]\n");
		return 0;
	}

	FILE *inf = fopen(argv[1], "rb");
	if (!inf) {
		fprintf(stderr, "Could not open file '%s'\n", argv[1]);
		return 1;
	}

	fseek(inf, 0, SEEK_END);
	int fs = ftell(inf);
	rewind(inf);

	char *buf = malloc(fs);
	fread(buf, 1, fs, inf);
	fclose(inf);

	instruction_table_init();
	partial_instruction_table_init();
	symbol_table_init();
	symbol_queue_init();

	tokenizer tk;
	tk.data = buf;
	tk.row = 0;
	tk.col = 0;
	char *msg;
	char first_pass_successful = 1;
	while (*tk.data) {
		token r = tokenizer_get_next(&tk);

		if (r.type == TOKEN_ERR) {
			first_pass_successful = 0;
			printf("%s\n", r.err);
			break;
		}

		if (r.type != TOKEN_SYM) {
			first_pass_successful = 0;
			printf("at [%i:%i] expected %s, got %s\n",
					r.row, r.col, token_name(TOKEN_SYM), token_name(r.type));
			break;
		}

		token a = tokenizer_peek_next(&tk);
		if (a.type == TOKEN_COL) {
			symbol_table_add((lenstring){r.sym, r.len}, get_loadat());
			tokenizer_get_next(&tk);
		} else {
			msg = load_instruction((lenstring){r.sym, r.len}, &tk);
			if (msg) {
				first_pass_successful = 0;
				printf("%s\n", msg);
				free(msg);
				break;
			}
		}
	}

	if (first_pass_successful) {
		msg = second_pass();
		if (msg) {
			printf("%s\n", msg);
			free(msg);
		}
	}

	// TODO: cleanup memory after assembling


	return 0;
}
