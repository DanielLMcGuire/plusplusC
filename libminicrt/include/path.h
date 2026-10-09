// ++C C Runtime Library (libminicrt)
// Copyright 2026 Daniel McGuire
// Licensed under the MIT License

#ifndef PATH_H
#define PATH_H

#include <def.h>
#include <dstr.h>

char *getcwd(char *buf, size_t size);

bool path_getcwd(dstr_t *out);

#endif /* PATH_H */
