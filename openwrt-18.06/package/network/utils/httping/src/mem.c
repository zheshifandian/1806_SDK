/* (C) 2007 by folkert@vanheusden.com
 * The GPL (GNU public license) applies to this sourcecode.
 */
#include <errno.h>
#include <regex.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

static void error_exit(char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	(void)vfprintf(stderr, format, ap);
	va_end(ap);
	if (errno)
		fprintf(stderr, "errno: %d=%s (if applicable)\n", errno, strerror(errno));

	exit(EXIT_FAILURE);
}

void myfree(void *p)
{
	free(p);
}

void * myrealloc(void *oldp, int new_size, char *what)
{
	void *newp = realloc(oldp, new_size);
	if (!newp)
		error_exit("Failed to reallocate a memory block (%s) to %d bytes.\n", what, new_size);

	return newp;
}


void * mymalloc(int size, char *what)
{
	return myrealloc(NULL, size, what);
}

char * mystrdup(char *in, char *what)
{
	int len = strlen(in) + 1;
	char *newp = (char *)mymalloc(len, what);

	memcpy(newp, in, len);

	return newp;
}
