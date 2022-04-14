typedef struct tokenizer tokenizer;
typedef struct lenstring lenstring;
typedef struct hash_table hash_table;

typedef const char *(*loader) (lenstring n, tokenizer* tk);

const char *load_mov(lenstring n, tokenizer *tk);
const char *load_mvi(lenstring n, tokenizer *tk);
const char *load_lxi(lenstring n, tokenizer *tk);
const char *load_ldax_stax(lenstring n, tokenizer *tk);
const char *load_push_pop (lenstring n, tokenizer *tk);
const char *load_arith_reg(lenstring n, tokenizer *tk);
const char *load_arith_rp (lenstring n, tokenizer *tk);
const char *load_imm_w (lenstring n, tokenizer *tk);
const char *load_imm_dw(lenstring n, tokenizer *tk);
const char *load_rst   (lenstring n, tokenizer *tk);
const char *load_opcode(lenstring n, tokenizer *tk);

void instruction_table_init();
void partial_instruction_table_init();
void *hash_table_get(hash_table *table, lenstring key);
const char *load_instruction_opcode(lenstring n, tokenizer *tk);
