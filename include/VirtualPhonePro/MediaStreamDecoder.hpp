#pragma once
/**
 * MediaStreamDecoder — FFmpeg-based H.264/H.265 → RGBA frame pipeline.
 *
 * Receives a raw Annex-B bitstream from a TCP socket (the scrcpy video
 * channel), feeds it through libavcodec, converts YUV420p to RGBA with
 * libswscale, and emits ready-to-display frames at up to 60 FPS.
 *
 * All decoding runs on a dedicated worker thread; decoded frames are
 * delivered to the consumer via a lock-free frame queue so the render
 * thread never blocks.
 */

#include <QObject>
#include <QThread>
#include <QTcpSocket>
#include <QMutex>
#include <QSemaphore>
#include <QQueue>
#include <QSize>
#include <QImage>
#include <functional>
#include <atomic>
#include <memory>

// Forward-declare FFmpeg structs to avoid pulling heavy C headers into every
// translation unit that includes this header.
struct AVCodecContext;
struct AVFrame;
struct AVPacket;
struct SwsContext;

namespace VirtualPhonePro {

struct DecodedFrame {
    QImage   image;       // RGBA888, ready for OpenGL upload
    qint64   ptsUs;       // presentation timestamp (µs)
    int      width;
    int      height;
};

class MediaStreamDecoder : public QObject {
    Q_OBJECT

public:
    explicit MediaStreamDecoder(QObject* parent = nullptr);
    ~MediaStreamDecoder();

    // ── Setup ─────────────────────────────────────────────────────────────────

    /** Target maximum frame rate (1-60). Default 60. */
    void setTargetFps(int fps);

    /** Maximum decode queue depth before oldest frames are dropped. */
    void setQueueDepth(int frames);

    // ── Lifecycle ─────────────────────────────────────────────────────────────

    /** Begin consuming raw H.264/H.265 Annex-B data from the socket. */
    bool start(QTcpSocket* videoSocket);

    /** Stop decoding and release all FFmpeg resources. */
    void stop();

    bool isRunning() const { return m_running.load(); }

    // ── Frame consumption (called from the render thread) ─────────────────────

    /** Returns true and fills `frame` if a new frame is available. */
    bool tryPopFrame(DecodedFrame& frame);

    /** Block until a frame is available or timeout_ms elapses. */
    bool waitForFrame(DecodedFrame& frame, int timeoutMs = 33);

    /** Current decoded resolution. */
    QSize resolution() const;

signals:
    void firstFrameDecoded(int width, int height);
    void decoderError(const QString& message);

private slots:
    void onSocketReadyRead();

private:
    // ── FFmpeg pipeline ───────────────────────────────────────────────────────
    bool initDecoder(int width, int height);
    void destroyDecoder();

    void feedData(const QByteArray& chunk);
    void decodePacket(AVPacket* pkt);
#ifdef VPP_FFMPEG_AVAILABLE
    QImage convertFrame(AVFrame* frame);
#endif

    // ── scrcpy stream parser ──────────────────────────────────────────────────
    // scrcpy prepends a 12-byte header per frame:
    //   PTS (8 bytes, big-endian) | packet size (4 bytes, big-endian)
    void parseStreamChunk(const QByteArray& chunk);

    QTcpSocket*      m_socket  = nullptr;
    QThread*         m_thread  = nullptr;

    AVCodecContext*  m_codecCtx = nullptr;
    SwsContext*      m_swsCtx   = nullptr;

    std::atomic<bool>   m_running{false};
    std::atomic<int>    m_width{0};
    std::atomic<int>    m_height{0};

    int  m_targetFps  = 60;
    int  m_queueDepth = 4;

    // Lock-free single-producer / single-consumer frame queue
    QQueue<DecodedFrame>  m_frameQueue;
    QMutex                m_queueMutex;
    QSemaphore            m_frameReady{0};

    // Partial-packet accumulation buffer
    QByteArray  m_streamBuf;
    qint64      m_pendingPts  = -1;
    int         m_pendingSize = 0;
    bool        m_headerRead  = false;

    // Decoded frame counter (used to emit firstFrameDecoded once)
    quint64     m_frameCount  = 0;
};

} // namespace VirtualPhonePro
