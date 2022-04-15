typedef struct tokenizer tokenizer;
typedef struct lenstring lenstring;
typedef struct hash_table hash_table;

typedef const char *(*loader) (lenstring n, tokenizer* tk);

const char *load_instruction(lenstring n, tokenizer *tk);

const char *load_instruction_mov(lenstring n, tokenizer *tk);
const char *load_instruction_mvi(lenstring n, tokenizer *tk);
const char *load_instruction_lxi(lenstring n, tokenizer *tk);
const char *load_instruction_ldax_stax(lenstring n, tokenizer *tk);
const char *load_instruction_push_pop(lenstring n, tokenizer *tk);
const char *load_instruction_arith_reg(lenstring n, tokenizer *tk);
const char *load_instruction_arith_rp(lenstring n, tokenizer *tk);
const char *load_instruction_imm_w(lenstring n, tokenizer *tk);
const char *load_instruction_imm_dw(lenstring n, tokenizer *tk);
const char *load_instruction_rst(lenstring n, tokenizer *tk);