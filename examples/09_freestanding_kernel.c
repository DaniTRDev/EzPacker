// EzPacker C Frontend: Standalone Freestanding Kernel Module
// Demonstrates ISO C11 freestanding compilation using bundled headers:
// <stdint.h>, <stddef.h>, <stdbool.h>, and <limits.h>.
//
// Implements a standalone ring-buffer and UART packet framing routine
// suitable for bare-metal operating system kernels, bootloaders, and firmware.

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limits.h>

#define RING_BUFFER_CAPACITY 64
#define PACKET_MAGIC 0x455A504BU /* 'E', 'Z', 'P', 'K' */

struct PacketHeader {
    uint32_t magic;      // Protocol magic identifier
    uint16_t length;     // Payload byte length
    uint16_t sequence;   // Monotonic packet sequence counter
};

struct RingBuffer {
    uint8_t buffer[RING_BUFFER_CAPACITY];
    size_t head;
    size_t tail;
    size_t count;
};

static struct RingBuffer g_rx_queue;
static uint32_t g_packets_processed = 0;

void ring_buffer_init(struct RingBuffer *rb) {
    if (rb == NULL)
        return;
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
}

bool ring_buffer_push(struct RingBuffer *rb, uint8_t byte) {
    if (rb == NULL || rb->count >= RING_BUFFER_CAPACITY)
        return false;

    rb->buffer[rb->head] = byte;
    rb->head = (rb->head + 1) % RING_BUFFER_CAPACITY;
    rb->count = rb->count + 1;
    return true;
}

bool ring_buffer_pop(struct RingBuffer *rb, uint8_t *out_byte) {
    if (rb == NULL || out_byte == NULL || rb->count == 0)
        return false;

    *out_byte = rb->buffer[rb->tail];
    rb->tail = (rb->tail + 1) % RING_BUFFER_CAPACITY;
    rb->count = rb->count - 1;
    return true;
}

size_t get_packet_header_size(void) {
    return sizeof(struct PacketHeader);
}

size_t get_payload_offset(void) {
    return offsetof(struct PacketHeader, sequence);
}

bool process_incoming_frame(const struct PacketHeader *pkt) {
    if (pkt == NULL)
        return false;

    if (pkt->magic == PACKET_MAGIC && pkt->length <= USHRT_MAX) {
        g_packets_processed = g_packets_processed + 1;
        return true;
    }
    return false;
}

uint32_t get_total_processed(void) {
    return g_packets_processed;
}
