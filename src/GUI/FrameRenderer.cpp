#include "VirtualPhonePro/FrameRenderer.hpp"
#include "VirtualPhonePro/MediaStreamDecoder.hpp"
#include "VirtualPhonePro/AdbSocketClient.hpp"
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
// Mouse → touch dispatch
// ─────────────────────────────────────────────────────────────────────────────
void FrameRenderer::mousePressEvent(QMouseEvent* e) {
    auto a = mapToAndroid(e->position());
    sendTouch(a.x(), a.y(), 0 /*ACTION_DOWN*/);
}
void FrameRenderer::mouseReleaseEvent(QMouseEvent* e) {
    auto a = mapToAndroid(e->position());
    sendTouch(a.x(), a.y(), 1 /*ACTION_UP*/);
}
void FrameRenderer::mouseMoveEvent(QMouseEvent* e) {
    if (!(e->buttons() & Qt::LeftButton)) return;
    auto a = mapToAndroid(e->position());
    sendTouch(a.x(), a.y(), 2 /*ACTION_MOVE*/);
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
void FrameRenderer::sendTouch(float ax, float ay, quint8 action, quint32 id) {
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
                                        quint8 action, quint32 id) const {
    // scrcpy inject-touch: 1 + 1 + 4 + 4 + 4 + 2 + 2 + 2 + 4 = 28 bytes
    QByteArray msg(28, '\0');
    auto* p = reinterpret_cast<quint8*>(msg.data());
    p[0] = static_cast<quint8>(ScrcpyControlMsg::InjectTouch);
    p[1] = action;
    // pointer id (8 bytes, big-endian)
    quint64 pid = qToBigEndian<quint64>(id);
    memcpy(p + 2, &pid, 8);
    // position
    auto bx = qToBigEndian<quint32>(static_cast<quint32>(ax));
    auto by = qToBigEndian<quint32>(static_cast<quint32>(ay));
    memcpy(p + 10, &bx, 4);
    memcpy(p + 14, &by, 4);
    // screen size
    auto bw = qToBigEndian<quint16>(static_cast<quint16>(m_androidRes.width()));
    auto bh = qToBigEndian<quint16>(static_cast<quint16>(m_androidRes.height()));
    memcpy(p + 18, &bw, 2);
    memcpy(p + 20, &bh, 2);
    // pressure (fixed-point 0–0xFFFF = 0.0–1.0)
    auto pressure = qToBigEndian<quint16>(0xFFFF); // full pressure
    memcpy(p + 22, &pressure, 2);
    // action button (AMOTION_EVENT_BUTTON_PRIMARY = 1)
    auto btn = qToBigEndian<quint32>(1u);
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
