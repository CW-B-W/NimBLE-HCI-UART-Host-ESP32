#include "hal/hal_uart.h"

#include <driver/uart.h>

static int (*set_rx_byte)(void *arg, uint8_t byte);
static int (*get_tx_byte)(void *arg);

static QueueHandle_t uart_queue;

static int uart_port = -1;

static TaskHandle_t uart_read_poll_func_handle;
static void uart_read_poll_func(void *_args);

static TaskHandle_t uart_write_poll_func_handle;
static void uart_write_poll_func(void *_args);

static SemaphoreHandle_t uart_write_start_sem;

static void uart_read_poll_func(void *_args)
{
    static uint8_t buf[128];
    int buf_size = 0;

    uart_event_t event;

    assert(set_rx_byte != NULL);

    while (1) {
        if(xQueueReceive(uart_queue, (void * )&event, (TickType_t)portMAX_DELAY)) {
            switch(event.type) {
                case UART_DATA: {
                    do {
                        ESP_ERROR_CHECK(uart_get_buffered_data_len(uart_port, (size_t*)&buf_size));
                        int bytes_to_read = buf_size > sizeof(buf) ? sizeof(buf) : buf_size;
                        int bytes_read = uart_read_bytes(uart_port, buf, bytes_to_read, (TickType_t)portMAX_DELAY);
                        if (bytes_read > 0) {
                            for (int i = 0; i < bytes_read; ++i) {
                                set_rx_byte(NULL, buf[i]);
                            }
                        }
                        buf_size -= bytes_read;
                    } while (buf_size > 0);
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
    int buf_size = 0;

    assert(get_tx_byte != NULL);
    
    while (1) {
        xSemaphoreTake(uart_write_start_sem, portMAX_DELAY);
        
        int val;
        buf_size = 0;
    
        while ((val = get_tx_byte(NULL)) >= 0) {
            buf[buf_size++] = val;
            if (buf_size >= sizeof(buf)) {
                uart_write_bytes(uart_port, buf, buf_size);
                buf_size = 0;
            }
        }
        
        if (buf_size > 0) {
            uart_write_bytes(uart_port, buf, buf_size);
            buf_size = 0;
        }
    }
}

int hal_uart_init_cbs(int port, int (*tx_char)(void *arg), void (*tx_done)(void *arg), int (*rx_char)(void *arg, uint8_t byte), void *args)
{
    set_rx_byte = rx_char;
    get_tx_byte = tx_char;
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

    uart_write_start_sem = xSemaphoreCreateBinary();

    xTaskCreate(uart_read_poll_func, "uart_read_poll_func", 8 * 1024, NULL, 1, &uart_read_poll_func_handle);
    xTaskCreate(uart_write_poll_func, "uart_write_poll_func", 8 * 1024, NULL, 2, &uart_write_poll_func_handle);

    uart_port = port;

    return 0;
}

int hal_uart_close(int port)
{
    if (uart_port != -1) {
        vTaskDelete(uart_read_poll_func_handle);
        vTaskDelete(uart_write_poll_func_handle);

        vSemaphoreDelete(uart_write_start_sem);

        if (uart_is_driver_installed(port)) {
            ESP_ERROR_CHECK(uart_driver_delete(port));
        }

        uart_port = -1;
    }
    return 0;
}

void hal_uart_start_tx(int port)
{
    xSemaphoreGive(uart_write_start_sem);
    return;
}
