#pragma once
#include <string.h>
#define IS_SPACE(ch) ((ch) == '\t' || (ch) == '\v' || (ch) == '\f' || (ch) == ' ')
#define IS_NEWLINE(ch) ((ch) == '\r' || (ch) == '\n')
#define IS_LCASE(ch) ((ch) >= 'a' && (ch) <= 'z')
#define IS_UCASE(ch) ((ch) >= 'A' && (ch) <= 'Z')
#define IS_ALPHA(ch) (IS_UCASE(ch) || IS_LCASE(ch))
#define IS_NUM(ch)   ((ch) >= '0' && (ch) <= '9')
#define IS_ALNUM(ch) (IS_ALPHA(ch) || IS_NUM(ch))

#define TO_UCASE(ch) ((ch) >= 'a' && (ch) <= 'z' ? (ch) + 'A' - 'a' : (ch))
#define TO_LCASE(ch) ((ch) >= 'A' && (ch) <= 'Z' ? (ch) + 'a' - 'A' : (ch))

#define LSFMT "%.*s"
#define LSARG(s) s.len, s.data

typedef struct lenstring {
	const char *data;
	size_t len;
} lenstring;

static inline lenstring ls_from_cstr(char *str) {
	return (lenstring) {str, strlen(str)};
}
