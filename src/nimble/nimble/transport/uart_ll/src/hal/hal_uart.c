#include "hal/hal_uart.h"

#include <driver/uart.h>

static TaskHandle_t hal_uart_read_poll_handle;
typedef struct {
    int port;
    int (*read_byte_callback)(void*, uint8_t);
} hal_uart_read_poll_args_t;
static void hal_uart_read_poll(void *_args)
{
    static uint8_t buf[128];
    hal_uart_read_poll_args_t *args = (hal_uart_read_poll_args_t*)_args;
    int port = args->port;
    int (*read_byte_callback)(void*, uint8_t) = args->read_byte_callback;

    assert(read_byte_callback != NULL);

    while (1) {
        int buffer_length = 0;
        ESP_ERROR_CHECK(uart_get_buffered_data_len(port, (size_t*)&buffer_length));
        while (buffer_length > 0) {
            int read_length = buffer_length > sizeof(buf) ? sizeof(buf) : buffer_length;
            read_length = uart_read_bytes(port, buf, read_length, 1000 / portTICK_PERIOD_MS);
            if (read_length > 0) {
            
            
                for (int i = 0; i < read_length; ++i) {
                    read_byte_callback(NULL, buf[i]);
                }
            }
            buffer_length -= read_length;
        
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

static volatile bool hal_uart_write_poll_started = false;
static TaskHandle_t hal_uart_write_poll_handle;
typedef struct {
    int port;
    int (*write_byte_get_value)(void*);
} hal_uart_write_poll_args_t;
static void hal_uart_write_poll(void *_args)
{
    static uint8_t buf[128];
    static int buf_size = 0;
    hal_uart_write_poll_args_t *args = (hal_uart_write_poll_args_t*)_args;
    int port = args->port;
    int (*write_byte_get_value)(void*) = args->write_byte_get_value;

    assert(write_byte_get_value != NULL);
    
    while (1) {
        if (hal_uart_write_poll_started != true) {
            vTaskDelay(50 / portTICK_PERIOD_MS);
            continue;
        }
        hal_uart_write_poll_started = false;
        
        int val;
        buf_size = 0;
    
        while ((val = write_byte_get_value(NULL)) >= 0) {
            buf[buf_size++] = val;
            if (buf_size >= sizeof(buf)) {
            
                uart_write_bytes(port, buf, buf_size);
            
                buf_size = 0;
            }
        }
        
        if (buf_size > 0) {
            uart_write_bytes(port, buf, buf_size);
        
            buf_size = 0;
        
        }
    }
}

static hal_uart_read_poll_args_t read_func_args;
static hal_uart_write_poll_args_t write_func_args;
int hal_uart_init_cbs(int port, int (*tx_char)(void *arg), void (*tx_done)(void *arg), int (*rx_char)(void *arg, uint8_t byte), void *args)
{
    read_func_args = (hal_uart_read_poll_args_t) {
        .port = port,
        .read_byte_callback = rx_char
    };

    write_func_args = (hal_uart_write_poll_args_t) {
        .port = port,
        .write_byte_get_value = tx_char
    };
    return 0;
}

static bool hal_uart_isopen = false;
int hal_uart_config(int port, int baud, int data_bits, int stop_bits, enum hal_uart_parity parity, enum hal_uart_flow_ctl flow_ctrl)
{
    uart_word_length_t data_bits_esp32;
    switch (data_bits)
    {
        case 5:
            data_bits_esp32 = UART_DATA_5_BITS;
            break;
        case 6:
            data_bits_esp32 = UART_DATA_6_BITS;
            break;
        case 7:
            data_bits_esp32 = UART_DATA_7_BITS;
            break;
        case 8:
        default:
            data_bits_esp32 = UART_DATA_8_BITS;
            break;
    }

    uart_parity_t parity_esp32 = (uart_parity_t)parity;

    uart_stop_bits_t stop_bits_esp32;
    switch (stop_bits)
    {
        case 15:
            stop_bits_esp32 = UART_STOP_BITS_1_5;
            break;
        case 2:
            stop_bits_esp32 = UART_STOP_BITS_2;
            break;
        case 1:
        default:
            stop_bits_esp32 = UART_STOP_BITS_1;
            break;
    }

    uart_hw_flowcontrol_t flow_ctrl_esp32 = (uart_hw_flowcontrol_t)flow_ctrl;

    uart_config_t uart_config = {
        .baud_rate = baud,
        .data_bits = data_bits_esp32,
        .parity    = parity_esp32,
        .stop_bits = stop_bits_esp32,
        .flow_ctrl = flow_ctrl_esp32,
        .rx_flow_ctrl_thresh = 122,
        .source_clk = UART_SCLK_APB
    };
    ESP_ERROR_CHECK(uart_param_config(port, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(port, ESP32_NIMBLE_UART_TX_PIN, ESP32_NIMBLE_UART_RX_PIN, ESP32_NIMBLE_UART_RTS_PIN, ESP32_NIMBLE_UART_CTS_PIN));
    // static QueueHandle_t uart_queue;
    // ESP_ERROR_CHECK(uart_driver_install(port, ESP32_NIMBLE_UART_BUF_SIZE, ESP32_NIMBLE_UART_BUF_SIZE, 10, &uart_queue, 0));
    ESP_ERROR_CHECK(uart_driver_install(port, ESP32_NIMBLE_UART_BUF_SIZE, 0, 0, NULL, 0));
    
    hal_uart_isopen = true;

    return 0;
}

int hal_uart_close(int port)
{
    if (hal_uart_isopen) {
        if (uart_is_driver_installed(port)) {
            ESP_ERROR_CHECK(uart_driver_delete(port));
        }

        vTaskDelete(hal_uart_read_poll_handle);

        vTaskDelete(hal_uart_write_poll_handle);
        hal_uart_write_poll_started = false;

        hal_uart_isopen = false;
    }
    return 0;
}

void hal_uart_start_tx(int port)
{
    if (hal_uart_write_poll_started == false) {
        xTaskCreate(hal_uart_read_poll, "hal_uart_read_poll", 8 * 1024, &read_func_args, 1, &hal_uart_read_poll_handle);
        xTaskCreate(hal_uart_write_poll, "hal_uart_write_poll", 8 * 1024, &write_func_args, 2, &hal_uart_write_poll_handle);
    }
    hal_uart_write_poll_started = true;
    return;
}
