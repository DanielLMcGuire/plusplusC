// ++C C Runtime Library (libminicrt)
// Copyright 2026 Daniel McGuire
// Licensed under the MIT License

#include <env.h>
#include <alloc.h>
#include <mem.h>
#include <str.h>

static bool valid_name(const char *name)
{
    if (name == nullptr || name[0] == '\0')
        return false;

    for (const char *p = name; *p != '\0'; p++)
    {
        if (*p == '=')
            return false;
    }
    return true;
}

#if defined(XXC_RAWSYS)

#include <xxc_sys.h>
#include <crt_lock.h>

#define ENV_NOT_FOUND ((size_t)-1)

static crt_lock_t g_env_lock = CRT_LOCK_INIT;
static char     **g_env;
static size_t     g_env_len;
static size_t     g_env_cap;

static size_t env_find(const char *name, size_t nlen)
{
    if (__xxc_environ == nullptr)
        return ENV_NOT_FOUND;

    for (size_t i = 0; __xxc_environ[i] != nullptr; i++)
    {
        const char *e = __xxc_environ[i];
        size_t k = 0;

        while (k < nlen && e[k] == name[k])
            k++;

        if (k == nlen && e[nlen] == '=')
            return i;
    }
    return ENV_NOT_FOUND;
}

static bool env_own(void)
{
    if (g_env != nullptr)
        return true;

    size_t n = 0;
    while (__xxc_environ != nullptr && __xxc_environ[n] != nullptr)
        n++;

    size_t cap  = n + 16;
    char **copy = (char **)malloc(cap * sizeof(char *));
    if (copy == nullptr)
        return false; /* GCOV_EXCL_LINE */

    for (size_t i = 0; i < n; i++)
    {
        size_t len = strlen(__xxc_environ[i]) + 1;
        copy[i] = (char *)malloc(len);

        /* GCOV_EXCL_START */
        if (copy[i] == nullptr)
        {
            while (i > 0)
                free(copy[--i]);
            free(copy);
            return false;
        }
        /* GCOV_EXCL_STOP */

        memcpy(copy[i], __xxc_environ[i], len);
    }
    copy[n] = nullptr;

    g_env        = copy;
    g_env_len    = n;
    g_env_cap    = cap;
    __xxc_environ = copy;
    return true;
}

dstr_t env_get(const char *name)
{
    dstr_t out = { nullptr, 0, 0 };

    if (!valid_name(name))
        return out;

    size_t nlen = strlen(name);

    __crt_lock_acquire(&g_env_lock);
    size_t at = env_find(name, nlen);
    if (at != ENV_NOT_FOUND)
        out = dstr_new(__xxc_environ[at] + nlen + 1);
    __crt_lock_release(&g_env_lock);

    return out;
}

int env_set(const char *name, const char *value, bool overwrite)
{
    if (!valid_name(name) || value == nullptr)
        return -PROC_EINVAL;

    size_t nlen = strlen(name);
    size_t vlen = strlen(value);
    int    rc   = 0;

    __crt_lock_acquire(&g_env_lock);

    size_t at = env_find(name, nlen);

    if (at != ENV_NOT_FOUND && !overwrite)
    {
        __crt_lock_release(&g_env_lock);
        return 0;
    }

    if (!env_own())
        rc = -PROC_ENOMEM; /* GCOV_EXCL_LINE */
    else
    {
        char *entry = (char *)malloc(nlen + 1 + vlen + 1);

        if (entry == nullptr)
            rc = -PROC_ENOMEM; /* GCOV_EXCL_LINE */
        else
        {
            memcpy(entry, name, nlen);
            entry[nlen] = '=';
            memcpy(entry + nlen + 1, value, vlen + 1);

            if (at != ENV_NOT_FOUND)
            {
                free(g_env[at]);
                g_env[at] = entry;
            }
            else
            {
                if (g_env_len + 2 > g_env_cap)
                {
                    char **grown = (char **)realloc(g_env, g_env_cap * 2 * sizeof(char *));

                    /* GCOV_EXCL_START */
                    if (grown == nullptr)
                    {
                        free(entry);
                        __crt_lock_release(&g_env_lock);
                        return -PROC_ENOMEM;
                    }
                    /* GCOV_EXCL_STOP */

                    g_env         = grown;
                    g_env_cap    *= 2;
                    __xxc_environ = grown;
                }

                g_env[g_env_len++] = entry;
                g_env[g_env_len]   = nullptr;
            }
        }
    }

    __crt_lock_release(&g_env_lock);
    return rc;
}

int env_unset(const char *name)
{
    if (!valid_name(name))
        return -PROC_EINVAL;

    size_t nlen = strlen(name);
    int    rc   = 0;

    __crt_lock_acquire(&g_env_lock);

    if (env_find(name, nlen) != ENV_NOT_FOUND)
    {
        if (!env_own())
            rc = -PROC_ENOMEM; /* GCOV_EXCL_LINE */
        else
        {
            size_t at;
            while ((at = env_find(name, nlen)) != ENV_NOT_FOUND)
            {
                free(g_env[at]);
                memmove(&g_env[at], &g_env[at + 1], (g_env_len - at) * sizeof(char *));
                g_env_len--;
            }
        }
    }

    __crt_lock_release(&g_env_lock);
    return rc;
}

parr_t env_list(void)
{
    parr_t list = parr_new(nullptr, 0);

    __crt_lock_acquire(&g_env_lock);
    for (size_t i = 0; __xxc_environ != nullptr && __xxc_environ[i] != nullptr; i++)
        parr_append_cstr(&list, __xxc_environ[i]);
    __crt_lock_release(&g_env_lock);

    return list;
}

void __xxc_env_fork_prepare(void)
{
    __crt_lock_acquire(&g_env_lock);
}

void __xxc_env_fork_release(void)
{
    __crt_lock_release(&g_env_lock);
}

#elif defined(_WIN32)

#include <windows.h>

dstr_t env_get(const char *name)
{
    dstr_t out = { nullptr, 0, 0 };

    if (!valid_name(name))
        return out;

    DWORD cap = 256;

    for (;;)
    {
        char *buf = (char *)malloc(cap);
        if (buf == nullptr)
            return out; /* GCOV_EXCL_LINE */

        SetLastError(ERROR_SUCCESS);
        DWORD n = GetEnvironmentVariableA(name, buf, cap);

        if (n == 0 && GetLastError() == ERROR_ENVVAR_NOT_FOUND)
        {
            free(buf);
            return out;
        }

        if (n >= cap)
        {
            free(buf);
            cap = n + 1;
            continue;
        }

        out = dstr_new(buf);
        free(buf);
        return out;
    }
}

int env_set(const char *name, const char *value, bool overwrite)
{
    if (!valid_name(name) || value == nullptr)
        return -PROC_EINVAL;

    if (!overwrite)
    {
        SetLastError(ERROR_SUCCESS);
        DWORD n = GetEnvironmentVariableA(name, nullptr, 0);
        if (n != 0 || GetLastError() != ERROR_ENVVAR_NOT_FOUND)
            return 0;
    }

    if (!SetEnvironmentVariableA(name, value))
        return -PROC_EINVAL; /* GCOV_EXCL_LINE */

    return 0;
}

int env_unset(const char *name)
{
    if (!valid_name(name))
        return -PROC_EINVAL;

    SetEnvironmentVariableA(name, nullptr);
    return 0;
}

parr_t env_list(void)
{
    parr_t list = parr_new(nullptr, 0);

    LPCH block = GetEnvironmentStringsA();
    if (block == nullptr)
        return list; /* GCOV_EXCL_LINE */

    for (const char *p = block; *p != '\0'; p += strlen(p) + 1)
    {
        if (*p != '=')
            parr_append_cstr(&list, p);
    }

    FreeEnvironmentStringsA(block);
    return list;
}

#else /* UEFI: no environment */

dstr_t env_get(const char *name)
{
    (void)name;
    dstr_t none = { nullptr, 0, 0 };
    return none;
}

int env_set(const char *name, const char *value, bool overwrite)
{
    (void)name; (void)value; (void)overwrite;
    return -PROC_ENOSYS;
}

int env_unset(const char *name)
{
    (void)name;
    return -PROC_ENOSYS;
}

parr_t env_list(void)
{
    return parr_new(nullptr, 0);
}

#endif
