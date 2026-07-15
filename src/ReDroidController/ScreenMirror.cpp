#include "ScreenMirror.hpp"
#include "ADBManager.hpp"
#include "Logger.hpp"
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
#endif

namespace VirtualPhonePro {

ScreenMirror::ScreenMirror()
    : m_mirroring(false)
    , m_recording(false)
    , m_initialized(false)
    , m_maxWidth(1920)
    , m_maxHeight(1080)
    , m_bitRate(8000000)
    , m_frameRate(30)
{
}

ScreenMirror::~ScreenMirror() {
    shutdown();
}

bool ScreenMirror::initialize() {
    if (m_initialized) return true;
    
    Logger::getInstance().info("Initializing ScreenMirror...");
    
    auto& adb = ADBManager::getInstance();
    if (!adb.isConnected()) {
        Logger::getInstance().error("Device not connected for screen mirror");
        return false;
    }
    
    m_currentScreen = getScreenInfo();
    
    m_initialized = true;
    Logger::getInstance().info("ScreenMirror initialized");
    
    return true;
}

bool ScreenMirror::shutdown() {
    stopMirror();
    stopRecording();
    m_initialized = false;
    return true;
}

bool ScreenMirror::startMirror() {
    if (m_mirroring) return true;
    
    Logger::getInstance().info("Starting screen mirror...");
    
    auto& adb = ADBManager::getInstance();
    
    std::string result = adb.executeShellCommand("screenrecord --time-limit 1 /sdcard/mirror_init.mp4 2>/dev/null");
    
    m_mirroring = true;
    Logger::getInstance().info("Screen mirror started");
    
    return true;
}

bool ScreenMirror::stopMirror() {
    if (!m_mirroring) return true;
    
    Logger::getInstance().info("Stopping screen mirror...");
    m_mirroring = false;
    
    return true;
}

bool ScreenMirror::isMirroring() const {
    return m_mirroring;
}

std::vector<unsigned char> ScreenMirror::captureScreen() {
    auto& adb = ADBManager::getInstance();
    
    std::string tempFile = "/sdcard/screenshot_" + std::to_string(time(nullptr)) + ".png";
    
    adb.executeShellCommand("screencap -p " + tempFile);
    
    std::vector<unsigned char> data;
    
    adb.pullFile(tempFile, "/tmp/screenshot.png");
    
    std::ifstream file("/tmp/screenshot.png", std::ios::binary);
    if (file.is_open()) {
        file.seekg(0, std::ios::end);
        size_t size = file.tellg();
        file.seekg(0, std::ios::beg);
        
        data.resize(size);
        file.read(reinterpret_cast<char*>(data.data()), size);
        file.close();
    }
    
    adb.executeShellCommand("rm " + tempFile);
    
    return data;
}

std::vector<unsigned char> ScreenMirror::captureFrame() {
    return captureScreen();
}

bool ScreenMirror::saveScreenshot(const std::string& filepath) {
    auto& adb = ADBManager::getInstance();
    
    std::string tempFile = "/sdcard/screenshot_temp.png";
    
    adb.executeShellCommand("screencap -p " + tempFile);
    
    bool success = adb.pullFile(tempFile, filepath);
    
    adb.executeShellCommand("rm " + tempFile);
    
    Logger::getInstance().info("Screenshot saved: " + filepath);
    
    return success;
}

ScreenInfo ScreenMirror::getScreenInfo() {
    ScreenInfo info;
    auto& adb = ADBManager::getInstance();
    
    std::string widthStr = adb.executeShellCommand("wm size | head -1 | awk '{print $3}' | cut -d'x' -f1");
    std::string heightStr = adb.executeShellCommand("wm size | head -1 | awk '{print $3}' | cut -d'x' -f2");
    
    widthStr.erase(std::remove(widthStr.begin(), widthStr.end(), '\n'), widthStr.end());
    heightStr.erase(std::remove(heightStr.begin(), heightStr.end(), '\n'), heightStr.end());
    
    try {
        info.width = std::stoi(widthStr);
        info.height = std::stoi(heightStr);
    } catch (...) {
        info.width = 1080;
        info.height = 1920;
    }
    
    std::string densityStr = adb.executeShellCommand("wm density | head -1 | awk '{print $3}'");
    densityStr.erase(std::remove(densityStr.begin(), densityStr.end(), '\n'), densityStr.end());
    
    try {
        info.density = std::stoi(densityStr);
    } catch (...) {
        info.density = 480;
    }
    
    info.orientation = (info.width > info.height) ? 0 : 1;
    info.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    info.format = "PNG";
    
    return info;
}

bool ScreenMirror::sendTouchEvent(const TouchEvent& event) {
    auto& adb = ADBManager::getInstance();
    
    std::string cmd = "input tap " + std::to_string(event.x) + " " + std::to_string(event.y);
    
    if (event.action == 0) {
        cmd = "input tap " + std::to_string(event.x) + " " + std::to_string(event.y);
    } else if (event.action == 1) {
        return true;
    } else if (event.action == 2) {
        cmd = "input swipe " + std::to_string(event.x) + " " + std::to_string(event.y) + 
              " " + std::to_string(event.x) + " " + std::to_string(event.y) + " 1";
    }
    
    adb.executeShellCommand(cmd);
    
    return true;
}

bool ScreenMirror::sendTap(int x, int y) {
    auto& adb = ADBManager::getInstance();
    
    std::string cmd = "input tap " + std::to_string(x) + " " + std::to_string(y);
    adb.executeShellCommand(cmd);
    
    Logger::getInstance().debug("Tap: " + cmd);
    
    return true;
}

bool ScreenMirror::sendLongPress(int x, int y, int durationMs) {
    auto& adb = ADBManager::getInstance();
    
    std::string cmd = "input swipe " + std::to_string(x) + " " + std::to_string(y) + 
                      " " + std::to_string(x) + " " + std::to_string(y) + " " + std::to_string(durationMs);
    adb.executeShellCommand(cmd);
    
    Logger::getInstance().debug("Long press: " + cmd);
    
    return true;
}

bool ScreenMirror::sendSwipe(const SwipeEvent& swipe) {
    auto& adb = ADBManager::getInstance();
    
    std::string cmd = "input swipe " + std::to_string(swipe.startX) + " " + std::to_string(swipe.startY) +
                      " " + std::to_string(swipe.endX) + " " + std::to_string(swipe.endY) +
                      " " + std::to_string(swipe.durationMs);
    adb.executeShellCommand(cmd);
    
    Logger::getInstance().debug("Swipe: " + cmd);
    
    return true;
}

bool ScreenMirror::sendMultiTouch(const std::vector<TouchEvent>& events) {
    for (const auto& event : events) {
        sendTouchEvent(event);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return true;
}

bool ScreenMirror::sendKeyEvent(int keyCode, bool longPress) {
    auto& adb = ADBManager::getInstance();
    
    if (longPress) {
        std::string cmd = "input keyevent --longpress " + std::to_string(keyCode);
        adb.executeShellCommand(cmd);
    } else {
        std::string cmd = "input keyevent " + std::to_string(keyCode);
        adb.executeShellCommand(cmd);
    }
    
    return true;
}

bool ScreenMirror::sendTextInput(const std::string& text) {
    auto& adb = ADBManager::getInstance();
    
    std::string escapedText;
    for (char c : text) {
        if (c == ' ') {
            escapedText += "%s";
        } else if (c == '\n') {
            escapedText += "\\n";
        } else {
            escapedText += c;
        }
    }
    
    std::string cmd = "input text " + escapedText;
    adb.executeShellCommand(cmd);
    
    Logger::getInstance().debug("Text input: " + text);
    
    return true;
}

bool ScreenMirror::sendHomeButton() {
    return sendKeyEvent(3);
}

bool ScreenMirror::sendBackButton() {
    return sendKeyEvent(4);
}

bool ScreenMirror::sendRecentsButton() {
    return sendKeyEvent(187);
}

bool ScreenMirror::sendPowerButton() {
    return sendKeyEvent(26);
}

bool ScreenMirror::startRecording(const RecordSettings& settings) {
    if (m_recording) return true;
    
    Logger::getInstance().info("Starting screen recording...");
    
    auto& adb = ADBManager::getInstance();
    
    std::string cmd = "screenrecord";
    
    if (settings.bitRate > 0) {
        cmd += " --bit-rate " + std::to_string(settings.bitRate);
    } else {
        cmd += " --bit-rate " + std::to_string(m_bitRate);
    }
    
    if (settings.frameRate > 0) {
        cmd += " --time-limit " + std::to_string(settings.maxDuration);
    }
    
    cmd += " " + settings.outputPath;
    
    m_recordOutputPath = settings.outputPath;
    m_recording = true;
    
    std::thread([this, cmd]() {
        ADBManager::getInstance().executeCommandAsync(cmd, [](const std::string&) {});
    }).detach();
    
    Logger::getInstance().info("Recording started: " + settings.outputPath);
    
    return true;
}

bool ScreenMirror::stopRecording() {
    if (!m_recording) return true;
    
    Logger::getInstance().info("Stopping screen recording...");
    
    auto& adb = ADBManager::getInstance();
    
    adb.executeShellCommand("pkill -f screenrecord");
    adb.executeShellCommand("killall screenrecord");
    
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    m_recording = false;
    
    Logger::getInstance().info("Recording stopped");
    
    return true;
}

bool ScreenMirror::isRecording() const {
    return m_recording;
}

float ScreenMirror::getRecordingDuration() const {
    return 0.0f;
}

void ScreenMirror::setMaxResolution(int width, int height) {
    m_maxWidth = width;
    m_maxHeight = height;
}

void ScreenMirror::setBitRate(int bps) {
    m_bitRate = bps;
}

void ScreenMirror::setFrameRate(int fps) {
    m_frameRate = fps;
}

void ScreenMirror::setOrientationLock(bool locked) {
    auto& adb = ADBManager::getInstance();
    
    if (locked) {
        adb.executeShellCommand("settings put system accelerometer_rotation 0");
    } else {
        adb.executeShellCommand("settings put system accelerometer_rotation 1");
    }
}

void ScreenMirror::setFrameCallback(std::function<void(const std::vector<unsigned char>&, const ScreenInfo&)> callback) {
    m_frameCallback = callback;
}

void ScreenMirror::setErrorCallback(std::function<void(const std::string&)> callback) {
    m_errorCallback = callback;
}

bool ScreenMirror::executeTouchCommand(const std::string& command) {
    return ADBManager::getInstance().executeShellCommand(command).find("error") == std::string::npos;
}

std::string ScreenMirror::generateTouchCommand(const TouchEvent& event) {
    std::stringstream ss;
    
    switch (event.action) {
        case 0:
            ss << "input tap " << event.x << " " << event.y;
            break;
        case 1:
            ss << "input tap " << event.x << " " << event.y;
            break;
        case 2:
            ss << "input swipe " << event.x << " " << event.y << " " 
               << event.x << " " << event.y << " 1";
            break;
    }
    
    return ss.str();
}

bool ScreenMirror::connectToStream() {
    return true;
}

bool ScreenMirror::startADBStream() {
    return true;
}

bool ScreenMirror::decodeFrame(const std::vector<unsigned char>& data) {
    return !data.empty();
}

bool ScreenMirror::startRecordingProcess() {
    return true;
}

bool ScreenMirror::stopRecordingProcess() {
    return true;
}

}
