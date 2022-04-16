#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

static char *fmtstr (char *fmt, ...) {
	va_list va, vc;
	va_start(va, fmt);
	va_copy(vc, va);
	int len = vsnprintf(NULL, 0, fmt, va) + 1;
	len = len > 512 ? 512 : len;
	char *str = (char *)malloc(len + 1);
	vsnprintf(str, len, fmt, vc);
	str[len] = 0;
	va_end(va);
	return str;
}
