#include "VirtualPhonePro/FrameRenderer.hpp"
#include "VirtualPhonePro/MediaStreamDecoder.hpp"
#include "VirtualPhonePro/AdbSocketClient.hpp"
#include "VirtualPhonePro/HyperRealisticTouchEmulator.hpp"
#include <QDateTime>
#include <QDebug>
#include <QtEndian>

namespace VirtualPhonePro {

FrameRenderer::FrameRenderer(QWidget* parent)
    : QOpenGLWidget(parent)
    , m_renderTimer(new QTimer(this))
{
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_AcceptTouchEvents);

    connect(m_renderTimer, &QTimer::timeout,
            this, &FrameRenderer::renderFrame);
}

FrameRenderer::~FrameRenderer() {
    stopRendering();
    makeCurrent();
    if (m_tex) glDeleteTextures(1, &m_tex);
    doneCurrent();
}

void FrameRenderer::setInstanceId(const QString& instanceId) {
    m_instanceId = instanceId;
    // Screen size and profile are set per-call in each mouse event handler
    // so concurrent FrameRenderer instances don't overwrite each other's config.
}

void FrameRenderer::setDecoder(MediaStreamDecoder* decoder) {
    m_decoder = decoder;
    if (decoder) {
        setAndroidResolution(decoder->resolution());
        connect(decoder, &MediaStreamDecoder::firstFrameDecoded,
                this, [this](int w, int h) {
            setAndroidResolution({w, h});
            emit firstFrameShown();
        });
    }
}

void FrameRenderer::setAdbClient(AdbSocketClient* adb, quint16 controlPort) {
    m_adb         = adb;
    m_controlPort = controlPort;
}

void FrameRenderer::setFps(int fps) {
    m_renderTimer->setInterval(1000 / qBound(1, fps, 60));
}

void FrameRenderer::setAndroidResolution(QSize size) {
    m_androidRes = size;
}

void FrameRenderer::startRendering() {
    if (m_rendering.exchange(true)) return;
    setFps(60);
    m_renderTimer->start();
}

void FrameRenderer::stopRendering() {
    if (!m_rendering.exchange(false)) return;
    m_renderTimer->stop();
}

// ─────────────────────────────────────────────────────────────────────────────
// OpenGL setup
// ─────────────────────────────────────────────────────────────────────────────
void FrameRenderer::initializeGL() {
    initializeOpenGLFunctions();
    glClearColor(0.05f, 0.05f, 0.05f, 1.f);
    initShaders();
    initQuad();
    glGenTextures(1, &m_tex);
    m_glReady = true;
}

void FrameRenderer::initShaders() {
    m_program.addShaderFromSourceCode(QOpenGLShader::Vertex,   k_vertSrc);
    m_program.addShaderFromSourceCode(QOpenGLShader::Fragment, k_fragSrc);
    m_program.link();
}

void FrameRenderer::initQuad() {
    // Full-screen NDC quad: pos(2) + uv(2) per vertex
    static const float quad[] = {
        -1.f,  1.f,   0.f, 0.f,   // top-left
        -1.f, -1.f,   0.f, 1.f,   // bottom-left
         1.f,  1.f,   1.f, 0.f,   // top-right
         1.f, -1.f,   1.f, 1.f,   // bottom-right
    };
    m_vao.create();
    m_vao.bind();
    m_vbo.create();
    m_vbo.bind();
    m_vbo.allocate(quad, sizeof(quad));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          reinterpret_cast<void*>(2 * sizeof(float)));
    m_vao.release();
    m_vbo.release();
}

void FrameRenderer::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
}

// ─────────────────────────────────────────────────────────────────────────────
// Render loop — called by QTimer at 60 Hz
// ─────────────────────────────────────────────────────────────────────────────
void FrameRenderer::renderFrame() {
    if (!m_decoder) return;

    DecodedFrame df;
    bool got = m_decoder->tryPopFrame(df);
    if (got && !df.image.isNull()) {
        QMutexLocker lk(&m_frameMutex);
        m_pendingFrame = std::move(df.image);
        m_newFrame = true;
    }
    update(); // triggers paintGL()

    // FPS counter
    ++m_frameCount;
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (m_fpsWindow == 0) m_fpsWindow = now;
    qint64 elapsed = now - m_fpsWindow;
    if (elapsed >= 1000) {
        float fps = m_frameCount * 1000.f / elapsed;
        m_measuredFps = fps;
        emit fpsChanged(fps);
        m_frameCount = 0;
        m_fpsWindow  = now;
    }
}

void FrameRenderer::paintGL() {
    if (!m_glReady) return;
    glClear(GL_COLOR_BUFFER_BIT);

    {
        QMutexLocker lk(&m_frameMutex);
        if (m_newFrame && !m_pendingFrame.isNull()) {
            uploadTexture(m_pendingFrame);
            m_newFrame = false;
        }
    }

    if (!m_tex) return;

    m_program.bind();
    m_vao.bind();
    glBindTexture(GL_TEXTURE_2D, m_tex);
    m_program.setUniformValue("uFrame", 0);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    m_vao.release();
    m_program.release();
}

void FrameRenderer::uploadTexture(const QImage& img) {
    QImage rgba = img.convertToFormat(QImage::Format_RGBA8888);
    glBindTexture(GL_TEXTURE_2D, m_tex);
    if (m_texSize != img.size()) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                     rgba.width(), rgba.height(), 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, rgba.constBits());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        m_texSize = img.size();
    } else {
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                        rgba.width(), rgba.height(),
                        GL_RGBA, GL_UNSIGNED_BYTE, rgba.constBits());
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Coordinate mapping
// ─────────────────────────────────────────────────────────────────────────────
QPointF FrameRenderer::mapToAndroid(const QPointF& wp) const {
    double ax = wp.x() * m_androidRes.width()  / qMax(1, width());
    double ay = wp.y() * m_androidRes.height() / qMax(1, height());
    return {ax, ay};
}

// ─────────────────────────────────────────────────────────────────────────────
// Touch emulator configuration
// ─────────────────────────────────────────────────────────────────────────────
void FrameRenderer::setTouchProfile(VirtualPhonePro::TouchProfile profile) {
    using namespace VirtualPhonePro;
    HyperRealisticTouchEmulator::instance().setProfile(profile);
}

void FrameRenderer::setTouchCharacteristics(
        const VirtualPhonePro::TouchCharacteristics& chars) {
    VirtualPhonePro::HyperRealisticTouchEmulator::instance()
        .configureTouchCharacteristics(chars);
}

// ─────────────────────────────────────────────────────────────────────────────
// Internal: dispatch a sequence of TouchPoints through the scrcpy control
// channel with realistic timing.  Runs on a worker thread so the render
// loop is never blocked.
// ─────────────────────────────────────────────────────────────────────────────
// ─────────────────────────────────────────────────────────────────────────────
// Internal: dispatch a sequence of TouchPoints through the scrcpy control
// channel.  Runs on a QtConcurrent worker thread to avoid blocking the UI,
// but serialised through m_gestureSemaphore so gestures never interleave.
// ─────────────────────────────────────────────────────────────────────────────
void FrameRenderer::dispatchTouchSequence(
        const QVector<VirtualPhonePro::TouchPoint>& points) {
    if (!m_adb || points.isEmpty()) return;

    AdbSocketClient* adb     = m_adb;
    QSize             screen = m_androidRes;

    // Fix 2: serialise gesture threads — acquire before spawning, release
    // at the end of the lambda so the next gesture waits its turn.
    m_gestureSemaphore.acquire();

    QtConcurrent::run([adb, screen, points, this]() {
        // Ensure release even if an exception escapes (Qt doesn't throw, but safe)
        struct Guard { QSemaphore& s; ~Guard(){ s.release(); } } guard{m_gestureSemaphore};

        qint64 prevTs = points.first().timestamp;

        for (const VirtualPhonePro::TouchPoint& tp : points) {
            qint64 delay = tp.timestamp - prevTs;
            if (delay > 0 && delay < 500)
                QThread::msleep(static_cast<unsigned long>(delay));
            prevTs = tp.timestamp;

            quint8 action = 0;
            switch (tp.action) {
                case VirtualPhonePro::TouchAction::DOWN:   action = 0; break;
                case VirtualPhonePro::TouchAction::MOVE:   action = 2; break;
                case VirtualPhonePro::TouchAction::UP:     action = 1; break;
                case VirtualPhonePro::TouchAction::CANCEL: action = 3; break;
            }

            QByteArray msg(28, '\0');
            auto* p = reinterpret_cast<quint8*>(msg.data());
            p[0] = 2;
            p[1] = action;
            memset(p + 2, 0, 8); // pointerId = 0

            auto bx = qToBigEndian<quint32>(static_cast<quint32>(qMax(0.f, tp.x)));
            auto by = qToBigEndian<quint32>(static_cast<quint32>(qMax(0.f, tp.y)));
            memcpy(p + 10, &bx, 4);
            memcpy(p + 14, &by, 4);

            auto bw = qToBigEndian<quint16>(static_cast<quint16>(screen.width()));
            auto bh = qToBigEndian<quint16>(static_cast<quint16>(screen.height()));
            memcpy(p + 18, &bw, 2);
            memcpy(p + 20, &bh, 2);

            float pc = qBound(0.f, tp.pressure, 1.f);
            auto bp = qToBigEndian<quint16>(static_cast<quint16>(pc * 0xFFFF));
            memcpy(p + 22, &bp, 2);

            quint32 btn = (action == 0 || action == 2) ? 1u : 0u;
            auto bbtn = qToBigEndian<quint32>(btn);
            memcpy(p + 24, &bbtn, 4);

            adb->sendControlMsg(msg);
        }
    });
}

// ─────────────────────────────────────────────────────────────────────────────
// Mouse → realistic human touch dispatch
// ─────────────────────────────────────────────────────────────────────────────
void FrameRenderer::mousePressEvent(QMouseEvent* e) {
    if (!m_adb) return;
    m_lastPressPos = e->position();
    m_pressTime    = QDateTime::currentMSecsSinceEpoch();

    auto a = mapToAndroid(e->position());

    using namespace VirtualPhonePro;
    auto& te = HyperRealisticTouchEmulator::instance();
    // Fix 1: configure with THIS instance's screen size per-call
    te.setScreenSize(m_androidRes.width(), m_androidRes.height(), 480);
    te.setProfile(TouchProfile::NATURAL);

    TouchPoint down = te.generateTouchPoint(0, a.x(), a.y(), TouchAction::DOWN);
    // Fix 3: store the exact DOWN timestamp so drag can build correct timeline
    m_downTimestamp = down.timestamp;
    dispatchTouchSequence({down});
}

void FrameRenderer::mouseReleaseEvent(QMouseEvent* e) {
    if (!m_adb) return;

    auto a     = mapToAndroid(e->position());
    auto aDown = mapToAndroid(m_lastPressPos);
    qint64 holdMs = QDateTime::currentMSecsSinceEpoch() - m_pressTime;

    float dx = a.x() - aDown.x();
    float dy = a.y() - aDown.y();
    float dist = std::sqrt(dx * dx + dy * dy);

    using namespace VirtualPhonePro;
    auto& te = HyperRealisticTouchEmulator::instance();
    // Fix 1: configure with THIS instance's screen size per-call
    te.setScreenSize(m_androidRes.width(), m_androidRes.height(), 480);
    te.setProfile(TouchProfile::NATURAL);

    QVector<TouchPoint> seq;

    if (dist < 20.f && holdMs < 300) {
        // Tap — DOWN already sent; add realistic UP
        TouchPoint up = te.generateTouchPoint(0, a.x(), a.y(), TouchAction::UP);
        up.pressure  = 0.f;
        // Fix 3: UP timestamp relative to actual DOWN time
        up.timestamp = m_downTimestamp + qMax(80LL, holdMs);
        seq.append(up);

    } else if (holdMs >= 300 && dist < 20.f) {
        // Long press
        seq = te.generateLongPress(a.x(), a.y(), static_cast<int>(holdMs));
        // Remove regenerated DOWN (already sent)
        if (!seq.isEmpty() && seq.first().action == TouchAction::DOWN)
            seq.removeFirst();
        // Fix 3: re-anchor timestamps from actual DOWN time
        if (!seq.isEmpty()) {
            qint64 offset = m_downTimestamp - seq.first().timestamp;
            for (auto& pt : seq) pt.timestamp += offset;
        }

    } else {
        // Swipe/drag
        GestureType dir = GestureType::SWIPE_RIGHT;
        if (std::abs(dx) > std::abs(dy))
            dir = (dx < 0) ? GestureType::SWIPE_LEFT : GestureType::SWIPE_RIGHT;
        else
            dir = (dy < 0) ? GestureType::SWIPE_UP   : GestureType::SWIPE_DOWN;

        seq = te.generateSwipe(dir, aDown.x(), aDown.y(), dist, true);
        // Remove regenerated DOWN (already sent)
        if (!seq.isEmpty() && seq.first().action == TouchAction::DOWN)
            seq.removeFirst();
        // Fix 3: re-anchor timestamps from actual DOWN time
        if (!seq.isEmpty()) {
            qint64 offset = m_downTimestamp - seq.first().timestamp;
            for (auto& pt : seq) pt.timestamp += offset;
        }
    }

    dispatchTouchSequence(seq);
}

void FrameRenderer::mouseMoveEvent(QMouseEvent* e) {
    if (!(e->buttons() & Qt::LeftButton)) return;

    qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (now - m_lastMoveTime < 16) return;
    m_lastMoveTime = now;

    auto a = mapToAndroid(e->position());
    using namespace VirtualPhonePro;
    auto& te = HyperRealisticTouchEmulator::instance();
    // Fix 1: configure per-call
    te.setScreenSize(m_androidRes.width(), m_androidRes.height(), 480);
    TouchPoint move = te.generateTouchPoint(0, a.x(), a.y(), TouchAction::MOVE);
    dispatchTouchSequence({move});
}
void FrameRenderer::wheelEvent(QWheelEvent* e) {
    auto a = mapToAndroid(e->position());
    float hScroll = e->angleDelta().x() / 120.f;
    float vScroll = e->angleDelta().y() / 120.f;
    sendScroll(a.x(), a.y(), hScroll, vScroll);
}
void FrameRenderer::keyPressEvent(QKeyEvent* e) {
    sendKeycode(static_cast<quint32>(e->nativeScanCode()), 0 /*DOWN*/);
}
void FrameRenderer::keyReleaseEvent(QKeyEvent* e) {
    sendKeycode(static_cast<quint32>(e->nativeScanCode()), 1 /*UP*/);
}

// ─────────────────────────────────────────────────────────────────────────────
// scrcpy control-channel message builders
// Reference: https://github.com/Genymobile/scrcpy/blob/master/app/src/control_msg.c
// ─────────────────────────────────────────────────────────────────────────────
void FrameRenderer::sendTouch(float ax, float ay, quint8 action, quint64 id) {
    if (!m_adb) return;
    QByteArray msg = buildTouchMsg(ax, ay, action, id);
    m_adb->sendControlMsg(msg);
}

void FrameRenderer::sendScroll(float ax, float ay, float hScroll, float vScroll) {
    if (!m_adb) return;
    QByteArray msg = buildScrollMsg(ax, ay, hScroll, vScroll);
    m_adb->sendControlMsg(msg);
}

void FrameRenderer::sendKeycode(quint32 keycode, quint32 action, quint32 meta) {
    if (!m_adb) return;
    QByteArray msg = buildKeyMsg(keycode, action, meta);
    m_adb->sendControlMsg(msg);
}

QByteArray FrameRenderer::buildTouchMsg(float ax, float ay,
                                        quint8 action, quint64 id) const {
    // scrcpy inject-touch v2: 28 bytes total
    // [0]    type        (1B)
    // [1]    action      (1B)  0=DOWN 1=UP 2=MOVE
    // [2-9]  pointerId   (8B, big-endian uint64)
    // [10-13] x          (4B, big-endian uint32)
    // [14-17] y          (4B, big-endian uint32)
    // [18-19] screenW    (2B, big-endian uint16)
    // [20-21] screenH    (2B, big-endian uint16)
    // [22-23] pressure   (2B, big-endian uint16, 0x0000–0xFFFF)
    // [24-27] actionBtn  (4B, big-endian uint32)
    QByteArray msg(28, '\0');
    auto* p = reinterpret_cast<quint8*>(msg.data());

    p[0] = static_cast<quint8>(ScrcpyControlMsg::InjectTouch);
    p[1] = action;

    // pointerId: widen quint32 → quint64 (no data loss, avoids UB)
    quint64 pid64 = qToBigEndian<quint64>(static_cast<quint64>(id));
    memcpy(p + 2, &pid64, 8);

    auto bx = qToBigEndian<quint32>(static_cast<quint32>(ax));
    auto by = qToBigEndian<quint32>(static_cast<quint32>(ay));
    memcpy(p + 10, &bx, 4);
    memcpy(p + 14, &by, 4);

    auto bw = qToBigEndian<quint16>(static_cast<quint16>(m_androidRes.width()));
    auto bh = qToBigEndian<quint16>(static_cast<quint16>(m_androidRes.height()));
    memcpy(p + 18, &bw, 2);
    memcpy(p + 20, &bh, 2);

    auto pressure = qToBigEndian<quint16>(0xFFFFu); // max pressure
    memcpy(p + 22, &pressure, 2);

    auto btn = qToBigEndian<quint32>(1u); // AMOTION_EVENT_BUTTON_PRIMARY
    memcpy(p + 24, &btn, 4);

    return msg;
}

QByteArray FrameRenderer::buildScrollMsg(float ax, float ay,
                                         float hScroll, float vScroll) const {
    QByteArray msg(20, '\0');
    auto* p = reinterpret_cast<quint8*>(msg.data());
    p[0] = static_cast<quint8>(ScrcpyControlMsg::InjectScroll);
    auto bx = qToBigEndian<quint32>(static_cast<quint32>(ax));
    auto by = qToBigEndian<quint32>(static_cast<quint32>(ay));
    memcpy(p + 1, &bx, 4);
    memcpy(p + 5, &by, 4);
    auto bw = qToBigEndian<quint16>(static_cast<quint16>(m_androidRes.width()));
    auto bh = qToBigEndian<quint16>(static_cast<quint16>(m_androidRes.height()));
    memcpy(p + 9, &bw, 2);
    memcpy(p + 11, &bh, 2);
    auto hs = qToBigEndian<quint16>(static_cast<quint16>(hScroll * 32));
    auto vs = qToBigEndian<quint16>(static_cast<quint16>(vScroll * 32));
    memcpy(p + 13, &hs, 2);
    memcpy(p + 15, &vs, 2);
    return msg;
}

QByteArray FrameRenderer::buildKeyMsg(quint32 keycode,
                                      quint32 action,
                                      quint32 meta) const {
    QByteArray msg(14, '\0');
    auto* p = reinterpret_cast<quint8*>(msg.data());
    p[0] = static_cast<quint8>(ScrcpyControlMsg::InjectKeycode);
    p[1] = static_cast<quint8>(action);
    auto bk = qToBigEndian<quint32>(keycode);
    auto br = qToBigEndian<quint32>(0u); // repeat
    auto bm = qToBigEndian<quint32>(meta);
    memcpy(p +  2, &bk, 4);
    memcpy(p +  6, &br, 4);
    memcpy(p + 10, &bm, 4);
    return msg;
}

} // namespace VirtualPhonePro
