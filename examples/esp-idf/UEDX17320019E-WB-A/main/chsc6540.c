/*
 * CHSC6540 — logic from arduino/lvgl_v8_port/touch.h (Wire → ESP-IDF I2C)
 */
#include "chsc6540.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_check.h"
#include "esp_log.h"

static const char *TAG = "chsc6540";

#define CHSC6540_ADDR        0x2E
#define CNT_FOR_POINT        3
#define CT_MAX_TOUCH         1
#define CHSC6540_CONFIG_SIZE (CNT_FOR_POINT * CT_MAX_TOUCH)

static i2c_port_t s_port = I2C_NUM_0;
static uint8_t s_last_ret;

static void chsc6540_hw_reset(gpio_num_t pin_int, gpio_num_t pin_rst)
{
    gpio_set_direction(pin_int, GPIO_MODE_OUTPUT);
    gpio_set_direction(pin_rst, GPIO_MODE_OUTPUT);
    gpio_set_level(pin_int, 0);
    gpio_set_level(pin_rst, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(pin_int, 1);
    vTaskDelay(pdMS_TO_TICKS(1));
    gpio_set_level(pin_rst, 1);
    vTaskDelay(pdMS_TO_TICKS(5));
    gpio_set_level(pin_int, 0);
    vTaskDelay(pdMS_TO_TICKS(50));
    gpio_set_direction(pin_int, GPIO_MODE_INPUT);
}

static uint8_t read_data(uint8_t *buf, uint8_t size)
{
    esp_err_t err = i2c_master_write_read_device(s_port, CHSC6540_ADDR, NULL, 0, buf, size, pdMS_TO_TICKS(50));
    if (err != ESP_OK) {
        return 0;
    }
    vTaskDelay(pdMS_TO_TICKS(2));
    return 1;
}

esp_err_t chsc6540_init(i2c_port_t port,
                        gpio_num_t pin_sda,
                        gpio_num_t pin_scl,
                        gpio_num_t pin_int,
                        gpio_num_t pin_rst)
{
    s_port = port;
    s_last_ret = 0;

    const i2c_config_t cfg = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = pin_sda,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = pin_scl,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 400000,
    };
    ESP_RETURN_ON_ERROR(i2c_param_config(port, &cfg), TAG, "i2c_param_config");
    ESP_RETURN_ON_ERROR(i2c_driver_install(port, cfg.mode, 0, 0, 0), TAG, "i2c_driver_install");

    chsc6540_hw_reset(pin_int, pin_rst);
    ESP_LOGI(TAG, "CHSC6540 ready (SDA=%d SCL=%d INT=%d RST=%d)", pin_sda, pin_scl, pin_int, pin_rst);
    return ESP_OK;
}

uint8_t chsc6540_scan(uint16_t *x, uint16_t *y)
{
    uint8_t buf[CHSC6540_CONFIG_SIZE] = {0};
    uint8_t x_h8 = 0, y_h8 = 0, ret = 0, ret2 = 0;
    uint16_t tx = 0, ty = 0, tpx = 0, tpy = 0;

    ret = read_data(buf, CHSC6540_CONFIG_SIZE);
    if (ret == 1) {
        y_h8 = buf[0] >> 8;
        x_h8 = (buf[0] >> 7);
        if (x_h8 == 1) {
            ty = 256 + buf[2];
        } else {
            ty = buf[2];
        }
        if (y_h8 == 1) {
            tx = 256 + buf[1];
        } else {
            tx = buf[1];
        }
        if (CHSC6540_TP_LEVEL) {
            tpx = ty;
            tpy = tx;
        } else {
            tpx = tx;
            tpy = ty;
        }
        if (CHSC6540_X_MIRRORING) {
            *x = CHSC6540_TOUCH_WIDTH - tpx;
        } else {
            *x = tpx;
        }
        if (CHSC6540_Y_MIRRORING) {
            *y = CHSC6540_TOUCH_HEIGHT - tpy;
        } else {
            *y = tpy;
        }
        ret2 = 1;
    }

    uint8_t stable = (ret2 == 1 && s_last_ret == 1) ? 1 : 0;
    s_last_ret = ret2;
    return stable;
}
