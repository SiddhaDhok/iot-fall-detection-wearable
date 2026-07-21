# IoT Fall and Faint Detection Wearable

A wearable IoT device that detects falls and fainting episodes using motion sensing, sounds a local alarm with a manual cancel option, notifies a family member or caregiver directly on their phone via Telegram, and pushes live status updates to a cloud dashboard.

## Why this project

Existing fall-detection solutions on the market (smartwatches, phone-based apps) rely on a paired smartphone, internet-dependent apps, and text-based alerts that can easily go unnoticed. This project takes a different approach:

- **Standalone hardware** — doesn't depend on a phone being nearby or charged
- **Multi-channel alerting** — a Telegram push notification (with sound) rather than a text message, designed to be hard to miss even if the recipient doesn't check texts often
- **Purpose-built, not multi-purpose** — a dedicated safety device, not a fitness tracker with a fall-detection feature bolted on
- **Wearable, permanent build** — soldered onto a compact perfboard and housed in an enclosure, not left on a breadboard

## Features

- **Real-time fall and faint detection** using an MPU6050 accelerometer + gyroscope, with a custom multi-stage detection algorithm (free-fall dip → impact spike → sustained stillness)
- **On-device OLED display** showing live sensor readings, current detection state, and a countdown timer during an active alarm
- **Local audible alarm** (buzzer) that triggers immediately on a confirmed fall
- **False-alarm cancellation** via a physical push button, with a 15-second grace window before the alert escalates
- **Telegram alert** — if the alarm isn't cancelled within the grace window, the device sends a message directly to a caregiver's phone via a Telegram bot, pushing a notification with sound even if the recipient doesn't have the app open
- **Live cloud dashboard** (Blynk) showing device status, alarm state, and cumulative fall count in real time
- **Compact, soldered build** — moved from a breadboard prototype to a permanent perfboard assembly, housed in a wearable enclosure
- **Battery-independent operation for demo purposes** — powered via USB/power bank, keeping the design simple and portable

## How this was built

### Sensing hardware and I2C

The device uses two sensors that both communicate over **I2C**, a two-wire protocol (SDA for data, SCL for a shared clock signal) that lets multiple devices share the same two physical wires on the ESP32. Each device on the bus has its own address, so the microcontroller can address the MPU6050 (`0x68`) and the OLED (`0x3C`) independently even though both sit on the same SDA/SCL lines. This is why only 4 wires (VCC, GND, SDA, SCL) are needed to run both the accelerometer and the display, rather than a separate set of wires for each.

During development, the standard high-level `Adafruit_MPU6050` library failed to initialize the specific MPU6050 board used in this build, even though the I2C scanner correctly detected it at `0x68`. This turned out to be a known issue with certain clone MPU6050 boards, whose chip identification register doesn't match what the library's safety check expects, even though the sensor otherwise communicates correctly. The fix was to bypass the high-level library entirely and communicate with the sensor via **direct I2C register access** — manually waking the sensor (writing to its power management register) and reading raw accelerometer/gyroscope values from its data registers. This is a lower-level but more portable approach that works reliably across MPU6050 clone boards.

### Fall detection algorithm

The device continuously reads raw accelerometer values and converts them to units of g-force (1g = normal gravity at rest), then combines all three axes into a single **acceleration magnitude**:

```
magnitude = sqrt(x^2 + y^2 + z^2)
```

This single number represents the total force experienced by the sensor regardless of its orientation, which matters because a person's body — and the device on them — can be tilted in any direction, and we don't want tilting alone to be mistaken for a fall.

Threshold values used in the final detection logic, tuned from real test data (shake tests, drop tests, and controlled body-collapse simulations performed during development):

| Parameter | Value | What it represents |
|---|---|---|
| Resting baseline | ~1.00g | Magnitude when the device is still (pure gravity) |
| Dip threshold | < 0.6g | Magnitude drop that may indicate free-fall |
| Impact threshold | > 1.5g | Magnitude spike that may indicate a collision with the ground |
| Impact window | 1000 ms | Impact must follow a dip within this time to be linked to the same event |
| Stillness band | 0.92g – 1.08g | Magnitude range considered "not moving" |
| Stillness duration | 5500 ms | How long the device must stay within the stillness band to confirm a fall |
| Cancel window | 15 seconds | Time available to press the cancel button before the alarm escalates |

A fall is only confirmed when all three stages occur in sequence: the magnitude dips below 0.6g, then spikes above 1.5g within one second, then settles and stays within the 0.92g–1.08g stillness band for a full 5.5 seconds. This sequence is what separates a genuine fall from everyday movement — a quick shake or a jump can momentarily dip and spike in a similar way, but rarely holds a truly sustained, still period immediately afterward, since normal activity involves continuing to move. Test data showed that casual shaking produced magnitude swings roughly between 0.4g and 3.0g, overlapping with real fall signatures, which is why the sustained stillness stage — rather than the magnitude spike or dip alone — is the most reliable part of the algorithm.

### Alarm and escalation flow

Once a fall is confirmed, the buzzer sounds immediately and the OLED displays a live countdown. Pressing the cancel button (a KY-004 push button module) during this window silences the alarm and resets the device to normal. If the countdown reaches zero without a button press, the alarm escalates:

- The Blynk cloud dashboard updates to show an active alarm state and increments the fall count
- A Telegram message is sent to a configured caregiver's phone through the project's bot, chosen specifically over SMS since a push notification with sound is harder to miss than a text message that may go unread

### Cloud dashboard

The ESP32 connects to WiFi and streams three pieces of live data to a Blynk IoT dashboard: current status (text), alarm trigger state (on/off), and cumulative fall count. This gives a caregiver or family member a way to check the device's state remotely, separate from the local buzzer and Telegram alert.

## Hardware components

| Component | Purpose |
|---|---|
| ESP32 DevKit (CP2102) | Main microcontroller, WiFi connectivity |
| MPU6050 | Accelerometer + gyroscope for motion sensing |
| 0.96" I2C OLED display (SSD1306) | Live status display |
| Active buzzer | Local audible alarm |
| KY-004 push button module | Manual false-alarm cancellation |
| Perfboard | Permanent soldered mounting for all components |
| Project enclosure box | Wearable housing |
| USB power bank | Portable power source |

## Software and tools

- **Arduino IDE** — firmware development
- **Blynk IoT** — cloud dashboard for live status monitoring
- **Telegram Bot API** — caregiver phone alerts
- Direct I2C register access for MPU6050 (rather than a high-level library) to ensure compatibility with clone sensor boards

## Wiring overview

Both the MPU6050 and OLED share the same I2C bus:

| Signal | ESP32 Pin |
|---|---|
| SDA | GPIO 21 |
| SCL | GPIO 22 |
| Buzzer | GPIO 25 |
| Button (signal) | GPIO 27 |

MPU6050 I2C address: `0x68` · OLED I2C address: `0x3C`

## Repository structure

```
iot-fall-detection-wearable/
├── README.md
├── firmware/
│   └── fall_detection.ino        # Main ESP32 sketch
└── docs/
    └── demo-photos/                # Enclosure and build photos
```

## Setup instructions

1. Install the Arduino ESP32 board package and the following libraries: `Adafruit SSD1306`, `Adafruit GFX`, `Blynk`, `UniversalTelegramBot` (or equivalent Telegram library), `WiFiClientSecure`
2. Wire the components as described above
3. Create a free Blynk account and set up a template with three datastreams: `Status` (String, V0), `Alarm Trigger` (Integer, V1), `Fall Count` (Integer, V2)
4. Create a Telegram bot via **@BotFather**, and get your personal Chat ID via **@userinfobot**
5. In `firmware/fall_detection.ino`, replace the placeholder values for WiFi SSID, WiFi password, Blynk Template ID, Blynk Auth Token, Telegram Bot Token, and Telegram Chat ID with your own
6. Upload the sketch to the ESP32
7. Open the Blynk dashboard to view live status, and confirm Telegram alerts arrive on an unacknowledged fall

## Building the permanent version

The prototype was validated on a breadboard, then moved to a soldered perfboard build for durability and a wearable form factor:

1. All fall-detection thresholds were tuned using real breadboard test data before transferring to the permanent build
2. Components were soldered onto a general-purpose perfboard, sized to fit the ESP32 and shared I2C junctions for the MPU6050 and OLED
3. The assembled board was mounted inside a project enclosure, with cutouts for the OLED window, cancel button, buzzer opening, and USB power cable

## Future improvements

Several extensions were explored during development but deferred due to the project timeline:

- **Reduced size and compactness** — the current build is sized around a full ESP32 DevKit (with its USB port and onboard regulator), making the enclosure noticeably bulkier than a typical wearable. A future revision could use a bare ESP32-WROOM module with an external USB-to-serial programmer, or a smaller ESP32-C3/S3 mini board, to shrink the overall footprint closer to a true pendant or wristband size
- **Heart rate sensing (MAX30102)** — fusing motion data with heart rate to improve faint detection specifically, since a faint often lacks the sharp motion signature of a fall
- **Custom patient dashboard website** — a dedicated web interface (beyond the Blynk dashboard) showing patient name, age, location, and medical history alongside live device status, with proper multi-patient support backed by a real database
- **GSM-based emergency calling** — using a SIM800L module to place an actual phone call to a family member or neighbour if an alert isn't acknowledged, as a fallback channel that doesn't depend on internet connectivity
- **Internal rechargeable battery** — replacing the USB power bank with an integrated LiPo battery, charging circuit, and boost converter for a fully self-contained wearable
- **Orientation-based false positive filtering** — using gyroscope/orientation data to distinguish a genuine fall (change in body orientation) from vigorous movement like jumping, which can otherwise trigger a false alarm under the current motion-only detection logic

Built By: Siddha Dhok
