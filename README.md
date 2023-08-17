# NimBLE-HCI-UART-Host-ESP32

# Introduction
![hci](https://github.com/CW-B-W/NimBLE-HCI-UART-Host-ESP32/assets/76680670/5cdd2fca-0683-48ee-be4b-a326e686180f)


# Quick start guide
## Setup up ESP32 Bluetooth Host only
1. Use VS Code with extension PlatformIO to open this project folder
2. Copy `examples/NimBLE-Arduino.cpp` into `src` folder
3. Build and upload the ESP32 firmware with PlatformIO
4. Follow [Connect the two ESP32](#connect-the-two-esp32)
5. Try to connect, the device name is `NimBLE-ESP32`

## Setup ESP32 Bluetooth Controller only
Use [ESP32-Controller-Only](https://github.com/CW-B-W/ESP32-Controller-Only)
1. Use VS Code with extension PlatformIO to open the `ESP32-Controller-Only` project folder
2. Build and upload the ESP32 firmware with PlatformIO
3. [Optional] Test whether it succeeded  
    Send reset command to the ESP32 bluetooth controller
    ```
    0x01 0x03 0x0C 0x00
    ```
    If it succeeded, we should get the response  
    (Note that the Serial Monitor in VS Code cannot properly display this response in ASCII)
    ```
    0x04 0x0e 0x04 0x05 0x03 0x0c 0x00
    ```

## Connect the two ESP32
![archesp](https://github.com/CW-B-W/NimBLE-HCI-UART-Host-ESP32/assets/76680670/75d873bf-b855-4d0e-a81a-b8ab84f62ee2)
### ESP32 Bluetooth Host
On the `Host Only ESP32`, we use [pin 18 as the Rx pin, pin 5 as the Tx pin, pin 23 as the RTS pin and pin 19 as the CTS pin](https://github.com/CW-B-W/NimBLE-HCI-UART-Host-ESP32/blob/nimble-v1.6.0rc/include/nimble/nimble/transport/uart_ll/include/hal/hal_uart.h#L36-L39).  
Because [CTS and RTS was disabled](https://github.com/CW-B-W/NimBLE-HCI-UART-Host-ESP32/blob/nimble-v1.6.0rc/include/nimble/nimble/transport/uart_ll/include/hal/hal_uart.h#L27C47-L27C47), we can ignore pin 23 and pin 19 on the `Host Only ESP32`.  
4 pins should be connected to the `Controller Only ESP32`, the Rx pin (pin 18) should be connected to the Tx pin of `Controller Only ESP32`, the Tx pin (pin 5) should be connected to the Rx pin of `Controller Only ESP32`, and the 5V and GND should also be connected to the 5V and GND of `Controller Only ESP32` as power for `Controller Only ESP32`.  
MicroUSB should be connected as the power supply of `Host Only ESP32`.

### ESP32 Bluetooth Controller
On the `Controller Only ESP32`, we use [pin 18 as the Rx pin, pin 5 as the Tx pin, pin 19 as the RTS pin and pin 23 as the CTS pin](https://github.com/CW-B-W/ESP32-Controller-Only/blob/master/src/controller_hci_uart_demo.c#L31).  
Because CTS and RTS was disabled, we short the RTS pin (pin 19) and CTS pin (pin 23) to GND.  
4 pins should be connected to the `Host Only ESP32`, the Rx pin (pin 18) should be connected to the Tx pin of `Controller Only ESP32`, the Tx pin (pin 5) should be connected to the Rx pin of `Controller Only ESP32`, and the 5V and GND should also be connected to the 5V and GND of `Controller Only ESP32` as power for `Controller Only ESP32`.  
MicroUSB is not necessary for `Controller Only ESP32`.

# How to port mynewt-NimBLE HCI UART (H4)
## What files are added in this project
Only two files are added to support HCI UART (H4) [hal_uart.h](https://github.com/CW-B-W/NimBLE-HCI-UART-Host-ESP32/blob/nimble-v1.6.0rc/include/nimble/nimble/transport/uart_ll/include/hal/hal_uart.h) and [hal_uart.c](https://github.com/CW-B-W/NimBLE-HCI-UART-Host-ESP32/blob/nimble-v1.6.0rc/src/nimble/nimble/transport/uart_ll/src/hal/hal_uart.c)

To see what files are added in this project, we can remove all the symbolic links with
```bash
# remove submodules
rm -r mynewt-nimble NimBLE-Arduino

# remove all symbolic links
rm -r $(find . -type l)

# remove all empty folders
while [ ! -z "$(find . -empty -type d)" ]
do
    rm -r $(find . -empty -type d)
done
```

## Slight modification of `mynewt-nimble`
To make `mynewt-nimble` and `NimBLE-Arduino` to run on ESP32, some patches, modifications and extra functions should be added.  
[What was modified in `mynewt-nimble`](https://github.com/apache/mynewt-nimble/compare/nimble_1_6_0_rc1_tag...CW-B-W:mynewt-nimble:v1.6.0rc-esp32-freertos)

# Use `NimBLE-Arduino` for simpler APIs
This project also includes [NimBLE-Arduino](https://github.com/h2zero/NimBLE-Arduino) for it simpler APIs.  
To make `NimBLE-Arduino` to run in this project, some modifications are applied.  
[What was modified in `NimBLE-Arduino`](https://github.com/h2zero/NimBLE-Arduino/compare/release/1.4...CW-B-W:NimBLE-Arduino:release/1.4)

# Project structure
In this project, symbolic links are used to link all the necessary files from `mynewt-nimble` to the `include` and `src` folders.  
To view all the linked files and directories, use 
```bash
find . -type l
```

The UART implementation files are placed at [`include/nimble/nimble/transport/uart_ll/include/hal`](https://github.com/CW-B-W/NimBLE-HCI-UART-Host-ESP32/tree/nimble-v1.6.0rc/include/nimble/nimble/transport/uart_ll/include/hal) and [`src/nimble/nimble/transport/uart_ll/src/hal/`](https://github.com/CW-B-W/NimBLE-HCI-UART-Host-ESP32/tree/nimble-v1.6.0rc/src/nimble/nimble/transport/uart_ll/src/hal)