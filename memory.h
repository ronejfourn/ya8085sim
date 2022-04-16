#pragma once
#include <stdint.h>
typedef struct token token;
typedef struct lenstring lenstring;
typedef struct hash_table hash_table;

typedef char *(*loader) (lenstring, token*, int*);

int get_loadat();
int get_load_pos();
void set_loadat(uint16_t v);
uint8_t *get_mem();
void symbol_queue_init();
char *symbol_resolve();
char *load_instruction(lenstring n, token *token_table, int *token_index);

char *load_opcode(lenstring n, token *tk, int *ti);
char *load_mov(lenstring n, token *token_table, int *token_index);
char *load_mvi(lenstring n, token *token_table, int *token_index);
char *load_lxi(lenstring n, token *token_table, int *token_index);
char *load_ldax_stax(lenstring n, token *token_table, int *token_index);
char *load_push_pop(lenstring n, token *token_table, int *token_index);
char *load_arith_reg(lenstring n, token *token_table, int *token_index);
char *load_arith_rp(lenstring n, token *token_table, int *token_index);
char *load_imm_w(lenstring n, token *token_table, int *token_index);
char *load_imm_dw(lenstring n, token *token_table, int *token_index);
char *load_rst(lenstring n, token *token_table, int *token_index);
