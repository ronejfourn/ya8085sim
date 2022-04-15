#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "tables.h"
#include "memory.h"

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
	if (table[hv % (*a)].key.len == 0) {
		table[hv % (*a)].key  = key;
		table[hv % (*a)].hash = hv;
		table[hv % (*a)].val  = value;
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
	void *tmp = calloc(1, sizeof(hash_table) * size + sizeof(int));
	int * a = tmp;
	*a = size;
	return tmp + sizeof(int);
}

void symbol_table_init() {
	stable = hash_table_new(8);
}

void symbol_table_add(lenstring key, int64_t value) {
	if (hash_table_add(stable, key, (void *)value)) return;

	int *a = (int *)stable - 1;
	hash_table *tmp = hash_table_new(*a * 2);
	for (int i = 0; i < *a; i ++)
		hash_table_add(tmp, stable[i].key, stable[i].val);
	free(a);
	stable = tmp;
	hash_table_add(stable, key, (void *)value);
}

void *symbol_table_get(lenstring s) {
	return hash_table_get(stable, s);
}

void instruction_table_init() {
	itable = hash_table_new(251);
	hash_table_add(itable, ls_from_cstr("MOV A,A") , (void *)0x7F);
	hash_table_add(itable, ls_from_cstr("MOV A,B") , (void *)0x78);
	hash_table_add(itable, ls_from_cstr("MOV A,C") , (void *)0x79);
	hash_table_add(itable, ls_from_cstr("MOV A,D") , (void *)0x7A);
	hash_table_add(itable, ls_from_cstr("MOV A,E") , (void *)0x7B);
	hash_table_add(itable, ls_from_cstr("MOV A,H") , (void *)0x7C);
	hash_table_add(itable, ls_from_cstr("MOV A,L") , (void *)0x7D);
	hash_table_add(itable, ls_from_cstr("MOV A,M") , (void *)0x7E);
	hash_table_add(itable, ls_from_cstr("MOV B,A") , (void *)0x47);
	hash_table_add(itable, ls_from_cstr("MOV B,B") , (void *)0x40);
	hash_table_add(itable, ls_from_cstr("MOV B,C") , (void *)0x41);
	hash_table_add(itable, ls_from_cstr("MOV B,D") , (void *)0x42);
	hash_table_add(itable, ls_from_cstr("MOV B,E") , (void *)0x43);
	hash_table_add(itable, ls_from_cstr("MOV B,H") , (void *)0x44);
	hash_table_add(itable, ls_from_cstr("MOV B,L") , (void *)0x45);
	hash_table_add(itable, ls_from_cstr("MOV B,M") , (void *)0x46);
	hash_table_add(itable, ls_from_cstr("MOV C,A") , (void *)0x4F);
	hash_table_add(itable, ls_from_cstr("MOV C,B") , (void *)0x48);
	hash_table_add(itable, ls_from_cstr("MOV C,C") , (void *)0x49);
	hash_table_add(itable, ls_from_cstr("MOV C,D") , (void *)0x4A);
	hash_table_add(itable, ls_from_cstr("MOV C,E") , (void *)0x4B);
	hash_table_add(itable, ls_from_cstr("MOV C,H") , (void *)0x4C);
	hash_table_add(itable, ls_from_cstr("MOV C,L") , (void *)0x4D);
	hash_table_add(itable, ls_from_cstr("MOV C,M") , (void *)0x4E);
	hash_table_add(itable, ls_from_cstr("MOV D,A") , (void *)0x57);
	hash_table_add(itable, ls_from_cstr("MOV D,B") , (void *)0x50);
	hash_table_add(itable, ls_from_cstr("MOV D,C") , (void *)0x51);
	hash_table_add(itable, ls_from_cstr("MOV D,D") , (void *)0x52);
	hash_table_add(itable, ls_from_cstr("MOV D,E") , (void *)0x53);
	hash_table_add(itable, ls_from_cstr("MOV D,H") , (void *)0x54);
	hash_table_add(itable, ls_from_cstr("MOV D,L") , (void *)0x55);
	hash_table_add(itable, ls_from_cstr("MOV D,M") , (void *)0x56);
	hash_table_add(itable, ls_from_cstr("MOV E,A") , (void *)0x5F);
	hash_table_add(itable, ls_from_cstr("MOV E,B") , (void *)0x58);
	hash_table_add(itable, ls_from_cstr("MOV E,C") , (void *)0x59);
	hash_table_add(itable, ls_from_cstr("MOV E,D") , (void *)0x5A);
	hash_table_add(itable, ls_from_cstr("MOV E,E") , (void *)0x5B);
	hash_table_add(itable, ls_from_cstr("MOV E,H") , (void *)0x5C);
	hash_table_add(itable, ls_from_cstr("MOV E,L") , (void *)0x5D);
	hash_table_add(itable, ls_from_cstr("MOV E,M") , (void *)0x5E);
	hash_table_add(itable, ls_from_cstr("MOV H,A") , (void *)0x67);
	hash_table_add(itable, ls_from_cstr("MOV H,B") , (void *)0x60);
	hash_table_add(itable, ls_from_cstr("MOV H,C") , (void *)0x61);
	hash_table_add(itable, ls_from_cstr("MOV H,D") , (void *)0x62);
	hash_table_add(itable, ls_from_cstr("MOV H,E") , (void *)0x63);
	hash_table_add(itable, ls_from_cstr("MOV H,H") , (void *)0x64);
	hash_table_add(itable, ls_from_cstr("MOV H,L") , (void *)0x65);
	hash_table_add(itable, ls_from_cstr("MOV H,M") , (void *)0x66);
	hash_table_add(itable, ls_from_cstr("MOV L,A") , (void *)0x6F);
	hash_table_add(itable, ls_from_cstr("MOV L,B") , (void *)0x68);
	hash_table_add(itable, ls_from_cstr("MOV L,C") , (void *)0x69);
	hash_table_add(itable, ls_from_cstr("MOV L,D") , (void *)0x6A);
	hash_table_add(itable, ls_from_cstr("MOV L,E") , (void *)0x6B);
	hash_table_add(itable, ls_from_cstr("MOV L,H") , (void *)0x6C);
	hash_table_add(itable, ls_from_cstr("MOV L,L") , (void *)0x6D);
	hash_table_add(itable, ls_from_cstr("MOV L,M") , (void *)0x6E);
	hash_table_add(itable, ls_from_cstr("MOV M,A") , (void *)0x77);
	hash_table_add(itable, ls_from_cstr("MOV M,B") , (void *)0x70);
	hash_table_add(itable, ls_from_cstr("MOV M,C") , (void *)0x71);
	hash_table_add(itable, ls_from_cstr("MOV M,D") , (void *)0x72);
	hash_table_add(itable, ls_from_cstr("MOV M,E") , (void *)0x73);
	hash_table_add(itable, ls_from_cstr("MOV M,H") , (void *)0x74);
	hash_table_add(itable, ls_from_cstr("MOV M,L") , (void *)0x75);
	hash_table_add(itable, ls_from_cstr("MVI A")   , (void *)0x3E);
	hash_table_add(itable, ls_from_cstr("MVI B")   , (void *)0x06);
	hash_table_add(itable, ls_from_cstr("MVI C")   , (void *)0x0E);
	hash_table_add(itable, ls_from_cstr("MVI D")   , (void *)0x16);
	hash_table_add(itable, ls_from_cstr("MVI E")   , (void *)0x1E);
	hash_table_add(itable, ls_from_cstr("MVI H")   , (void *)0x26);
	hash_table_add(itable, ls_from_cstr("MVI L")   , (void *)0x2E);
	hash_table_add(itable, ls_from_cstr("MVI M")   , (void *)0x36);
	hash_table_add(itable, ls_from_cstr("XCHG" )   , (void *)0xEB);
	hash_table_add(itable, ls_from_cstr("LXI B" )  , (void *)0x01);
	hash_table_add(itable, ls_from_cstr("LXI D" )  , (void *)0x11);
	hash_table_add(itable, ls_from_cstr("LXI H" )  , (void *)0x21);
	hash_table_add(itable, ls_from_cstr("LXI SP")  , (void *)0x31);
	hash_table_add(itable, ls_from_cstr("LDAX B")  , (void *)0x0A);
	hash_table_add(itable, ls_from_cstr("LDAX D")  , (void *)0x1A);
	hash_table_add(itable, ls_from_cstr("LHLD"  )  , (void *)0x2A);
	hash_table_add(itable, ls_from_cstr("LDA"   )  , (void *)0x3A);
	hash_table_add(itable, ls_from_cstr("STAX B")  , (void *)0x02);
	hash_table_add(itable, ls_from_cstr("STAX D")  , (void *)0x12);
	hash_table_add(itable, ls_from_cstr("SHLD"  )  , (void *)0x22);
	hash_table_add(itable, ls_from_cstr("STA"   )  , (void *)0x32);
	hash_table_add(itable, ls_from_cstr("ADD A")   , (void *)0x87);
	hash_table_add(itable, ls_from_cstr("ADD B")   , (void *)0x80);
	hash_table_add(itable, ls_from_cstr("ADD C")   , (void *)0x81);
	hash_table_add(itable, ls_from_cstr("ADD D")   , (void *)0x82);
	hash_table_add(itable, ls_from_cstr("ADD E")   , (void *)0x83);
	hash_table_add(itable, ls_from_cstr("ADD H")   , (void *)0x84);
	hash_table_add(itable, ls_from_cstr("ADD L")   , (void *)0x85);
	hash_table_add(itable, ls_from_cstr("ADD M")   , (void *)0x86);
	hash_table_add(itable, ls_from_cstr("ADC A")   , (void *)0x8F);
	hash_table_add(itable, ls_from_cstr("ADC B")   , (void *)0x88);
	hash_table_add(itable, ls_from_cstr("ADC C")   , (void *)0x89);
	hash_table_add(itable, ls_from_cstr("ADC D")   , (void *)0x8A);
	hash_table_add(itable, ls_from_cstr("ADC E")   , (void *)0x8B);
	hash_table_add(itable, ls_from_cstr("ADC H")   , (void *)0x8C);
	hash_table_add(itable, ls_from_cstr("ADC L")   , (void *)0x8D);
	hash_table_add(itable, ls_from_cstr("ADC M")   , (void *)0x8E);
	hash_table_add(itable, ls_from_cstr("SUB A")   , (void *)0x97);
	hash_table_add(itable, ls_from_cstr("SUB B")   , (void *)0x90);
	hash_table_add(itable, ls_from_cstr("SUB C")   , (void *)0x91);
	hash_table_add(itable, ls_from_cstr("SUB D")   , (void *)0x92);
	hash_table_add(itable, ls_from_cstr("SUB E")   , (void *)0x93);
	hash_table_add(itable, ls_from_cstr("SUB H")   , (void *)0x94);
	hash_table_add(itable, ls_from_cstr("SUB L")   , (void *)0x95);
	hash_table_add(itable, ls_from_cstr("SUB M")   , (void *)0x96);
	hash_table_add(itable, ls_from_cstr("SBB A")   , (void *)0x9F);
	hash_table_add(itable, ls_from_cstr("SBB B")   , (void *)0x98);
	hash_table_add(itable, ls_from_cstr("SBB C")   , (void *)0x99);
	hash_table_add(itable, ls_from_cstr("SBB D")   , (void *)0x9A);
	hash_table_add(itable, ls_from_cstr("SBB E")   , (void *)0x9B);
	hash_table_add(itable, ls_from_cstr("SBB H")   , (void *)0x9C);
	hash_table_add(itable, ls_from_cstr("SBB L")   , (void *)0x9D);
	hash_table_add(itable, ls_from_cstr("SBB M")   , (void *)0x9E);
	hash_table_add(itable, ls_from_cstr("INR A")   , (void *)0x3C);
	hash_table_add(itable, ls_from_cstr("INR B")   , (void *)0x04);
	hash_table_add(itable, ls_from_cstr("INR C")   , (void *)0x0C);
	hash_table_add(itable, ls_from_cstr("INR D")   , (void *)0x14);
	hash_table_add(itable, ls_from_cstr("INR E")   , (void *)0x1C);
	hash_table_add(itable, ls_from_cstr("INR H")   , (void *)0x24);
	hash_table_add(itable, ls_from_cstr("INR L")   , (void *)0x2C);
	hash_table_add(itable, ls_from_cstr("INR M")   , (void *)0x34);
	hash_table_add(itable, ls_from_cstr("DCR A")   , (void *)0x3D);
	hash_table_add(itable, ls_from_cstr("DCR B")   , (void *)0x05);
	hash_table_add(itable, ls_from_cstr("DCR C")   , (void *)0x0D);
	hash_table_add(itable, ls_from_cstr("DCR D")   , (void *)0x15);
	hash_table_add(itable, ls_from_cstr("DCR E")   , (void *)0x1D);
	hash_table_add(itable, ls_from_cstr("DCR H")   , (void *)0x25);
	hash_table_add(itable, ls_from_cstr("DCR L")   , (void *)0x2D);
	hash_table_add(itable, ls_from_cstr("DCR M")   , (void *)0x35);
	hash_table_add(itable, ls_from_cstr("INX B" )  , (void *)0x03);
	hash_table_add(itable, ls_from_cstr("INX D" )  , (void *)0x13);
	hash_table_add(itable, ls_from_cstr("INX H" )  , (void *)0x23);
	hash_table_add(itable, ls_from_cstr("INX SP")  , (void *)0x33);
	hash_table_add(itable, ls_from_cstr("DCX B" )  , (void *)0x0B);
	hash_table_add(itable, ls_from_cstr("DCX D" )  , (void *)0x1B);
	hash_table_add(itable, ls_from_cstr("DCX H" )  , (void *)0x2B);
	hash_table_add(itable, ls_from_cstr("DCX SP")  , (void *)0x3B);
	hash_table_add(itable, ls_from_cstr("DAD B" )  , (void *)0x09);
	hash_table_add(itable, ls_from_cstr("DAD D" )  , (void *)0x19);
	hash_table_add(itable, ls_from_cstr("DAD H" )  , (void *)0x29);
	hash_table_add(itable, ls_from_cstr("DAD SP")  , (void *)0x39);
	hash_table_add(itable, ls_from_cstr("DAA")     , (void *)0x27);
	hash_table_add(itable, ls_from_cstr("CMA")     , (void *)0x2F);
	hash_table_add(itable, ls_from_cstr("STC")     , (void *)0x37);
	hash_table_add(itable, ls_from_cstr("CMC")     , (void *)0x3F);
	hash_table_add(itable, ls_from_cstr("RLC")     , (void *)0x07);
	hash_table_add(itable, ls_from_cstr("RRC")     , (void *)0x0F);
	hash_table_add(itable, ls_from_cstr("RAL")     , (void *)0x17);
	hash_table_add(itable, ls_from_cstr("RAR")     , (void *)0x1F);
	hash_table_add(itable, ls_from_cstr("ANA A")   , (void *)0xA7);
	hash_table_add(itable, ls_from_cstr("ANA B")   , (void *)0xA0);
	hash_table_add(itable, ls_from_cstr("ANA C")   , (void *)0xA1);
	hash_table_add(itable, ls_from_cstr("ANA D")   , (void *)0xA2);
	hash_table_add(itable, ls_from_cstr("ANA E")   , (void *)0xA3);
	hash_table_add(itable, ls_from_cstr("ANA H")   , (void *)0xA4);
	hash_table_add(itable, ls_from_cstr("ANA L")   , (void *)0xA5);
	hash_table_add(itable, ls_from_cstr("ANA M")   , (void *)0xA6);
	hash_table_add(itable, ls_from_cstr("XRA A")   , (void *)0xAF);
	hash_table_add(itable, ls_from_cstr("XRA B")   , (void *)0xA8);
	hash_table_add(itable, ls_from_cstr("XRA C")   , (void *)0xA9);
	hash_table_add(itable, ls_from_cstr("XRA D")   , (void *)0xAA);
	hash_table_add(itable, ls_from_cstr("XRA E")   , (void *)0xAB);
	hash_table_add(itable, ls_from_cstr("XRA H")   , (void *)0xAC);
	hash_table_add(itable, ls_from_cstr("XRA L")   , (void *)0xAD);
	hash_table_add(itable, ls_from_cstr("XRA M")   , (void *)0xAE);
	hash_table_add(itable, ls_from_cstr("ORA A")   , (void *)0xB7);
	hash_table_add(itable, ls_from_cstr("ORA B")   , (void *)0xB0);
	hash_table_add(itable, ls_from_cstr("ORA C")   , (void *)0xB1);
	hash_table_add(itable, ls_from_cstr("ORA D")   , (void *)0xB2);
	hash_table_add(itable, ls_from_cstr("ORA E")   , (void *)0xB3);
	hash_table_add(itable, ls_from_cstr("ORA H")   , (void *)0xB4);
	hash_table_add(itable, ls_from_cstr("ORA L")   , (void *)0xB5);
	hash_table_add(itable, ls_from_cstr("ORA M")   , (void *)0xB6);
	hash_table_add(itable, ls_from_cstr("CMP A")   , (void *)0xBF);
	hash_table_add(itable, ls_from_cstr("CMP B")   , (void *)0xB8);
	hash_table_add(itable, ls_from_cstr("CMP C")   , (void *)0xB9);
	hash_table_add(itable, ls_from_cstr("CMP D")   , (void *)0xBA);
	hash_table_add(itable, ls_from_cstr("CMP E")   , (void *)0xBB);
	hash_table_add(itable, ls_from_cstr("CMP H")   , (void *)0xBC);
	hash_table_add(itable, ls_from_cstr("CMP L")   , (void *)0xBD);
	hash_table_add(itable, ls_from_cstr("CMP M")   , (void *)0xBE);
	hash_table_add(itable, ls_from_cstr("ADI")     , (void *)0xC6);
	hash_table_add(itable, ls_from_cstr("ACI")     , (void *)0xCE);
	hash_table_add(itable, ls_from_cstr("SUI")     , (void *)0xD6);
	hash_table_add(itable, ls_from_cstr("SBI")     , (void *)0xDE);
	hash_table_add(itable, ls_from_cstr("ANI")     , (void *)0xE6);
	hash_table_add(itable, ls_from_cstr("XRI")     , (void *)0xEE);
	hash_table_add(itable, ls_from_cstr("ORI")     , (void *)0xF6);
	hash_table_add(itable, ls_from_cstr("CPI")     , (void *)0xFE);
	hash_table_add(itable, ls_from_cstr("JMP")     , (void *)0xC3);
	hash_table_add(itable, ls_from_cstr("JNZ")     , (void *)0xC2);
	hash_table_add(itable, ls_from_cstr("JZ" )     , (void *)0xCA);
	hash_table_add(itable, ls_from_cstr("JNC")     , (void *)0xD2);
	hash_table_add(itable, ls_from_cstr("JC" )     , (void *)0xDA);
	hash_table_add(itable, ls_from_cstr("JPO")     , (void *)0xE2);
	hash_table_add(itable, ls_from_cstr("JPE")     , (void *)0xEA);
	hash_table_add(itable, ls_from_cstr("JP" )     , (void *)0xF2);
	hash_table_add(itable, ls_from_cstr("JM" )     , (void *)0xFA);
	hash_table_add(itable, ls_from_cstr("CALL")    , (void *)0xCD);
	hash_table_add(itable, ls_from_cstr("CNZ" )    , (void *)0xC4);
	hash_table_add(itable, ls_from_cstr("CZ"  )    , (void *)0xCC);
	hash_table_add(itable, ls_from_cstr("CNC" )    , (void *)0xD4);
	hash_table_add(itable, ls_from_cstr("CC"  )    , (void *)0xDC);
	hash_table_add(itable, ls_from_cstr("CPO" )    , (void *)0xE4);
	hash_table_add(itable, ls_from_cstr("CPE" )    , (void *)0xEC);
	hash_table_add(itable, ls_from_cstr("CP"  )    , (void *)0xF4);
	hash_table_add(itable, ls_from_cstr("CM"  )    , (void *)0xFC);
	hash_table_add(itable, ls_from_cstr("RET")     , (void *)0xC9);
	hash_table_add(itable, ls_from_cstr("RNZ")     , (void *)0xC0);
	hash_table_add(itable, ls_from_cstr("RZ" )     , (void *)0xC8);
	hash_table_add(itable, ls_from_cstr("RNC")     , (void *)0xD0);
	hash_table_add(itable, ls_from_cstr("RC" )     , (void *)0xD8);
	hash_table_add(itable, ls_from_cstr("RPO")     , (void *)0xE0);
	hash_table_add(itable, ls_from_cstr("RPE")     , (void *)0xE8);
	hash_table_add(itable, ls_from_cstr("RP" )     , (void *)0xF0);
	hash_table_add(itable, ls_from_cstr("RM" )     , (void *)0xF8);
	hash_table_add(itable, ls_from_cstr("PCHL")    , (void *)0xE9);
	hash_table_add(itable, ls_from_cstr("PUSH B")  , (void *)0xC5);
	hash_table_add(itable, ls_from_cstr("PUSH D")  , (void *)0xE5);
	hash_table_add(itable, ls_from_cstr("PUSH H")  , (void *)0xD5);
	hash_table_add(itable, ls_from_cstr("PUSH PSW"), (void *)0xF5);
	hash_table_add(itable, ls_from_cstr("POP B")   , (void *)0xC1);
	hash_table_add(itable, ls_from_cstr("POP D")   , (void *)0xE1);
	hash_table_add(itable, ls_from_cstr("POP H")   , (void *)0xD1);
	hash_table_add(itable, ls_from_cstr("POP PSW") , (void *)0xF1);
	hash_table_add(itable, ls_from_cstr("XTHL")    , (void *)0xE3);
	hash_table_add(itable, ls_from_cstr("SPHL")    , (void *)0xF9);
	hash_table_add(itable, ls_from_cstr("IN" )     , (void *)0xD3);
	hash_table_add(itable, ls_from_cstr("OUT")     , (void *)0xDB);
	hash_table_add(itable, ls_from_cstr("DI" )     , (void *)0xF3);
	hash_table_add(itable, ls_from_cstr("EI" )     , (void *)0xFB);
	hash_table_add(itable, ls_from_cstr("NOP")     , (void *)0x00);
	hash_table_add(itable, ls_from_cstr("HLT")     , (void *)0x76);
	hash_table_add(itable, ls_from_cstr("RST 0")   , (void *)0xC7);
	hash_table_add(itable, ls_from_cstr("RST 1")   , (void *)0xCF);
	hash_table_add(itable, ls_from_cstr("RST 2")   , (void *)0xD7);
	hash_table_add(itable, ls_from_cstr("RST 3")   , (void *)0xDF);
	hash_table_add(itable, ls_from_cstr("RST 4")   , (void *)0xE7);
	hash_table_add(itable, ls_from_cstr("RST 5")   , (void *)0xEF);
	hash_table_add(itable, ls_from_cstr("RST 6")   , (void *)0xF7);
	hash_table_add(itable, ls_from_cstr("RST 7")   , (void *)0xFF);
}

void partial_instruction_table_init() {
	ptable = hash_table_new(67);
	hash_table_add(ptable, ls_from_cstr("MOV"), (void *) load_instruction_mov);
	hash_table_add(ptable, ls_from_cstr("MVI"), (void *) load_instruction_mvi);
	hash_table_add(ptable, ls_from_cstr("LXI"), (void *) load_instruction_lxi);
	hash_table_add(ptable, ls_from_cstr("LDAX"), (void *) load_instruction_ldax_stax);
	hash_table_add(ptable, ls_from_cstr("STAX"), (void *) load_instruction_ldax_stax);
	hash_table_add(ptable, ls_from_cstr("PUSH"), (void *) load_instruction_push_pop);
	hash_table_add(ptable, ls_from_cstr("POP" ), (void *) load_instruction_push_pop);
	hash_table_add(ptable, ls_from_cstr("ADD"), (void *) load_instruction_arith_reg);
	hash_table_add(ptable, ls_from_cstr("ADC"), (void *) load_instruction_arith_reg);
	hash_table_add(ptable, ls_from_cstr("SUB"), (void *) load_instruction_arith_reg);
	hash_table_add(ptable, ls_from_cstr("SBB"), (void *) load_instruction_arith_reg);
	hash_table_add(ptable, ls_from_cstr("INR"), (void *) load_instruction_arith_reg);
	hash_table_add(ptable, ls_from_cstr("DCR"), (void *) load_instruction_arith_reg);
	hash_table_add(ptable, ls_from_cstr("ANA"), (void *) load_instruction_arith_reg);
	hash_table_add(ptable, ls_from_cstr("XRA"), (void *) load_instruction_arith_reg);
	hash_table_add(ptable, ls_from_cstr("ORA"), (void *) load_instruction_arith_reg);
	hash_table_add(ptable, ls_from_cstr("CMP"), (void *) load_instruction_arith_reg);
	hash_table_add(ptable, ls_from_cstr("DAD"), (void *) load_instruction_arith_rp);
	hash_table_add(ptable, ls_from_cstr("INX"), (void *) load_instruction_arith_rp);
	hash_table_add(ptable, ls_from_cstr("DCX"), (void *) load_instruction_arith_rp);
	hash_table_add(ptable, ls_from_cstr("ADI"), (void *) load_instruction_imm_w);
	hash_table_add(ptable, ls_from_cstr("ACI"), (void *) load_instruction_imm_w);
	hash_table_add(ptable, ls_from_cstr("SUI"), (void *) load_instruction_imm_w);
	hash_table_add(ptable, ls_from_cstr("SBI"), (void *) load_instruction_imm_w);
	hash_table_add(ptable, ls_from_cstr("ANI"), (void *) load_instruction_imm_w);
	hash_table_add(ptable, ls_from_cstr("XRI"), (void *) load_instruction_imm_w);
	hash_table_add(ptable, ls_from_cstr("ORI"), (void *) load_instruction_imm_w);
	hash_table_add(ptable, ls_from_cstr("CPI"), (void *) load_instruction_imm_w);
	hash_table_add(ptable, ls_from_cstr("IN" ), (void *) load_instruction_imm_w);
	hash_table_add(ptable, ls_from_cstr("OUT"), (void *) load_instruction_imm_w);
	hash_table_add(ptable, ls_from_cstr("LHLD"), (void *) load_instruction_imm_dw);
	hash_table_add(ptable, ls_from_cstr("SHLD"), (void *) load_instruction_imm_dw);
	hash_table_add(ptable, ls_from_cstr("LDA" ), (void *) load_instruction_imm_dw);
	hash_table_add(ptable, ls_from_cstr("STA" ), (void *) load_instruction_imm_dw);
	hash_table_add(ptable, ls_from_cstr("JMP" ), (void *) load_instruction_imm_dw);
	hash_table_add(ptable, ls_from_cstr("JNZ" ), (void *) load_instruction_imm_dw);
	hash_table_add(ptable, ls_from_cstr("JZ"  ), (void *) load_instruction_imm_dw);
	hash_table_add(ptable, ls_from_cstr("JNC" ), (void *) load_instruction_imm_dw);
	hash_table_add(ptable, ls_from_cstr("JC"  ), (void *) load_instruction_imm_dw);
	hash_table_add(ptable, ls_from_cstr("JPO" ), (void *) load_instruction_imm_dw);
	hash_table_add(ptable, ls_from_cstr("JPE" ), (void *) load_instruction_imm_dw);
	hash_table_add(ptable, ls_from_cstr("JP"  ), (void *) load_instruction_imm_dw);
	hash_table_add(ptable, ls_from_cstr("JM"  ), (void *) load_instruction_imm_dw);
	hash_table_add(ptable, ls_from_cstr("CALL"), (void *) load_instruction_imm_dw);
	hash_table_add(ptable, ls_from_cstr("CNZ" ), (void *) load_instruction_imm_dw);
	hash_table_add(ptable, ls_from_cstr("CZ"  ), (void *) load_instruction_imm_dw);
	hash_table_add(ptable, ls_from_cstr("CNC" ), (void *) load_instruction_imm_dw);
	hash_table_add(ptable, ls_from_cstr("CC"  ), (void *) load_instruction_imm_dw);
	hash_table_add(ptable, ls_from_cstr("CPO" ), (void *) load_instruction_imm_dw);
	hash_table_add(ptable, ls_from_cstr("CPE" ), (void *) load_instruction_imm_dw);
	hash_table_add(ptable, ls_from_cstr("CP"  ), (void *) load_instruction_imm_dw);
	hash_table_add(ptable, ls_from_cstr("CM"  ), (void *) load_instruction_imm_dw);
	hash_table_add(ptable, ls_from_cstr("RET" ), (void *) load_instruction_imm_dw);
	hash_table_add(ptable, ls_from_cstr("RNZ" ), (void *) load_instruction_imm_dw);
	hash_table_add(ptable, ls_from_cstr("RZ"  ), (void *) load_instruction_imm_dw);
	hash_table_add(ptable, ls_from_cstr("RNC" ), (void *) load_instruction_imm_dw);
	hash_table_add(ptable, ls_from_cstr("RC"  ), (void *) load_instruction_imm_dw);
	hash_table_add(ptable, ls_from_cstr("RPO" ), (void *) load_instruction_imm_dw);
	hash_table_add(ptable, ls_from_cstr("RPE" ), (void *) load_instruction_imm_dw);
	hash_table_add(ptable, ls_from_cstr("RP"  ), (void *) load_instruction_imm_dw);
	hash_table_add(ptable, ls_from_cstr("RM"  ), (void *) load_instruction_imm_dw);
	hash_table_add(ptable, ls_from_cstr("RST"), (void *) load_instruction_rst);
}
