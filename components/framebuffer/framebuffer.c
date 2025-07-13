#include "framebuffer.h"
#include <stdio.h>
#include <string.h>

// define global pointers 
LedEdgeConfigState_t* currentLedConfigState;
LedEdgeConfigState_t* nextLedConfigState ;

//framebuffer intialization
BaseType_t framebuffer_init(uint8_t num_edges, uint32_t* num_led_per_edge){
    // allocate memory for global pointers
    currentLedConfigState = (LedEdgeConfigState_t*) malloc(sizeof(LedEdgeConfigState_t));
    nextLedConfigState = (LedEdgeConfigState_t*) malloc(sizeof(LedEdgeConfigState_t)); 

    if(currentLedConfigState==NULL || nextLedConfigState==NULL){
        return pdFAIL;
    }

    //intilize global structure members
    currentLedConfigState->num_edges = num_edges;
    nextLedConfigState->num_edges = num_edges;

    //allocate memory for LedState_t** data
    currentLedConfigState->data = pvPortMalloc(sizeof(LedState_t*)* num_edges);
    nextLedConfigState->data = pvPortMalloc(sizeof(LedState_t*)* num_edges);

    //allocate memory to 2d array pointers of global struct pointers
    for(int i=0; i<num_edges; i++){
        currentLedConfigState->data[i] = pvPortMalloc(sizeof(LedState_t) * num_led_per_edge[i]);
        nextLedConfigState->data[i] = pvPortMalloc(sizeof(LedState_t) * num_led_per_edge[i]);
        if (!currentLedConfigState->data[i] || !nextLedConfigState->data[i]){
            return pdFAIL;
        }

        memset(currentLedConfigState->data[i], 0, num_leds_per_edge[i] * sizeof(LedState_t));
        memset(nextLedConfigState->data[i], 0, num_leds_per_edge[i] * sizeof(LedState_t));
    }
    return pdPASS;
}