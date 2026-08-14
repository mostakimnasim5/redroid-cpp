#pragma once
/**
 * FrameRenderer — QOpenGLWidget-based 30-60 FPS canvas renderer.
 *
 * Accepts decoded RGBA frames from MediaStreamDecoder and renders them
 * to the screen using a simple fullscreen-quad OpenGL shader.  The
 * frame is uploaded once per paint cycle as a GL_TEXTURE_2D so the GPU
 * does the final scaling with bilinear filtering.
 *
 * Input dispatch:
 *   Mouse/touch events are translated back to Android screen coordinates
 *   and forwarded to AdbSocketClient via the scrcpy control channel.
 */

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLTexture>
#include <QTimer>
#include <QMutex>
#include <QSize>
#include <QImage>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QTouchEvent>
#include <memory>
#include <atomic>
#include <functional>

namespace VirtualPhonePro {

// Forward declarations
class AdbSocketClient;
class MediaStreamDecoder;

/** scrcpy control-channel message types */
enum class ScrcpyControlMsg : quint8 {
    InjectTouch     = 2,
    InjectScroll    = 3,
    InjectKeycode   = 0,
    InjectText      = 1,
    BackOrScreen    = 4,
    ExpandNotifPanel= 5,
    CollapseNotif   = 6,
    GetClipboard    = 7,
    SetClipboard    = 9,
    SetScreenPower  = 10,
    RotateDevice    = 11,
};

class FrameRenderer : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT

public:
    explicit FrameRenderer(QWidget* parent = nullptr);
    ~FrameRenderer();

    // ── Wiring ────────────────────────────────────────────────────────────────

    /** Provide the decoder and the ADB client used for input dispatch. */
    void setDecoder(MediaStreamDecoder* decoder);
    void setAdbClient(AdbSocketClient* adb, quint16 controlPort);

    /** Target render rate (1-60 Hz). */
    void setFps(int fps);

    /** Android screen size — needed for coordinate mapping. */
    void setAndroidResolution(QSize size);

    // ── Control ───────────────────────────────────────────────────────────────
    void startRendering();
    void stopRendering();
    bool isRendering() const { return m_rendering.load(); }

    /** Current measured FPS. */
    float measuredFps() const { return m_measuredFps.load(); }

signals:
    void fpsChanged(float fps);
    void firstFrameShown();

protected:
    // ── QOpenGLWidget ─────────────────────────────────────────────────────────
    void initializeGL()    override;
    void resizeGL(int w, int h) override;
    void paintGL()         override;

    // ── Input events ──────────────────────────────────────────────────────────
    void mousePressEvent  (QMouseEvent*  e) override;
    void mouseReleaseEvent(QMouseEvent*  e) override;
    void mouseMoveEvent   (QMouseEvent*  e) override;
    void wheelEvent       (QWheelEvent*  e) override;
    void keyPressEvent    (QKeyEvent*    e) override;
    void keyReleaseEvent  (QKeyEvent*    e) override;

private slots:
    void renderFrame();   // driven by m_renderTimer

private:
    // ── OpenGL helpers ────────────────────────────────────────────────────────
    void initShaders();
    void initQuad();
    void uploadTexture(const QImage& img);

    // ── Input dispatch ────────────────────────────────────────────────────────
    QPointF mapToAndroid(const QPointF& widgetPos) const;

    void sendTouch(float ax, float ay, quint8 action, quint32 pointerId = 0);
    void sendScroll(float ax, float ay, float hScroll, float vScroll);
    void sendKeycode(quint32 keycode, quint32 action, quint32 metaState = 0);

    QByteArray buildTouchMsg (float ax, float ay, quint8 action, quint32 id) const;
    QByteArray buildScrollMsg(float ax, float ay, float hScroll, float vScroll) const;
    QByteArray buildKeyMsg   (quint32 keycode, quint32 action, quint32 meta) const;

    // ── Members ───────────────────────────────────────────────────────────────
    MediaStreamDecoder*      m_decoder     = nullptr;
    AdbSocketClient*         m_adb         = nullptr;
    quint16                  m_controlPort = 0;

    QTimer*                  m_renderTimer = nullptr;
    std::atomic<bool>        m_rendering{false};

    // OpenGL objects
    QOpenGLShaderProgram     m_program;
    QOpenGLBuffer            m_vbo{QOpenGLBuffer::VertexBuffer};
    QOpenGLVertexArrayObject m_vao;
    GLuint                   m_tex = 0;
    QSize                    m_texSize;
    bool                     m_glReady = false;

    // Pending frame from decoder
    QImage  m_pendingFrame;
    QMutex  m_frameMutex;
    bool    m_newFrame = false;

    // Android coordinate mapping
    QSize   m_androidRes{1080, 1920};

    // FPS measurement
    std::atomic<float>  m_measuredFps{0.f};
    int                 m_frameCount = 0;
    qint64              m_fpsWindow  = 0;

    // Vertex shader source (fullscreen-quad, NDC coordinates)
    static constexpr const char* k_vertSrc = R"(
        #version 330 core
        layout(location = 0) in vec2 aPos;
        layout(location = 1) in vec2 aUV;
        out vec2 vUV;
        void main() {
            gl_Position = vec4(aPos, 0.0, 1.0);
            vUV = aUV;
        }
    )";

    // Fragment shader — bilinear sampling, gamma-correct sRGB output
    static constexpr const char* k_fragSrc = R"(
        #version 330 core
        in vec2 vUV;
        out vec4 fragColor;
        uniform sampler2D uFrame;
        void main() {
            fragColor = texture(uFrame, vUV);
        }
    )";
};

} // namespace VirtualPhonePro
