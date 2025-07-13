#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <stdint.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

// LED state structure
typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t intensity;
} LedState_t;

// Configuration for LED edges
typedef struct {
    uint8_t num_edges;
    uint32_t* num_led_per_edge;  // Support max 8 edges
    LedState_t **data;  // 2D array: data[edge][led_index]
} LedEdgeConfigState_t;

// Global frame buffers
extern LedEdgeConfigState_t *currentLedConfigState;
extern LedEdgeConfigState_t *nextLedConfigState;

// Synchronization
extern SemaphoreHandle_t framebuffer_mutex;

// Function prototypes
BaseType_t framebuffer_init(uint8_t num_edges, uint32_t *num_led_per_edge);
BaseType_t framebuffer_cleanup(void);
void framebuffer_swap(void);
void framebuffer_clear_next(void);

#endif // FRAMEBUFFER_H