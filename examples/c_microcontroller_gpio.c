// EzPacker C Frontend: Microcontroller MMIO GPIO & Timer Peripheral Driver
// Demonstrates standalone freestanding C11 compilation without standard library.
//
// Target use cases: Baremetal firmware, microcontrollers (ARM Cortex-M, RISC-V),
// bootloaders, and embedded hardware interfaces using memory-mapped I/O (MMIO).

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Simulated Peripheral Base Addresses (aligned to 4KB hardware page)
#define PERIPH_BASE        0x40000000UL
#define GPIOA_BASE         (PERIPH_BASE + 0x20000UL)
#define TIMER_BASE         (PERIPH_BASE + 0x10000UL)

// Hardware Register Offsets
#define REG_GPIO_MODER     (*(volatile uint32_t *)(GPIOA_BASE + 0x00))
#define REG_GPIO_OTYPER    (*(volatile uint32_t *)(GPIOA_BASE + 0x04))
#define REG_GPIO_OSPEEDR   (*(volatile uint32_t *)(GPIOA_BASE + 0x08))
#define REG_GPIO_PUPDR     (*(volatile uint32_t *)(GPIOA_BASE + 0x0C))
#define REG_GPIO_IDR       (*(volatile uint32_t *)(GPIOA_BASE + 0x10))
#define REG_GPIO_ODR       (*(volatile uint32_t *)(GPIOA_BASE + 0x14))
#define REG_GPIO_BSRR      (*(volatile uint32_t *)(GPIOA_BASE + 0x18))

#define REG_TIM_CR1        (*(volatile uint32_t *)(TIMER_BASE + 0x00))
#define REG_TIM_CNT        (*(volatile uint32_t *)(TIMER_BASE + 0x24))
#define REG_TIM_PSC        (*(volatile uint32_t *)(TIMER_BASE + 0x28))
#define REG_TIM_ARR        (*(volatile uint32_t *)(TIMER_BASE + 0x2C))

// Pin Configuration Modes
typedef enum {
    GPIO_MODE_INPUT     = 0x00,
    GPIO_MODE_OUTPUT    = 0x01,
    GPIO_MODE_ALTFUNC   = 0x02,
    GPIO_MODE_ANALOG    = 0x03
} GpioPinMode;

// Hardware State Storage for Baremetal Telemetry
static uint32_t g_pin_toggle_count = 0;
static uint32_t g_error_flags = 0;

void gpio_init_pin(uint8_t pin, GpioPinMode mode) {
    if (pin > 15) {
        g_error_flags |= 0x01;
        return;
    }

    uint32_t shift = (uint32_t)pin * 2U;
    uint32_t mask = 0x3U << shift;
    uint32_t val = ((uint32_t)mode & 0x3U) << shift;

    // Atomic modify with read-modify-write
    REG_GPIO_MODER = (REG_GPIO_MODER & ~mask) | val;
}

void gpio_set_pin(uint8_t pin) {
    if (pin > 15) return;
    // Fast atomic bit set using Bit Set/Reset Register (BSRR)
    REG_GPIO_BSRR = (1UL << pin);
    g_pin_toggle_count = g_pin_toggle_count + 1;
}

void gpio_reset_pin(uint8_t pin) {
    if (pin > 15) return;
    // Fast atomic bit reset using upper 16 bits of BSRR
    REG_GPIO_BSRR = (1UL << (pin + 16));
    g_pin_toggle_count = g_pin_toggle_count + 1;
}

bool gpio_read_pin(uint8_t pin) {
    if (pin > 15) return false;
    uint32_t idr = REG_GPIO_IDR;
    return (idr & (1UL << pin)) != 0;
}

void gpio_toggle_pin(uint8_t pin) {
    if (pin > 15) return;
    uint32_t odr = REG_GPIO_ODR;
    if (odr & (1UL << pin)) {
        gpio_reset_pin(pin);
    } else {
        gpio_set_pin(pin);
    }
}

void timer_init_busywait(uint32_t prescaler, uint32_t autoreload) {
    REG_TIM_PSC = prescaler;
    REG_TIM_ARR = autoreload;
    REG_TIM_CNT = 0;
    // Enable timer counter (CEN = bit 0)
    REG_TIM_CR1 |= 0x01;
}

void delay_cycles(uint32_t count) {
    // Pure volatile loop preventing optimization removal
    volatile uint32_t c = count;
    while (c > 0) {
        c = c - 1;
    }
}

uint32_t get_toggle_telemetry(void) {
    return g_pin_toggle_count;
}

uint32_t get_error_telemetry(void) {
    return g_error_flags;
}

