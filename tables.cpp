#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "tables.h"
#include "fmtstr.h"
#include "memory.h"
#include "iset.h"

static void *hash_table_get(hash_table *table, lenstring key);
static void *hash_table_get_nocase(hash_table *table, lenstring key);

static hash_table *itable, *ptable, *stable;

void *instruction_table_get(lenstring key) {
	return hash_table_get_nocase(itable, key);
}
 void *partial_instruction_table_get(lenstring key) {
	return hash_table_get_nocase(ptable, key);
 }

int64_t hash(lenstring key) {
	int64_t hashVal = 0;
	for (int i = 0; i < key.len; i++) {
		hashVal = (hashVal << 4) + key.data[i];
		int64_t g = hashVal & 0xF0000000L;
		if (g != 0) hashVal ^= g >> 24;
		hashVal &= ~g;
	}
	return hashVal;
}

static int64_t hash_nocase(lenstring key) {
	int64_t hashVal = 0;
	for (int i = 0; i < key.len; i++) {
		hashVal = (hashVal << 4) + TO_UCASE(key.data[i]);
		int64_t g = hashVal & 0xF0000000L;
		if (g != 0) hashVal ^= g >> 24;
		hashVal &= ~g;
	}
	return hashVal;
}

static char hash_table_add(hash_table *table, lenstring key, void *value) {
	int64_t hv = hash(key);
	int *a = (int *)table - 1;
	int64_t in = hv % (*a);
	if (table[in].key.len == 0 || table[in].hash == hv) {
		table[in].key  = key;
		table[in].hash = hv;
		table[in].val  = value;
		return 1;
	} else {
		for (int i = (hv % (*a) + 1) % (*a); i != hv % (*a); i = (i + 1) % (*a)) {
			if (table[i].key.len == 0) {
				table[i].key  = key;
				table[i].hash = hv;
				table[i].val  = value;
				return 1;
			}
		}
	}
	return 0;
}

static void *hash_table_get_nocase(hash_table *table, lenstring key) {
	int64_t hv = hash_nocase(key);
	int *a = (int *)table - 1;
	int index = hv % (*a);
	if (table[index].hash == hv) {
		return table[index].val;
	} else {
		for (int i = (hv % (*a) + 1) % (*a); i != hv % (*a); i = (i + 1) % (*a)) {
			if (table[i].hash == hv) {
				return table[i].val;
			}
		}
	}
	return (void *)-1;
}

static void *hash_table_get(hash_table *table, lenstring key) {
	int64_t hv = hash(key);
	int *a = (int *)table - 1;
	int index = hv % (*a);
	if (table[index].hash == hv) {
		return table[index].val;
	} else {
		for (int i = (hv % (*a) + 1) % (*a); i != hv % (*a); i = (i + 1) % (*a)) {
			if (table[i].hash == hv) {
				return table[i].val;
			}
		}
	}
	return (void *)-1;
}

static hash_table *hash_table_new (int size) {
	char *tmp = (char *)calloc(1, sizeof(hash_table) * size + sizeof(int));
	int *a = (int *)tmp;
	*a = size;
	return (hash_table *)(tmp + sizeof(int));
}

static void hash_table_end(hash_table *ht) {
	int *a = (int *)ht - 1;
	free(a);
}

void symbol_table_init() {
	stable = hash_table_new(8);
}

void symbol_table_end() {
	hash_table_end(stable);
	stable = nullptr;
}

char *symbol_table_add(lenstring key, int64_t value) {
	if (symbol_table_get(key) != (void *)-1)
		return fmtstr("redefinition of key");
	if (hash_table_add(stable, key, (void *)value))
		return NULL;
	int *a = (int *)stable - 1;
	hash_table *tmp = hash_table_new(*a * 2);
	for (int i = 0; i < *a; i ++)
		hash_table_add(tmp, stable[i].key, stable[i].val);
	free(a);
	stable = tmp;
	hash_table_add(stable, key, (void *)value);
	return NULL;
}

void *symbol_table_get(lenstring s) {
	return hash_table_get(stable, s);
}

void instruction_table_init() {
	itable = hash_table_new(244);
	hash_table_add(itable, ls_from_cstr("MOV A,A") , (void *)MOV_A_A);
	hash_table_add(itable, ls_from_cstr("MOV A,B") , (void *)MOV_A_B);
	hash_table_add(itable, ls_from_cstr("MOV A,C") , (void *)MOV_A_C);
	hash_table_add(itable, ls_from_cstr("MOV A,D") , (void *)MOV_A_D);
	hash_table_add(itable, ls_from_cstr("MOV A,E") , (void *)MOV_A_E);
	hash_table_add(itable, ls_from_cstr("MOV A,H") , (void *)MOV_A_H);
	hash_table_add(itable, ls_from_cstr("MOV A,L") , (void *)MOV_A_L);
	hash_table_add(itable, ls_from_cstr("MOV A,M") , (void *)MOV_A_M);
	hash_table_add(itable, ls_from_cstr("MOV B,A") , (void *)MOV_B_A);
	hash_table_add(itable, ls_from_cstr("MOV B,B") , (void *)MOV_B_B);
	hash_table_add(itable, ls_from_cstr("MOV B,C") , (void *)MOV_B_C);
	hash_table_add(itable, ls_from_cstr("MOV B,D") , (void *)MOV_B_D);
	hash_table_add(itable, ls_from_cstr("MOV B,E") , (void *)MOV_B_E);
	hash_table_add(itable, ls_from_cstr("MOV B,H") , (void *)MOV_B_H);
	hash_table_add(itable, ls_from_cstr("MOV B,L") , (void *)MOV_B_L);
	hash_table_add(itable, ls_from_cstr("MOV B,M") , (void *)MOV_B_M);
	hash_table_add(itable, ls_from_cstr("MOV C,A") , (void *)MOV_C_A);
	hash_table_add(itable, ls_from_cstr("MOV C,B") , (void *)MOV_C_B);
	hash_table_add(itable, ls_from_cstr("MOV C,C") , (void *)MOV_C_C);
	hash_table_add(itable, ls_from_cstr("MOV C,D") , (void *)MOV_C_D);
	hash_table_add(itable, ls_from_cstr("MOV C,E") , (void *)MOV_C_E);
	hash_table_add(itable, ls_from_cstr("MOV C,H") , (void *)MOV_C_H);
	hash_table_add(itable, ls_from_cstr("MOV C,L") , (void *)MOV_C_L);
	hash_table_add(itable, ls_from_cstr("MOV C,M") , (void *)MOV_C_M);
	hash_table_add(itable, ls_from_cstr("MOV D,A") , (void *)MOV_D_A);
	hash_table_add(itable, ls_from_cstr("MOV D,B") , (void *)MOV_D_B);
	hash_table_add(itable, ls_from_cstr("MOV D,C") , (void *)MOV_D_C);
	hash_table_add(itable, ls_from_cstr("MOV D,D") , (void *)MOV_D_D);
	hash_table_add(itable, ls_from_cstr("MOV D,E") , (void *)MOV_D_E);
	hash_table_add(itable, ls_from_cstr("MOV D,H") , (void *)MOV_D_H);
	hash_table_add(itable, ls_from_cstr("MOV D,L") , (void *)MOV_D_L);
	hash_table_add(itable, ls_from_cstr("MOV D,M") , (void *)MOV_D_M);
	hash_table_add(itable, ls_from_cstr("MOV E,A") , (void *)MOV_E_A);
	hash_table_add(itable, ls_from_cstr("MOV E,B") , (void *)MOV_E_B);
	hash_table_add(itable, ls_from_cstr("MOV E,C") , (void *)MOV_E_C);
	hash_table_add(itable, ls_from_cstr("MOV E,D") , (void *)MOV_E_D);
	hash_table_add(itable, ls_from_cstr("MOV E,E") , (void *)MOV_E_E);
	hash_table_add(itable, ls_from_cstr("MOV E,H") , (void *)MOV_E_H);
	hash_table_add(itable, ls_from_cstr("MOV E,L") , (void *)MOV_E_L);
	hash_table_add(itable, ls_from_cstr("MOV E,M") , (void *)MOV_E_M);
	hash_table_add(itable, ls_from_cstr("MOV H,A") , (void *)MOV_H_A);
	hash_table_add(itable, ls_from_cstr("MOV H,B") , (void *)MOV_H_B);
	hash_table_add(itable, ls_from_cstr("MOV H,C") , (void *)MOV_H_C);
	hash_table_add(itable, ls_from_cstr("MOV H,D") , (void *)MOV_H_D);
	hash_table_add(itable, ls_from_cstr("MOV H,E") , (void *)MOV_H_E);
	hash_table_add(itable, ls_from_cstr("MOV H,H") , (void *)MOV_H_H);
	hash_table_add(itable, ls_from_cstr("MOV H,L") , (void *)MOV_H_L);
	hash_table_add(itable, ls_from_cstr("MOV H,M") , (void *)MOV_H_M);
	hash_table_add(itable, ls_from_cstr("MOV L,A") , (void *)MOV_L_A);
	hash_table_add(itable, ls_from_cstr("MOV L,B") , (void *)MOV_L_B);
	hash_table_add(itable, ls_from_cstr("MOV L,C") , (void *)MOV_L_C);
	hash_table_add(itable, ls_from_cstr("MOV L,D") , (void *)MOV_L_D);
	hash_table_add(itable, ls_from_cstr("MOV L,E") , (void *)MOV_L_E);
	hash_table_add(itable, ls_from_cstr("MOV L,H") , (void *)MOV_L_H);
	hash_table_add(itable, ls_from_cstr("MOV L,L") , (void *)MOV_L_L);
	hash_table_add(itable, ls_from_cstr("MOV L,M") , (void *)MOV_L_M);
	hash_table_add(itable, ls_from_cstr("MOV M,A") , (void *)MOV_M_A);
	hash_table_add(itable, ls_from_cstr("MOV M,B") , (void *)MOV_M_B);
	hash_table_add(itable, ls_from_cstr("MOV M,C") , (void *)MOV_M_C);
	hash_table_add(itable, ls_from_cstr("MOV M,D") , (void *)MOV_M_D);
	hash_table_add(itable, ls_from_cstr("MOV M,E") , (void *)MOV_M_E);
	hash_table_add(itable, ls_from_cstr("MOV M,H") , (void *)MOV_M_H);
	hash_table_add(itable, ls_from_cstr("MOV M,L") , (void *)MOV_M_L);
	hash_table_add(itable, ls_from_cstr("MVI A")   , (void *)MVI_A);
	hash_table_add(itable, ls_from_cstr("MVI B")   , (void *)MVI_B);
	hash_table_add(itable, ls_from_cstr("MVI C")   , (void *)MVI_C);
	hash_table_add(itable, ls_from_cstr("MVI D")   , (void *)MVI_D);
	hash_table_add(itable, ls_from_cstr("MVI E")   , (void *)MVI_E);
	hash_table_add(itable, ls_from_cstr("MVI H")   , (void *)MVI_H);
	hash_table_add(itable, ls_from_cstr("MVI L")   , (void *)MVI_L);
	hash_table_add(itable, ls_from_cstr("MVI M")   , (void *)MVI_M);
	hash_table_add(itable, ls_from_cstr("XCHG" )   , (void *)XCHG);
	hash_table_add(itable, ls_from_cstr("LXI B" )  , (void *)LXI_B );
	hash_table_add(itable, ls_from_cstr("LXI D" )  , (void *)LXI_D );
	hash_table_add(itable, ls_from_cstr("LXI H" )  , (void *)LXI_H );
	hash_table_add(itable, ls_from_cstr("LXI SP")  , (void *)LXI_SP);
	hash_table_add(itable, ls_from_cstr("LDAX B")  , (void *)LDAX_B);
	hash_table_add(itable, ls_from_cstr("LDAX D")  , (void *)LDAX_D);
	hash_table_add(itable, ls_from_cstr("LHLD"  )  , (void *)LHLD);
	hash_table_add(itable, ls_from_cstr("LDA"   )  , (void *)LDA);
	hash_table_add(itable, ls_from_cstr("STAX B")  , (void *)STAX_B);
	hash_table_add(itable, ls_from_cstr("STAX D")  , (void *)STAX_D);
	hash_table_add(itable, ls_from_cstr("SHLD"  )  , (void *)SHLD);
	hash_table_add(itable, ls_from_cstr("STA"   )  , (void *)STA);
	hash_table_add(itable, ls_from_cstr("ADD A")   , (void *)ADD_A);
	hash_table_add(itable, ls_from_cstr("ADD B")   , (void *)ADD_B);
	hash_table_add(itable, ls_from_cstr("ADD C")   , (void *)ADD_C);
	hash_table_add(itable, ls_from_cstr("ADD D")   , (void *)ADD_D);
	hash_table_add(itable, ls_from_cstr("ADD E")   , (void *)ADD_E);
	hash_table_add(itable, ls_from_cstr("ADD H")   , (void *)ADD_H);
	hash_table_add(itable, ls_from_cstr("ADD L")   , (void *)ADD_L);
	hash_table_add(itable, ls_from_cstr("ADD M")   , (void *)ADD_M);
	hash_table_add(itable, ls_from_cstr("ADC A")   , (void *)ADC_A);
	hash_table_add(itable, ls_from_cstr("ADC B")   , (void *)ADC_B);
	hash_table_add(itable, ls_from_cstr("ADC C")   , (void *)ADC_C);
	hash_table_add(itable, ls_from_cstr("ADC D")   , (void *)ADC_D);
	hash_table_add(itable, ls_from_cstr("ADC E")   , (void *)ADC_E);
	hash_table_add(itable, ls_from_cstr("ADC H")   , (void *)ADC_H);
	hash_table_add(itable, ls_from_cstr("ADC L")   , (void *)ADC_L);
	hash_table_add(itable, ls_from_cstr("ADC M")   , (void *)ADC_M);
	hash_table_add(itable, ls_from_cstr("SUB A")   , (void *)SUB_A);
	hash_table_add(itable, ls_from_cstr("SUB B")   , (void *)SUB_B);
	hash_table_add(itable, ls_from_cstr("SUB C")   , (void *)SUB_C);
	hash_table_add(itable, ls_from_cstr("SUB D")   , (void *)SUB_D);
	hash_table_add(itable, ls_from_cstr("SUB E")   , (void *)SUB_E);
	hash_table_add(itable, ls_from_cstr("SUB H")   , (void *)SUB_H);
	hash_table_add(itable, ls_from_cstr("SUB L")   , (void *)SUB_L);
	hash_table_add(itable, ls_from_cstr("SUB M")   , (void *)SUB_M);
	hash_table_add(itable, ls_from_cstr("SBB A")   , (void *)SBB_A);
	hash_table_add(itable, ls_from_cstr("SBB B")   , (void *)SBB_B);
	hash_table_add(itable, ls_from_cstr("SBB C")   , (void *)SBB_C);
	hash_table_add(itable, ls_from_cstr("SBB D")   , (void *)SBB_D);
	hash_table_add(itable, ls_from_cstr("SBB E")   , (void *)SBB_E);
	hash_table_add(itable, ls_from_cstr("SBB H")   , (void *)SBB_H);
	hash_table_add(itable, ls_from_cstr("SBB L")   , (void *)SBB_L);
	hash_table_add(itable, ls_from_cstr("SBB M")   , (void *)SBB_M);
	hash_table_add(itable, ls_from_cstr("INR A")   , (void *)INR_A);
	hash_table_add(itable, ls_from_cstr("INR B")   , (void *)INR_B);
	hash_table_add(itable, ls_from_cstr("INR C")   , (void *)INR_C);
	hash_table_add(itable, ls_from_cstr("INR D")   , (void *)INR_D);
	hash_table_add(itable, ls_from_cstr("INR E")   , (void *)INR_E);
	hash_table_add(itable, ls_from_cstr("INR H")   , (void *)INR_H);
	hash_table_add(itable, ls_from_cstr("INR L")   , (void *)INR_L);
	hash_table_add(itable, ls_from_cstr("INR M")   , (void *)INR_M);
	hash_table_add(itable, ls_from_cstr("DCR A")   , (void *)DCR_A);
	hash_table_add(itable, ls_from_cstr("DCR B")   , (void *)DCR_B);
	hash_table_add(itable, ls_from_cstr("DCR C")   , (void *)DCR_C);
	hash_table_add(itable, ls_from_cstr("DCR D")   , (void *)DCR_D);
	hash_table_add(itable, ls_from_cstr("DCR E")   , (void *)DCR_E);
	hash_table_add(itable, ls_from_cstr("DCR H")   , (void *)DCR_H);
	hash_table_add(itable, ls_from_cstr("DCR L")   , (void *)DCR_L);
	hash_table_add(itable, ls_from_cstr("DCR M")   , (void *)DCR_M);
	hash_table_add(itable, ls_from_cstr("INX B" )  , (void *)INX_B);
	hash_table_add(itable, ls_from_cstr("INX D" )  , (void *)INX_D);
	hash_table_add(itable, ls_from_cstr("INX H" )  , (void *)INX_H);
	hash_table_add(itable, ls_from_cstr("INX SP")  , (void *)INX_SP);
	hash_table_add(itable, ls_from_cstr("DCX B" )  , (void *)DCX_B);
	hash_table_add(itable, ls_from_cstr("DCX D" )  , (void *)DCX_D);
	hash_table_add(itable, ls_from_cstr("DCX H" )  , (void *)DCX_H);
	hash_table_add(itable, ls_from_cstr("DCX SP")  , (void *)DCX_SP);
	hash_table_add(itable, ls_from_cstr("DAD B" )  , (void *)DAD_B);
	hash_table_add(itable, ls_from_cstr("DAD D" )  , (void *)DAD_D);
	hash_table_add(itable, ls_from_cstr("DAD H" )  , (void *)DAD_H);
	hash_table_add(itable, ls_from_cstr("DAD SP")  , (void *)DAD_SP);
	hash_table_add(itable, ls_from_cstr("DAA")     , (void *)DAA);
	hash_table_add(itable, ls_from_cstr("CMA")     , (void *)CMA);
	hash_table_add(itable, ls_from_cstr("STC")     , (void *)STC);
	hash_table_add(itable, ls_from_cstr("CMC")     , (void *)CMC);
	hash_table_add(itable, ls_from_cstr("RLC")     , (void *)RLC);
	hash_table_add(itable, ls_from_cstr("RRC")     , (void *)RRC);
	hash_table_add(itable, ls_from_cstr("RAL")     , (void *)RAL);
	hash_table_add(itable, ls_from_cstr("RAR")     , (void *)RAR);
	hash_table_add(itable, ls_from_cstr("ANA A")   , (void *)ANA_A);
	hash_table_add(itable, ls_from_cstr("ANA B")   , (void *)ANA_B);
	hash_table_add(itable, ls_from_cstr("ANA C")   , (void *)ANA_C);
	hash_table_add(itable, ls_from_cstr("ANA D")   , (void *)ANA_D);
	hash_table_add(itable, ls_from_cstr("ANA E")   , (void *)ANA_E);
	hash_table_add(itable, ls_from_cstr("ANA H")   , (void *)ANA_H);
	hash_table_add(itable, ls_from_cstr("ANA L")   , (void *)ANA_L);
	hash_table_add(itable, ls_from_cstr("ANA M")   , (void *)ANA_M);
	hash_table_add(itable, ls_from_cstr("XRA A")   , (void *)XRA_A);
	hash_table_add(itable, ls_from_cstr("XRA B")   , (void *)XRA_B);
	hash_table_add(itable, ls_from_cstr("XRA C")   , (void *)XRA_C);
	hash_table_add(itable, ls_from_cstr("XRA D")   , (void *)XRA_D);
	hash_table_add(itable, ls_from_cstr("XRA E")   , (void *)XRA_E);
	hash_table_add(itable, ls_from_cstr("XRA H")   , (void *)XRA_H);
	hash_table_add(itable, ls_from_cstr("XRA L")   , (void *)XRA_L);
	hash_table_add(itable, ls_from_cstr("XRA M")   , (void *)XRA_M);
	hash_table_add(itable, ls_from_cstr("ORA A")   , (void *)ORA_A);
	hash_table_add(itable, ls_from_cstr("ORA B")   , (void *)ORA_B);
	hash_table_add(itable, ls_from_cstr("ORA C")   , (void *)ORA_C);
	hash_table_add(itable, ls_from_cstr("ORA D")   , (void *)ORA_D);
	hash_table_add(itable, ls_from_cstr("ORA E")   , (void *)ORA_E);
	hash_table_add(itable, ls_from_cstr("ORA H")   , (void *)ORA_H);
	hash_table_add(itable, ls_from_cstr("ORA L")   , (void *)ORA_L);
	hash_table_add(itable, ls_from_cstr("ORA M")   , (void *)ORA_M);
	hash_table_add(itable, ls_from_cstr("CMP A")   , (void *)CMP_A);
	hash_table_add(itable, ls_from_cstr("CMP B")   , (void *)CMP_B);
	hash_table_add(itable, ls_from_cstr("CMP C")   , (void *)CMP_C);
	hash_table_add(itable, ls_from_cstr("CMP D")   , (void *)CMP_D);
	hash_table_add(itable, ls_from_cstr("CMP E")   , (void *)CMP_E);
	hash_table_add(itable, ls_from_cstr("CMP H")   , (void *)CMP_H);
	hash_table_add(itable, ls_from_cstr("CMP L")   , (void *)CMP_L);
	hash_table_add(itable, ls_from_cstr("CMP M")   , (void *)CMP_M);
	hash_table_add(itable, ls_from_cstr("ADI")     , (void *)ADI);
	hash_table_add(itable, ls_from_cstr("ACI")     , (void *)ACI);
	hash_table_add(itable, ls_from_cstr("SUI")     , (void *)SUI);
	hash_table_add(itable, ls_from_cstr("SBI")     , (void *)SBI);
	hash_table_add(itable, ls_from_cstr("ANI")     , (void *)ANI);
	hash_table_add(itable, ls_from_cstr("XRI")     , (void *)XRI);
	hash_table_add(itable, ls_from_cstr("ORI")     , (void *)ORI);
	hash_table_add(itable, ls_from_cstr("CPI")     , (void *)CPI);
	hash_table_add(itable, ls_from_cstr("JMP")     , (void *)JMP);
	hash_table_add(itable, ls_from_cstr("JNZ")     , (void *)JNZ);
	hash_table_add(itable, ls_from_cstr("JZ" )     , (void *)JZ);
	hash_table_add(itable, ls_from_cstr("JNC")     , (void *)JNC);
	hash_table_add(itable, ls_from_cstr("JC" )     , (void *)JC);
	hash_table_add(itable, ls_from_cstr("JPO")     , (void *)JPO);
	hash_table_add(itable, ls_from_cstr("JPE")     , (void *)JPE);
	hash_table_add(itable, ls_from_cstr("JP" )     , (void *)JP);
	hash_table_add(itable, ls_from_cstr("JM" )     , (void *)JM);
	hash_table_add(itable, ls_from_cstr("CALL")    , (void *)CALL);
	hash_table_add(itable, ls_from_cstr("CNZ" )    , (void *)CNZ);
	hash_table_add(itable, ls_from_cstr("CZ"  )    , (void *)CZ);
	hash_table_add(itable, ls_from_cstr("CNC" )    , (void *)CNC);
	hash_table_add(itable, ls_from_cstr("CC"  )    , (void *)CC);
	hash_table_add(itable, ls_from_cstr("CPO" )    , (void *)CPO);
	hash_table_add(itable, ls_from_cstr("CPE" )    , (void *)CPE);
	hash_table_add(itable, ls_from_cstr("CP"  )    , (void *)CP);
	hash_table_add(itable, ls_from_cstr("CM"  )    , (void *)CM);
	hash_table_add(itable, ls_from_cstr("RET")     , (void *)RET);
	hash_table_add(itable, ls_from_cstr("RNZ")     , (void *)RNZ);
	hash_table_add(itable, ls_from_cstr("RZ" )     , (void *)RZ);
	hash_table_add(itable, ls_from_cstr("RNC")     , (void *)RNC);
	hash_table_add(itable, ls_from_cstr("RC" )     , (void *)RC);
	hash_table_add(itable, ls_from_cstr("RPO")     , (void *)RPO);
	hash_table_add(itable, ls_from_cstr("RPE")     , (void *)RPE);
	hash_table_add(itable, ls_from_cstr("RP" )     , (void *)RP);
	hash_table_add(itable, ls_from_cstr("RM" )     , (void *)RM);
	hash_table_add(itable, ls_from_cstr("PCHL")    , (void *)PCHL);
	hash_table_add(itable, ls_from_cstr("PUSH B")  , (void *)PUSH_B);
	hash_table_add(itable, ls_from_cstr("PUSH D")  , (void *)PUSH_D);
	hash_table_add(itable, ls_from_cstr("PUSH H")  , (void *)PUSH_H);
	hash_table_add(itable, ls_from_cstr("PUSH PSW"), (void *)PUSH_PSW);
	hash_table_add(itable, ls_from_cstr("POP B")   , (void *)POP_B);
	hash_table_add(itable, ls_from_cstr("POP D")   , (void *)POP_D);
	hash_table_add(itable, ls_from_cstr("POP H")   , (void *)POP_H);
	hash_table_add(itable, ls_from_cstr("POP PSW") , (void *)POP_PSW);
	hash_table_add(itable, ls_from_cstr("XTHL")    , (void *)XTHL);
	hash_table_add(itable, ls_from_cstr("SPHL")    , (void *)SPHL);
	hash_table_add(itable, ls_from_cstr("IN" )     , (void *)IN);
	hash_table_add(itable, ls_from_cstr("OUT")     , (void *)OUT);
	hash_table_add(itable, ls_from_cstr("DI" )     , (void *)DI);
	hash_table_add(itable, ls_from_cstr("EI" )     , (void *)EI);
	hash_table_add(itable, ls_from_cstr("NOP")     , (void *)NOP);
	hash_table_add(itable, ls_from_cstr("HLT")     , (void *)HLT);
	hash_table_add(itable, ls_from_cstr("RST 0")   , (void *)RST_0);
	hash_table_add(itable, ls_from_cstr("RST 1")   , (void *)RST_1);
	hash_table_add(itable, ls_from_cstr("RST 2")   , (void *)RST_2);
	hash_table_add(itable, ls_from_cstr("RST 3")   , (void *)RST_3);
	hash_table_add(itable, ls_from_cstr("RST 4")   , (void *)RST_4);
	hash_table_add(itable, ls_from_cstr("RST 5")   , (void *)RST_5);
	hash_table_add(itable, ls_from_cstr("RST 6")   , (void *)RST_6);
	hash_table_add(itable, ls_from_cstr("RST 7")   , (void *)RST_7);
}

void partial_instruction_table_init() {
	ptable = hash_table_new(62);
	hash_table_add(ptable, ls_from_cstr("MOV"), (void *) load_mov);
	hash_table_add(ptable, ls_from_cstr("MVI"), (void *) load_mvi);
	hash_table_add(ptable, ls_from_cstr("LXI"), (void *) load_lxi);
	hash_table_add(ptable, ls_from_cstr("LDAX"), (void *) load_ldax_stax);
	hash_table_add(ptable, ls_from_cstr("STAX"), (void *) load_ldax_stax);
	hash_table_add(ptable, ls_from_cstr("PUSH"), (void *) load_push_pop);
	hash_table_add(ptable, ls_from_cstr("POP" ), (void *) load_push_pop);
	hash_table_add(ptable, ls_from_cstr("ADD"), (void *) load_arith_reg);
	hash_table_add(ptable, ls_from_cstr("ADC"), (void *) load_arith_reg);
	hash_table_add(ptable, ls_from_cstr("SUB"), (void *) load_arith_reg);
	hash_table_add(ptable, ls_from_cstr("SBB"), (void *) load_arith_reg);
	hash_table_add(ptable, ls_from_cstr("INR"), (void *) load_arith_reg);
	hash_table_add(ptable, ls_from_cstr("DCR"), (void *) load_arith_reg);
	hash_table_add(ptable, ls_from_cstr("ANA"), (void *) load_arith_reg);
	hash_table_add(ptable, ls_from_cstr("XRA"), (void *) load_arith_reg);
	hash_table_add(ptable, ls_from_cstr("ORA"), (void *) load_arith_reg);
	hash_table_add(ptable, ls_from_cstr("CMP"), (void *) load_arith_reg);
	hash_table_add(ptable, ls_from_cstr("DAD"), (void *) load_arith_rp);
	hash_table_add(ptable, ls_from_cstr("INX"), (void *) load_arith_rp);
	hash_table_add(ptable, ls_from_cstr("DCX"), (void *) load_arith_rp);
	hash_table_add(ptable, ls_from_cstr("ADI"), (void *) load_imm_w);
	hash_table_add(ptable, ls_from_cstr("ACI"), (void *) load_imm_w);
	hash_table_add(ptable, ls_from_cstr("SUI"), (void *) load_imm_w);
	hash_table_add(ptable, ls_from_cstr("SBI"), (void *) load_imm_w);
	hash_table_add(ptable, ls_from_cstr("ANI"), (void *) load_imm_w);
	hash_table_add(ptable, ls_from_cstr("XRI"), (void *) load_imm_w);
	hash_table_add(ptable, ls_from_cstr("ORI"), (void *) load_imm_w);
	hash_table_add(ptable, ls_from_cstr("CPI"), (void *) load_imm_w);
	hash_table_add(ptable, ls_from_cstr("IN" ), (void *) load_imm_w);
	hash_table_add(ptable, ls_from_cstr("OUT"), (void *) load_imm_w);
	hash_table_add(ptable, ls_from_cstr("LHLD"), (void *) load_imm_dw);
	hash_table_add(ptable, ls_from_cstr("SHLD"), (void *) load_imm_dw);
	hash_table_add(ptable, ls_from_cstr("LDA" ), (void *) load_imm_dw);
	hash_table_add(ptable, ls_from_cstr("STA" ), (void *) load_imm_dw);
	hash_table_add(ptable, ls_from_cstr("JMP" ), (void *) load_imm_dw);
	hash_table_add(ptable, ls_from_cstr("JNZ" ), (void *) load_imm_dw);
	hash_table_add(ptable, ls_from_cstr("JZ"  ), (void *) load_imm_dw);
	hash_table_add(ptable, ls_from_cstr("JNC" ), (void *) load_imm_dw);
	hash_table_add(ptable, ls_from_cstr("JC"  ), (void *) load_imm_dw);
	hash_table_add(ptable, ls_from_cstr("JPO" ), (void *) load_imm_dw);
	hash_table_add(ptable, ls_from_cstr("JPE" ), (void *) load_imm_dw);
	hash_table_add(ptable, ls_from_cstr("JP"  ), (void *) load_imm_dw);
	hash_table_add(ptable, ls_from_cstr("JM"  ), (void *) load_imm_dw);
	hash_table_add(ptable, ls_from_cstr("CALL"), (void *) load_imm_dw);
	hash_table_add(ptable, ls_from_cstr("CNZ" ), (void *) load_imm_dw);
	hash_table_add(ptable, ls_from_cstr("CZ"  ), (void *) load_imm_dw);
	hash_table_add(ptable, ls_from_cstr("CNC" ), (void *) load_imm_dw);
	hash_table_add(ptable, ls_from_cstr("CC"  ), (void *) load_imm_dw);
	hash_table_add(ptable, ls_from_cstr("CPO" ), (void *) load_imm_dw);
	hash_table_add(ptable, ls_from_cstr("CPE" ), (void *) load_imm_dw);
	hash_table_add(ptable, ls_from_cstr("CP"  ), (void *) load_imm_dw);
	hash_table_add(ptable, ls_from_cstr("CM"  ), (void *) load_imm_dw);
	hash_table_add(ptable, ls_from_cstr("RET" ), (void *) load_imm_dw);
	hash_table_add(ptable, ls_from_cstr("RNZ" ), (void *) load_imm_dw);
	hash_table_add(ptable, ls_from_cstr("RZ"  ), (void *) load_imm_dw);
	hash_table_add(ptable, ls_from_cstr("RNC" ), (void *) load_imm_dw);
	hash_table_add(ptable, ls_from_cstr("RC"  ), (void *) load_imm_dw);
	hash_table_add(ptable, ls_from_cstr("RPO" ), (void *) load_imm_dw);
	hash_table_add(ptable, ls_from_cstr("RPE" ), (void *) load_imm_dw);
	hash_table_add(ptable, ls_from_cstr("RP"  ), (void *) load_imm_dw);
	hash_table_add(ptable, ls_from_cstr("RM"  ), (void *) load_imm_dw);
	hash_table_add(ptable, ls_from_cstr("RST"), (void *) load_rst);
}
