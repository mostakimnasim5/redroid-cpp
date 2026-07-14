/**
 * @file HyperRealisticTouchEmulator.cpp
 * @brief Enterprise-Grade Touch Simulation Implementation
 * @version 2.0.0
 */

#include "VirtualPhonePro/HyperRealisticTouchEmulator.hpp"
#include "VirtualPhonePro/ReDroidController.h"

#include <QDebug>
#include <QRandomGenerator>
#include <QThread>

namespace VirtualPhonePro {

HyperRealisticTouchEmulator* HyperRealisticTouchEmulator::s_instance = nullptr;

HyperRealisticTouchEmulator& HyperRealisticTouchEmulator::instance() {
    if (!s_instance) {
        s_instance = new HyperRealisticTouchEmulator();
    }
    return *s_instance;
}

HyperRealisticTouchEmulator::HyperRealisticTouchEmulator()
    : m_generator(QRandomGenerator::global()->generate())
{
    setProfile(TouchProfile::NATURAL);
}

void HyperRealisticTouchEmulator::setProfile(TouchProfile profile) {
    m_profile = profile;
    
    switch (profile) {
        case TouchProfile::NATURAL:
            m_chars = {0.7f, 0.1f, 0.8f, 0.2f, 11.0f, 2.0f, 150.0f, 0.95f, 0.9f, 0.95f};
            break;
        case TouchProfile::CAREFUL:
            m_chars = {0.5f, 0.05f, 0.5f, 0.1f, 8.0f, 1.0f, 200.0f, 0.99f, 0.95f, 0.98f};
            break;
        case TouchProfile::AGGRESSIVE:
            m_chars = {0.9f, 0.15f, 1.5f, 0.3f, 15.0f, 3.0f, 80.0f, 0.85f, 0.8f, 0.9f};
            break;
        case TouchProfile::ELDERLY:
            m_chars = {0.4f, 0.2f, 0.3f, 0.15f, 14.0f, 4.0f, 300.0f, 0.7f, 0.7f, 0.8f};
            break;
        case TouchProfile::POWER_USER:
            m_chars = {0.8f, 0.1f, 1.2f, 0.15f, 10.0f, 1.5f, 100.0f, 0.98f, 0.92f, 0.97f};
            break;
        case TouchProfile::GESTURE_MASTER:
            m_chars = {0.75f, 0.08f, 1.0f, 0.1f, 9.0f, 1.0f, 120.0f, 0.99f, 0.95f, 0.99f};
            break;
    }
}

void HyperRealisticTouchEmulator::configureTouchCharacteristics(const TouchCharacteristics& chars) {
    m_chars = chars;
}

void HyperRealisticTouchEmulator::setScreenSize(int width, int height, int dpi) {
    m_screenWidth = width;
    m_screenHeight = height;
    m_dpi = dpi;
}

void HyperRealisticTouchEmulator::setDeviceModel(const QString& manufacturer, const QString& model) {
    m_manufacturer = manufacturer;
    m_model = model;
}

float HyperRealisticTouchEmulator::generateGaussian(float mean, float stddev) {
    std::normal_distribution<float> dist(mean, stddev);
    return dist(m_generator);
}

float HyperRealisticTouchEmulator::generateSwipeVelocity() {
    return generateGaussian(m_chars.avgSwipeSpeed, m_chars.swipeSpeedVariance);
}

float HyperRealisticTouchEmulator::generateTouchPressure() {
    float pressure = generateGaussian(m_chars.avgPressure, m_chars.pressureVariance);
    return std::max(0.1f, std::min(1.0f, pressure));
}

float HyperRealisticTouchEmulator::generateTouchSize() {
    float size = generateGaussian(m_chars.avgTouchSize, m_chars.touchSizeVariance);
    return std::max(5.0f, std::min(20.0f, size));
}

float HyperRealisticTouchEmulator::generateCurveOffset(float progress) {
    // Natural swipe curves - slightly curved path
    // Use sine wave for natural curve
    float curve = std::sin(progress * 3.14159f) * 20.0f;
    // Add slight randomness
    curve += generateGaussian(0, 5.0f);
    return curve;
}

TouchPoint HyperRealisticTouchEmulator::generateTouchPoint(int id, float x, float y, TouchAction action) {
    TouchPoint point;
    point.id = id;
    point.x = x;
    point.y = y;
    point.pressure = generateTouchPressure();
    point.size = generateTouchSize();
    point.orientation = 0.0f;
    point.tiltX = generateGaussian(0, 5.0f);
    point.tiltY = generateGaussian(0, 5.0f);
    point.timestamp = QDateTime::currentMSecsSinceEpoch();
    point.action = action;
    return point;
}

QVector<TouchPoint> HyperRealisticTouchEmulator::interpolatePoints(const TouchPoint& start, const TouchPoint& end, int steps) {
    QVector<TouchPoint> points;
    points.reserve(steps);
    
    for (int i = 0; i <= steps; i++) {
        float t = static_cast<float>(i) / steps;
        
        TouchPoint point;
        point.id = start.id;
        point.x = start.x + (end.x - start.x) * t;
        point.y = start.y + (end.y - start.y) * t;
        point.pressure = generateGaussian(m_chars.avgPressure, m_chars.pressureVariance * 0.5f);
        point.size = generateGaussian(m_chars.avgTouchSize, m_chars.touchSizeVariance * 0.5f);
        point.orientation = start.orientation;
        point.tiltX = generateGaussian(0, 3.0f);
        point.tiltY = generateGaussian(0, 3.0f);
        point.timestamp = start.timestamp + static_cast<qint64>(t * (end.timestamp - start.timestamp));
        point.action = TouchAction::MOVE;
        
        points.append(point);
    }
    
    return points;
}

QVector<TouchPoint> HyperRealisticTouchEmulator::generateTap(float x, float y, int tapCount) {
    QVector<TouchPoint> points;
    
    for (int tap = 0; tap < tapCount; tap++) {
        // Touch down
        TouchPoint down = generateTouchPoint(0, x, y, TouchAction::DOWN);
        down.timestamp += tap * 200; // Interval between taps
        points.append(down);
        
        // Touch up
        TouchPoint up = generateTouchPoint(0, x, y, TouchAction::UP);
        up.timestamp = down.timestamp + static_cast<qint64>(generateGaussian(100, 20));
        up.pressure = 0.0f;
        points.append(up);
    }
    
    return points;
}

QVector<TouchPoint> HyperRealisticTouchEmulator::generateDoubleTap(float x, float y) {
    return generateTap(x, y, 2);
}

QVector<TouchPoint> HyperRealisticTouchEmulator::generateLongPress(float x, float y, int durationMs) {
    QVector<TouchPoint> points;
    
    // Touch down
    TouchPoint down = generateTouchPoint(0, x, y, TouchAction::DOWN);
    points.append(down);
    
    // Small movements during long press (simulating slight finger movement)
    int intervals = durationMs / 50;
    for (int i = 1; i < intervals; i++) {
        TouchPoint move = down;
        move.timestamp += i * 50;
        move.x += generateGaussian(0, 2.0f);
        move.y += generateGaussian(0, 2.0f);
        move.action = TouchAction::MOVE;
        points.append(move);
    }
    
    // Touch up
    TouchPoint up = down;
    up.timestamp += durationMs;
    up.action = TouchAction::UP;
    up.pressure = 0.0f;
    points.append(up);
    
    return points;
}

QVector<TouchPoint> HyperRealisticTouchEmulator::generateSwipe(GestureType direction, float startX, float startY, 
                                                               float distance, bool withCurve) {
    float endX = startX;
    float endY = startY;
    
    switch (direction) {
        case GestureType::SWIPE_LEFT:
            endX = startX - distance;
            break;
        case GestureType::SWIPE_RIGHT:
            endX = startX + distance;
            break;
        case GestureType::SWIPE_UP:
            endY = startY - distance;
            break;
        case GestureType::SWIPE_DOWN:
            endY = startY + distance;
            break;
        default:
            break;
    }
    
    return generateSwipeLeft(startX, startY, distance);
}

QVector<TouchPoint> HyperRealisticTouchEmulator::generateSwipeLeft(float startX, float startY, float distance) {
    QVector<TouchPoint> points;
    
    float velocity = generateSwipeVelocity();
    float duration = distance / velocity; // pixels / (pixels/ms) = ms
    int steps = static_cast<int>(duration / 5); // Point every 5ms
    
    // Touch down
    TouchPoint down = generateTouchPoint(0, startX, startY, TouchAction::DOWN);
    points.append(down);
    
    // Interpolate with curve
    for (int i = 1; i < steps; i++) {
        float progress = static_cast<float>(i) / steps;
        
        TouchPoint point;
        point.id = 0;
        point.x = startX - (distance * progress);
        point.y = startY;
        
        if (std::abs(distance) > 200) {
            // Add curve to longer swipes
            point.x += generateCurveOffset(progress) * 0.5f;
        }
        
        point.pressure = generateGaussian(m_chars.avgPressure, m_chars.pressureVariance * 0.3f);
        point.size = generateTouchSize();
        point.timestamp = down.timestamp + static_cast<qint64>(progress * duration);
        point.action = TouchAction::MOVE;
        
        points.append(point);
    }
    
    // Touch up
    TouchPoint up = points.last();
    up.action = TouchAction::UP;
    up.pressure = 0.0f;
    up.timestamp = down.timestamp + static_cast<qint64>(duration);
    points.append(up);
    
    return points;
}

QVector<TouchPoint> HyperRealisticTouchEmulator::generateSwipeRight(float startX, float startY, float distance) {
    QVector<TouchPoint> points = generateSwipeLeft(startX, startY, distance);
    
    // Mirror X coordinates
    for (auto& point : points) {
        point.x = startX + (startX - point.x);
    }
    
    return points;
}

QVector<TouchPoint> HyperRealisticTouchEmulator::generateSwipeUp(float startX, float startY, float distance) {
    QVector<TouchPoint> points;
    
    float velocity = generateSwipeVelocity();
    float duration = distance / velocity;
    int steps = static_cast<int>(duration / 5);
    
    // Touch down
    TouchPoint down = generateTouchPoint(0, startX, startY, TouchAction::DOWN);
    points.append(down);
    
    // Interpolate with slight curve
    for (int i = 1; i < steps; i++) {
        float progress = static_cast<float>(i) / steps;
        
        TouchPoint point;
        point.id = 0;
        point.x = startX + generateGaussian(0, 3.0f);
        point.y = startY - (distance * progress);
        
        if (distance > 200) {
            point.x += generateCurveOffset(progress) * 0.3f;
        }
        
        point.pressure = generateGaussian(m_chars.avgPressure, m_chars.pressureVariance * 0.3f);
        point.size = generateTouchSize();
        point.timestamp = down.timestamp + static_cast<qint64>(progress * duration);
        point.action = TouchAction::MOVE;
        
        points.append(point);
    }
    
    // Touch up
    TouchPoint up = points.last();
    up.action = TouchAction::UP;
    up.pressure = 0.0f;
    points.append(up);
    
    return points;
}

QVector<TouchPoint> HyperRealisticTouchEmulator::generateSwipeDown(float startX, float startY, float distance) {
    QVector<TouchPoint> points = generateSwipeUp(startX, startY, distance);
    
    // Mirror Y coordinates
    for (auto& point : points) {
        point.y = startY + (startY - point.y);
    }
    
    return points;
}

QVector<TouchPoint> HyperRealisticTouchEmulator::generatePinch(float centerX, float centerY, float scale) {
    QVector<TouchPoint> points;
    
    // Two fingers starting apart, moving together
    float fingerDistance = 200.0f;
    float velocity = generateSwipeVelocity() * 0.7f;
    
    // Left finger positions
    float leftStartX = centerX - fingerDistance;
    float leftEndX = centerX - (fingerDistance * scale);
    
    // Right finger positions
    float rightStartX = centerX + fingerDistance;
    float rightEndX = centerX + (fingerDistance * scale);
    
    float duration = fingerDistance / velocity;
    int steps = static_cast<int>(duration / 5);
    
    qint64 baseTime = QDateTime::currentMSecsSinceEpoch();
    
    // Touch down for both fingers
    TouchPoint leftDown = generateTouchPoint(0, leftStartX, centerY, TouchAction::DOWN);
    leftDown.timestamp = baseTime;
    points.append(leftDown);
    
    TouchPoint rightDown = generateTouchPoint(1, rightStartX, centerY, TouchAction::DOWN);
    rightDown.timestamp = baseTime;
    points.append(rightDown);
    
    // Move fingers together
    for (int i = 1; i < steps; i++) {
        float progress = static_cast<float>(i) / steps;
        
        TouchPoint left = leftDown;
        left.x = leftStartX + (leftEndX - leftStartX) * progress;
        left.timestamp = baseTime + static_cast<qint64>(progress * duration);
        left.action = TouchAction::MOVE;
        points.append(left);
        
        TouchPoint right = rightDown;
        right.x = rightStartX + (rightEndX - rightStartX) * progress;
        right.timestamp = baseTime + static_cast<qint64>(progress * duration);
        right.action = TouchAction::MOVE;
        points.append(right);
    }
    
    // Touch up for both fingers
    TouchPoint leftUp = points.last();
    leftUp.id = 0;
    leftUp.action = TouchAction::UP;
    leftUp.pressure = 0.0f;
    points.append(leftUp);
    
    TouchPoint rightUp = leftUp;
    rightUp.id = 1;
    points.append(rightUp);
    
    return points;
}

QVector<TouchPoint> HyperRealisticTouchEmulator::generateSpread(float centerX, float centerY, float scale) {
    // Spread is just inverse pinch
    return generatePinch(centerX, centerY, 1.0f / scale);
}

QVector<TouchPoint> HyperRealisticTouchEmulator::generateDrag(float startX, float startY, float endX, float endY) {
    QVector<TouchPoint> points;
    
    float distance = std::sqrt(std::pow(endX - startX, 2) + std::pow(endY - startY, 2));
    float velocity = generateSwipeVelocity() * 0.6f; // Slower for drag
    float duration = distance / velocity;
    int steps = static_cast<int>(duration / 5);
    
    // Touch down
    TouchPoint down = generateTouchPoint(0, startX, startY, TouchAction::DOWN);
    points.append(down);
    
    // Interpolate
    for (int i = 1; i < steps; i++) {
        float progress = static_cast<float>(i) / steps;
        
        TouchPoint point;
        point.id = 0;
        point.x = startX + (endX - startX) * progress + generateGaussian(0, 1.0f);
        point.y = startY + (endY - startY) * progress + generateGaussian(0, 1.0f);
        point.pressure = generateGaussian(m_chars.avgPressure, m_chars.pressureVariance * 0.3f);
        point.size = generateTouchSize();
        point.timestamp = down.timestamp + static_cast<qint64>(progress * duration);
        point.action = TouchAction::MOVE;
        
        points.append(point);
    }
    
    // Touch up
    TouchPoint up = points.last();
    up.action = TouchAction::UP;
    up.pressure = 0.0f;
    points.append(up);
    
    return points;
}

bool HyperRealisticTouchEmulator::executeGesture(const QString& instanceId, const QVector<TouchPoint>& points) {
    ReDroidController& ctrl = ReDroidController::instance();
    
    // Send touch events via ADB
    for (const TouchPoint& point : points) {
        QString action;
        switch (point.action) {
            case TouchAction::DOWN: action = "down"; break;
            case TouchAction::MOVE: action = "move"; break;
            case TouchAction::UP: action = "up"; break;
            case TouchAction::CANCEL: action = "cancel"; break;
        }
        
        // ADB touch command
        QString cmd = QString("input touchscreen tap %1 %2")
                         .arg(static_cast<int>(point.x))
                         .arg(static_cast<int>(point.y));
        
        ctrl.executeShell(instanceId, cmd);
        
        // Small delay between events
        QThread::msleep(5);
    }
    
    return true;
}

bool HyperRealisticTouchEmulator::executeTap(const QString& instanceId, float x, float y) {
    QVector<TouchPoint> points = generateTap(x, y, 1);
    return executeGesture(instanceId, points);
}

bool HyperRealisticTouchEmulator::executeSwipe(const QString& instanceId, GestureType direction, float startX, float startY) {
    QVector<TouchPoint> points;
    
    switch (direction) {
        case GestureType::SWIPE_LEFT:
            points = generateSwipeLeft(startX, startY);
            break;
        case GestureType::SWIPE_RIGHT:
            points = generateSwipeRight(startX, startY);
            break;
        case GestureType::SWIPE_UP:
            points = generateSwipeUp(startX, startY);
            break;
        case GestureType::SWIPE_DOWN:
            points = generateSwipeDown(startX, startY);
            break;
        default:
            return false;
    }
    
    return executeGesture(instanceId, points);
}

} // namespace VirtualPhonePro
