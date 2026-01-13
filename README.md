# BOQU EC Sensor Smart Calibration System

**Automated Valuation & Temperature Compensation Testing for IOT-485-EC4A Sensors**

This project provides a complete, automated environment for testing and validating the temperature compensation algorithms of BOQU Conductivity Sensors. It specifically targets inaccuracies in the sensor's default linear compensation at low temperatures (5°C - 15°C) by implementing a dynamic coefficient algorithm.

**Key Feature:** Seamless integration between Windows (device handling) and WSL 2 (high-performance C++ execution) using a single automated script.

---

## 🚀 Quick Start (The "One-Click" Way)

You do **not** need to manually compile code or attach USB devices. Everything is handled by the automation script.

### Prerequisites
1.  **Windows 10/11**
2.  **WSL 2** installed (Ubuntu distribution recommended).
3.  **[usbipd-win](https://github.com/dorssel/usbipd-win)** installed (`winget install usbipd`).
4.  **USB-RS485 Adapter** with CH340 chipset connected.

### How to Run
1.  Open this folder in VS Code or File Explorer.
2.  Right-click `run_test.bat` and select **Run as Administrator**.
    *   *Note: Administrator privileges are required to attach the USB device to WSL.*

**The script will automatically:**
1.  ✅ Check for necessary tools (`usbipd`).
2.  ✅ Detect your USB-RS485 adapter (CH340).
3.  ✅ Wake up WSL 2.
4.  ✅ Attach the USB device to Linux.
5.  ✅ Compile the C++ Smart Logger (if needed).
6.  ✅ Run the data logging session.
7.  ✅ Offer to detach the device when finished.

---

## 📊 Visualization

After running a test, data is saved to `ec_data_log.csv`. To compare the Sensor's default values vs. the Smart Algorithm:

```bash
python plot_data.py
```
*(Requires `pandas` and `matplotlib`)*

---

## 📂 Project Structure

| File | Description |
| :--- | :--- |
| **`run_test.bat`** | **Start here.** Windows automation script that bridges Windows hardware with WSL software. |
| `smart_logger.cpp` | Main C++ program. Reads Modbus data, applies dynamic math, and logs results. |
| `auto_detect_sensor.cpp` | Helper utility to scan standard Modbus ports for the sensor. |
| `plot_data.py` | Python script to generate graphs of Temperature vs. EC Deviation. |
| `SMART_LOGGER_README.md` | Detailed documentation on the math and C++ implementation. |

---

## 🔧 Technical Details

### The Problem
The sensor uses a fixed temperature coefficient ($2.0\%/^\circ C$). This causes significant error when measuring 12.88 mS/cm standard solution at temperatures far from $25^\circ C$ (e.g., $5^\circ C$).

### The Solution: Smart Algorithm
We apply a **Dynamic Coefficient** based on the current temperature tier:

```cpp
double get_dynamic_k(double temp) {
    if (temp <= 5.0) return 0.0180;  // 1.80%
    if (temp <= 15.0) return 0.0190;  // 1.90%
    // ... etc
}
```

### Architecture
*   **Host:** Windows (Handles physical USB connection).
*   **Bridge:** `usbipd` (Passes USB straight through to the Linux kernel).
*   **Logic:** WSL 2 Ubuntu (Runs `libmodbus` C++ code for reliable real-time communication).

---

## 🛠️ Troubleshooting

**"The term 'sudo' is not recognized..."**
*   You are trying to run Linux commands in PowerShell. Use `.\run_test.bat` instead.

**"usbipd: error: There is no WSL 2 distribution running"**
*   The script should handle this now, but if it fails, open a separate Ubuntu terminal to keep WSL awake.

**"Sensor not found!"**
*   Check your wiring: A=A, B=B.
*   Ensure the sensor is powered (12V/24V).
*   Ensure the Slave ID is factory default (4).
