// -----------------------------------------------------------------------
// printf.c

#include <string.h>
#include <ctype.h>
#include <limits.h>

#include "stdio_console.h"

/**
 * printf - Format a string and toss it out
 * @buf: The buffer to place the result into
 * @fmt: The format string to use
 * @...: Arguments for the format string
 */
int printf(const char *fmt, ...)
{
	va_list args;
	int i;
        char locbuf[80] ;
        
	va_start(args, fmt) ;
	i=vsnprintf(locbuf, sizeof(locbuf), fmt, args) ;
	va_end(args) ;
	for(i=0 ; (locbuf[i]) && (i<sizeof(locbuf)) ; i++)
	    putchar(locbuf[i]) ;
	return i;
}

