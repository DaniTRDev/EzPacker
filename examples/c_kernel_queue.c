// EzPacker C Frontend: Freestanding Kernel Event Queue & Packet Dispatcher
// Demonstrates standalone freestanding C11 compilation without standard library.
//
// Target use cases: Operating system kernels, interrupt handlers, device drivers,
// embedded message buses, and deterministic baremetal real-time queues.

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <limits.h>

#define EVENT_QUEUE_CAPACITY 32
#define EVENT_MAGIC 0x4B524E4CUL /* 'K', 'R', 'N', 'L' */

typedef enum {
    EVENT_PRIORITY_LOW    = 0,
    EVENT_PRIORITY_NORMAL = 1,
    EVENT_PRIORITY_HIGH   = 2,
    EVENT_PRIORITY_URGENT = 3
} EventPriority;

struct KernelEvent {
    uint32_t magic;
    uint16_t event_id;
    uint8_t  priority;
    uint8_t  flags;
    uint64_t timestamp;
    uint32_t payload;
};

struct EventQueue {
    struct KernelEvent events[EVENT_QUEUE_CAPACITY];
    size_t head;
    size_t tail;
    size_t count;
    uint32_t dropped_count;
    uint32_t dispatched_count;
};

static struct EventQueue g_kernel_queue;

void kernel_queue_init(struct EventQueue *q) {
    if (q == NULL) return;
    q->head = 0;
    q->tail = 0;
    q->count = 0;
    q->dropped_count = 0;
    q->dispatched_count = 0;
}

bool kernel_queue_enqueue(struct EventQueue *q, const struct KernelEvent *evt) {
    if (q == NULL || evt == NULL) return false;

    // Validate event magic identifier
    if (evt->magic != EVENT_MAGIC) {
        q->dropped_count = q->dropped_count + 1;
        return false;
    }

    if (q->count >= EVENT_QUEUE_CAPACITY) {
        q->dropped_count = q->dropped_count + 1;
        return false;
    }

    q->events[q->head] = *evt;
    q->head = (q->head + 1) % EVENT_QUEUE_CAPACITY;
    q->count = q->count + 1;
    return true;
}

bool kernel_queue_dequeue(struct EventQueue *q, struct KernelEvent *out_evt) {
    if (q == NULL || out_evt == NULL || q->count == 0) return false;

    *out_evt = q->events[q->tail];
    q->tail = (q->tail + 1) % EVENT_QUEUE_CAPACITY;
    q->count = q->count - 1;
    q->dispatched_count = q->dispatched_count + 1;
    return true;
}

uint32_t compute_event_checksum(const struct KernelEvent *evt) {
    if (evt == NULL) return 0;

    uint32_t hash = 0x811c9dc5U; // FNV-1a 32-bit offset basis
    const uint8_t *bytes = (const uint8_t *)evt;
    size_t len = sizeof(struct KernelEvent);

    for (size_t i = 0; i < len; i = i + 1) {
        hash = hash ^ (uint32_t)bytes[i];
        hash = hash * 0x01000193U; // FNV-1a prime
    }
    return hash;
}

bool is_urgent_event(const struct KernelEvent *evt) {
    if (evt == NULL) return false;
    return evt->priority == EVENT_PRIORITY_URGENT;
}

size_t get_queue_occupancy(const struct EventQueue *q) {
    return (q != NULL) ? q->count : 0;
}

uint32_t get_dispatched_total(const struct EventQueue *q) {
    return (q != NULL) ? q->dispatched_count : 0;
}

