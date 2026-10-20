// EzPacker YARP End-to-End Test 05: Structures, Aggregates & Global Variables
// Pure standalone C11 translation unit without standard library dependency.

#include <stdint.h>
#include <stdbool.h>

struct Point3D {
    int32_t x;
    int32_t y;
    int32_t z;
};

struct PacketPayload {
    uint32_t header_id;
    uint16_t length;
    uint8_t  flags;
    uint8_t  padding;
    uint64_t data_word;
};

static uint32_t g_internal_counter = 100;
uint32_t g_exported_status = 0xCAFE;

int32_t transform_point(struct Point3D *pt, int32_t dx, int32_t dy, int32_t dz) {
    if (pt == 0) return 0;
    pt->x = pt->x + dx;
    pt->y = pt->y + dy;
    pt->z = pt->z + dz;
    g_internal_counter = g_internal_counter + 1;
    return pt->x + pt->y + pt->z;
}

uint64_t process_packet_payload(struct PacketPayload *pkt, uint64_t mask) {
    if (pkt == 0) return 0;
    pkt->data_word = (pkt->data_word ^ mask) + (uint64_t)pkt->header_id;
    g_exported_status = (g_exported_status ^ (uint32_t)pkt->flags) + 1;
    return pkt->data_word;
}

uint32_t get_internal_counter_val(void) {
    return g_internal_counter;
}

