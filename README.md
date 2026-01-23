# 🛴 Scooter Dual TFT Display

This project drives a **dual-screen TFT dashboard** for an electric scooter using an **ESP32**, **TFT_eSPI**, and **NimBLE**. It visualizes ride data such as shock response, g‑force/boost, tilt, and compass heading, and supports BLE connectivity for live telemetry.

---

## ✨ Features

* 📟 **Two TFT displays** controlled via separate CS pins
* 🧭 **Animated compass** with heading rotation
* 🚀 **Boost / G‑force arc gauge** with color zones
* 🛞 **Shock sensor visualization** for front & back wheels
* 🛴 **Scooter bitmap overlay**
* 🧠 **Sprite-based rendering** for smooth animations
* 🔵 **BLE client support (NimBLE)** for remote telemetry

---

## 🧰 Hardware

* ESP32
* 2× 240×240 TFT Displays (SPI)
* Shock sensors (front & rear)
* IMU / accelerometer (for g‑force & tilt)
* Optional GPS / compass module

### Chip Select Pins

```cpp
#define screen_0_CS 10
#define screen_1_CS 2
```

---

## 📦 Libraries Used

* **TFT_eSPI** – display & sprite rendering
* **NimBLE-Arduino** – BLE client
* **Wire** – I2C
* **math.h** – trigonometry

Install via Arduino Library Manager.

---

## 🖥 Display Modes

### Screen 0

* Compass with rotating heading
* Boost / g‑force arc
* Shock heat visualization
* Scooter graphic overlay

### Screen 1

* Map / secondary visualization
* Status circle
* Scooter bitmap

---

## 🔵 BLE Integration

The dashboard connects to a BLE server and reads a characteristic:

```cpp
static NimBLEUUID serviceUUID("c32cdd1f-baf2-46bc-87a1-69ab9637bfe0");
static NimBLEUUID charUUID("23680ff2-e66b-4a4e-a051-422d2665c443");
```

`fetchDataFromBLE()` reads telemetry which can be parsed and mapped into:

* `gForceValueZ`
* `tiltAngleValue`
* `compassValue`
* `shockSensorFrontValue`
* `shockSensorBackValue`

---

## 🚀 Getting Started

1. Clone the repo

```bash
git clone https://github.com/yourname/scooter-display.git
```

2. Open in Arduino IDE

3. Configure **User_Setup.h** in TFT_eSPI for your screen

4. Upload to ESP32

---

## ⚙️ Main Loop Example

```cpp
updateScreen1();
compassValue++;
shockSensorBackValue += 8;
shockSensorFrontValue += 8;
```

The loop animates compass rotation and shock intensity for testing.

---

## 🛠 Customization Ideas

* Add speedometer
* GPS map overlay
* Battery voltage gauge
* Bluetooth OTA updates
* Ride logging to SD card

---

## 📸 Preview

Add screenshots or GIFs here once mounted on the scooter.

---

## 🤝 Contributing

Pull requests are welcome. For major changes, open an issue first to discuss what you’d like to improve.

---

## 📜 License

MIT License

---

Built for a smart scooter dashboard project using ESP32 + TFT + BLE. 🛴
