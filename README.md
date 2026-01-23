#  Scooter Dual TFT Display

This project drives a **dual-screen TFT dashboard** for an electric scooter using an **ESP32**, **TFT_eSPI**, and **NimBLE**. It visualizes ride data such as shock response, g‑force/boost, tilt, and compass heading, and supports BLE connectivity for live telemetry.

---

##  Features

*  **Two TFT displays** controlled via separate CS pins
*  **Animated compass** with heading rotation
*  **Boost / G‑force arc gauge** with color zones
*  **Shock sensor visualization** for front & back wheels
*  **Scooter bitmap overlay**
*  **Sprite-based rendering** for smooth animations
*  **BLE client support (NimBLE)** for remote telemetry

---

##  Hardware

* ESP32
* 2× 240×240 TFT Displays (SPI)
* Shock sensors (front & rear)
* IMU / accelerometer (for g‑force & tilt)
* Optional GPS / compass module
