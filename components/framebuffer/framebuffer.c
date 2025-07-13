#include "framebuffer.h"
#include <stdio.h>
#include <string.h>

// Global frame buffers
LedEdgeConfigState_t *currentLedConfigState = NULL;
LedEdgeConfigState_t *nextLedConfigState = NULL;

// Synchronization
SemaphoreHandle_t framebuffer_mutex = NULL;

// Initialize framebuffer system
BaseType_t framebuffer_init(uint8_t num_edges, uint32_t *num_led_per_edge)
{
    // Create mutex for synchronization
    framebuffer_mutex = xSemaphoreCreateMutex();
    if (framebuffer_mutex == NULL) {
        return pdFAIL;
    }

    // Allocate memory for both frame buffers
    currentLedConfigState = (LedEdgeConfigState_t *)pvPortMalloc(sizeof(LedEdgeConfigState_t));
    nextLedConfigState = (LedEdgeConfigState_t *)pvPortMalloc(sizeof(LedEdgeConfigState_t));

    if (currentLedConfigState == NULL || nextLedConfigState == NULL) {
        return pdFAIL;
    }

    // Initialize structure members
    currentLedConfigState->num_edges = num_edges;
    nextLedConfigState->num_edges = num_edges;

    // Allocate memory for num_led_per_edge arrays
    currentLedConfigState->num_led_per_edge = (uint32_t *)pvPortMalloc(sizeof(uint32_t) * num_edges);
    nextLedConfigState->num_led_per_edge = (uint32_t *)pvPortMalloc(sizeof(uint32_t) * num_edges);

    if (!currentLedConfigState->num_led_per_edge || !nextLedConfigState->num_led_per_edge) {
        return pdFAIL;
    }

    // Copy LED counts per edge
    for (int i = 0; i < num_edges; i++) {
        currentLedConfigState->num_led_per_edge[i] = num_led_per_edge[i];
        nextLedConfigState->num_led_per_edge[i] = num_led_per_edge[i];
    }

    // Allocate memory for LED data arrays
    currentLedConfigState->data = (LedState_t **)pvPortMalloc(sizeof(LedState_t *) * num_edges);
    nextLedConfigState->data = (LedState_t **)pvPortMalloc(sizeof(LedState_t *) * num_edges);
    
    if (!currentLedConfigState->data || !nextLedConfigState->data) {
        return pdFAIL;
    }

    // Allocate memory for each edge's LED array
    for (int i = 0; i < num_edges; i++) {
        currentLedConfigState->data[i] = (LedState_t *)pvPortMalloc(sizeof(LedState_t) * num_led_per_edge[i]);
        nextLedConfigState->data[i] = (LedState_t *)pvPortMalloc(sizeof(LedState_t) * num_led_per_edge[i]);
        
        if (!currentLedConfigState->data[i] || !nextLedConfigState->data[i]) {
            return pdFAIL;
        }

        // Initialize with zeros (all LEDs off)
        memset(currentLedConfigState->data[i], 0, num_led_per_edge[i] * sizeof(LedState_t));
        memset(nextLedConfigState->data[i], 0, num_led_per_edge[i] * sizeof(LedState_t));
    }

    return pdPASS;
}

// Clean up framebuffer system
BaseType_t framebuffer_cleanup(void) 
{
    if (framebuffer_mutex) {
        vSemaphoreDelete(framebuffer_mutex);
        framebuffer_mutex = NULL;
    }

    LedEdgeConfigState_t* states[] = {currentLedConfigState, nextLedConfigState};
    LedEdgeConfigState_t** statePointers[] = {&currentLedConfigState, &nextLedConfigState};
    
    for (int s = 0; s < 2; s++) {
        if (states[s] != NULL) {
            if (states[s]->data != NULL) {
                for (int i = 0; i < states[s]->num_edges; i++) {
                    if (states[s]->data[i] != NULL) {
                        vPortFree(states[s]->data[i]);
                    }
                }
                vPortFree(states[s]->data);
            }
            
            // Free the num_led_per_edge array
            if (states[s]->num_led_per_edge != NULL) {
                vPortFree(states[s]->num_led_per_edge);
            }
            
            vPortFree(states[s]);
            *(statePointers[s]) = NULL;
        }
    }
    
    return pdPASS;
}

// Swap current and next frame buffers 
void framebuffer_swap(void)
{
    if (xSemaphoreTake(framebuffer_mutex, portMAX_DELAY) == pdTRUE) {
        LedEdgeConfigState_t *temp = currentLedConfigState;
        currentLedConfigState = nextLedConfigState;
        nextLedConfigState = temp;
        xSemaphoreGive(framebuffer_mutex);
        
    }
}

// Clear the next frame buffer
void framebuffer_clear_next(void)
{
    if (!nextLedConfigState) return;
    
    for (int i = 0; i < nextLedConfigState->num_edges; i++) {
        memset(nextLedConfigState->data[i], 0, 
               nextLedConfigState->num_led_per_edge[i] * sizeof(LedState_t));
    }
}