#include "tokenizer.h"
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

	tokenizer tk;
	tk.data = buf;
	tk.row = 0;
	tk.col = 0;

	while (*tk.data) {
		token r = tokenizer_get_next(&tk);
		token_print(r);
		if (r.type == TOKEN_ERR)
			break;
	}

	return 0;
}
