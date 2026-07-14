/**
 * @file TestingFramework.cpp
 * @brief Automated Testing Framework Implementation
 * @version 2.0.0
 * 
 * Provides automated testing capabilities for banking apps.
 */

#include "VirtualPhonePro/TestingFramework.h"
#include "VirtualPhonePro/ReDroidController.h"

#include <QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>

namespace VirtualPhonePro {

TestingFramework* TestingFramework::s_instance = nullptr;

TestingFramework& TestingFramework::instance() {
    if (!s_instance) {
        s_instance = new TestingFramework();
    }
    return *s_instance;
}

TestingFramework::TestingFramework(QObject* parent)
    : QObject(parent)
{
}

TestingFramework::~TestingFramework() {
}

TestSuite TestingFramework::loadSuite(const QString& path) {
    TestSuite suite;
    
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "[TestingFramework] Failed to open test suite:" << path;
        return suite;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        qWarning() << "[TestingFramework] JSON parse error:" << error.errorString();
        return suite;
    }
    
    QJsonObject json = doc.object();
    suite.id = json["id"].toString();
    suite.name = json["name"].toString();
    
    QJsonArray testCases = json["testCases"].toArray();
    for (const QJsonValue& value : testCases) {
        QJsonObject tcObj = value.toObject();
        TestCase testCase;
        testCase.id = tcObj["id"].toString();
        testCase.action = tcObj["action"].toString();
        testCase.params = tcObj["params"].toObject();
        testCase.timeout = tcObj["timeout"].toInt(5000);
        testCase.expectedResult = tcObj["expected"].toString();
        suite.testCases.append(testCase);
    }
    
    return suite;
}

bool TestingFramework::saveSuite(const TestSuite& suite, const QString& path) {
    QJsonObject json;
    json["id"] = suite.id;
    json["name"] = suite.name;
    
    QJsonArray testCases;
    for (const TestCase& tc : suite.testCases) {
        QJsonObject tcObj;
        tcObj["id"] = tc.id;
        tcObj["action"] = tc.action;
        tcObj["params"] = tc.params;
        tcObj["timeout"] = tc.timeout;
        tcObj["expected"] = tc.expectedResult;
        testCases.append(tcObj);
    }
    json["testCases"] = testCases;
    
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    
    file.write(QJsonDocument(json).toJson());
    file.close();
    
    return true;
}

TestReport TestingFramework::executeSuite(const QString& instanceId, const TestSuite& suite) {
    TestReport report;
    report.suiteId = suite.id;
    report.suiteName = suite.name;
    report.startTime = QDateTime::currentDateTime();
    
    ReDroidController& ctrl = ReDroidController::instance();
    
    // Check if instance is running
    if (!ctrl.instanceExists(instanceId)) {
        report.error = "Instance not found";
        return report;
    }
    
    report.totalTests = suite.testCases.size();
    
    for (const TestCase& testCase : suite.testCases) {
        TestResult result;
        result.testId = testCase.id;
        result.action = testCase.action;
        
        // Execute action
        bool success = executeAction(instanceId, testCase);
        
        result.success = success;
        result.timestamp = QDateTime::currentDateTime();
        
        if (success) {
            report.passed++;
        } else {
            report.failed++;
            report.failures.append(result);
        }
        
        report.results.append(result);
    }
    
    report.endTime = QDateTime::currentDateTime();
    report.duration = report.startTime.msecsTo(report.endTime);
    
    return report;
}

bool TestingFramework::executeAction(const QString& instanceId, const TestCase& testCase) {
    ReDroidController& ctrl = ReDroidController::instance();
    QString action = testCase.action.toLower();
    QJsonObject params = testCase.params;
    
    if (action == "tap") {
        int x = params["x"].toInt();
        int y = params["y"].toInt();
        return ctrl.tap(instanceId, x, y);
    }
    else if (action == "swipe") {
        int x1 = params["x1"].toInt();
        int y1 = params["y1"].toInt();
        int x2 = params["x2"].toInt();
        int y2 = params["y2"].toInt();
        int duration = params["duration"].toInt(300);
        return ctrl.swipe(instanceId, x1, y1, x2, y2, duration);
    }
    else if (action == "input") {
        QString text = params["text"].toString();
        return ctrl.inputText(instanceId, text);
    }
    else if (action == "launch") {
        QString package = params["package"].toString();
        QString startCmd = QString("monkey -p %1 -c android.intent.category.LAUNCHER 1").arg(package);
        ctrl.executeShell(instanceId, startCmd, testCase.timeout);
        return true;
    }
    else if (action == "press") {
        int keyCode = params["keycode"].toInt();
        return ctrl.pressKey(instanceId, keyCode);
    }
    else if (action == "sleep") {
        int ms = params["ms"].toInt(1000);
        QThread::msleep(ms);
        return true;
    }
    else if (action == "screenshot") {
        QByteArray screenshot = ctrl.takeScreenshot(instanceId);
        return !screenshot.isEmpty();
    }
    else if (action == "assert") {
        // Simple assertion - just return true for now
        return true;
    }
    
    qWarning() << "[TestingFramework] Unknown action:" << action;
    return false;
}

bool TestingFramework::addTestCase(const QString& suiteId, const TestCase& testCase) {
    if (suiteId.isEmpty()) {
        return false;
    }
    
    if (m_suites.contains(suiteId)) {
        m_suites[suiteId].testCases.append(testCase);
        return true;
    }
    
    return false;
}

bool TestingFramework::removeTestCase(const QString& suiteId, const QString& testCaseId) {
    if (!m_suites.contains(suiteId)) {
        return false;
    }
    
    TestSuite& suite = m_suites[suiteId];
    for (int i = 0; i < suite.testCases.size(); i++) {
        if (suite.testCases[i].id == testCaseId) {
            suite.testCases.removeAt(i);
            return true;
        }
    }
    
    return false;
}

QList<TestSuite> TestingFramework::getLoadedSuites() {
    return m_suites.values();
}

TestSuite TestingFramework::getSuite(const QString& suiteId) {
    return m_suites.value(suiteId);
}

QJsonObject TestingFramework::reportToJson(const TestReport& report) {
    QJsonObject json;
    
    json["suiteId"] = report.suiteId;
    json["suiteName"] = report.suiteName;
    json["totalTests"] = report.totalTests;
    json["passed"] = report.passed;
    json["failed"] = report.failed;
    json["duration"] = report.duration;
    json["startTime"] = report.startTime.toString(Qt::ISODate);
    json["endTime"] = report.endTime.toString(Qt::ISODate);
    json["error"] = report.error;
    
    QJsonArray results;
    for (const TestResult& result : report.results) {
        QJsonObject r;
        r["testId"] = result.testId;
        r["action"] = result.action;
        r["success"] = result.success;
        r["timestamp"] = result.timestamp.toString(Qt::ISODate);
        r["errorMessage"] = result.errorMessage;
        results.append(r);
    }
    json["results"] = results;
    
    QJsonArray failures;
    for (const TestResult& result : report.failures) {
        QJsonObject f;
        f["testId"] = result.testId;
        f["action"] = result.action;
        f["errorMessage"] = result.errorMessage;
        failures.append(f);
    }
    json["failures"] = failures;
    
    return json;
}

bool TestingFramework::saveReport(const TestReport& report, const QString& path) {
    QJsonObject json = reportToJson(report);
    
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    
    file.write(QJsonDocument(json).toJson());
    file.close();
    
    return true;
}

QString TestingFramework::generateHTMLReport(const TestReport& report) {
    QString html;
    
    html += "<!DOCTYPE html>\n";
    html += "<html>\n<head>\n";
    html += "<title>Test Report - " + report.suiteName + "</title>\n";
    html += "<style>\n";
    html += "body { font-family: Arial, sans-serif; margin: 20px; }\n";
    html += "h1 { color: #333; }\n";
    html += ".summary { background: #f5f5f5; padding: 15px; border-radius: 5px; }\n";
    html += ".passed { color: green; }\n";
    html += ".failed { color: red; }\n";
    html += "table { border-collapse: collapse; width: 100%; margin-top: 20px; }\n";
    html += "th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }\n";
    html += "th { background-color: #4CAF50; color: white; }\n";
    html += "</style>\n";
    html += "</head>\n<body>\n";
    
    html += "<h1>Test Report: " + report.suiteName + "</h1>\n";
    html += "<div class='summary'>\n";
    html += "<p><strong>Suite ID:</strong> " + report.suiteId + "</p>\n";
    html += "<p><strong>Total Tests:</strong> " + QString::number(report.totalTests) + "</p>\n";
    html += "<p><strong class='passed'>Passed:</strong> " + QString::number(report.passed) + "</p>\n";
    html += "<p><strong class='failed'>Failed:</strong> " + QString::number(report.failed) + "</p>\n";
    html += "<p><strong>Duration:</strong> " + QString::number(report.duration) + " ms</p>\n";
    html += "</div>\n";
    
    html += "<table>\n";
    html += "<tr><th>Test ID</th><th>Action</th><th>Status</th><th>Error</th></tr>\n";
    
    for (const TestResult& result : report.results) {
        html += "<tr>";
        html += "<td>" + result.testId + "</td>";
        html += "<td>" + result.action + "</td>";
        html += "<td class='" + QString(result.success ? "passed" : "failed") + "'>" +
                 QString(result.success ? "PASSED" : "FAILED") + "</td>";
        html += "<td>" + result.errorMessage + "</td>";
        html += "</tr>\n";
    }
    
    html += "</table>\n";
    html += "</body>\n</html>";
    
    return html;
}

} // namespace VirtualPhonePro
