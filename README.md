# ESP32 WebSocket Server with Python Client

This repository contains code to set up an ESP32 as a WebSocket server that sends a UUID, the current time, and the status of a connected button to any clients. A Python script is included to connect to this WebSocket server and print out the received messages.

## Files in This Repository

- **`main.c`**: The C code for the ESP32 to set up a WebSocket server.
- **`test.py`**: The Python script to connect to the WebSocket server and receive messages.

## ESP32 Setup

### 1. Requirements

- **Arduino IDE** (or another IDE compatible with ESP32) with the ESP32 board support installed.
- **WiFi** credentials for network access.
- **Hardware**: An ESP32 board and a button connected to GPIO 0.

### 2. Hardware Connections

- **Button:** Connect a button to GPIO 0 of the ESP32, using the internal pull-up resistor (`INPUT_PULLUP`).

### 3. ESP32 Code

The ESP32 code is contained in the `main.c` file. It handles:
- **WiFi connection** to your specified network.
- **Time synchronization** using NTP servers.
- **UUID generation**.
- **WebSocket server** setup to communicate with clients.

#### 3.1 How to Use

1. **Open** `main.c` in your IDE.
2. **Update the WiFi credentials** in the code with your network's SSID and password.
3. **Upload** the code to your ESP32.
4. **Monitor** the serial output for connection status and debugging.

### 4. WebSocket Endpoint

The WebSocket server will be hosted at `ws://<ESP32_IP>:932/ws`. Replace `<ESP32_IP>` with the actual IP address of your ESP32 when connecting from a client.

## Python WebSocket Client

### 1. Requirements

- **Python 3.x** installed on your machine.
- **`websockets`** library.

### 2. Installation

Install the required Python library using pip:

```bash
pip install websockets
