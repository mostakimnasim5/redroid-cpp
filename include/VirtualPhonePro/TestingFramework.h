#pragma once

#ifndef VIRTUALPHONEPRO_TESTING_FRAMEWORK_H
#define VIRTUALPHONEPRO_TESTING_FRAMEWORK_H

#include <QString>
#include <QStringList>
#include <QMap>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariantMap>

namespace VirtualPhonePro {

/**
 * @brief Test case definition
 */
struct TestCase {
    QString id;
    QString name;
    QString description;
    QString packageName;
    QString action;  // "tap", "swipe", "input", "launch", "assert"
    QVariantMap params;  // Parameters for the action
    int timeoutMs;
    bool critical;
};

/**
 * @brief Test suite definition
 */
struct TestSuite {
    QString id;
    QString name;
    QString description;
    QList<TestCase> testCases;
    int repeatCount;
    bool stopOnFailure;
};

/**
 * @brief Test result
 */
struct TestResult {
    QString testId;
    bool passed;
    QString errorMessage;
    qint64 durationMs;
    QString screenshot;
    QJsonObject logs;
};

/**
 * @brief Test execution report
 */
struct TestReport {
    QString suiteId;
    QDateTime startTime;
    QDateTime endTime;
    int totalTests;
    int passed;
    int failed;
    int skipped;
    QList<TestResult> results;
    QMap<QString, int> metrics;
};

/**
 * @brief TestingFramework - Automated testing for emulators
 * 
 * Provides automated testing capabilities including UI automation,
 * screenshot comparison, and test reporting.
 */
class TestingFramework {
public:
    static TestingFramework& instance();
    
    // =========================================================================
    // Test Suite Management
    // =========================================================================
    
    /**
     * @brief Create test suite from JSON
     * @param json JSON definition
     * @return TestSuite object
     */
    TestSuite createSuiteFromJson(const QJsonObject& json);
    
    /**
     * @brief Load test suite from file
     * @param filePath Path to JSON file
     * @return TestSuite object
     */
    TestSuite loadSuite(const QString& filePath);
    
    /**
     * @brief Save test suite to file
     * @param suite Test suite to save
     * @param filePath Path to save to
     * @return true if successful
     */
    bool saveSuite(const TestSuite& suite, const QString& filePath);
    
    // =========================================================================
    // Test Execution
    // =========================================================================
    
    /**
     * @brief Execute test suite
     * @param instanceId Target instance
     * @param suite Test suite to execute
     * @return TestReport with results
     */
    TestReport executeSuite(const QString& instanceId, const TestSuite& suite);
    
    /**
     * @brief Execute single test case
     * @param instanceId Target instance
     * @param testCase Test case to execute
     * @return TestResult
     */
    TestResult executeTest(const QString& instanceId, const TestCase& testCase);
    
    /**
     * @brief Stop current execution
     */
    void stopExecution();
    
    // =========================================================================
    // Actions
    // =========================================================================
    
    /**
     * @brief Tap at coordinates
     */
    bool tap(const QString& instanceId, int x, int y);
    
    /**
     * @brief Swipe gesture
     */
    bool swipe(const QString& instanceId, int x1, int y1, int x2, int y2, int durationMs = 300);
    
    /**
     * @brief Input text
     */
    bool inputText(const QString& instanceId, const QString& text);
    
    /**
     * @brief Launch app
     */
    bool launchApp(const QString& instanceId, const QString& packageName);
    
    /**
     * @brief Press button/key
     */
    bool pressKey(const QString& instanceId, int keyCode);
    
    /**
     * @brief Wait for element to appear
     */
    bool waitForElement(const QString& instanceId, const QString& elementId, int timeoutMs);
    
    /**
     * @brief Take screenshot
     */
    QByteArray takeScreenshot(const QString& instanceId);
    
    /**
     * @brief Compare screenshots
     */
    bool compareScreenshots(const QByteArray& screenshot1, const QByteArray& screenshot2);
    
    // =========================================================================
    // Assertions
    // =========================================================================
    
    /**
     * @brief Assert element exists
     */
    bool assertElementExists(const QString& instanceId, const QString& elementId);
    
    /**
     * @brief Assert text present
     */
    bool assertTextPresent(const QString& instanceId, const QString& text);
    
    /**
     * @brief Assert app installed
     */
    bool assertAppInstalled(const QString& instanceId, const QString& packageName);
    
signals:
    void testStarted(const QString& testId);
    void testCompleted(const QString& testId, bool passed);
    void suiteStarted(const QString& suiteId);
    void suiteCompleted(const TestReport& report);
    void progress(int current, int total);

private:
    static TestingFramework* s_instance;
    TestingFramework() = default;
    
    bool m_stopRequested = false;
    
    bool executeCommand(const QString& instanceId, const QString& command);
    QString executeCommandSync(const QString& instanceId, const QString& command);
    
    TestResult runTap(const QString& instanceId, const TestCase& test);
    TestResult runSwipe(const QString& instanceId, const TestCase& test);
    TestResult runInput(const QString& instanceId, const TestCase& test);
    TestResult runLaunch(const QString& instanceId, const TestCase& test);
    TestResult runAssert(const QString& instanceId, const TestCase& test);
};

} // namespace VirtualPhonePro

#endif // VIRTUALPHONEPRO_TESTING_FRAMEWORK_H
