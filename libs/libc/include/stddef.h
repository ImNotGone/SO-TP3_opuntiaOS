#ifndef _LIBC_STDDEF_H
#define _LIBC_STDDEF_H

#include <sys/cdefs.h>
#include <sys/types.h>

__BEGIN_DECLS

#ifdef __i386__
typedef unsigned long size_t;
typedef long ssize_t;
#elif __arm__
typedef unsigned int size_t;
typedef int ssize_t;
#endif

#define NULL ((void*)0)

__END_DECLS

#endif // _LIBC_STDDEF_H