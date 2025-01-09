#pragma once

#include <stdint.h>

#if defined(__cplusplus)
    #define ALIGNOF(type) alignof(type)
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
    #define ALIGNOF(type) _Alignof(type)
#elif defined(__GNUC__) || defined(__clang__)
    #define ALIGNOF(type) __alignof__(type)
#elif defined(_MSC_VER)
    #define ALIGNOF(type) __alignof(type)
#else
    #define ALIGNOF(type) ((size_t)(&((struct { char c; type d; }*)0)->d))
#endif

typedef struct {
    uint8_t* start;
    uint8_t* end;
    uint8_t* current;
} bump_allocator_t;

static inline void bump_init(bump_allocator_t *alloc, void *memory, size_t size) {
    alloc->start = (uint8_t*)memory;
    alloc->current = alloc->start;
    alloc->end = alloc->start + size;
}

static inline uintptr_t align_up(uintptr_t addr, size_t align) {
    return (addr + (align - 1) & ~(align - 1));
}

static inline void *bump_alloc(bump_allocator_t *alloc, size_t size, size_t align) {
    uint8_t *aligned = (uint8_t*)align_up((uintptr_t)alloc->current, align);
    uint8_t *new_current = aligned + size;

    // Check if we have enough space by address comparison
    if (new_current > alloc->end) {
        return 0;
    }

    alloc->current = new_current;
    return aligned;
}

static inline void bump_reset(bump_allocator_t *alloc) {
    alloc->current = alloc->start;
}

static inline size_t bump_available(const bump_allocator_t *alloc) {
    return alloc->end - alloc->current;
}
