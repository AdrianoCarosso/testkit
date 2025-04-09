// ------------------------------------------------------------------------
// stdio.h
// reduced version for console I/O

#ifndef _STDIO_H_
#define	_STDIO_H_

#include <stdarg.h>

/*
 * Functions defined in ANSI C standard.
 */

#ifdef __GNUC__
#define __VALIST __gnuc_va_list
#else
#define __VALIST char*
#endif

int printf(const char *, ...) ;
int scanf(const char *, ...) ;
int sscanf(const char *, const char *, ...) ;
int vprintf(const char *, __VALIST) ;
int vsprintf(char *, const char *, __VALIST) ;
int vsnprintf(char *, const unsigned long, const char *, __VALIST) ;
char * gets(char *) ;
int puts(const char *) ;
int sprintf(char *, const char *, ...) ;

int getchar(void) ;
int putchar(int c) ;

// internal functions
int skip_atoi(const char **s) ;
unsigned long simple_strtoul(const char *cp,char **endp,unsigned int base) ;
long simple_strtol(const char *cp,char **endp,unsigned int base) ;
unsigned long long simple_strtoull(const char *cp,char **endp,unsigned int base) ;
long long simple_strtoll(const char *cp,char **endp,unsigned int base) ;

#endif /* _STDIO_H_ */

