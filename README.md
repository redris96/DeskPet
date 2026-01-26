# DeskPet Watch - Firmware V1.0

A custom smartwatch firmware for the **Lilygo T-Encoder-Pro**, featuring a virtual pet, weather updates, and dual-control brightness settings.

## 🌟 Base Project & Credits
This project is built upon the excellent work of **nikthefix**:
*   **Base Project**: [SquareLine Studio 1.4.1 Smart Watch Demo](https://github.com/nikthefix)
*   **Hardware**: Lilygo T-Encoder-Pro (ESP32-S3 + AMOLED Display)
*   **Display Driver**: Custom SH8601 driver implementation

## 🚀 Key Features Added
We have significantly enhanced the base demo with the following features:

### 1. DeskPet (Virtual Pet)
*   **New App**: A dedicated "Pet" app integrated into the carousel.
*   **Moods**: The pet has dynamic moods (Happy, Neutral, Sleeping) based on interaction time.
*   **Interaction**: Tap the pet to make it happy (plays squeak sound). Ignore it, and it gets bored.

### 2. Dual-Control Brightness Settings
*   **Integrated UI**: Replaced the unused "Call" screen with a professional **Brightness Control** menu.
*   **Dual Input**: Adjust brightness using **Touch** (sliding the arc) OR the **Rotary Encoder** (dial).
*   **Safety**: Brightness is safely clamped between 10-255 to prevent blackouts.
*   **Stability**: Navigation buttons are disabled during adjustment to prevent accidental app switching.

### 3. UI & Layout Refinements
*   **Optimized Layout**: Fixed digital clock overlapping issues and font sizes.
*   **Network Resilience**: Added timeouts to weather requests to prevent loop freezing on bad WiFi.
*   **Iconography**: Updated Digital and Analog watch faces with functional Settings (Gear) icons.

## 🛠️ How to Flash

### Prerequisites
*   **VS Code** with **PlatformIO** extension installed.
*   **Lilygo T-Encoder-Pro** hardware.

### Steps
1.  **Clone/Open**: Open this project folder in VS Code.
2.  **WiFi Setup**:
    *   Open `src/sls_encoder_pro_watch.ino`.
    *   Update `ssid` and `password` variables with your network credentials.
3.  **Build**: Click the PlatformIO **Build** (Checkmark) icon.
4.  **Upload**: Connect your watch via USB-C and click **Upload** (Arrow) icon.
5.  **Enjoy**: The watch will boot, connect to WiFi, sync time via NTP, and load the interface.

## 🎮 Controls
*   **Rotary Encoder**: Rotate to switch apps (Digital -> Analog -> Pet -> Weather -> etc.).
*   **Touch**: 
    *   Tap icons to open apps.
    *   Swipe to navigate.
    *   Tap the Pet to interact.
*   **Settings**: 
    *   Tap the **Gear Icon** (top-right of clock) to open Brightness.
    *   Use Knob or Touch to adjust.
    *   Tap **Home** to return.

---
*Built with ❤️ by Rishith & Antigravity*