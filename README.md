# People Counting Gate System

Final project for the **Embedded System Lab (Fall 2026)**.

An IoT gate that counts the people who walk through it. An **STM32** reads the sensors and drives the actuators. It sends the data over UART to an **ESP32-S3 (NodeMCU)**, which works as the WiFi bridge and sends it to the cloud. A **web dashboard** shows the count.

> **Status:** early setup. The requirements, sensors, and stack are still TBD.

## System overview

```
 Sensors / actuators          UART                  WiFi
 ───────────────────►  STM32  ◄────►  ESP32-S3  ─────────►  Cloud  ◄────  Web dashboard
                      (NUCLEO-F411RE)  (NodeMCU)           (TBD)
```

## Repository structure

| Folder   | What it holds                                                         |
| -------- | --------------------------------------------------------------------- |
| `stm32/` | STM32CubeIDE project for the NUCLEO-F411RE (sensor reading, control)  |
| `mcu/`   | ESP32-S3 (NodeMCU) firmware: WiFi bridge between the STM32 and the cloud |
| `web/`   | Web dashboard / UI                                                    |

## Hardware

| Part             | Details                                     |
| ---------------- | ------------------------------------------- |
| Main MCU         | STM32F411RET6 on a **NUCLEO-F411RE** board  |
| WiFi bridge      | ESP32-S3 (NodeMCU)                          |
| Sensor/actuator 1 | TBD                                        |
| Sensor/actuator 2 | TBD                                        |
| Sensor/actuator 3 | TBD                                        |

### STM32 pin map (current CubeMX configuration)

| Pin      | Function                           |
| -------- | ---------------------------------- |
| PA2      | USART2_TX                          |
| PA3      | USART2_RX                          |
| PA5      | GPIO output (on-board LED LD2)     |
| PC13     | EXTI13 (on-board user button B1)   |
| PA13/PA14 | SWD debug (SWDIO / SWCLK)         |

> On the NUCLEO board, USART2 is connected to the ST-LINK virtual COM port by default. TBD: which UART will connect to the ESP32.

## Tech stack

| Part          | Choice |
| ------------- | ------ |
| STM32         | STM32CubeIDE + HAL (C) |
| ESP32-S3      | TBD (Arduino IDE / PlatformIO / ESP-IDF) |
| Cloud         | TBD (NETPIE / Firebase / …) |
| Web           | TBD |

## Getting started

### STM32
1. Open STM32CubeIDE.
2. **File → Import → General → Existing Projects into Workspace**, then choose the `stm32/` folder.
3. Build and flash it to the NUCLEO-F411RE.
4. To change pins or peripherals, open `stm32/Embedded_Project.ioc` in CubeMX and regenerate the code. Write your own code only between the `/* USER CODE BEGIN */` and `/* USER CODE END */` markers so regenerating doesn't erase it.

### ESP32-S3 (`mcu/`)
TBD

### Web (`web/`)
TBD

> Never commit WiFi passwords or cloud keys. Put them in a `secrets.h` / `.env` file (these are gitignored) and commit an example file instead.

## Requirements (from the course brief)

- [ ] Communication and some processing between the STM32 and the NodeMCU
- [ ] At least three different types of sensors and/or actuators
- [ ] Connected to the Internet and a cloud system

## Team

| Name | Student ID | Role |
| ---- | ---------- | ---- |
|      |            |      |
|      |            |      |
|      |            |      |
|      |            |      |
