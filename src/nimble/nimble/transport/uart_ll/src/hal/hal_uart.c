#include "hal/hal_uart.h"

#include <driver/uart.h>

typedef struct {
    int port;
    int (*set_rx_byte)(void*, uint8_t);
} uart_read_poll_func_args_t;

typedef struct {
    int port;
    int (*get_tx_byte)(void*);
} uart_write_poll_func_args_t;


static QueueHandle_t uart_queue;

static bool uart_isopen = false;

static TaskHandle_t uart_read_poll_func_handle;
static uart_read_poll_func_args_t uart_read_poll_func_args;
static void uart_read_poll_func(void *_args);

static TaskHandle_t uart_write_poll_func_handle;
static uart_write_poll_func_args_t uart_write_poll_func_args;
static void uart_write_poll_func(void *_args);

static SemaphoreHandle_t uart_write_start_sem;

static void uart_read_poll_func(void *_args)
{
    uart_event_t event;
    static uint8_t buf[128];
    uart_read_poll_func_args_t *args = (uart_read_poll_func_args_t*)_args;
    int port = args->port;
    int (*set_rx_byte)(void*, uint8_t) = args->set_rx_byte;

    assert(set_rx_byte != NULL);

    while (1) {
        if(xQueueReceive(uart_queue, (void * )&event, (TickType_t)portMAX_DELAY)) {
            switch(event.type) {
                case UART_DATA: {
                    int buffer_length;
                    do {
                        ESP_ERROR_CHECK(uart_get_buffered_data_len(port, (size_t*)&buffer_length));
                        int bytes_to_read = buffer_length > sizeof(buf) ? sizeof(buf) : buffer_length;
                        int bytes_read = uart_read_bytes(port, buf, bytes_to_read, (TickType_t)portMAX_DELAY);
                        if (bytes_read > 0) {
                            for (int i = 0; i < bytes_read; ++i) {
                                set_rx_byte(NULL, buf[i]);
                            }
                        }
                        buffer_length -= bytes_read;
                    } while (buffer_length > 0);
                    break;
                }
                default: {
                    break;
                }
            }
        }
    }
}

static void uart_write_poll_func(void *_args)
{
    static uint8_t buf[128];
    static int buf_size = 0;
    uart_write_poll_func_args_t *args = (uart_write_poll_func_args_t*)_args;
    int port = args->port;
    int (*get_tx_byte)(void*) = args->get_tx_byte;

    assert(get_tx_byte != NULL);
    
    while (1) {
        xSemaphoreTake(uart_write_start_sem, portMAX_DELAY);
        
        int val;
        buf_size = 0;
    
        while ((val = get_tx_byte(NULL)) >= 0) {
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

int hal_uart_init_cbs(int port, int (*tx_char)(void *arg), void (*tx_done)(void *arg), int (*rx_char)(void *arg, uint8_t byte), void *args)
{
    uart_read_poll_func_args = (uart_read_poll_func_args_t) {
        .port = port,
        .set_rx_byte = rx_char
    };

    uart_write_poll_func_args = (uart_write_poll_func_args_t) {
        .port = port,
        .get_tx_byte = tx_char
    };
    return 0;
}

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
    ESP_ERROR_CHECK(uart_driver_install(port, ESP32_NIMBLE_UART_BUF_SIZE, ESP32_NIMBLE_UART_BUF_SIZE, 10, &uart_queue, 0));
    // ESP_ERROR_CHECK(uart_driver_install(port, ESP32_NIMBLE_UART_BUF_SIZE, 0, 0, NULL, 0));

    uart_write_start_sem = xSemaphoreCreateBinary();

    xTaskCreate(uart_read_poll_func, "uart_read_poll_func", 8 * 1024, &uart_read_poll_func_args, 1, &uart_read_poll_func_handle);
    xTaskCreate(uart_write_poll_func, "uart_write_poll_func", 8 * 1024, &uart_write_poll_func_args, 2, &uart_write_poll_func_handle);

    uart_isopen = true;

    return 0;
}

int hal_uart_close(int port)
{
    if (uart_isopen) {
        if (uart_is_driver_installed(port)) {
            ESP_ERROR_CHECK(uart_driver_delete(port));
        }

        vTaskDelete(uart_read_poll_func_handle);
        vTaskDelete(uart_write_poll_func_handle);

        vSemaphoreDelete(uart_write_start_sem);

        uart_isopen = false;
    }
    return 0;
}

void hal_uart_start_tx(int port)
{
    xSemaphoreGive(uart_write_start_sem);
    return;
}
