#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "utils.h"

int asprintf(char **strp, const char *fmt, ...)
{
	va_list ap;
	int rc;

	va_start(ap, fmt);
	rc = vasprintf(strp, fmt, ap);
	va_end(ap);

	return rc;
}

int vasprintf(char **strp, const char *fmt, va_list ap)
{
	int expstrlen;
	va_list tmpap;

	va_copy(tmpap, ap);
	if ((expstrlen = vsnprintf(NULL, 0, fmt, tmpap)) < 0) {
		return expstrlen;
	}
	va_end(tmpap);

	if (!(*strp = malloc(expstrlen + 1))) {
		return -1;
	}

	return vsnprintf(*strp, expstrlen + 1, fmt, ap);
}

long readFileToString(char *path, char **bytes)
{
	FILE *fp = NULL;

	fp = fopen(path, "rb");
	if (fp == NULL) {
		return -1;
	}

	fseek(fp, 0, SEEK_END);
	long size = ftell(fp);
	*bytes = malloc(sizeof(*bytes) * size);
	rewind(fp);
	fread(*bytes, 1, size, fp);

	fclose(fp);

	return size;
}

void srgbToLinear(float vector[3])
{
	for (size_t i = 0; i < 3; ++i) {
		vector[i] = pow(vector[i], 2.2);
	}
}
