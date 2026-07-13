/**
 * RedroidCPP - Professional Android Emulator Manager v3.0.0
 * Complete Device Profile with all properties
 * 
 * Features:
 * - Device IDs: IMEI, IMEI2, Serial, Android ID, GSF ID, Advertising ID
 * - MAC Addresses: WiFi, Bluetooth, Ethernet
 * - SIM: ICCID, IMSI
 * - Hardware: CPU, GPU, RAM, Battery
 * - Build: Fingerprint, Bootloader, Build ID, Security Patch
 * - Verified Boot: State (green), VBMeta digest
 * - Network: Hostname, TCP buffers, DNS
 * - GPS: Latitude, Longitude, Altitude, Accuracy
 * - Sensors: Accelerometer, Gyro, Mag, Baro, Light, Proximity
 * - Security: KNOX ID, SELinux, Hardware attestation
 */

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <array>
#include <random>

// Include the device profile header
#include "Core/DeviceProfile.h"

using namespace RedroidCPP;

// ============================================================================
// ANSI Color Codes
// ============================================================================
#define COLOR_RESET   "\033[0m"
#define COLOR_BOLD    "\033[1m"
#define COLOR_DIM     "\033[2m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_WHITE   "\033[37m"

// ============================================================================
// Utility Functions
// ============================================================================

namespace {
std::string exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    FILE* pipe = popen(cmd, "r");
    if (!pipe) return "";
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        result += buffer.data();
    }
    pclose(pipe);
    return result;
}

bool checkDocker() {
    return !exec("docker --version 2>/dev/null").empty();
}

std::string trim(const std::string& s) {
    auto start = std::find_if(s.begin(), s.end(), [](unsigned char c) { return !std::isspace(c); });
    auto end = std::find_if(s.rbegin(), s.rend(), [](unsigned char c) { return !std::isspace(c); }).base();
    return (start < end) ? std::string(start, end) : "";
}

void printBanner() {
    std::cout << COLOR_CYAN << R"(
╔══════════════════════════════════════════════════════════════════════════════════╗
║                                                                              ║
║   ██████╗ ███████╗███╗   ██╗███████╗████████╗███████╗██████╗ ███╗   ███╗  ║
║   ██╔══██╗██╔════╝████╗  ██║██╔════╝╚══██╔══╝██╔════╝██╔══██╗████╗ ████║  ║
║   ██████╔╝█████╗  ██╔██╗ ██║███████╗   ██║   █████╗  ██████╔╝██╔████╔██║  ║
║   ██╔═══╝ ██╔══╝  ██║╚██╗██║╚════██║   ██║   ██╔══╝  ██╔══██╗██║╚██╔╝██║  ║
║   ██║     ███████╗██║ ╚████║███████║   ██║   ███████╗██║  ██║██║ ╚═╝ ██║  ║
║   ╚═╝     ╚══════╝╚═╝  ╚═══╝╚══════╝   ╚═╝   ╚══════╝╚═╝  ╚═╝╚═╝     ╚═╝  ║
║                                                                              ║
║                 Android Emulator Manager v3.0.0                              ║
║            Complete Device Profile with All Properties                       ║
║                                                                              ║
╚══════════════════════════════════════════════════════════════════════════════════╝
)" << COLOR_RESET << std::endl;
}
}

// ============================================================================
// Device Manager (Simple in-memory storage)
// ============================================================================

class DeviceManager {
public:
    static DeviceManager& getInstance() {
        static DeviceManager instance;
        return instance;
    }
    
    struct Device {
        std::string id;
        std::string name;
        std::string manufacturer;
        std::string androidVersion;
        DeviceProfile profile;
        std::string status;
        int port;
        std::chrono::system_clock::time_point createdAt;
    };
    
    bool initialize() {
        std::cout << COLOR_GREEN << "[*] " << COLOR_RESET << "Checking Docker..." << std::flush;
        if (checkDocker()) {
            std::string version = exec("docker version --format '{{.Server.Version}}' 2>/dev/null");
            if (!version.empty()) {
                std::cout << COLOR_GREEN << " OK (" << version.substr(0, version.find('\n')) << ")" << COLOR_RESET << std::endl;
                return true;
            }
        }
        std::cout << COLOR_YELLOW << " Not running" << COLOR_RESET << std::endl;
        return true;
    }
    
    Device createDevice(const std::string& manufacturer = "",
                       const std::string& androidVersion = "14") {
        Device device;
        device.profile = DeviceProfile(manufacturer.empty() ? "Samsung" : manufacturer);
        device.profile.generate(manufacturer.empty() ? "Samsung" : manufacturer, "", androidVersion);
        
        device.id = device.profile.profileId;
        device.name = device.profile.profileName;
        device.manufacturer = device.profile.manufacturer;
        device.androidVersion = device.profile.androidVersion.versionNumber;
        device.status = "created";
        device.port = 5555 + m_devices.size();
        device.createdAt = std::chrono::system_clock::now();
        
        m_devices.push_back(device);
        return device;
    }
    
    bool deleteDevice(const std::string& id) {
        auto it = std::find_if(m_devices.begin(), m_devices.end(),
                              [&id](const Device& d) { return d.id == id; });
        if (it != m_devices.end()) {
            m_devices.erase(it);
            return true;
        }
        return false;
    }
    
    std::optional<Device> getDevice(const std::string& id) {
        auto it = std::find_if(m_devices.begin(), m_devices.end(),
                              [&id](const Device& d) { return d.id == id; });
        if (it != m_devices.end()) {
            return *it;
        }
        return std::nullopt;
    }
    
    std::vector<Device> getAllDevices() const { return m_devices; }
    size_t getDeviceCount() const { return m_devices.size(); }

private:
    DeviceManager() = default;
    std::vector<Device> m_devices;
};

// ============================================================================
// CLI Commands
// ============================================================================

void printHelp() {
    std::cout << "\n" << COLOR_BOLD << "USAGE:" << COLOR_RESET << std::endl;
    std::cout << "  redroid <command> [options]\n" << std::endl;
    
    std::cout << COLOR_BOLD << "COMMANDS:" << COLOR_RESET << std::endl;
    std::cout << "  create [options]          Create a new virtual device\n";
    std::cout << "  list                      List all devices\n";
    std::cout << "  info <device-id>          Show detailed device information\n";
    std::cout << "  delete <device-id>        Delete a device\n";
    std::cout << "  profile [options]         Generate device profile only\n";
    std::cout << "  status                    Show system status\n";
    std::cout << "  manufacturers             List supported manufacturers\n";
    std::cout << "  validate <imei>           Validate IMEI number\n";
    std::cout << "  props <device-id>         Show all properties for device\n";
    std::cout << "  help                     Show this help\n" << std::endl;
    
    std::cout << COLOR_BOLD << "OPTIONS:" << COLOR_RESET << std::endl;
    std::cout << "  -m, --manufacturer       Device manufacturer\n";
    std::cout << "  -a, --android            Android version (14, 15, 16)\n";
    std::cout << "  -n, --name               Device name\n" << std::endl;
    
    std::cout << COLOR_BOLD << "MANUFACTURERS:" << COLOR_RESET << std::endl;
    auto& db = TACDatabase::getInstance();
    auto manufacturers = db.getManufacturers();
    for (size_t i = 0; i < manufacturers.size(); ++i) {
        std::cout << "  " << std::left << std::setw(12) << manufacturers[i];
        if ((i + 1) % 4 == 0) std::cout << "\n";
    }
    std::cout << "\n" << std::endl;
    
    std::cout << COLOR_BOLD << "EXAMPLES:" << COLOR_RESET << std::endl;
    std::cout << "  redroid create -m Samsung -a 14\n";
    std::cout << "  redroid create -m Google --name \"My Pixel\"\n";
    std::cout << "  redroid profile -m Xiaomi\n";
    std::cout << "  redroid info dev_12345678\n";
    std::cout << "  redroid props dev_12345678\n";
    std::cout << "  redroid validate 358751090123456\n" << std::endl;
}

void printManufacturers() {
    std::cout << "\n" << COLOR_BOLD << "Supported Manufacturers:" << COLOR_RESET << std::endl;
    std::cout << "─────────────────────────────────────────────────────────────────\n";
    
    auto& db = TACDatabase::getInstance();
    auto manufacturers = db.getManufacturers();
    
    for (size_t i = 0; i < manufacturers.size(); ++i) {
        auto entries = db.getByManufacturer(manufacturers[i]);
        std::cout << "  [" << std::setw(2) << (i + 1) << "] " 
                  << std::left << std::setw(12) << manufacturers[i]
                  << " (" << entries.size() << " models)\n";
    }
    std::cout << std::endl;
}

void printDeviceList(const std::vector<DeviceManager::Device>& devices) {
    if (devices.empty()) {
        std::cout << COLOR_YELLOW << "\nNo devices found. Create one with 'redroid create'\n" << COLOR_RESET << std::endl;
        return;
    }
    
    std::cout << "\n" << COLOR_BOLD;
    std::cout << "┌──────────┬────────────────────────────┬────────────────┬─────────────┬────────┐\n";
    std::cout << "│ ID       │ Name                        │ Manufacturer   │ Android    │ Status │\n";
    std::cout << "├──────────┼────────────────────────────┼────────────────┼─────────────┼────────┤\n";
    std::cout << COLOR_RESET;
    
    for (const auto& device : devices) {
        std::string shortId = device.id.length() > 8 ? device.id.substr(0, 8) : device.id;
        std::string shortName = device.name.length() > 26 ? device.name.substr(0, 23) + "..." : device.name;
        
        std::cout << "│ " << std::left << std::setw(8) << shortId 
                  << " │ " << std::setw(26) << shortName
                  << " │ " << std::setw(14) << device.manufacturer
                  << " │ " << std::setw(11) << ("Android " + device.androidVersion)
                  << " │ " << std::setw(6) << device.status << " │" << std::endl;
    }
    
    std::cout << COLOR_BOLD;
    std::cout << "└──────────┴────────────────────────────┴────────────────┴─────────────┴────────┘\n";
    std::cout << COLOR_RESET;
    std::cout << "Total: " << devices.size() << " device(s)\n" << std::endl;
}

void printDeviceInfo(const DeviceManager::Device& device) {
    device.profile.print();
}

void printProperties(const DeviceManager::Device& device) {
    auto categories = device.profile.getPropertiesByCategory();
    
    std::cout << "\n" << COLOR_BOLD << "═══════════════════════════════════════════════════════════════════════════════════" << COLOR_RESET << std::endl;
    std::cout << "                    ALL DEVICE PROPERTIES - " << device.name << std::endl;
    std::cout << COLOR_BOLD << "═══════════════════════════════════════════════════════════════════════════════════" << COLOR_RESET << std::endl;
    
    for (const auto& [category, props] : categories) {
        std::cout << "\n" << COLOR_CYAN << "[ " << category << " ]" << COLOR_RESET << "\n";
        std::cout << std::string(70, '-') << "\n";
        
        for (const auto& [key, value] : props) {
            std::string displayValue = value;
            if (value.length() > 50) {
                displayValue = value.substr(0, 47) + "...";
            }
            std::cout << "  " << std::left << std::setw(40) << key << " " 
                      << COLOR_DIM << displayValue << COLOR_RESET << "\n";
        }
    }
    
    std::cout << "\n" << COLOR_BOLD << "═══════════════════════════════════════════════════════════════════════════════════" << COLOR_RESET << std::endl;
}

void printStatus() {
    std::cout << "\n" << COLOR_BOLD << "════════════════════════════════════════" << COLOR_RESET << std::endl;
    std::cout << "            SYSTEM STATUS             " << std::endl;
    std::cout << COLOR_BOLD << "════════════════════════════════════════" << COLOR_RESET << std::endl;
    
    std::cout << "\n" << COLOR_CYAN << "[ DOCKER ]" << COLOR_RESET << std::endl;
    std::string dockerVersion = exec("docker version --format '{{.Server.Version}}' 2>/dev/null");
    std::string containers = exec("docker ps -a 2>/dev/null | tail -n +2 | wc -l");
    std::string running = exec("docker ps 2>/dev/null | tail -n +2 | wc -l");
    
    if (dockerVersion.empty()) {
        std::cout << "  " << COLOR_RED << "Docker not available" << COLOR_RESET << std::endl;
    } else {
        std::cout << "  Version:     " << dockerVersion.substr(0, dockerVersion.find('\n')) << std::endl;
        std::cout << "  Containers:   " << trim(containers) << " total, " << trim(running) << " running" << std::endl;
    }
    
    std::cout << "\n" << COLOR_CYAN << "[ DEVICES ]" << COLOR_RESET << std::endl;
    auto& manager = DeviceManager::getInstance();
    std::cout << "  Total:       " << manager.getDeviceCount() << std::endl;
    
    std::cout << "\n" << COLOR_BOLD << "════════════════════════════════════════" << COLOR_RESET << std::endl;
}

// ============================================================================
// Main Entry Point
// ============================================================================

int main(int argc, char* argv[]) {
    printBanner();
    
    DeviceManager& manager = DeviceManager::getInstance();
    if (!manager.initialize()) {
        return 1;
    }
    
    if (argc < 2) {
        printHelp();
        return 0;
    }
    
    std::string command = argv[1];
    
    if (command == "help" || command == "--help" || command == "-h") {
        printHelp();
    }
    else if (command == "manufacturers") {
        printManufacturers();
    }
    else if (command == "list") {
        printDeviceList(manager.getAllDevices());
    }
    else if (command == "status") {
        printStatus();
    }
    else if (command == "create") {
        std::string manufacturer, androidVersion = "14", name;
        
        for (int i = 2; i < argc; ++i) {
            std::string arg = argv[i];
            if ((arg == "-m" || arg == "--manufacturer") && i + 1 < argc) {
                manufacturer = argv[++i];
            } else if ((arg == "-a" || arg == "--android") && i + 1 < argc) {
                androidVersion = argv[++i];
            } else if ((arg == "-n" || arg == "--name") && i + 1 < argc) {
                name = argv[++i];
            }
        }
        
        std::cout << "\n" << COLOR_GREEN << "[*] Creating device..." << COLOR_RESET << std::endl;
        
        auto device = manager.createDevice(manufacturer, androidVersion);
        std::cout << COLOR_GREEN << "[+] Device created successfully!" << COLOR_RESET << std::endl;
        printDeviceInfo(device);
    }
    else if (command == "info") {
        if (argc < 3) {
            std::cout << COLOR_RED << "Error: Device ID required" << COLOR_RESET << std::endl;
            return 1;
        }
        
        auto deviceOpt = manager.getDevice(argv[2]);
        if (deviceOpt) {
            printDeviceInfo(*deviceOpt);
        } else {
            std::cout << COLOR_RED << "Error: Device not found" << COLOR_RESET << std::endl;
            return 1;
        }
    }
    else if (command == "props") {
        if (argc < 3) {
            std::cout << COLOR_RED << "Error: Device ID required" << COLOR_RESET << std::endl;
            return 1;
        }
        
        auto deviceOpt = manager.getDevice(argv[2]);
        if (deviceOpt) {
            printProperties(*deviceOpt);
        } else {
            std::cout << COLOR_RED << "Error: Device not found" << COLOR_RESET << std::endl;
            return 1;
        }
    }
    else if (command == "delete") {
        if (argc < 3) {
            std::cout << COLOR_RED << "Error: Device ID required" << COLOR_RESET << std::endl;
            return 1;
        }
        
        if (manager.deleteDevice(argv[2])) {
            std::cout << COLOR_GREEN << "[+] Device deleted" << COLOR_RESET << std::endl;
        } else {
            std::cout << COLOR_RED << "[-] Device not found" << COLOR_RESET << std::endl;
            return 1;
        }
    }
    else if (command == "profile") {
        std::string manufacturer;
        for (int i = 2; i < argc; ++i) {
            std::string arg = argv[i];
            if ((arg == "-m" || arg == "--manufacturer") && i + 1 < argc) {
                manufacturer = argv[++i];
            }
        }
        
        DeviceProfile profile;
        profile.generate(manufacturer.empty() ? "Samsung" : manufacturer);
        
        DeviceManager::Device device;
        device.id = profile.profileId;
        device.name = profile.profileName;
        device.manufacturer = profile.manufacturer;
        device.profile = profile;
        
        printDeviceInfo(device);
    }
    else if (command == "validate") {
        if (argc < 3) {
            std::cout << COLOR_RED << "Error: IMEI required" << COLOR_RESET << std::endl;
            return 1;
        }
        
        std::string imei = argv[2];
        if (IMEIGenerator::validate(imei)) {
            std::cout << COLOR_GREEN << "[+] Valid IMEI" << COLOR_RESET << std::endl;
        } else {
            std::cout << COLOR_RED << "[-] Invalid IMEI" << COLOR_RESET << std::endl;
            return 1;
        }
    }
    else {
        std::cout << COLOR_RED << "Unknown command: " << command << COLOR_RESET << std::endl;
        std::cout << "Type 'redroid help' for usage\n";
        return 1;
    }
    
    return 0;
}
