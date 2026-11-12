# Requirements: People Counting Gate

> Status: **agreed draft**, 2026-11-12. Items marked *(design call)* were proposed by the assistant and accepted by the team as a group. They can still be changed.

## 1. Purpose

An IoT occupancy counter for **room doors**. Each door has one sensor set. People go in and out through the **same door**. The device works out the direction of each person, so the system always knows **how many people are in the room right now**. A website shows every room's live count and whether it is **empty**, **available** or **full**.

This is the final project for the Embedded System Lab (Fall 2026).

## 2. Users

| User   | Needs |
| ------ | ----- |
| Viewer | Open the website and see, for every room, how many people are inside and whether it is empty or full. No login. |
| Admin  | Log in to the website, add rooms, set each room's **maximum capacity**, and correct a room's count when it drifts. |

## 3. Scope

- The system is designed for **many rooms**, each with its own device. One device = one room = one door.
- The demo runs **one physical device**.

### Non-goals
- Identifying *who* entered (no cameras, no IDs).
- Rooms with more than one door.
- Correctly counting two people crossing **side by side at the same moment** (known limitation, see §7).
- History charts, notifications, a mobile app.

### Anti-goals (count as a failure even if everything else works)
- The count goes below 0.
- The website shows an old count as if it were live (a device that stopped reporting must be shown as **Offline**).
- WiFi passwords or cloud keys are committed to the public repo.
- The beam sensor stops working because of room light or sunlight during the demo.

## 4. Functional requirements

### Device (STM32 + sensors)
| ID | Requirement |
| -- | ----------- |
| FR-1 | Detect a person crossing the door using two sensors placed a short distance apart: a **laser beam** (KY-008 + KY-018) on the **corridor side** and an **ultrasonic** sensor on the **room side**. |
| FR-2 | Work out the direction from which sensor triggers first: laser → ultrasonic = **IN** (+1), ultrasonic → laser = **OUT** (−1). |
| FR-3 | If the second sensor does not trigger within **1.5 s** of the first, ignore the event (someone peeked in and backed out). *(design call)* |
| FR-4 | The count never goes below 0. An OUT at 0 is ignored. |
| FR-5 | The STM32 switches the laser on and off and uses the "laser off" light reading to adapt the beam threshold to the room's light. This makes the laser an MCU-controlled **actuator**. *(design call)* |
| FR-6 | At boot, the ultrasonic measures the empty doorway and uses that distance as its baseline. *(design call)* |
| FR-7 | The on-board LED **LD2** is on while the room is full (count ≥ capacity). |
| FR-8 | Pressing the on-board **B1** button resets the count to 0. *(design call)* |
| FR-9 | The STM32 sends the **full current count** (not just +1/−1) to the ESP32 after every change and as a heartbeat every 10 s. |
| FR-10 | After an STM32 reboot, the count is restored from the cloud through the ESP32 (if it is reachable within 5 s). |

### Gateway (ESP32-S3)
| ID | Requirement |
| -- | ----------- |
| FR-11 | Connect to WiFi and Firebase. Reconnect on its own after a drop. |
| FR-12 | Write the room's count, last event and timestamp to Firebase whenever the STM32 reports a change. |
| FR-13 | Write a "last seen" timestamp at least every 20 s so the website can tell whether the device is online. |
| FR-14 | Forward capacity changes and admin count corrections from Firebase to the STM32. |

### Website
| ID | Requirement |
| -- | ----------- |
| FR-15 | Public dashboard: one card per room showing the name, `count / capacity` and the status. |
| FR-16 | Status: **Empty** (count = 0), **Available** (0 < count < capacity), **Full** (count ≥ capacity). If no capacity is set, show the count only. |
| FR-17 | Show **Offline** on a room if its device has not been seen for 60 s. Also show "last updated" time. |
| FR-18 | Updates appear live, without reloading the page. |
| FR-19 | Admin page behind email/password login: add a room, rename it, set capacity, set/reset the count. |
| FR-20 | Only admins can change rooms; only a room's own device can write its count. |

## 5. Non-functional requirements

| ID | Requirement |
| -- | ----------- |
| NFR-1 | **Latency:** from a person crossing to the website updating ≤ **2 s** (with working WiFi). |
| NFR-2 | **Accuracy:** ≥ **90 %** correct over 20 single-file crossings (mixed in/out) at normal walking speed. |
| NFR-3 | Works under normal classroom lighting, including with the room lights switched on and off. |
| NFR-4 | If WiFi drops, the device keeps counting and the website catches up when it reconnects (no lost people). |
| NFR-5 | The website works on a phone screen. |
| NFR-6 | Stays inside the Firebase free plan. |
| NFR-7 | No secrets in the repo. Each part has an example config file instead. |

## 6. Course constraints (from the brief)

- STM32 and NodeMCU (ESP32-S3) must communicate and do some processing.
- At least **3 different types** of sensors/actuators. This project uses:
  1. **KY-008 laser**: actuator (light emitter, switched by the STM32)
  2. **KY-018 photoresistor**: light sensor
  3. **Ultrasonic module (HC-SR04, to be confirmed)**: distance sensor
- Connected to the Internet and a cloud system (Firebase).
- Public repository + source archive, weekly progress paragraph on MCV, a 4-minute demo, and a report of at least 4 pages (each member writes at least 1 page about their role).

## 7. Known limitations

- Two people crossing side by side, or one right behind the other with no gap, may be counted as one.
- A person who stops between the two sensors for more than 1.5 s and then continues will not be counted.
- If both the ESP32 and WiFi are down when the STM32 reboots, the count restarts at 0. The admin can correct it on the website.

## 8. Demo acceptance (4 minutes)

1. The dashboard shows the demo room as **Empty**, plus at least one other room created by the admin.
2. The admin sets the demo room's capacity to **3**.
3. Three people walk in one by one → the count goes 1, 2, 3 within 2 s each; at 3 the card turns **Full** and LD2 lights.
4. One person walks out → the count shows 2, the status shows **Available**, and LD2 turns off.
5. Someone steps into the laser and backs out → no change.
6. The room lights are switched off and on → the counting still works.
7. The ESP32 is unplugged → within 60 s the card shows **Offline**. It is plugged back in → the count is correct again.

## 9. Team

| Name | Student ID | Role |
| ---- | ---------- | ---- |
|      |            | System architecture & integration |
|      |            | STM32 firmware (sensors, counting) |
|      |            | ESP32 firmware & Firebase |
|      |            | Website (UI/UX) |
