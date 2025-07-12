#include "framebuffer.h"
#include <stdio.h>

// Static (private) variables
static FrameBuffer_t g_frame_buffer;
QueueHandle_t g_matrix_queue= xQueueCreate(10, sizeof(Matrix2D_t));  // Global queue handle
Semapphorehandle_t queueSemaphore = xSemaphoreCreateBinary();
// Initialize frame buffer and queue
// BaseType_t framebuffer_init(void) {
//     g_frame_buffer.max_frames = FRAME_BUFFER_SIZE;
//     g_frame_buffer.write_index = 0;
//     g_frame_buffer.read_index = 0;
//     g_frame_buffer.count = 0;
    
//     // Allocate memory for frame buffer
//     g_frame_buffer.matrices = (Matrix2D_t*)pvPortMalloc(FRAME_BUFFER_SIZE * sizeof(Matrix2D_t));
//     if (g_frame_buffer.matrices == NULL) {
//         printf("Error: Failed to allocate frame buffer memory\n");
//         return pdFAIL;
//     }
    
//     // Initialize each matrix in the buffer
//     for (int i = 0; i < FRAME_BUFFER_SIZE; i++) {
//         g_frame_buffer.matrices[i].data = allocate_2d_matrix(MATRIX_WIDTH, MATRIX_HEIGHT);
//         if (g_frame_buffer.matrices[i].data == NULL) {
//             printf("Error: Failed to allocate matrix %d\n", i);
//             // Clean up previously allocated matrices
//             for (int j = 0; j < i; j++) {
//                 free_2d_matrix(g_frame_buffer.matrices[j].data, MATRIX_WIDTH);
//             }
//             vPortFree(g_frame_buffer.matrices);
//             return pdFAIL;
//         }
//         g_frame_buffer.matrices[i].width = MATRIX_WIDTH;
//         g_frame_buffer.matrices[i].height = MATRIX_HEIGHT;
//         g_frame_buffer.matrices[i].timestamp = 0;
//         g_frame_buffer.matrices[i].frame_id = 0;
//     }
    
//     // Create mutex for thread safety
//     g_frame_buffer.mutex = xSemaphoreCreateMutex();
//     if (g_frame_buffer.mutex == NULL) {
//         printf("Error: Failed to create frame buffer mutex\n");
//         framebuffer_cleanup();
//         return pdFAIL;
//     }
    
//     // Create queue
//     g_matrix_queue = xQueueCreate(QUEUE_LENGTH, sizeof(QueueMessage_t));
//     if (g_matrix_queue == NULL) {
//         printf("Error: Failed to create matrix queue\n");
//         framebuffer_cleanup();
//         return pdFAIL;
//     }
    
//     printf("Frame buffer initialized successfully\n");
//     return pdPASS;
// }

// Write matrix to frame buffer
BaseType_t framebuffer_write(float** source_matrix, uint32_t timestamp, uint8_t frame_id) {
    BaseType_t result = pdFAIL;
    if (uxQueueSpacesAvailable(matrixQueue) == 0) {
        // Queue is full
        printf("Queue is full\n");
        
    }
    return result;
}

// Read matrix from frame buffer
BaseType_t framebuffer_read(Matrix2D_t** matrix_ptr) {
    BaseType_t result = pdFAIL;
    
    // Take mutex with timeout
    if (xSemaphoreTake(g_frame_buffer.mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        
        // Check if buffer has data
        if (g_frame_buffer.count > 0) {
            *matrix_ptr = &g_frame_buffer.matrices[g_frame_buffer.read_index];
            
            // Update buffer indices
            g_frame_buffer.read_index = (g_frame_buffer.read_index + 1) % g_frame_buffer.max_frames;
            g_frame_buffer.count--;
            
            result = pdPASS;
        }
        
        xSemaphoreGive(g_frame_buffer.mutex);
    }
    
    return result;
}

// Get buffer status
void framebuffer_get_status(uint8_t* count, uint8_t* max_frames) {
    if (xSemaphoreTake(g_frame_buffer.mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        *count = g_frame_buffer.count;
        *max_frames = g_frame_buffer.max_frames;
        xSemaphoreGive(g_frame_buffer.mutex);
    }
}

// Get queue handle
QueueHandle_t framebuffer_get_queue(void) {
    return g_matrix_queue;
}

// Cleanup frame buffer
void framebuffer_cleanup(void) {
    if (g_frame_buffer.matrices != NULL) {
        for (int i = 0; i < g_frame_buffer.max_frames; i++) {
            free_2d_matrix(g_frame_buffer.matrices[i].data, MATRIX_WIDTH);
        }
        vPortFree(g_frame_buffer.matrices);
        g_frame_buffer.matrices = NULL;
    }
    
    if (g_frame_buffer.mutex != NULL) {
        vSemaphoreDelete(g_frame_buffer.mutex);
        g_frame_buffer.mutex = NULL;
    }
    
    if (g_matrix_queue != NULL) {
        vQueueDelete(g_matrix_queue);
        g_matrix_queue = NULL;
    }
    
    printf("Frame buffer cleanup completed\n");
}