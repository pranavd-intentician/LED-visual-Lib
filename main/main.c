#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>

#include "led_handler.h"

#define LED_STRIP_PIN 17  // Set WS2812 strip to GPIO 16

static const char *TAG = "MAIN";

void app_main(void) {
    ESP_LOGI(TAG, "Starting LED Test Application on GPIO %d", LED_STRIP_PIN);
    
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // Initialize LED handler (with GPIO override if your led_handler supports it)
    led_handler_init();
    
    // Wait a bit for initialization
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    ESP_LOGI(TAG, "Running LED demo sequence...");
    
    // Run the demo sequence
    led_handler_demo();
    
    ESP_LOGI(TAG, "Testing individual patterns...");
    
    led_config_t config = {
        .red = 255,
        .green = 0,
        .blue = 0,
        .intensity = 100,
        .speed_ms = 100,
        .duration_ms = 2000,
        .pattern = LED_PATTERN_STATIC
    };
    
    ESP_LOGI(TAG, "Static red pattern");
    led_handler_update_config(&config);
    vTaskDelay(pdMS_TO_TICKS(3000));
    
    config.pattern = LED_PATTERN_RAINBOW;
    config.speed_ms = 50;
    config.duration_ms = 5000;
    config.intensity = 150;
    
    ESP_LOGI(TAG, "Rainbow pattern");
    led_handler_update_config(&config);
    vTaskDelay(pdMS_TO_TICKS(6000));
    
    ESP_LOGI(TAG, "Custom gradient: Blue to Green");
    led_handler_set_custom_gradient(0, 0, 255, 0, 255, 0, 180);
    vTaskDelay(pdMS_TO_TICKS(3000));
    
    config.red = 255;
    config.green = 255;
    config.blue = 255;
    config.intensity = 200;
    config.speed_ms = 100;
    config.duration_ms = 4000;
    config.pattern = LED_PATTERN_TWINKLE;
    
    ESP_LOGI(TAG, "Twinkle white pattern");
    led_handler_update_config(&config);
    vTaskDelay(pdMS_TO_TICKS(5000));
    
    config.red = 0;
    config.green = 255;
    config.blue = 128;
    config.intensity = 200;
    config.speed_ms = 80;
    config.duration_ms = 6000;
    config.pattern = LED_PATTERN_BREATH;
    
    ESP_LOGI(TAG, "Breathing cyan pattern");
    led_handler_update_config(&config);
    vTaskDelay(pdMS_TO_TICKS(7000));
    
    ESP_LOGI(TAG, "Clearing all LEDs");
    led_handler_clear();
    
    while (1) {
        ESP_LOGI(TAG, "Main loop running - add your BLE handler here");
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
