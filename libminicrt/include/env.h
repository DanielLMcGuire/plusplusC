// ++C C Runtime Library (libminicrt)
// Copyright 2026 Daniel McGuire
// Licensed under the MIT License

#ifndef ENV_H
#define ENV_H

#include <def.h>
#include <dstr.h>
#include <parr.h>
#include <proc.h>

dstr_t env_get(const char *name);
int env_set(const char *name, const char *value, bool overwrite);
int env_unset(const char *name);
parr_t env_list(void);

#endif /* ENV_H */
