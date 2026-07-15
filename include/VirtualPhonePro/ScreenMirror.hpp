#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <atomic>

namespace AntiDetect {

struct ScreenInfo {
    int width;
    int height;
    int density;
    int orientation;
    long long timestamp;
    std::string format;
};

struct TouchEvent {
    int action;      // 0=down, 1=up, 2=move
    int x;
    int y;
    int pressure;
    int id;          // For multi-touch
};

struct SwipeEvent {
    int startX;
    int startY;
    int endX;
    int endY;
    int durationMs;
};

struct RecordSettings {
    std::string outputPath;
    int bitRate;           // bps
    int frameRate;         // fps
    int maxDuration;       // seconds, 0 = unlimited
    std::string format;    // mp4, mkv, webm
    bool recordAudio;
};

class ScreenMirror {
public:
    ScreenMirror();
    ~ScreenMirror();
    
    bool initialize();
    bool shutdown();
    
    // Connection
    bool startMirror();
    bool stopMirror();
    bool isMirroring() const;
    
    // Screen Capture
    std::vector<unsigned char> captureScreen();
    std::vector<unsigned char> captureFrame();
    bool saveScreenshot(const std::string& filepath);
    
    ScreenInfo getScreenInfo();
    
    // Touch Control
    bool sendTouchEvent(const TouchEvent& event);
    bool sendTap(int x, int y);
    bool sendLongPress(int x, int y, int durationMs);
    bool sendSwipe(const SwipeEvent& swipe);
    bool sendMultiTouch(const std::vector<TouchEvent>& events);
    
    // Key Events
    bool sendKeyEvent(int keyCode, bool longPress = false);
    bool sendTextInput(const std::string& text);
    bool sendHomeButton();
    bool sendBackButton();
    bool sendRecentsButton();
    bool sendPowerButton();
    
    // Recording
    bool startRecording(const RecordSettings& settings);
    bool stopRecording();
    bool isRecording() const;
    float getRecordingDuration() const;
    
    // Settings
    void setMaxResolution(int width, int height);
    void setBitRate(int bps);
    void setFrameRate(int fps);
    void setOrientationLock(bool locked);
    
    // Callbacks
    void setFrameCallback(std::function<void(const std::vector<unsigned char>&, const ScreenInfo&)> callback);
    void setErrorCallback(std::function<void(const std::string&)> callback);

private:
    bool connectToStream();
    bool startADBStream();
    bool decodeFrame(const std::vector<unsigned char>& data);
    
    bool executeTouchCommand(const std::string& command);
    std::string generateTouchCommand(const TouchEvent& event);
    
    bool startRecordingProcess();
    bool stopRecordingProcess();
    
    std::atomic<bool> m_mirroring;
    std::atomic<bool> m_recording;
    std::atomic<bool> m_initialized;
    
    int m_maxWidth;
    int m_maxHeight;
    int m_bitRate;
    int m_frameRate;
    
    std::string m_recordOutputPath;
    std::string m_recordingProcess;
    
    ScreenInfo m_currentScreen;
    
    std::function<void(const std::vector<unsigned char>&, const ScreenInfo&)> m_frameCallback;
    std::function<void(const std::string&)> m_errorCallback;
};

}
