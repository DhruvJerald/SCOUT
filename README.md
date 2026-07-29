<div align="center">

# SCOUT
<img width="666" height="375" alt="rover-removebg-preview" src="https://github.com/user-attachments/assets/4bab35ab-d2d3-4475-a19b-1955650a718c" />


**A Rover built from cheap and easily available parts**



---

![rover render](https://github.com/user-attachments/assets/00bb53da-2263-4230-8190-7b21b68df76f)

</div>

---

## What is it?

It's a RC rover that can be controlled from nearly everywhere. The idea follows a phone mounted in the chassis that does the heavy lifting such as streaming video, providing GPS, accelerometer, gyroscope, and cellular data while the ESP32 handles motors, sensors, and near Realtime control.
The entire rover has M3 mounting holes all over and doesn't use and glues or zip ties to make the final project closer to a product than a mere prototype and also being 3D printed in PLA.

## Why did i make it?
I really liked the idea of using the phones IMU, Accelerometer, GPS, Cellular connectivity, Camera, Microphone, Speakers, Flashlight and the onboard CPU of a phone which is really compact and possess a lot of expensive features of a real rover which would cost a fortune to source individual parts and i wanted to build a rover at a fraction of the price

---

## Goals

- **Repairable first** — Should be easy to make changes(spacious), Should be easy to mount things (M3 mounts), Should allow for future additions (Future proof with additional GPIO extensions)
- **Phone as compute** — SIM, Camera, GPS, IMU, Microphone, Speaker, Display, Accelerometer (All from a cheap android)
- **Cheap and sourceable** — every component available on Amazon.in or Robu.in or can be salvaged from old electronics
- **Controllable from anywhere but maintain fastest connectivity** — (fastest connection is made) between WIFI-AP, 4G cellular and Bluetooth

---



---

## Design

<img width="1190" height="894" alt="1" src="https://github.com/user-attachments/assets/d518493c-3983-4eab-9a86-a07f1d825c37" />





---

## Schematic

![schematic](https://github.com/user-attachments/assets/0d0c123a-ad87-43aa-9124-d182ff4040d1)

---

## BOM 

| Item | Price (INR) |
|------|------------|
| ESP32 DevKit V1 | 409 |
| 7.4V 6000mAh 2S LiPo | 1199 |
| XT60 connector pair | 260 |
| Rocker switch | 45 |
| LM2596 buck converter | 98 |
| TB6612FNG × 2 | 249 |
| INA219 | 275 |
| SSD1306 OLED | 329 |
| 5V brushless fan | 180 |
| AO3400A MOSFET | 84 |
| Resistors (10kΩ + 100Ω) | 154 |
| Capacitors (1000µF + 100µF × 2 + 0.1µF × 2) | 247 |
| M3 screw kit | 360 |
| 12V 200RPM motors × 4 | 500 |
| DHT22 | 160 |
| JST-XH connectors | 109 |
| 18AWG silicone wire | 300 |
| 85mm wheels × 4 | 749 |
| M3 standoffs | 300 |
| Perfboard | 125 |
| Phone mount (rubber bands) | 150 |
| 2S LiPo charger | 676 |
| White PLA 1kg | 899 |
| **Total** | **~₹7,875** |

> Many of these :- capacitors, resistors, perfboard, fan, MOSFET — >can be salvaged from old electronics.

---

## Features
- Battery saver mode — shuts down OLED, Fan,other non crucial sensors
- Live voltage, current, and power via INA219 
- Ambient temperature to keep control of overheating within the rover 
- Status display on SSD1306 OLED
- All telemetry streamed to PC/Phone dashboard (web dashboard)

**Expansion**

The chassis has mounting holes and an I2C header for adding modules without redesigning anything so any future extension can be connected to the extension port and the new code could be compiled to work with it

---
## How to build it

1)Gather all components listed in the BOM and a Android device with a working SIM card with cellular connection

2)3D print all the parts from https://github.com/DhruvJerald/SCOUT/tree/956e7d2e782e086f435cfa4e5dd025103bdec9e7/3D%20Prints and assemble it with M3 screws 

3)Add the Perfboard and the Electronics to the enclosure with M3 standoffs

4)Wire it according to the schematic https://github.com/DhruvJerald/SCOUT/blob/956e7d2e782e086f435cfa4e5dd025103bdec9e7/Schematic_Rover.png

5)Upload the firmware to the ESP (Not current one as its AI made and purely for proof of concept and not to be used in the final build_

6)Set up VPS on PC side

7)run the code on both devices 



---

## Images

<img width="1920" height="1080" alt="Untitled" src="https://github.com/user-attachments/assets/be50580b-9adf-422a-95dc-94cb3671b728" />

![chassis render](https://github.com/user-attachments/assets/c412a633-3a0c-42e1-b531-9bf9a19d7bf3)

![second render](https://github.com/user-attachments/assets/7b8ea1f7-f10d-40bc-89fd-80195fb45253)

<img width="1920" height="1080" alt="SIDE" src="https://github.com/user-attachments/assets/01174261-f2e5-4899-bad6-86e21faecbc9" />

<img width="1191" height="872" alt="Screenshot 2026-06-20 171409" src="https://github.com/user-attachments/assets/a7a3f5fb-b938-49da-b097-1526e166bd79" />

<img width="1370" height="720" alt="Screenshot 2026-06-07 170747" src="https://github.com/user-attachments/assets/97f8afd9-ea4d-438c-99db-1f1b252c59df" />

<img width="1425" height="547" alt="Screenshot 2026-06-24 152214" src="https://github.com/user-attachments/assets/f971fea5-c02f-495b-b47a-613e151041ec" />



---


