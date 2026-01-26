# DeskPet Watch - Firmware V1.0

A custom smartwatch firmware for the **Lilygo T-Encoder-Pro**, featuring a virtual pet, weather updates, a **Smart Bluetooth Reader**, and dual-control brightness settings.

## 🌟 Base Project & Credits
This project is built upon the excellent work of **nikthefix**:
*   **Base Project**: [Lilygo_Support_T_Encoder_Pro_Smart_Watch](https://github.com/nikthefix/Lilygo_Support_T_Encoder_Pro_Smart_Watch)
*   **Hardware**: Lilygo T-Encoder-Pro (ESP32-S3 + AMOLED Display)
*   **Display Driver**: Custom SH8601 driver implementation

## 🚀 Key Features Added
We have significantly enhanced the base demo with the following features:

### 1. 📖 Smart Reader Controller (Bluetooth HID)
Turn your watch into a premium physical scroll controller for iPad/PC.
*   **Smart Scroll Engine**: Uses an "Accumulate & Accelerate" algorithm.
    *   **Gentle Ramp**: Slow turns give precise scrolling (3x speed). Fast flicks trigger momentum skimming (up to 10x speed).
    *   **Momentum Guard**: Filters out "reverse-jerk" noise caused by fast spinning.
*   **Lazy Bluetooth**: Radio is **OFF** by default to save battery. Connects *only* when you click the knob to enter Scroll Mode.
*   **Auto-Focus Shake**: When connecting, the watch vigorously "shakes" the scroll (-2, +2) to instantly wake up the iPad and grab focus.
*   **Zero Drift**: The system tracks cursor movement and auto-centers it when you finish scrolling, preventing the "cursor drift" issue.

### 2. 👾 DeskPet (Virtual Pet)
*   **New App**: A dedicated "Pet" app integrated into the carousel.
*   **Moods**: The pet has dynamic moods (Happy, Neutral, Sleeping) based on interaction time.
*   **Interaction**: Tap the pet to make it happy (plays squeak sound). Ignore it, and it gets bored.

### 3. ☀️ Dual-Control Brightness Settings
*   **Integrated UI**: Replaced the unused "Call" screen with a professional **Brightness Control** menu.
*   **Dual Input**: Adjust brightness using **Touch** (sliding the arc) OR the **Rotary Encoder** (dial).
*   **Safety**: Brightness is safely clamped between 10-255 to prevent blackouts.

### 4. 🔒 Security & Optimization
*   **Secure Secrets**: WiFi and API keys are stored in a git-ignored `secrets.h` file.
*   **Network Resilience**: Added timeouts to weather requests.

## 🛠️ How to Flash

### Prerequisites
*   **VS Code** with **PlatformIO** extension installed.
*   **Lilygo T-Encoder-Pro** hardware.

### Steps
1.  **Clone/Open**: Open this project folder in VS Code.
2.  **Configuration**:
    *   Navigate to `sls_encoder_pro_watch/include/`.
    *   Rename `secrets_example.h` to `secrets.h` (or create it).
    *   Enter your `WIFI_SSID`, `WIFI_PASSWORD`, and `OPEN_WEATHER_API_KEY`.
3.  **Build**: Click the PlatformIO **Build** (Checkmark) icon.
4.  **Upload**: Connect your watch via USB-C and click **Upload** (Arrow) icon.
5.  **Enjoy**: The watch will boot, connect to WiFi, sync time via NTP, and load the interface.

## 🎮 Controls
*   **Rotary Encoder**: Rotate to switch apps (Digital -> Analog -> Pet -> Weather -> Reader).
*   **Reader App**:
    *   **Click Knob**: Connect Bluetooth & Lock Scroll Mode (Status turns Cyan).
    *   **Rotate**: Scroll remotely.
    *   **Click Again**: Disconnect/Unlock to navigate apps.
*   **Touch**: 
    *   Tap icons to open apps.
    *   Swipe to navigate.
    *   Tap the Pet to interact.
*   **Settings**: 
    *   Tap the **Gear Icon** (top-right of clock) to open Brightness.

---
*Built with ❤️ by Rishith & Antigravity*