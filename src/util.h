#ifndef PYSQLITE_UTIL_H
#define PYSQLITE_UTIL_H

#ifdef PYSQLITE_DEBUG
#include <sys/types.h>
#include <unistd.h>
#define Dprintf(fmt, args...) fprintf(stderr, "::pysqlite:: " fmt "\n", ## args)
#else
#define Dprintf(fmt, args...)
#endif

#endif
