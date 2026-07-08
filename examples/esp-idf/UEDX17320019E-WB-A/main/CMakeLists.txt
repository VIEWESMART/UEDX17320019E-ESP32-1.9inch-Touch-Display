idf_component_register(
    SRCS "main.c" "board.c" "chsc6540.c"
    INCLUDE_DIRS "."
    REQUIRES esp_lcd esp_lcd_gc9a01 esp_lvgl_port lvgl driver esp_timer button
)
