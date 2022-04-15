#pragma once
#include "lenstring.h"
#include <stdint.h>
typedef struct hash_table hash_table;

void instruction_table_init();
void partial_instruction_table_init();
void *instruction_table_get(lenstring key);
void *partial_instruction_table_get(lenstring key);
void *symbol_table_get(lenstring s);
int64_t hash(lenstring key);
void symbol_table_init();
void symbol_table_add(lenstring key, int64_t value);

struct hash_table{
	lenstring key;
	void *val;
	int64_t hash;
};
