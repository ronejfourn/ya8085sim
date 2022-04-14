#include "tokenizer.h"
#include "memory.h"
#include <stdio.h>
#include <stdlib.h>

unsigned char memory[0x10000];
int  loadat;
extern hash_table *pitable;

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

	tokenizer tk;
	tk.data = buf;
	tk.row = 0;
	tk.col = 0;

	instruction_table_init();
	partial_instruction_table_init();

	while (*tk.data) {
		token r = tokenizer_get_next(&tk);
		token_println(r);
		if (r.type == TOKEN_ERR)
			break;
		int64_t c = (int64_t)hash_table_get(pitable, (lenstring){r.sym, r.len});
		if (c != -1) {
			((loader)c)((lenstring){r.sym, r.len}, &tk);
		} else {
			const char *msg = load_instruction_opcode((lenstring){r.sym, r.len}, &tk);
			if (msg) {
				printf("%s\n", msg);
				break;
			}
		}
	}

	return 0;
}
