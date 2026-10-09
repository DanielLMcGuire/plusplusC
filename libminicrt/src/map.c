// ++C C Runtime Library (libminicrt)
// Copyright 2026 Daniel McGuire
// Licensed under the MIT License

#include <map.h>
#include <alloc.h>
#include <mem.h>
#include <str.h>

#define MAP_DEFAULT_CAP 16

u64 map_hash_bytes(const void *data, size_t len)
{
    const u8 *p = (const u8 *)data;
    u64 h = 0xcbf29ce484222325ull;
    for (size_t i = 0; i < len; i++)
    {
        h ^= p[i];
        h *= 0x100000001b3ull;
    }
    return h;
}

static size_t next_pow2(size_t v)
{
    if (v < 8) return 8;
    v--;
    v |= v >> 1; v |= v >> 2; v |= v >> 4;
    v |= v >> 8; v |= v >> 16;
#if defined(__LP64__) || defined(_WIN64) || defined(__x86_64__) || defined(__aarch64__)
    v |= v >> 32;
#endif
    return v + 1;
}

map_t map_new(size_t initial_cap, void (*value_free)(void *value))
{
    map_t m;
    m.cap = next_pow2(initial_cap ? initial_cap : MAP_DEFAULT_CAP);
    m.len = 0;
    m.value_free = value_free;
    m.entries = (map_entry_t *)calloc(m.cap, sizeof(map_entry_t));
    if (m.entries == nullptr)
        m.cap = 0;
    return m;
}

void map_free(map_t *m)
{
    if (m == nullptr) return;
    map_clear(m);
    free(m->entries);
    m->entries = nullptr;
    m->cap = 0;
}

void map_clear(map_t *m)
{
    if (m == nullptr || m->entries == nullptr) return;
    for (size_t i = 0; i < m->cap; i++)
    {
        if (!m->entries[i].occupied) continue;
        if (m->value_free) m->value_free(m->entries[i].value);
        free(m->entries[i].key);
        m->entries[i].occupied = false;
    }
    m->len = 0;
}

size_t map_len(const map_t *m)
{
    return m ? m->len : 0;
}

static bool key_matches(const map_entry_t *e, u64 hash, const char *key, size_t key_len)
{
    return e->hash == hash && e->key_len == key_len && memcmp(e->key, key, key_len) == 0;
}

static size_t probe(const map_t *m, u64 hash, const char *key, size_t key_len, bool *found)
{
    size_t mask = m->cap - 1;
    size_t i = (size_t)hash & mask;
    for (;;)
    {
        map_entry_t *e = &m->entries[i];
        if (!e->occupied)
        { 
            *found = false; 
            return i; 
        }
        if (key_matches(e, hash, key, key_len))
        { 
            *found = true; 
            return i; 
        }
        i = (i + 1) & mask;
    }
}

static void raw_insert(map_t *m, u64 hash, char *key, size_t key_len, void *value)
{
    size_t mask = m->cap - 1;
    size_t i = (size_t)hash & mask;
    while (m->entries[i].occupied)
        i = (i + 1) & mask;
    m->entries[i].hash = hash;
    m->entries[i].key = key;
    m->entries[i].key_len = key_len;
    m->entries[i].value = value;
    m->entries[i].occupied = true;
}

static bool grow(map_t *m)
{
    size_t old_cap = m->cap;
    map_entry_t *old = m->entries;

    size_t new_cap = old_cap ? old_cap * 2 : MAP_DEFAULT_CAP;
    map_entry_t *fresh = (map_entry_t *)calloc(new_cap, sizeof(map_entry_t));
    if (fresh == nullptr) return false;

    m->entries = fresh;
    m->cap = new_cap;
    for (size_t i = 0; i < old_cap; i++)
        if (old[i].occupied)
            raw_insert(m, old[i].hash, old[i].key, old[i].key_len, old[i].value);

    free(old);
    return true;
}

bool map_set(map_t *m, const char *key, size_t key_len, void *value)
{
    if (m == nullptr) return false;
    if (m->entries == nullptr && !grow(m)) return false;

    u64 hash = map_hash_bytes(key, key_len);
    bool found;
    size_t i = probe(m, hash, key, key_len, &found);

    if (found)
    {
        map_entry_t *e = &m->entries[i];
        if (m->value_free && e->value != value)
            m->value_free(e->value);
        e->value = value;
        return true;
    }

    if ((m->len + 1) * 8 > m->cap * 7)
    {
        if (!grow(m)) return false;
    }

    char *owned = (char *)malloc(key_len + 1);
    if (owned == nullptr) return false;
    memcpy(owned, key, key_len);
    owned[key_len] = '\0';

    raw_insert(m, hash, owned, key_len, value);
    m->len++;
    return true;
}

bool map_set_cstr(map_t *m, const char *key, void *value)
{
    return map_set(m, key, key ? strlen(key) : 0, value);
}

bool map_try_get(const map_t *m, const char *key, size_t key_len, void **out)
{
    if (m == nullptr || m->entries == nullptr) return false;
    u64 hash = map_hash_bytes(key, key_len);
    bool found;
    size_t i = probe(m, hash, key, key_len, &found);
    if (!found) return false;
    if (out) *out = m->entries[i].value;
    return true;
}

bool map_try_get_cstr(const map_t *m, const char *key, void **out)
{
    return map_try_get(m, key, key ? strlen(key) : 0, out);
}

void *map_get(const map_t *m, const char *key, size_t key_len)
{
    void *v = nullptr;
    map_try_get(m, key, key_len, &v);
    return v;
}

void *map_get_cstr(const map_t *m, const char *key)
{
    return map_get(m, key, key ? strlen(key) : 0);
}

bool map_contains(const map_t *m, const char *key, size_t key_len)
{
    return map_try_get(m, key, key_len, nullptr);
}

bool map_contains_cstr(const map_t *m, const char *key)
{
    return map_contains(m, key, key ? strlen(key) : 0);
}

bool map_remove(map_t *m, const char *key, size_t key_len)
{
    if (m == nullptr || m->entries == nullptr) return false;
    u64 hash = map_hash_bytes(key, key_len);
    bool found;
    size_t i = probe(m, hash, key, key_len, &found);
    if (!found) return false;

    size_t mask = m->cap - 1;
    if (m->value_free) m->value_free(m->entries[i].value);
    free(m->entries[i].key);
    m->entries[i].occupied = false;
    m->len--;

    size_t gap = i;
    size_t j = i;
    for (;;)
    {
        j = (j + 1) & mask;
        map_entry_t *ej = &m->entries[j];
        if (!ej->occupied) break;

        size_t home = (size_t)ej->hash & mask;
        size_t dist_home_to_gap = (gap - home) & mask;
        size_t dist_home_to_j   = (j - home) & mask;

        if (dist_home_to_gap <= dist_home_to_j)
        {
            m->entries[gap] = *ej;
            ej->occupied = false;
            gap = j;
        }
    }
    return true;
}

bool map_remove_cstr(map_t *m, const char *key)
{
    return map_remove(m, key, key ? strlen(key) : 0);
}

bool map_iterate(const map_t *m, size_t *cursor, const char **key_out, size_t *key_len_out, void **value_out)
{
    if (m == nullptr || m->entries == nullptr || cursor == nullptr) return false;
    for (size_t i = *cursor; i < m->cap; i++)
    {
        if (!m->entries[i].occupied) continue;
        if (key_out)     *key_out = m->entries[i].key;
        if (key_len_out) *key_len_out = m->entries[i].key_len;
        if (value_out)   *value_out = m->entries[i].value;
        *cursor = i + 1;
        return true;
    }
    *cursor = m->cap;
    return false;
}
