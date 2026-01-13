#include <iostream>
#include <modbus.h>

int main() {
    std::cout << "Testing COM10 connection..." << std::endl;
    
    // Create connection to COM10
    modbus_t *ctx = modbus_new_rtu("\\\\.\\COM10", 9600, 'N', 8, 1);
    
    if (ctx == NULL) {
        std::cerr << "Failed to create Modbus context" << std::endl;
        return -1;
    }
    
    modbus_set_slave(ctx, 1);
    modbus_set_response_timeout(ctx, 1, 0);
    
    if (modbus_connect(ctx) == -1) {
        std::cerr << "Connection failed: " << modbus_strerror(errno) << std::endl;
        modbus_free(ctx);
        return -1;
    }
    
    std::cout << "Connected to COM10!" << std::endl;
    
    uint16_t reg[1];
    int rc = modbus_read_registers(ctx, 8, 1, reg);
    
    if (rc != -1) {
        std::cout << "SUCCESS! Device ID: " << reg[0] << std::endl;
    } else {
        std::cerr << "Read failed: " << modbus_strerror(errno) << std::endl;
    }
    
    modbus_close(ctx);
    modbus_free(ctx);
    return 0;
}
