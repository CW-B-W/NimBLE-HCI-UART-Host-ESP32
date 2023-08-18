#ifndef H_HAL_UART_
#define H_HAL_UART_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum hal_uart_parity {
    HAL_UART_PARITY_NONE = 0x0, // uart_parity_t.UART_PARITY_DISABLE
    HAL_UART_PARITY_EVEN = 0x2, // uart_parity_t.UART_PARITY_EVEN
    HAL_UART_PARITY_ODD  = 0x3  // uart_parity_t.UART_PARITY_ODD
};

enum hal_uart_flow_ctl {
    HAL_UART_FLOW_CTL_NONE    = 0x0, // uart_hw_flowcontrol_t.UART_HW_FLOWCTRL_DISABLE
    HAL_UART_FLOW_CTL_RTS_CTS = 0x3  // uart_hw_flowcontrol_t.UART_HW_FLOWCTRL_CTS_RTS
};

#define MYNEWT_VAL_BLE_TRANSPORT_UART_PORT                 (1)
#define MYNEWT_VAL_BLE_TRANSPORT_UART_BAUDRATE             (115200)
#define MYNEWT_VAL_BLE_TRANSPORT_UART_DATA_BITS            (8)
#define MYNEWT_VAL_BLE_TRANSPORT_UART_STOP_BITS            (1)
#define MYNEWT_VAL_BLE_TRANSPORT_UART_PARITY__odd          (0)
#define MYNEWT_VAL_BLE_TRANSPORT_UART_PARITY__even         (0)
#define MYNEWT_VAL_BLE_TRANSPORT_UART_FLOW_CONTROL__rtscts (0)

/* Ref: https://mynewt.apache.org/latest/os/modules/hal/hal_uart/hal_uart.html#c.hal_uart_init_cbs */
int  hal_uart_init_cbs(int port, int (*tx_char)(void *arg), void (*tx_done)(void *arg), int (*rx_char)(void *arg, uint8_t byte), void *args);
int  hal_uart_config(int port, int baud, int data_bits, int stop_bits, enum hal_uart_parity parity, enum hal_uart_flow_ctl flow_ctrl);
int  hal_uart_close(int port);
void hal_uart_start_tx(int port);


#define ESP32_NIMBLE_UART_TX_PIN                (5)
#define ESP32_NIMBLE_UART_RX_PIN                (18)
#define ESP32_NIMBLE_UART_RTS_PIN               (23)
#define ESP32_NIMBLE_UART_CTS_PIN               (19)
#define ESP32_NIMBLE_UART_BUF_SIZE              (2 * 128)

#ifdef __cplusplus
}
#endif

#endif
