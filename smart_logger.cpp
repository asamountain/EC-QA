#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <vector>
#include <ctime>
#include <cmath>
#include <cstring>
#include <modbus.h>
#include <unistd.h>
#include <cerrno>

// ===========================
// DYNAMIC COEFFICIENT LOOKUP
// ===========================
double get_dynamic_k(double temp) {
    if (temp <= 5.0) {
        return 0.0180;  // 1.80%
    } else if (temp <= 10.0) {
        return 0.0184;  // 1.84%
    } else if (temp <= 15.0) {
        return 0.0190;  // 1.90%
    } else if (temp <= 25.0) {
        return 0.0190;  // 1.90% (flat range)
    } else {
        return 0.0192;  // 1.92%
    }
}

// ===========================
// SMART ALGORITHM
// ===========================
double calculate_smart_ec(double raw_ec, double temp) {
    double k = get_dynamic_k(temp);
    // C25 = raw_ec / (1 + k * (temp - 25))
    return raw_ec / (1.0 + k * (temp - 25.0));
}

// ===========================
// PORT AUTO-DISCOVERY
// ===========================
std::string find_sensor_port() {
    std::vector<std::string> ports;
    
    // Scan /dev/ttyS0 through /dev/ttyS20 (WSL1/Legacy mode)
    for (int i = 0; i <= 20; i++) {
        ports.push_back("/dev/ttyS" + std::to_string(i));
    }
    
    // Also scan USB ports in case user switches to WSL2 USB passthrough
    for (int i = 0; i < 5; i++) {
        ports.push_back("/dev/ttyUSB" + std::to_string(i));
        ports.push_back("/dev/ttyACM" + std::to_string(i));
    }
    
    std::cout << "🔍 Scanning ports for BOQU IOT-485-EC4A (Slave ID: 4)..." << std::endl;
    
    uint16_t test_reg[2];
    
    for (const auto &port : ports) {
        modbus_t *ctx = modbus_new_rtu(port.c_str(), 9600, 'N', 8, 1);
        if (ctx == NULL) continue;
        
        modbus_set_slave(ctx, 4);  // CRITICAL: Slave ID 4, not 1
        modbus_set_response_timeout(ctx, 0, 100000);  // 100ms timeout
        
        if (modbus_connect(ctx) != -1) {
            // Try to read temperature register (60-61) as handshake
            int rc = modbus_read_registers(ctx, 60, 2, test_reg);
            
            if (rc != -1) {
                std::cout << "✅ FOUND SENSOR at: " << port << std::endl;
                modbus_close(ctx);
                modbus_free(ctx);
                return port;
            }
            modbus_close(ctx);
        }
        modbus_free(ctx);
    }
    
    return "";
}

// ===========================
// FLOAT CONVERSION (ABCD Big Endian)
// ===========================
float modbus_get_float_abcd(const uint16_t *src) {
    // ABCD format: [AB][CD] -> Big Endian
    uint32_t i;
    float f;
    
    // Combine two 16-bit registers into one 32-bit value
    // src[0] contains high word (AB), src[1] contains low word (CD)
    i = (((uint32_t)src[0]) << 16) | src[1];
    
    // Reinterpret as float
    memcpy(&f, &i, sizeof(float));
    
    return f;
}

// ===========================
// CLEAR SCREEN (Cross-platform)
// ===========================
void clear_screen() {
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif
}

// ===========================
// GET TIMESTAMP
// ===========================
std::string get_timestamp() {
    time_t now = time(0);
    struct tm tstruct;
    char buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tstruct);
    return buf;
}

// ===========================
// MAIN PROGRAM
// ===========================
int main() {
    // Step 1: Auto-discover the sensor
    std::string port = find_sensor_port();
    
    if (port.empty()) {
        std::cerr << "❌ ERROR: Sensor not found!" << std::endl;
        std::cerr << "   Check: USB connection, Slave ID (must be 4), Baud Rate (9600)" << std::endl;
        return -1;
    }
    
    // Step 2: Establish main connection
    modbus_t *ctx = modbus_new_rtu(port.c_str(), 9600, 'N', 8, 1);
    if (ctx == NULL) {
        std::cerr << "❌ Failed to create Modbus context" << std::endl;
        return -1;
    }
    
    modbus_set_slave(ctx, 4);
    modbus_set_response_timeout(ctx, 1, 0);  // 1 second for main loop
    
    if (modbus_connect(ctx) == -1) {
        std::cerr << "❌ Connection failed: " << modbus_strerror(errno) << std::endl;
        modbus_free(ctx);
        return -1;
    }
    
    std::cout << "\n🚀 Connected to sensor on " << port << std::endl;
    std::cout << "📊 Starting Smart Logger..." << std::endl;
    std::cout << "📝 Data will be logged to: ec_data_log.csv" << std::endl;
    std::cout << "   Press Ctrl+C to stop.\n" << std::endl;
    
    sleep(2);
    
    // Step 3: Create/Open CSV file
    std::ofstream csv_file;
    bool file_exists = (access("ec_data_log.csv", F_OK) != -1);
    
    csv_file.open("ec_data_log.csv", std::ios::app);
    
    // Write header if new file
    if (!file_exists) {
        csv_file << "Timestamp,Temperature,Raw_EC,Sensor_Default_EC,Smart_Calc_EC,Coefficient_Used,Deviation\n";
    }
    
    // Step 4: Main data acquisition loop
    uint16_t reg_data[2];
    int loop_count = 0;
    
    while (true) {
        loop_count++;
        
        // Read Temperature (Reg 60-61)
        double temp = 0.0;
        if (modbus_read_registers(ctx, 60, 2, reg_data) != -1) {
            temp = modbus_get_float_abcd(reg_data);
        } else {
            std::cerr << "⚠️  Failed to read temperature" << std::endl;
            sleep(1);
            continue;
        }
        
        // Read Raw EC (Reg 45-46)
        double raw_ec = 0.0;
        if (modbus_read_registers(ctx, 45, 2, reg_data) != -1) {
            raw_ec = modbus_get_float_abcd(reg_data);
        } else {
            std::cerr << "⚠️  Failed to read raw EC" << std::endl;
            sleep(1);
            continue;
        }
        
        // Read Sensor's Internal EC (Reg 41-42) - "The Wrong Value"
        double sensor_ec = 0.0;
        if (modbus_read_registers(ctx, 41, 2, reg_data) != -1) {
            sensor_ec = modbus_get_float_abcd(reg_data);
        } else {
            std::cerr << "⚠️  Failed to read sensor EC" << std::endl;
            sleep(1);
            continue;
        }
        
        // Calculate Smart EC
        double smart_ec = calculate_smart_ec(raw_ec, temp);
        double k_used = get_dynamic_k(temp);
        double deviation = sensor_ec - smart_ec;
        
        // Clear screen and display dashboard
        clear_screen();
        
        std::cout << "╔═══════════════════════════════════════════════════════════════╗\n";
        std::cout << "║         BOQU IOT-485-EC4A SMART COMPENSATION LOGGER          ║\n";
        std::cout << "╚═══════════════════════════════════════════════════════════════╝\n\n";
        
        std::cout << "  📡 Port: " << port << " | Slave ID: 4 | Samples: " << loop_count << "\n";
        std::cout << "  🕐 Time: " << get_timestamp() << "\n\n";
        
        std::cout << "┌───────────────────────────────────────────────────────────────┐\n";
        std::cout << "│ 🌡️  Temperature:        " << std::fixed << std::setprecision(2) 
                  << std::setw(10) << temp << " °C                    │\n";
        std::cout << "├───────────────────────────────────────────────────────────────┤\n";
        std::cout << "│ 📊 Raw EC (Uncomp):     " << std::setw(10) << raw_ec << " mS/cm                 │\n";
        std::cout << "│ 🔴 Sensor Default EC:   " << std::setw(10) << sensor_ec 
                  << " mS/cm (k=0.02 fixed)  │\n";
        std::cout << "│ 🟢 Smart Calc EC:       " << std::setw(10) << smart_ec 
                  << " mS/cm (k=" << std::setprecision(4) << k_used << ")      │\n";
        std::cout << "├───────────────────────────────────────────────────────────────┤\n";
        std::cout << "│ ⚖️  Deviation:           " << std::setprecision(4) << std::setw(10) << deviation 
                  << " mS/cm                 │\n";
        std::cout << "│                         " << std::setprecision(2) 
                  << std::setw(10) << (fabs(sensor_ec) > 0.01 ? (deviation/sensor_ec)*100.0 : 0.0) 
                  << " %                      │\n";
        std::cout << "└───────────────────────────────────────────────────────────────┘\n\n";
        
        std::cout << "  💡 Expected: 12.88 mS/cm @ 25°C (Standard Solution)\n";
        std::cout << "  📈 Goal: Prove Smart Algorithm reduces deviation\n\n";
        
        // Log to CSV
        csv_file << get_timestamp() << ","
                 << temp << ","
                 << raw_ec << ","
                 << sensor_ec << ","
                 << smart_ec << ","
                 << k_used << ","
                 << deviation << "\n";
        csv_file.flush();
        
        // Wait 1 second before next reading
        sleep(1);
    }
    
    // Cleanup (unreachable, but good practice)
    csv_file.close();
    modbus_close(ctx);
    modbus_free(ctx);
    
    return 0;
}
