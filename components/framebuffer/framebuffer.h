#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H
// Configuration constants
#define FRAME_BUFFER_SIZE 5     // Number of frames to buffer
#define QUEUE_LENGTH 10         // Queue length
#define MATRIX_WIDTH 64         // Your matrix dimensions
#define MATRIX_HEIGHT 64

#include "FreeRTOS.h"
#include "semphr.h"
#include "queue.h"
// Global queue handle (extern declaration)
extern QueueHandle_t g_matrix_queue;

// Define your 2D matrix structure
typedef struct {
    float** data;           // 2D array pointer
    uint16_t width;         // X dimension
    uint16_t height;        // Y dimension
    uint32_t timestamp;     // Frame timestamp
    uint8_t frame_id;       // Frame identifier
} Matrix2D_t;



// Function declarations
BaseType_t framebuffer_init(void);


#endif