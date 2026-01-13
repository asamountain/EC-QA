# BOQU IOT-485-EC4A Smart Temperature Compensation Logger

## 🎯 Project Overview

This project proves that the BOQU IOT-485-EC4A Conductivity Sensor's internal "Linear Temperature Compensation" (fixed at k=2.0%) is **inaccurate at low temperatures** for 12.88 mS/cm Standard Solution.

We implement a **Smart Algorithm with Dynamic Temperature Coefficients** that provides more accurate compensation, and log both values to CSV for comparison analysis.

---

## 📋 Hardware Requirements

- **Device**: BOQU IOT-485-EC4A Conductivity Sensor
- **Protocol**: RS485 Modbus RTU
- **Slave ID**: 4 (Factory Default)
- **Baud Rate**: 9600, N, 8, 1
- **Adapter**: USB-RS485 Converter
- **Environment**: WSL2 Ubuntu on Windows

---

## 🔧 Software Dependencies

### C++ Compilation (WSL2/Ubuntu)

```bash
# Install libmodbus
sudo apt-get update
sudo apt-get install libmodbus-dev

# Install build tools (if not already installed)
sudo apt-get install build-essential pkg-config
```

### Python Visualization

```bash
# Install Python dependencies
pip3 install pandas matplotlib numpy

# Or using apt
sudo apt-get install python3-pandas python3-matplotlib python3-numpy
```

---

## 🚀 Compilation Instructions

### Compile the Smart Logger

```bash
cd /mnt/c/Users/iocrops\ admin/Coding/EC-QA

# Compile with pkg-config (recommended)
g++ -o smart_logger smart_logger.cpp $(pkg-config --cflags --libs libmodbus)

# OR manually specify libmodbus
g++ -o smart_logger smart_logger.cpp -lmodbus
```

---

## 📡 WSL2 USB Device Setup

If you're using WSL2 (not WSL1), you need to pass the USB device through:

### Step 1: Install usbipd-win on Windows

```powershell
# In Windows PowerShell (as Administrator)
winget install usbipd
```

### Step 2: Find Your USB-RS485 Device

```powershell
# List USB devices
usbipd list
```

Look for something like: `USB-SERIAL CH340(COM3)` with BUSID `2-1`

### Step 3: Bind and Attach

```powershell
# One-time bind (persists across reboots)
usbipd bind --busid 2-1

# Attach to WSL (needed each time WSL restarts)
usbipd attach --wsl --busid 2-1
```

### Step 4: Verify in WSL

```bash
# Check if device appears
ls -l /dev/ttyUSB* /dev/ttyACM*

# Should see something like:
# /dev/ttyUSB0 or /dev/ttyACM0
```

### Step 5: Detach When Done (to use in Windows again)

```powershell
usbipd detach --busid 2-1
```

---

## 🏃 Running the Logger

### Option 1: With Auto-Discovery (Recommended)

```bash
# Run with sudo (required for serial port access)
sudo ./smart_logger
```

The program will automatically scan all available ports and find the sensor.

### Option 2: Add User to dialout Group (No sudo needed)

```bash
# Add your user to dialout group
sudo usermod -a -G dialout $USER

# Log out and log back in for changes to take effect
# Then run without sudo:
./smart_logger
```

---

## 📊 Output

### Terminal Dashboard (Updates every 1 second)

```
╔═══════════════════════════════════════════════════════════════╗
║         BOQU IOT-485-EC4A SMART COMPENSATION LOGGER          ║
╚═══════════════════════════════════════════════════════════════╝

  📡 Port: /dev/ttyUSB0 | Slave ID: 4 | Samples: 42
  🕐 Time: 2026-01-13 15:30:42

┌───────────────────────────────────────────────────────────────┐
│ 🌡️  Temperature:           8.50 °C                          │
├───────────────────────────────────────────────────────────────┤
│ 📊 Raw EC (Uncomp):       14.23 mS/cm                         │
│ 🔴 Sensor Default EC:     13.10 mS/cm (k=0.02 fixed)          │
│ 🟢 Smart Calc EC:         12.88 mS/cm (k=0.0184)              │
├───────────────────────────────────────────────────────────────┤
│ ⚖️  Deviation:             0.22 mS/cm                         │
│                            1.68 %                             │
└───────────────────────────────────────────────────────────────┘

  💡 Expected: 12.88 mS/cm @ 25°C (Standard Solution)
  📈 Goal: Prove Smart Algorithm reduces deviation
```

### CSV Log File

Data is saved to: `ec_data_log.csv`

**Columns:**
- `Timestamp`: Date and time of reading
- `Temperature`: Measured temperature (°C)
- `Raw_EC`: Uncompensated conductivity (mS/cm)
- `Sensor_Default_EC`: Sensor's internal calculation (k=0.02)
- `Smart_Calc_EC`: Our smart algorithm result
- `Coefficient_Used`: Dynamic k value used
- `Deviation`: Difference between Sensor and Smart values

---

## 📈 Data Visualization

After collecting data (let it run for at least 10-15 minutes), generate comparison charts:

```bash
# Make the script executable
chmod +x plot_data.py

# Run visualization
python3 plot_data.py
```

### Generated Charts

1. **ec_comparison_chart.png**: 
   - Top panel: Conductivity vs Temperature for both algorithms
   - Bottom panel: Deviation analysis
   - Includes statistical summary

2. **coefficient_analysis.png**:
   - Shows which k coefficient was used at each temperature
   - Visualizes the dynamic coefficient strategy

---

## 🧮 The Smart Algorithm

### Mathematical Formula

$$C_{25} = \frac{\text{raw\_ec}}{1 + k \times (\text{temp} - 25)}$$

### Dynamic Coefficient Lookup Table

| Temperature Range | Coefficient k | Percentage |
|-------------------|---------------|------------|
| T ≤ 5°C           | 0.0180        | 1.80%      |
| 5°C < T ≤ 10°C    | 0.0184        | 1.84%      |
| 10°C < T ≤ 15°C   | 0.0190        | 1.90%      |
| 15°C < T ≤ 25°C   | 0.0190        | 1.90%      |
| T > 25°C          | 0.0192        | 1.92%      |

**Sensor Default**: Uses k = 0.02 (2.0%) for all temperatures ❌

**Smart Algorithm**: Uses temperature-dependent k values ✅

---

## 📚 Modbus Register Map

| Register | Description | Format | Variable |
|----------|-------------|--------|----------|
| 41-42    | Sensor Internal EC | Float ABCD | `sensor_ec` |
| 45-46    | Raw EC (Uncompensated) | Float ABCD | `raw_ec` |
| 60-61    | Temperature | Float ABCD | `temp` |

**Float Format**: ABCD (Big Endian, 2 registers per value)

---

## 🛠️ Troubleshooting

### Issue: "Sensor not found on any port!"

**Solutions:**
1. Check USB connection
2. Verify sensor is powered on
3. Confirm Slave ID is 4 (not default 1)
4. Check baud rate: 9600, N, 8, 1
5. For WSL2: Ensure USB device is attached via `usbipd`

### Issue: "Permission denied" on serial port

**Solutions:**
```bash
# Option 1: Run with sudo
sudo ./smart_logger

# Option 2: Add user to dialout group
sudo usermod -a -G dialout $USER
# Then log out and back in
```

### Issue: Compilation error "modbus.h: No such file"

**Solution:**
```bash
sudo apt-get install libmodbus-dev
```

### Issue: Python plot shows "No module named 'pandas'"

**Solution:**
```bash
pip3 install pandas matplotlib numpy
```

---

## 🎯 Expected Results

When testing with **12.88 mS/cm Standard Solution**:

✅ **Smart Algorithm**: Should show ~12.88 mS/cm across all temperatures (stable)

❌ **Sensor Default**: Will show drift/overcompensation at low temperatures

### Success Criteria

- Smart Algorithm Standard Deviation < Sensor Default Std Dev
- Smart Algorithm RMSE < Sensor Default RMSE
- Smart Algorithm stays closer to 12.88 mS/cm reference

---

## 📝 Notes

1. **Data Collection**: Let the logger run for at least 15-30 minutes to collect meaningful data across temperature variations

2. **Temperature Range**: For best results, test across 5°C to 30°C range

3. **Stopping the Logger**: Press `Ctrl+C` to stop data collection

4. **CSV Appending**: Data is appended to CSV, so you can restart the logger without losing previous data

5. **Backup Data**: Consider backing up `ec_data_log.csv` before running new experiments

---

## 👨‍💻 Author

Senior Embedded Systems Engineer & Data Scientist  
Specializing in Industrial IoT and Modbus Protocols

---

## 📜 License

This project is provided as-is for research and validation purposes.

---

## 🔗 Quick Reference Commands

```bash
# Compile
g++ -o smart_logger smart_logger.cpp $(pkg-config --cflags --libs libmodbus)

# Run logger
sudo ./smart_logger

# Generate plots (after data collection)
python3 plot_data.py

# Check USB devices (Windows PowerShell)
usbipd list

# Attach USB to WSL (Windows PowerShell as Admin)
usbipd attach --wsl --busid 2-1
```

---

**Happy Data Logging! 📊🎉**
