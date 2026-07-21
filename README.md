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

## How it works

### Detection algorithm

The device continuously reads acceleration data from the MPU6050 and computes the total acceleration magnitude (combining all three axes). A fall is confirmed only when three conditions occur in sequence:

1. **Dip** — magnitude drops below a threshold, indicating a possible free-fall
2. **Impact** — magnitude spikes above a threshold shortly after the dip, indicating a collision with the ground
3. **Sustained stillness** — magnitude stays close to resting baseline (~1g) for several continuous seconds afterward, indicating the person is down and not moving

This three-stage sequence is what distinguishes a genuine fall from ordinary movement like walking, sitting down, or shaking the device — motion alone (a spike or dip) is common in daily activity, but the specific sequence of dip, then impact, then sustained stillness is a much stronger signal of an actual fall or fainting episode. Thresholds were tuned using real test data gathered by simulating shakes, drops, and controlled body-collapse tests during development.

### Alarm and escalation flow

Once a fall is confirmed, the buzzer sounds and the OLED shows a countdown. If the wearer presses the cancel button within the countdown window, the alarm stops and the device resets to normal. If no button press occurs, the alarm escalates:

- The Blynk dashboard updates to reflect an active alarm state
- A Telegram message is sent to a configured caregiver's phone via the project's bot

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
    ├── wiring-diagram.png         # Circuit reference
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

- **Heart rate sensing (MAX30102)** — fusing motion data with heart rate to improve faint detection specifically, since a faint often lacks the sharp motion signature of a fall
- **Custom patient dashboard website** — a dedicated web interface (beyond the Blynk dashboard) showing patient name, age, location, and medical history alongside live device status, with proper multi-patient support backed by a real database
- **GSM-based emergency calling** — using a SIM800L module to place an actual phone call to a family member or neighbour if an alert isn't acknowledged, as a fallback channel that doesn't depend on internet connectivity
- **Internal rechargeable battery** — replacing the USB power bank with an integrated LiPo battery, charging circuit, and boost converter for a fully self-contained wearable
- **Orientation-based false positive filtering** — using gyroscope/orientation data to distinguish a genuine fall (change in body orientation) from vigorous movement like jumping, which can otherwise trigger a false alarm under the current motion-only detection logic

Siddha Dhok
