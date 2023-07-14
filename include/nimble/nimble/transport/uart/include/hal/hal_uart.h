#ifndef H_HAL_UART_
#define H_HAL_UART_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MYNEWT_VAL_BLE_HCI_UART_PORT            (1)
#define MYNEWT_VAL_BLE_HCI_UART_BAUD            (115200)
#define MYNEWT_VAL_BLE_HCI_UART_DATA_BITS       (8)
#define MYNEWT_VAL_BLE_HCI_UART_STOP_BITS       (1)
#define MYNEWT_VAL_BLE_HCI_UART_PARITY          (0)
#define MYNEWT_VAL_BLE_HCI_UART_FLOW_CTRL       (0)

#define ESP32_NIMBLE_UART_TX_PIN                (5)
#define ESP32_NIMBLE_UART_RX_PIN                (18)
#define ESP32_NIMBLE_UART_RTS_PIN               (23)
#define ESP32_NIMBLE_UART_CTS_PIN               (19)
#define ESP32_NIMBLE_UART_BUF_SIZE              (2 * 1024)

/* Ref: https://mynewt.apache.org/latest/os/modules/hal/hal_uart/hal_uart.html#c.hal_uart_init_cbs */
int  hal_uart_init_cbs(int port, int (*tx_char)(void *arg), void (*tx_done)(void *arg), int (*rx_char)(void *arg, uint8_t byte), void *args);
int  hal_uart_config(int port, int baud, int data_bits, int stop_bits, int parity, int flow_ctrl);
int  hal_uart_close(int port);
void hal_uart_start_tx(int port);

#ifdef __cplusplus
}
#endif

#endif
