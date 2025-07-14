#ifndef MAIN_H
#define MAIN_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_err.h"
#include "esp_log.h"

// Task configuration constants
#define LED_RENDER_TASK_STACK_SIZE      4096
#define LED_DISPLAY_TASK_STACK_SIZE     4096
#define LED_RENDER_TASK_PRIORITY        5
#define LED_DISPLAY_TASK_PRIORITY       4
#define LED_RENDER_TASK_CORE            1       // Core 1 for render task
#define LED_DISPLAY_TASK_CORE           0       // Core 0 for display task

// Timing constants
#define LED_RENDER_PERIOD_MS            50      // 20 FPS
#define LED_DISPLAY_TIMEOUT_MS          100     // Timeout for waiting for render notification

// Task handles - global declarations
extern TaskHandle_t render_engine_task_handle;
extern TaskHandle_t physical_led_task_handle;

;
//get time in ms
extern uint32_t get_current_time_ms(void);

extern LEDController* led_controller;

// Function declarations for LED tasks
esp_err_t led_tasks_init(void);
void led_tasks_cleanup(void);
void led_render_task(void *param);
void led_display_task(void *param);



// Task notification values
#define LED_FRAME_READY_NOTIFICATION    0x01

// Task names
#define LED_RENDER_TASK_NAME           "led_render"
#define LED_DISPLAY_TASK_NAME          "led_display"


#endif // MAIN_H