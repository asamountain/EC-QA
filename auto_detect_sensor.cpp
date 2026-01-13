#include <iostream>
#include <string>
#include <vector>
#include <modbus.h>
#include <cerrno>

// Function to generate port names based on OS
std::vector<std::string> get_candidate_ports() {
    std::vector<std::string> ports;
    
    #ifdef _WIN32
    // Windows: Scan COM1 to COM20
    // NOTE: Ports above COM9 require the "\\\\.\\" prefix
    for (int i = 1; i <= 20; i++) {
        ports.push_back("\\\\.\\COM" + std::to_string(i));
    }
    #else
    // Linux: Scan standard USB and ACM ports
    for (int i = 0; i < 10; i++) {
        ports.push_back("/dev/ttyUSB" + std::to_string(i));
        ports.push_back("/dev/ttyACM" + std::to_string(i));
    }
    #endif
    
    return ports;
}

// THE DISCOVERY FUNCTION
std::string find_sensor_port() {
    std::vector<std::string> port_list = get_candidate_ports();
    uint16_t tab_reg[1]; // Storage for the "Handshake" read

    std::cout << "Scanning ports for sensor..." << std::endl;

    for (const auto &port : port_list) {
        // 1. Create a temporary context for this port
        // Settings: 9600 Baud, N, 8, 1 (Must match sensor default)
        modbus_t *ctx = modbus_new_rtu(port.c_str(), 9600, 'N', 8, 1);
        
        if (ctx == NULL) continue;

        // 2. Set Slave ID (Default is 1)
        modbus_set_slave(ctx, 1);

        // 3. IMPORTANT: Set a Short Timeout
        // If a port is empty, we don't want to wait 5 seconds.
        // Set timeout to 200ms (0 sec, 200000 usec)
        modbus_set_response_timeout(ctx, 0, 200000);

        // 4. Try to Open
        if (modbus_connect(ctx) != -1) {
            // 5. The "Handshake": Try to read Register 8 (Device Address)
            // This confirms it is actually YOUR sensor, not a mouse or printer.
            int rc = modbus_read_registers(ctx, 8, 1, tab_reg);

            if (rc != -1) {
                // SUCCESS! We got a valid reply.
                std::cout << " >> FOUND SENSOR at: " << port << std::endl;
                std::cout << " >> Device ID: " << tab_reg[0] << std::endl;
                modbus_close(ctx);
                modbus_free(ctx);
                return port; // Return the valid port string
            }
            modbus_close(ctx);
        }
        modbus_free(ctx);
    }

    return ""; // Return empty string if not found
}

// --- MAIN PROGRAM ---
int main() {
    // Step 1: Auto-Detect the Port
    std::string valid_port = find_sensor_port();

    if (valid_port.empty()) {
        std::cerr << "ERROR: Sensor not found on any port!" << std::endl;
        std::cerr << "Check USB connection and power." << std::endl;
        return -1;
    }

    // Step 2: Use the Found Port for the Real Connection
    std::cout << "Connecting to live sensor on " << valid_port << "..." << std::endl;
    
    modbus_t *main_ctx = modbus_new_rtu(valid_port.c_str(), 9600, 'N', 8, 1);
    modbus_set_slave(main_ctx, 1);
    
    if (modbus_connect(main_ctx) == -1) {
        std::cerr << "Connection failed." << std::endl;
        modbus_free(main_ctx);
        return -1;
    }

    // ... Proceed with your Smart Algorithm Loop Here ...

    modbus_close(main_ctx);
    modbus_free(main_ctx);
    return 0;
}