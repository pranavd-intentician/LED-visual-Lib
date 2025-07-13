#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include "FreeRTOS.h"


typedef struct
{
    uint8_t r; // red_color
    uint8_t g; // green_color
    uint8_t b; // blue_color
    uint8_t intensity;
} LedState_t;

typedef struct
{
    LedState_t **data; // pointer to 2D array
    uint8_t num_edges; // total edges
    uint *num_leds_per_edge;
} LedEdgeConfigState_t;

// global pointera to framebuffer
extern LedEdgeConfigState_t* currentLedConfigState;
extern LedEdgeConfigState_t* nextLedConfigState;

// Function declarations
BaseType_t framebuffer_init(uint8_t num_edges, uint32_t* num_led_per_edge);

#endif