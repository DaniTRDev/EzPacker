// EzPacker YARP End-to-End Test 05 Driver: Structs & Globals Verifier
// Zero standard library dependencies. Returns 0 on success, non-zero on error.

#include "freestanding_driver.h"

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

extern uint32_t g_exported_status;

ABI_ATTR extern int32_t transform_point(struct Point3D *pt, int32_t dx, int32_t dy, int32_t dz);
ABI_ATTR extern uint64_t process_packet_payload(struct PacketPayload *pkt, uint64_t mask);
ABI_ATTR extern uint32_t get_internal_counter_val(void);

int main(void) {
    uint64_t seed = 0x98765432ULL;

    struct Point3D pt = { 10, 20, 30 };
    int32_t sum = transform_point(&pt, 5, -10, 15); // pt: 15, 10, 45 -> sum: 70
    if (sum != 70 || pt.x != 15 || pt.y != 10 || pt.z != 45) return 1;
    hash_step(&seed, (uint64_t)sum);

    uint32_t cnt = get_internal_counter_val();
    if (cnt != 101) return 2;
    hash_step(&seed, (uint64_t)cnt);

    struct PacketPayload pkt;
    pkt.header_id = 0x1234;
    pkt.length = 64;
    pkt.flags = 0x5;
    pkt.padding = 0;
    pkt.data_word = 0xAA0000ULL;

    uint64_t dw = process_packet_payload(&pkt, 0x00FF00ULL);
    // dw = (0xAA0000 ^ 0x00FF00) + 0x1234 = 0xAAFF00 + 0x1234 = 0xAB1134
    if (dw != 0xAB1134ULL) return 3;
    hash_step(&seed, dw);
    hash_step(&seed, (uint64_t)g_exported_status);

    if (seed == 0) return 4;
    return 0; // Success
}
