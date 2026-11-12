# People Counting Gate

Final project for the **Embedded System Lab (Fall 2026)**.

An IoT occupancy counter for room doors. A laser beam and an ultrasonic sensor placed a little apart in the doorway tell the **STM32** which way each person is walking, so it always knows how many people are inside. The STM32 sends the count over UART to an **ESP32-S3 (NodeMCU)**, which syncs it with **Firebase**. A **website** shows every room live as **Empty**, **Available** or **Full**. An admin sets each room's capacity.

> **Status:** requirements and architecture agreed; implementation not started.

## Docs

- [Requirements](docs/requirement.md): what the system must do, demo checklist
- [System architecture](docs/system_architecture.md): hardware, pin map, STM32 ↔ ESP32 protocol, Firebase data model
- [AI.md](AI.md): rules for AI coding assistants (Claude / Gemini)

## System overview

```
 Laser + LDR (corridor side) ─┐
                              ├─► STM32 ──UART──► ESP32-S3 ──WiFi──► Firebase ◄── Website
 Ultrasonic  (room side)  ────┘  (count)         (bridge)           (RTDB+Auth)   (React)
```

## Repository structure

| Folder   | What it holds |
| -------- | ------------- |
| `stm32/` | STM32CubeIDE project for the NUCLEO-F411RE: sensors, direction logic, count |
| `mcu/`   | ESP32-S3 (NodeMCU) firmware: WiFi bridge between the STM32 and Firebase |
| `web/`   | Dashboard (public) and admin page (capacity, count correction) |
| `docs/`  | Requirements and architecture |

## Hardware

| Part | Role |
| ---- | ---- |
| NUCLEO-F411RE (STM32F411RET6) | Main MCU: reads sensors, counts |
| ESP32-S3 (NodeMCU) | WiFi bridge |
| KY-008 laser module | **Actuator:** beam emitter, switched by the STM32 |
| KY-018 photoresistor module | **Sensor:** detects when the beam is blocked |
| Ultrasonic module (HC-SR04) | **Sensor:** second presence sensor, for direction |
| MP1584EN | 5 V step-down power supply |

Full pin map and wiring: [system_architecture.md §3](docs/system_architecture.md#3-hardware).

## Tech stack

| Part | Choice |
| ---- | ------ |
| STM32 | STM32CubeIDE + HAL (C) |
| ESP32-S3 | PlatformIO + Arduino framework (C++) |
| Cloud | Firebase Realtime Database + Firebase Auth |
| Web | React + Vite, hosted on Firebase Hosting |

## Getting started

### STM32 (`stm32/`)
1. Open STM32CubeIDE.
2. **File → Import → General → Existing Projects into Workspace**, then choose the `stm32/` folder.
3. Build and flash it to the NUCLEO-F411RE.
4. To change pins or peripherals, open `stm32/Embedded_Project.ioc` in CubeMX and regenerate the code. Write your own code only between the `/* USER CODE BEGIN */` and `/* USER CODE END */` markers so regenerating doesn't erase it.

### ESP32-S3 (`mcu/`)
Not set up yet. It will be a PlatformIO project: copy `include/secrets.example.h` to `include/secrets.h`, fill it in, then run `pio run -t upload`.

### Web (`web/`)
Not set up yet. It will be a Vite app: copy `.env.example` to `.env`, fill it in, then run `npm install` and `npm run dev`.

> Never commit WiFi passwords or cloud keys. `secrets.h` and `.env` are gitignored.

## Course requirements

- [ ] Communication and some processing between the STM32 and the NodeMCU
- [ ] At least three different types of sensors and/or actuators (laser, photoresistor, ultrasonic)
- [ ] Connected to the Internet and a cloud system (Firebase)

## Team

| Name | Student ID | Role |
| ---- | ---------- | ---- |
|      |            | System architecture & integration |
|      |            | STM32 firmware |
|      |            | ESP32 firmware & Firebase |
|      |            | Website |
