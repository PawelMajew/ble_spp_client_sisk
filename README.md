# BLE SPP Client Project

This project was created as part of the "Standards and Communication Systems" laboratory at AGH University of Science and Technology, within the Electronics and Telecommunications program, 2nd degree, 2nd semester, 2023/2024.

## Authors
- Paweł Majewski
- Łukasz Piasecki

## Project Description
The project focuses on implementing a Bluetooth Low Energy (BLE) client with Serial Port Profile (SPP) support. The main goal is to create a system that enables wireless communication between a central device and a peripheral device using the BLE SPP standard.

## Project Structure

### 1. `ble_spp_client`
The main project directory contains the `main.c` file, responsible for the initialization of necessary modules for BLE SPP handling. This file serves as the core of the project where client configuration and startup take place.

### 2. `gap`
The `gap` file contains code responsible for handling the Generic Access Profile (GAP). This module deals with general aspects of BLE communication, such as device initialization, connection management, and overall event handling.

### 3. `gatt`
The `gatt` file contains code related to handling the Generic Attribute Profile (GATT) and Serial Port Profile (SPP). GATT defines the structure and data exchange between the central and peripheral devices, while SPP allows for emulating a serial port over BLE

## Running the Project

1. Clone the repository to your local environment.
   ```bash
   git clone https://github.com/your-username/ble_spp_client.git