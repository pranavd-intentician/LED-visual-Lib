idf_component_register(
    SRCS
        "main.c"
    INCLUDE_DIRS "." 
    REQUIRES render_engine physical_led_updater framebuffer nvs_flash esp_timer


)
