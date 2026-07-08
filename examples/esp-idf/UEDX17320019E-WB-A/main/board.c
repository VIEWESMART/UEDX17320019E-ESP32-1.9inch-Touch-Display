#include "board.h"
#include "chsc6540.h"
#include "esp_lvgl_port.h"
#include "esp_check.h"
#include "esp_log.h"

#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_gc9a01.h"
#include "lvgl.h"

static const char *TAG = "board";

static esp_lcd_panel_io_handle_t s_lcd_io = NULL;
static esp_lcd_panel_handle_t s_lcd_panel = NULL;
static lv_display_t *s_lvgl_disp = NULL;
static lv_indev_t *s_lvgl_touch = NULL;

static void chsc6540_lv_read(lv_indev_t *indev, lv_indev_data_t *data)
{
    uint16_t tx = 0, ty = 0;
    (void)indev;

    if (chsc6540_scan(&tx, &ty)) {
#if BOARD_TOUCH_SWAP_AXES_FOR_LVGL
        data->point.x = ty;
        data->point.y = tx;
#else
        data->point.x = tx;
        data->point.y = ty;
#endif
#if BOARD_PANEL_MIRROR_X
        data->point.x = (lv_coord_t)(EXAMPLE_LCD_H_RES - 1 - data->point.x);
#endif
#if BOARD_PANEL_MIRROR_Y
        data->point.y = (lv_coord_t)(EXAMPLE_LCD_V_RES - 1 - data->point.y);
#endif
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

esp_err_t app_lcd_init(void)
{
    ESP_LOGI(TAG, "SPI + GC9A01 init");

    gpio_config_t bk = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << EXAMPLE_LCD_PIN_BL,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&bk), TAG, "backlight gpio");

    const size_t max_dma = (size_t)EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES * sizeof(uint16_t);
    const spi_bus_config_t buscfg = {
        .sclk_io_num = EXAMPLE_LCD_PIN_SCLK,
        .mosi_io_num = EXAMPLE_LCD_PIN_MOSI,
        .miso_io_num = GPIO_NUM_NC,
        .quadwp_io_num = GPIO_NUM_NC,
        .quadhd_io_num = GPIO_NUM_NC,
        .max_transfer_sz = max_dma,
    };
    ESP_RETURN_ON_ERROR(spi_bus_initialize(EXAMPLE_LCD_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO), TAG, "spi_bus");

    const esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = EXAMPLE_LCD_PIN_DC,
        .cs_gpio_num = EXAMPLE_LCD_PIN_CS,
        .pclk_hz = EXAMPLE_LCD_SPI_PCLK_HZ,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)EXAMPLE_LCD_SPI_HOST, &io_config, &s_lcd_io),
                        TAG, "panel_io");

    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = EXAMPLE_LCD_PIN_RST,
#if BOARD_LCD_PANEL_BGR_ORDER
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
#else
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
#endif
        .bits_per_pixel = 16,
        .flags = {0},
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_gc9a01(s_lcd_io, &panel_config, &s_lcd_panel), TAG, "gc9a01");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_reset(s_lcd_panel), TAG, "reset");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_init(s_lcd_panel), TAG, "init");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_invert_color(s_lcd_panel, BOARD_LCD_INVERT_COLOR), TAG, "invert");
    /* 水平/垂直镜像由 lvgl_port 的 rotation 统一设置，勿在此处再调 esp_lcd_panel_mirror，否则会叠两次 */
    ESP_RETURN_ON_ERROR(esp_lcd_panel_set_gap(s_lcd_panel, EXAMPLE_LCD_X_GAP, EXAMPLE_LCD_Y_GAP), TAG, "gap");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_disp_on_off(s_lcd_panel, true), TAG, "disp on");

    gpio_set_level(EXAMPLE_LCD_PIN_BL, 1);
    return ESP_OK;
}

esp_err_t app_touch_init(void)
{
    return chsc6540_init(EXAMPLE_TOUCH_I2C_NUM,
                         EXAMPLE_TOUCH_I2C_SDA,
                         EXAMPLE_TOUCH_I2C_SCL,
                         EXAMPLE_TOUCH_PIN_INT,
                         EXAMPLE_TOUCH_PIN_RST);
}

esp_err_t app_lvgl_init(void)
{
    const lvgl_port_cfg_t lvgl_cfg = {
        .task_priority = 4,
        .task_stack = 6144 * 2,
        .task_affinity = -1,
        .task_max_sleep_ms = 500,
        .timer_period_ms = 5,
    };
    ESP_RETURN_ON_ERROR(lvgl_port_init(&lvgl_cfg), TAG, "lvgl_port_init");

    const uint32_t buf_lines = 40;
    uint32_t buff_size = EXAMPLE_LCD_H_RES * buf_lines;

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = s_lcd_io,
        .panel_handle = s_lcd_panel,
        .control_handle = NULL,
        .buffer_size = buff_size,
        .double_buffer = true,
        .hres = EXAMPLE_LCD_H_RES,
        .vres = EXAMPLE_LCD_V_RES,
        .monochrome = false,
#if LVGL_VERSION_MAJOR >= 9
        .color_format = LV_COLOR_FORMAT_RGB565,
#endif
        .rotation = {
            .swap_xy = false,
            .mirror_x = BOARD_PANEL_MIRROR_X,
            .mirror_y = BOARD_PANEL_MIRROR_Y,
        },
        .flags = {
            .buff_dma = true,
            .buff_spiram = false,
            .full_refresh = false,
            .direct_mode = false,
#if LVGL_VERSION_MAJOR >= 9
            .swap_bytes = BOARD_LCD_LVGL_SWAP_BYTES,
#endif
        },
    };

    s_lvgl_disp = lvgl_port_add_disp(&disp_cfg);
    if (s_lvgl_disp == NULL) {
        return ESP_FAIL;
    }

    lvgl_port_lock(0);
    s_lvgl_touch = lv_indev_create();
    lv_indev_set_type(s_lvgl_touch, LV_INDEV_TYPE_POINTER);
    lv_indev_set_display(s_lvgl_touch, s_lvgl_disp);
    lv_indev_set_read_cb(s_lvgl_touch, chsc6540_lv_read);
    lvgl_port_unlock();

    ESP_LOGI(TAG, "LVGL display + CHSC6540 indev OK");
    return ESP_OK;
}
