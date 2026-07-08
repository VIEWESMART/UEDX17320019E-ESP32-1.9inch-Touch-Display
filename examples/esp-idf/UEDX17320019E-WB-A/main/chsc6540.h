/*
 * CHSC6540 capacitive touch (I2C), ported from arduino/lvgl_v8_port/touch.h
 */
#pragma once

#include <stdint.h>
#include "esp_err.h"
#include "driver/gpio.h"
#include "driver/i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Same mapping as Arduino reference: TP_LEVEL / X_MIRRORING / Y_MIRRORING */
#define CHSC6540_TOUCH_WIDTH   320
#define CHSC6540_TOUCH_HEIGHT  170
#define CHSC6540_TP_LEVEL        1
#define CHSC6540_X_MIRRORING    0
#define CHSC6540_Y_MIRRORING    1

esp_err_t chsc6540_init(i2c_port_t port,
                        gpio_num_t pin_sda,
                        gpio_num_t pin_scl,
                        gpio_num_t pin_int,
                        gpio_num_t pin_rst);

/**
 * @return 1 if a stable press (two consecutive hits, same as Arduino), else 0
 */
uint8_t chsc6540_scan(uint16_t *x, uint16_t *y);

#ifdef __cplusplus
}
#endif
