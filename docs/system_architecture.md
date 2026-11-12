# System Architecture: People Counting Gate

> Status: **agreed draft**, 2026-11-12. See [requirement.md](requirement.md) for the *what*; this file is the *how*.
> Items marked **(verify)** must be checked against the real hardware or datasheet before relying on them.

## 1. Overview

```
 Corridor side                         Room side
 ┌───────────┐  beam   ┌─────────┐   ┌────────────┐
 │ KY-008    │ ──────► │ KY-018  │   │ Ultrasonic │   (10–20 cm apart, both across the doorway)
 │ laser     │         │ LDR     │   │ HC-SR04    │
 └─────┬─────┘         └────┬────┘   └─────┬──────┘
       │ GPIO               │ ADC          │ GPIO trig + TIM input capture
       └────────────────────┴──────────────┘
                            │
                   ┌────────▼─────────┐   USART1 115200   ┌──────────────┐   WiFi   ┌───────────────────┐
                   │ STM32 NUCLEO-F411RE ├─────────────────►│  ESP32-S3    ├─────────►│ Firebase Realtime │
                   │ owns the count      │◄─────────────────┤  (NodeMCU)   │◄─────────┤ Database + Auth   │
                   └────────┬─────────┘                   └──────────────┘          └─────────┬─────────┘
                            │ USART2 (ST-LINK USB) = debug log                                │ live listener
                            ▼                                                     ┌────────▼────────┐
                         PC terminal                                              │ Website (React) │
                                                                                  │ Firebase Hosting│
                                                                                  └─────────────────┘
```

**Rule of ownership:** the **STM32 owns the live count**. Firebase stores a copy and the admin's settings. The website never writes `count` directly. It sends a command, the ESP32 forwards it, and the STM32 confirms.

## 2. Tech stack

| Part | Choice | Why |
| ---- | ------ | --- |
| STM32 | STM32CubeIDE + CubeMX + HAL, C | Already set up; course standard |
| ESP32-S3 | PlatformIO (VS Code) + Arduino framework, C++ | Library versions pinned in `platformio.ini`, same build for every teammate |
| ESP32 → Firebase | `FirebaseClient` library by mobizt **(verify current version)** | Maintained Firebase RTDB client for ESP32 with auth + streaming |
| Cloud | Firebase Realtime Database + Firebase Auth | Live updates to the web, built-in login, free plan |
| Website | React + Vite, Firebase JS SDK, hosted on Firebase Hosting | Live listeners, public URL, simple deploy |

## 3. Hardware

### 3.1 Parts
| Part | Role |
| ---- | ---- |
| NUCLEO-F411RE (STM32F411RET6) | Reads sensors, runs the counting logic |
| ESP32-S3 (NodeMCU) | WiFi bridge to Firebase |
| KY-008 laser module | Beam emitter, switched by the STM32 (actuator) |
| KY-018 photoresistor module | Beam receiver (light sensor), analog output |
| Ultrasonic module, assumed **HC-SR04** **(verify model)** | Second presence sensor |
| MP1584EN buck converter | 9–12 V → 5 V supply for standalone use |

### 3.2 STM32 pin map (proposed, apply in CubeMX)
| Signal | Pin | Arduino header | Peripheral | Notes |
| ------ | --- | -------------- | ---------- | ----- |
| LDR analog in | PA0 | A0 | ADC1_IN0 | KY-018 powered from **3.3 V** so output ≤ 3.3 V |
| Laser enable | PB5 | D4 | GPIO output | Through NPN switch, see 3.4 |
| Ultrasonic TRIG | PA8 | D7 | GPIO output | 10 µs pulse |
| Ultrasonic ECHO | PB4 | D5 | TIM3_CH1 input capture | **Through voltage divider**, see 3.4 |
| ESP32 link TX | PA9 | D8 | USART1_TX | → ESP32 RX |
| ESP32 link RX | PA10 | D2 | USART1_RX | ← ESP32 TX |
| Debug log | PA2 / PA3 | (D1/D0) | USART2 | ST-LINK virtual COM port (already configured) |
| Full indicator | PA5 | D13 | GPIO output | On-board LD2 (already configured) |
| Reset count | PC13 | — | EXTI13 | On-board B1 (already configured) |
| SWD | PA13 / PA14 / PB3 | — | SYS | Keep free |

ESP32-S3 side: UART1 on **GPIO17 (TX) / GPIO18 (RX)** **(verify against your board's pinout)**. 3.3 V logic on both sides, so wire them directly: STM32 TX → ESP32 RX, STM32 RX ← ESP32 TX, and **connect the grounds**.

### 3.3 Power
- **Development:** both boards on USB. The NUCLEO's 5 V pin can feed the HC-SR04 and the laser.
- **Standalone / demo:** 9–12 V adapter → **MP1584EN set to 5.0 V** → 5 V rail feeding the NUCLEO (E5V input, jumper JP5 moved to E5V **(verify in UM1724)**), the ESP32-S3 5 V pin, the HC-SR04 and the laser. KY-018 stays on the NUCLEO's 3.3 V pin.
- ⚠️ **Adjust and measure the MP1584EN output with a multimeter before connecting any board.** These modules often ship set to a higher voltage.
- All grounds are common.

### 3.4 Hardware risks
| Risk | Mitigation |
| ---- | ---------- |
| The KY-008 can draw ~20–40 mA **(verify by measuring)**. An STM32 pin should not source more than a few mA. | Switch the laser with an NPN transistor (e.g. 2N2222 / S8050, ~1 kΩ base resistor from PB5), laser powered from 5 V. |
| HC-SR04 ECHO is 5 V; the STM32 is 3.3 V. | Voltage divider on ECHO (e.g. 1 kΩ series + 2 kΩ to GND). |
| LDRs react slowly (tens of ms) and room light changes the reading. | Slow laser on/off cycle with an adaptive threshold (§4.2), and a short tube or tape shroud around the LDR facing the laser. |

## 4. STM32 firmware

### 4.1 Structure
CubeMX-generated files stay as they are; our code goes in **USER CODE** blocks and in our own modules:

| File (in `stm32/Core/Src`, header in `Core/Inc`) | Job | Depends on HAL? |
| ------------------------------------------------- | --- | --------------- |
| `app_main.c` | `app_init()` / `app_loop()` called from `main.c` USER CODE blocks; scheduling | yes |
| `beam.c` | Laser on/off cycle, ADC read, adaptive threshold → `beam_blocked()` | yes |
| `ultrasonic.c` | Trigger pulse, echo capture, baseline → `us_blocked()` | yes |
| `counter.c` | Direction state machine and count (pure logic) | **no**, so it can be unit-tested on a PC |
| `link.c` | UART line protocol: parse and format messages (pure logic + thin UART glue) | parsing: no |

Rules: no `HAL_Delay` in the main loop (use `HAL_GetTick()` timestamps); keep interrupt handlers short; no dynamic memory.

### 4.2 Beam sensor (laser + LDR)
- The laser is **on** by default.
- Every **2 s**, the STM32 turns the laser **off for ~100 ms** and reads the LDR. This is the **ambient** level.
- With the laser on, the reading with an unbroken beam is the **lit** level.
- Threshold = midpoint between ambient and lit. The beam counts as **blocked** when the reading moves past the threshold for **2 samples in a row** (sample every 10 ms).
- If lit and ambient are too close (the laser is misaligned), send `ERR,BEAM` and stop counting until it recovers.
- Detection pauses during the 100 ms "off" window. A person blocks the beam for well over 100 ms, so this is safe.

The ADC direction (whether more light gives a higher or lower value) depends on the KY-018 wiring **(verify)**. Use the difference |lit − ambient| so the code works either way.

### 4.3 Ultrasonic
- Ping every **60 ms** (HC-SR04 minimum cycle).
- Echo pulse width is measured with TIM3 input capture (rising and falling edge) at a 1 µs tick; distance (cm) ≈ µs / 58.
- At boot, average 10 pings of the empty doorway → **baseline**.
- **Blocked** when distance < baseline − 15 cm for **2 pings in a row**. No echo (timeout ~25 ms) counts as "not blocked".

### 4.4 Direction state machine
```
           beam blocked                    ultrasonic blocked
 IDLE ─────────────────────► WAIT_US ─────────────────────────► count IN  ─┐
   │                            │ 1.5 s timeout                             │
   │                            ▼                                           ▼
   │                        WAIT_CLEAR ◄─────────────────────────────── WAIT_CLEAR
   │  ultrasonic blocked                    beam blocked                    │
   └───────────────────────► WAIT_BEAM ──────────────────────► count OUT ───┤
                                │ 1.5 s timeout                             │
                                └──────────► WAIT_CLEAR                     │
 WAIT_CLEAR: wait until both sensors are clear for 200 ms, then IDLE ◄──────┘
```
- `count OUT` at 0 does nothing (never negative).
- Laser is mounted on the **corridor side**, ultrasonic on the **room side**, **10–20 cm** apart.
- Every count change → send `CNT` (§5), update LD2 (`count >= capacity && capacity > 0`).

### 4.5 Boot sequence
1. Init peripherals, laser on, measure ultrasonic baseline and the beam levels.
2. Send `HELLO` every 1 s for up to **5 s**, waiting for `SET` (restored count). Counting is paused in this window.
3. If `SET` arrives, use it. Otherwise start at 0.
4. Start counting. Send `CNT,<n>,SYNC` every 10 s as a heartbeat.

## 5. STM32 ↔ ESP32 protocol

UART, **115200 8N1**, ASCII lines ending in `\n` (ignore `\r`). Max 32 characters per line. Unknown or malformed lines are ignored. No ACKs are needed: every message carries an absolute value, and the heartbeat repeats it.

| Direction | Message | Meaning |
| --------- | ------- | ------- |
| STM32 → ESP32 | `HELLO` | STM32 just booted, asking for its last count |
| STM32 → ESP32 | `CNT,<n>,<reason>` | Current count. `reason` = `IN`, `OUT`, `SET`, `BTN`, `SYNC` |
| STM32 → ESP32 | `ERR,<code>` | Sensor problem: `BEAM` (beam not aligned), `US` (no ultrasonic echo at baseline) |
| ESP32 → STM32 | `SET,<n>` | Set count to n (restore after boot, or admin correction) |
| ESP32 → STM32 | `CAP,<n>` | Room capacity (0 = not set). Used for LD2 |
| ESP32 → STM32 | `GET` | Ask for the current count. STM32 replies `CNT,<n>,SYNC` |

Example:
```
STM32: HELLO
ESP32: SET,4
ESP32: CAP,30
STM32: CNT,4,SET
STM32: CNT,5,IN
STM32: CNT,5,SYNC
```

**Any change to this table must be made in the STM32 code, the ESP32 code and this file in the same commit.**

## 6. ESP32 firmware

- Settings in `mcu/include/secrets.h` (gitignored), copied from `secrets.example.h`: WiFi SSID/password, Firebase API key, database URL, device account email/password, `ROOM_ID`.
- Signs in with its own **device account**. Its uid is mapped to one room in `/devices`.
- On boot: connect WiFi + Firebase → send `CAP` and `GET` to the STM32 (the STM32 may still be running).
- On `HELLO` from the STM32: read `/rooms/<ROOM_ID>/count` and reply `SET,<n>` (only in reply to `HELLO`).
- On `CNT`: write `count`, `lastEvent`, `updatedAt` (server timestamp).
- Every 20 s: write `lastSeen` (server timestamp).
- Listens (stream) on `/rooms/<ROOM_ID>/capacity` → `CAP`, and on `/rooms/<ROOM_ID>/command` → `SET`. Once the STM32 confirms with `CNT,<n>,SET`, it writes the count and deletes `command`.
- If WiFi/Firebase is down, it keeps the latest `CNT` and writes it after reconnecting.
- Mirrors protocol traffic to its USB serial for debugging.

## 7. Firebase

### 7.1 Data model (Realtime Database)
```jsonc
{
  "rooms": {
    "room-101": {
      "name": "Room 101",
      "capacity": 30,          // set by admin, 0 = not set
      "count": 12,             // written by the device only
      "lastEvent": "IN",       // IN | OUT | SET | BTN | SYNC
      "updatedAt": 1763000000000,  // server timestamp of last count change
      "lastSeen": 1763000005000,   // server timestamp of last heartbeat
      "command": { "type": "setCount", "value": 0 }  // optional, written by admin, deleted by device
    }
  },
  "admins":  { "<adminUid>": true },
  "devices": { "<deviceUid>": "room-101" }
}
```
Room status is **computed on the website** from `count` and `capacity` and is not stored.

### 7.2 Security rules (sketch, finalize in `web/database.rules.json`)
```jsonc
{
  "rules": {
    "rooms": {
      ".read": true,
      "$roomId": {
        ".write": "auth != null && root.child('admins').child(auth.uid).val() === true",
        "count":     { ".write": "auth != null && root.child('devices').child(auth.uid).val() === $roomId",
                       ".validate": "newData.isNumber() && newData.val() >= 0" },
        "lastEvent": { ".write": "auth != null && root.child('devices').child(auth.uid).val() === $roomId" },
        "updatedAt": { ".write": "auth != null && root.child('devices').child(auth.uid).val() === $roomId" },
        "lastSeen":  { ".write": "auth != null && root.child('devices').child(auth.uid).val() === $roomId" },
        "command":   { ".write": "auth != null && root.child('devices').child(auth.uid).val() === $roomId && !newData.exists()" }
      }
    },
    "admins":  { ".read": false, ".write": false },
    "devices": { ".read": false, ".write": false }
  }
}
```
`admins` and `devices` entries are added by hand in the Firebase console.

## 8. Website

- **Dashboard `/`** (public): grid of room cards. Each card shows the name, big `count / capacity`, a status chip (Empty = grey, Available = green, Full = red), "updated X s ago", and an **Offline** badge if `now − lastSeen > 60 s`. Live via `onValue('/rooms')`.
- **Admin `/admin`** (Firebase Auth email/password, must be in `/admins`): add room (id + name), rename, set capacity, set/reset count (writes `command`).
- Firebase web config comes from `web/.env` (`VITE_FIREBASE_*`), with `web/.env.example` committed.
- Deploy: `npm run build` → `firebase deploy` (Hosting + database rules).

## 9. Repository layout (target)

```
stm32/   CubeIDE project. Our code: Core/Src/app_*.c, beam.c, ultrasonic.c, counter.c, link.c
mcu/     PlatformIO project: platformio.ini, src/main.cpp, include/secrets.example.h
web/     Vite + React app, firebase.json, database.rules.json, .env.example
docs/    requirement.md, system_architecture.md (course PDF is gitignored)
AI.md    shared instructions for AI assistants (CLAUDE.md / GEMINI.md point to it)
```

## 10. CubeMX changes still to make

The current `.ioc` only has USART2, LD2 and B1. Still to add:
- ADC1 IN0 on PA0 (single conversion, software trigger)
- TIM3 CH1 input capture on PB4, 1 MHz tick, both edges
- GPIO outputs PB5 (`LASER_EN`) and PA8 (`US_TRIG`)
- USART1 on PA9/PA10, 115200 8N1, RX interrupt enabled
- NVIC: USART1 and TIM3 interrupts
