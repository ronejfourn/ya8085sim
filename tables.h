#pragma once
#include "lenstring.h"
typedef struct hash_table hash_table;

void instruction_table_init();
void partial_instruction_table_init();
void *instruction_table_get(lenstring key);
void *partial_instruction_table_get(lenstring key);

struct hash_table{
	lenstring key;
	void *val;
	long hash;
};
