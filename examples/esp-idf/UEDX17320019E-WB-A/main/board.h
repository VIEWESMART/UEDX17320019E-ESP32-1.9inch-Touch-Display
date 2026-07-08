/**
 * 1.9" GC9A01 (SPI) + CHSC6540 (I2C) — 引脚与分辨率与 arduino/lvgl_v8_port 一致。
 */
#pragma once

#include "esp_err.h"
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 逻辑分辨率（与 Arduino_GFX GC9A01 构造参数 width=170, height=320 一致） */
#define EXAMPLE_LCD_H_RES 170
#define EXAMPLE_LCD_V_RES 320

/** 面板在控制器内的像素偏移（与 Arduino 的 col offset 35 一致） */
#define EXAMPLE_LCD_X_GAP 35
#define EXAMPLE_LCD_Y_GAP 0

/* SPI LCD（Arduino: DC=11 CS=10 SCK=12 MOSI=13 RST=1） */
#define EXAMPLE_LCD_SPI_HOST     SPI2_HOST
#define EXAMPLE_LCD_PIN_SCLK     GPIO_NUM_12
#define EXAMPLE_LCD_PIN_MOSI     GPIO_NUM_13
#define EXAMPLE_LCD_PIN_DC       GPIO_NUM_11
#define EXAMPLE_LCD_PIN_CS       GPIO_NUM_10
#define EXAMPLE_LCD_PIN_RST      GPIO_NUM_1
#define EXAMPLE_LCD_PIN_BL       GPIO_NUM_38
#define EXAMPLE_LCD_SPI_PCLK_HZ  (40 * 1000 * 1000)

/* CHSC6540 触摸 I2C（Arduino touch.h） */
#define EXAMPLE_TOUCH_I2C_NUM    I2C_NUM_0
#define EXAMPLE_TOUCH_I2C_SDA    GPIO_NUM_9
#define EXAMPLE_TOUCH_I2C_SCL    GPIO_NUM_46
#define EXAMPLE_TOUCH_PIN_INT    GPIO_NUM_8
#define EXAMPLE_TOUCH_PIN_RST    GPIO_NUM_3

/**
 * 1：将触摸 320×170 映射到 LVGL 170×320（横纵交换）。
 * 0：与 Arduino my_touchpad_read 一致，直接把 CHSC6540 的 x、y 交给 LVGL。
 */
#define BOARD_TOUCH_SWAP_AXES_FOR_LVGL 1

/**
 * 与 esp_lcd_panel_mirror / LVGL port rotation 一致；修正左右反字时把 MIRROR_X 置 1。
 * 触摸会按同样镜像补偿，避免点不准。
 */
#define BOARD_PANEL_MIRROR_X 1
#define BOARD_PANEL_MIRROR_Y 0

/**
 * RGB565 颜色校准（只改下面三个 0/1，每次改一项烧录对比）：
 * - PANEL_BGR_ORDER：MADCTL 里 BGR 位（红蓝对调时试）。
 * - LVGL_SWAP_BYTES：LVGL 刷图前是否交换 565 高低字节（多数 ESP SPI 屏为 1）。
 * - INVERT_COLOR：面板 INVON。整屏像底片（黑底应白、人像反色）说明应置 0。
 *   白底正常但高亮/主题蓝变黄：BGR=1 + SWAP=1 + INV=0（当前默认）。
 */
#define BOARD_LCD_PANEL_BGR_ORDER 1
#define BOARD_LCD_LVGL_SWAP_BYTES 1
#define BOARD_LCD_INVERT_COLOR    0

/* BOOT 键（可按板子修改） */
#define BOOT_BUTTON_NUM       GPIO_NUM_0
#define BUTTON_ACTIVE_LEVEL   0

esp_err_t app_lcd_init(void);
esp_err_t app_touch_init(void);
esp_err_t app_lvgl_init(void);

#ifdef __cplusplus
}
#endif
