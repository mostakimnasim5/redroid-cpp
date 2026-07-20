#include "VirtualPhonePro/ADBManager.hpp"
#include "VirtualPhonePro/Logger.hpp"
#include <cstdlib>
#include <sstream>
#include <array>
#include <chrono>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#include <process.h>
#define popen _popen
#define pclose _pclose
#else
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#endif

namespace VirtualPhonePro {

ADBManager& ADBManager::getInstance() {
    static ADBManager instance;
    return instance;
}

ADBManager::ADBManager()
    : m_adbPath(DEFAULT_ADB_PATH)
    , m_connectionState(ADBConnectionState::DISCONNECTED)
{
}

ADBManager::~ADBManager() {
    disconnectAll();
}

bool ADBManager::initialize() {
    Logger::getInstance().info("Initializing ADB Manager...");
    
    m_adbPath = findADBPath();
    
    if (m_adbPath.empty()) {
        m_lastError = "ADB not found in system PATH";
        Logger::getInstance().error(m_lastError);
        m_connectionState = ADBConnectionState::ERROR;
        return false;
    }
    
    std::vector<std::string> initArgs = {m_adbPath, "start-server"};
    if (!runADBCommand(initArgs)) {
        m_lastError = "Failed to start ADB server";
        Logger::getInstance().error(m_lastError);
        m_connectionState = ADBConnectionState::ERROR;
        return false;
    }
    
    std::vector<DeviceInfo> devices = getDevices();
    if (!devices.empty()) {
        m_connectionState = ADBConnectionState::CONNECTED;
        m_selectedDevice = devices[0].serial;
        Logger::getInstance().info("ADB initialized. Found " + std::to_string(devices.size()) + " device(s)");
        return true;
    }
    
    m_connectionState = ADBConnectionState::DISCONNECTED;
    Logger::getInstance().warning("ADB initialized but no devices connected");
    return true;
}

std::string ADBManager::findADBPath() {
    std::vector<std::string> searchPaths = {
#ifdef _WIN32
        "adb.exe",
        "adb",
        "C:\\Android\\platform-tools\\adb.exe",
        "C:\\Program Files\\Android\\platform-tools\\adb.exe",
        "C:\\Users\\" + std::string(getenv("USERNAME") ? getenv("USERNAME") : "") + "\\AppData\\Local\\Android\\Sdk\\platform-tools\\adb.exe"
#else
        "/usr/bin/adb",
        "/usr/local/bin/adb",
        "/opt/android-sdk/platform-tools/adb",
        "/Users/Shared/Android/sdk/platform-tools/adb",
        "adb"
#endif
    };
    
    for (const auto& path : searchPaths) {
        if (path.empty()) continue;
        
        std::string cmd = 
#ifdef _WIN32
            "where " + path + " 2>nul";
#else
            "which " + path + " 2>/dev/null";
#endif
        
        FILE* pipe = popen(cmd.c_str(), "r");
        if (pipe) {
            char buffer[512];
            if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                pclose(pipe);
                std::string result(buffer);
                result.erase(result.find_last_not_of(" \n\r\t") + 1);
                return result;
            }
            pclose(pipe);
        }
    }
    
    return "adb";
}

bool ADBManager::verifyADBInstalled() {
    std::string result = executeADBCommand({"version"});
    return !result.empty() && result.find("Android Debug Bridge") != std::string::npos;
}

bool ADBManager::isConnected() const {
    return m_connectionState == ADBConnectionState::CONNECTED && !m_selectedDevice.empty();
}

std::string ADBManager::getLastError() const {
    return m_lastError;
}

std::vector<DeviceInfo> ADBManager::getDevices() {
    std::vector<DeviceInfo> devices;
    
    std::string output = executeADBCommand({"devices", "-l"});
    if (output.empty()) {
        return devices;
    }
    
    std::istringstream stream(output);
    std::string line;
    bool skipFirst = true;
    
    while (std::getline(stream, line)) {
        if (skipFirst) {
            skipFirst = false;
            continue;
        }
        
        if (line.empty() || line.find("List of devices") != std::string::npos) {
            continue;
        }
        
        std::istringstream iss(line);
        DeviceInfo info;
        
        iss >> info.serial >> info.state;
        
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            std::string props = line.substr(colonPos + 1);
            
            auto extractProp = [&props](const std::string& key) -> std::string {
                size_t pos = props.find(key + ":");
                if (pos != std::string::npos) {
                    size_t start = pos + key.length() + 1;
                    size_t end = props.find(' ', start);
                    if (end == std::string::npos) end = props.length();
                    return props.substr(start, end - start);
                }
                return "";
            };
            
            info.product = extractProp("product");
            info.model = extractProp("model");
            info.device = extractProp("device");
            info.transportId = extractProp("transport_id");
        }
        
        if (!info.serial.empty() && info.serial != "offline") {
            devices.push_back(info);
        }
    }
    
    return devices;
}

bool ADBManager::connect(const std::string& deviceAddress) {
    Logger::getInstance().info("Connecting to device: " + deviceAddress);
    
    m_connectionState = ADBConnectionState::CONNECTING;
    
    std::vector<std::string> args = {"connect", deviceAddress};
    std::string result = executeADBCommand(args);
    
    if (result.find("connected") != std::string::npos || 
        result.find("already connected") != std::string::npos) {
        
        m_connectionState = ADBConnectionState::CONNECTED;
        m_selectedDevice = deviceAddress;
        Logger::getInstance().info("Successfully connected to: " + deviceAddress);
        return true;
    }
    
    m_connectionState = ADBConnectionState::ERROR;
    m_lastError = "Failed to connect: " + result;
    Logger::getInstance().error(m_lastError);
    return false;
}

bool ADBManager::disconnect(const std::string& deviceAddress) {
    Logger::getInstance().info("Disconnecting from device: " + deviceAddress);
    
    std::vector<std::string> args = {"disconnect", deviceAddress};
    bool success = runADBCommand(args);
    
    if (success && m_selectedDevice == deviceAddress) {
        m_selectedDevice.clear();
        m_connectionState = ADBConnectionState::DISCONNECTED;
    }
    
    return success;
}

bool ADBManager::disconnectAll() {
    Logger::getInstance().info("Disconnecting all devices...");
    
    std::vector<std::string> args = {"disconnect"};
    bool success = runADBCommand(args);
    
    m_selectedDevice.clear();
    m_connectionState = ADBConnectionState::DISCONNECTED;
    
    return success;
}

bool ADBManager::selectDevice(const std::string& serial) {
    std::vector<DeviceInfo> devices = getDevices();
    
    for (const auto& device : devices) {
        if (device.serial == serial) {
            m_selectedDevice = serial;
            Logger::getInstance().info("Selected device: " + serial);
            return true;
        }
    }
    
    m_lastError = "Device not found: " + serial;
    Logger::getInstance().error(m_lastError);
    return false;
}

std::string ADBManager::getSelectedDevice() const {
    return m_selectedDevice;
}

std::string ADBManager::executeADBCommand(const std::vector<std::string>& args) {
    return executeADBCommand(args, DEFAULT_TIMEOUT_MS);
}

std::string ADBManager::executeADBCommand(const std::vector<std::string>& args, int timeoutMs) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::string cmd = m_adbPath;
    
    if (!m_selectedDevice.empty()) {
        cmd += " -s " + m_selectedDevice;
    }
    
    for (const auto& arg : args) {
        cmd += " " + arg;
    }
    
    Logger::getInstance().debug("Executing: " + cmd);
    
    std::array<char, 128> buffer;
    std::string result;
    
#ifdef _WIN32
    FILE* pipe = _popen(cmd.c_str(), "r");
#else
    FILE* pipe = popen(cmd.c_str(), "r");
#endif
    
    if (!pipe) {
        m_lastError = "Failed to execute command: " + cmd;
        Logger::getInstance().error(m_lastError);
        return "";
    }
    
    auto startTime = std::chrono::steady_clock::now();
    
    while (true) {
        if (std::chrono::steady_clock::now() - startTime > std::chrono::milliseconds(timeoutMs)) {
            Logger::getInstance().warning("Command timeout exceeded");
#ifdef _WIN32
            _pclose(pipe);
#else
            pclose(pipe);
#endif
            return result;
        }
        
#ifdef _WIN32
        char* fgetsResult = fgets(buffer.data(), buffer.size(), pipe);
#else
        char* fgetsResult = fgets(buffer.data(), buffer.size(), pipe);
#endif
        
        if (fgetsResult == nullptr) {
            break;
        }
        result += fgetsResult;
    }
    
#ifdef _WIN32
    int exitCode = _pclose(pipe);
#else
    int exitCode = pclose(pipe);
#endif
    
    if (exitCode != 0) {
        Logger::getInstance().warning("Command returned non-zero exit code: " + std::to_string(exitCode));
    }
    
    return result;
}

bool ADBManager::runADBCommand(const std::vector<std::string>& args) {
    std::string result = executeADBCommand(args);
    return !result.empty();
}

std::string ADBManager::executeShellCommand(const std::string& command) {
    return executeShellCommand(command, DEFAULT_TIMEOUT_MS);
}

std::string ADBManager::executeShellCommand(const std::string& command, int timeoutMs) {
    std::vector<std::string> args = {"shell", command};
    return executeADBCommand(args, timeoutMs);
}

bool ADBManager::executeCommandAsync(const std::string& command,
                                       std::function<void(const std::string&)> callback) {
    std::thread([this, command, callback]() {
        std::string result = executeShellCommand(command);
        if (callback) {
            callback(result);
        }
    }).detach();
    
    return true;
}

bool ADBManager::pushFile(const std::string& localPath, const std::string& remotePath) {
    Logger::getInstance().info("Pushing file: " + localPath + " -> " + remotePath);
    
    std::vector<std::string> args = {"push", localPath, remotePath};
    std::string result = executeADBCommand(args, 60000);
    
    return result.find("pushed") != std::string::npos;
}

bool ADBManager::pullFile(const std::string& remotePath, const std::string& localPath) {
    Logger::getInstance().info("Pulling file: " + remotePath + " -> " + localPath);
    
    std::vector<std::string> args = {"pull", remotePath, localPath};
    std::string result = executeADBCommand(args, 60000);
    
    return result.find("pulled") != std::string::npos;
}

bool ADBManager::installAPK(const std::string& apkPath) {
    Logger::getInstance().info("Installing APK: " + apkPath);
    
    std::vector<std::string> args = {"install", "-r", apkPath};
    std::string result = executeADBCommand(args, 120000);
    
    return result.find("Success") != std::string::npos;
}

bool ADBManager::uninstallPackage(const std::string& packageName) {
    Logger::getInstance().info("Uninstalling package: " + packageName);
    
    std::vector<std::string> args = {"uninstall", packageName};
    std::string result = executeADBCommand(args);
    
    return result.find("Success") != std::string::npos;
}

bool ADBManager::rebootDevice(const std::string& mode) {
    Logger::getInstance().info("Rebooting device" + (mode.empty() ? "" : " to " + mode));
    
    std::vector<std::string> args = {"reboot"};
    if (!mode.empty()) {
        args.push_back(mode);
    }
    
    bool success = runADBCommand(args);
    
    if (success) {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        m_connectionState = ADBConnectionState::DISCONNECTED;
        m_selectedDevice.clear();
    }
    
    return success;
}

bool ADBManager::rebootBootloader() {
    return rebootDevice("bootloader");
}

bool ADBManager::rebootRecovery() {
    return rebootDevice("recovery");
}

bool ADBManager::setProperty(const std::string& property, const std::string& value) {
    Logger::getInstance().info("Setting property: " + property + " = " + value);
    
    std::string cmd = "setprop " + property + " \"" + value + "\"";
    std::string result = executeShellCommand(cmd);
    
    return result.empty() || result.find("error") == std::string::npos;
}

std::string ADBManager::getProperty(const std::string& property) {
    std::string cmd = "getprop " + property;
    std::string result = executeShellCommand(cmd);
    
    result.erase(std::remove_if(result.begin(), result.end(), 
        [](char c) { return c == '\n' || c == '\r'; }), result.end());
    
    return result;
}

bool ADBManager::writeFile(const std::string& remotePath, const std::string& content) {
    Logger::getInstance().info("Writing to remote file: " + remotePath);
    
    std::string escapedContent;
    for (char c : content) {
        if (c == '"') escapedContent += "\\\"";
        else if (c == '$') escapedContent += "\\$";
        else if (c == '`') escapedContent += "\\`";
        else escapedContent += c;
    }
    
    std::string cmd = "echo -n \"" + escapedContent + "\" > " + remotePath;
    std::string result = executeShellCommand(cmd);
    
    return result.empty();
}

std::string ADBManager::readFile(const std::string& remotePath) {
    return executeShellCommand("cat " + remotePath);
}

void ADBManager::setConnectionStateCallback(std::function<void(ADBConnectionState)> callback) {
    m_stateCallback = callback;
}

}
