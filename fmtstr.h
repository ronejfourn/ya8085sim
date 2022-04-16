#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

static char *fmtstr (char *fmt, ...) {
	va_list va, vc;
	va_start(va, fmt);
	va_copy(vc, va);
	int len = vsnprintf(NULL, 0, fmt, va) < 512 ? len : 511;
	char *str = (char *)malloc(len + 1);
	vsnprintf(str, len, fmt, vc);
	str[len] = 0;
	va_end(va);
	return str;
}
