#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <mutex>
#include <atomic>

namespace AntiDetect {

struct DeviceInfo {
    std::string serial;
    std::string state;
    std::string product;
    std::string model;
    std::string device;
    std::string transportId;
};

enum class ADBConnectionState {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    ERROR
};

class ADBManager {
public:
    static ADBManager& getInstance();
    ADBManager();
    ~ADBManager();
    
    
    bool initialize();
    bool isConnected() const;
    std::string getLastError() const;
    
    std::vector<DeviceInfo> getDevices();
    bool connect(const std::string& deviceAddress);
    bool disconnect(const std::string& deviceAddress);
    bool disconnectAll();
    
    bool selectDevice(const std::string& serial);
    std::string getSelectedDevice() const;
    
    std::string executeShellCommand(const std::string& command);
    std::string executeShellCommand(const std::string& command, int timeoutMs);
    bool executeCommandAsync(const std::string& command, 
                             std::function<void(const std::string&)> callback);
    
    bool pushFile(const std::string& localPath, const std::string& remotePath);
    bool pullFile(const std::string& remotePath, const std::string& localPath);
    
    bool installAPK(const std::string& apkPath);
    bool uninstallPackage(const std::string& packageName);
    
    bool rebootDevice(const std::string& mode = "");
    bool rebootBootloader();
    bool rebootRecovery();
    
    bool setProperty(const std::string& property, const std::string& value);
    std::string getProperty(const std::string& property);
    
    bool writeFile(const std::string& remotePath, const std::string& content);
    std::string readFile(const std::string& remotePath);
    void setConnectionStateCallback(std::function<void(ADBConnectionState)> callback);
private:
    
    ADBManager(const ADBManager&) = delete;
    ADBManager& operator=(const ADBManager&) = delete;
    
    std::string executeADBCommand(const std::vector<std::string>& args);
    std::string executeADBCommand(const std::vector<std::string>& args, int timeoutMs);
    bool runADBCommand(const std::vector<std::string>& args);
    
    std::string findADBPath();
    bool verifyADBInstalled();
    
    std::string m_adbPath;
    std::string m_selectedDevice;
    std::string m_lastError;
    std::atomic<ADBConnectionState> m_connectionState;
    std::mutex m_mutex;
    std::function<void(ADBConnectionState)> m_stateCallback;
    
    static constexpr const char* DEFAULT_ADB_PATH = "adb";
    static constexpr int DEFAULT_TIMEOUT_MS = 30000;
};

}
