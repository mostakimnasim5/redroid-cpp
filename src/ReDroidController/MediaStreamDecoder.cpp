#include "VirtualPhonePro/MediaStreamDecoder.hpp"
#include <QDebug>
#include <QDateTime>

// FFmpeg headers: only available when CMake found/downloaded FFmpeg
#ifdef VPP_FFMPEG_AVAILABLE
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}
#endif

namespace VirtualPhonePro {

#ifndef VPP_FFMPEG_AVAILABLE
// ── Stub implementation when FFmpeg is not available ──────────────────────────
MediaStreamDecoder::MediaStreamDecoder(QObject* parent) : QObject(parent) {
    qWarning() << "[MediaStreamDecoder] FFmpeg not available - using screencap fallback";
}
MediaStreamDecoder::~MediaStreamDecoder() {}
void MediaStreamDecoder::start(const QString&, quint16) {
    emit decoderError("FFmpeg not available");
}
void MediaStreamDecoder::stop() {}
bool MediaStreamDecoder::isRunning() const { return false; }
// End of stub
#else

// scrcpy video stream header per packet: 8-byte PTS + 4-byte size
static constexpr int SCRCPY_HDR = 12;

MediaStreamDecoder::MediaStreamDecoder(QObject* parent)
    : QObject(parent)
{
    // avcodec_register_all() was deprecated in FFmpeg 4.0 and removed in
    // later releases (the BtbN master-latest build we download is FFmpeg 7+).
    // Modern FFmpeg auto-registers codecs at build time, so the call is
    // unnecessary; guard it for old FFmpeg only.
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(58, 9, 100)
    avcodec_register_all();
#endif
}

MediaStreamDecoder::~MediaStreamDecoder() {
    stop();
}

void MediaStreamDecoder::setTargetFps(int fps) {
    m_targetFps = qBound(1, fps, 60);
}

void MediaStreamDecoder::setQueueDepth(int frames) {
    m_queueDepth = qBound(1, frames, 16);
}

// ─────────────────────────────────────────────────────────────────────────────
// start() — connect readyRead and wait for the first scrcpy metadata frame
// ─────────────────────────────────────────────────────────────────────────────
bool MediaStreamDecoder::start(QTcpSocket* videoSocket) {
    if (m_running) return true;

    m_socket = videoSocket;
    connect(m_socket, &QTcpSocket::readyRead,
            this, &MediaStreamDecoder::onSocketReadyRead,
            Qt::QueuedConnection);

    // scrcpy sends a 68-byte device-info header first:
    //   device name (64 bytes, null-padded) + width (2B BE) + height (2B BE)
    if (!m_socket->waitForReadyRead(5000) || m_socket->bytesAvailable() < 68) {
        emit decoderError("Timed out waiting for scrcpy device header");
        return false;
    }
    QByteArray meta = m_socket->read(68);
    int w = (quint8(meta[64]) << 8) | quint8(meta[65]);
    int h = (quint8(meta[66]) << 8) | quint8(meta[67]);
    qDebug() << "[Decoder] Device:" << meta.left(64).trimmed()
             << "resolution:" << w << "x" << h;

    if (!initDecoder(w, h)) return false;

    m_running = true;
    return true;
}

void MediaStreamDecoder::stop() {
    if (!m_running.exchange(false)) return;
    if (m_socket) {
        QObject::disconnect(m_socket, nullptr, this, nullptr);
        m_socket = nullptr;
    }
    destroyDecoder();

    // Drain the queue
    QMutexLocker lk(&m_queueMutex);
    m_frameQueue.clear();
}

// ─────────────────────────────────────────────────────────────────────────────
// FFmpeg codec setup
// ─────────────────────────────────────────────────────────────────────────────
bool MediaStreamDecoder::initDecoder(int width, int height) {
    m_width  = width;
    m_height = height;

    const AVCodec* codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!codec) {
        emit decoderError("H.264 decoder not found in libavcodec");
        return false;
    }

    m_codecCtx = avcodec_alloc_context3(codec);
    if (!m_codecCtx) return false;

    // Low-latency flags
    m_codecCtx->flags  |= AV_CODEC_FLAG_LOW_DELAY;
    m_codecCtx->flags2 |= AV_CODEC_FLAG2_FAST;
    av_opt_set(m_codecCtx->priv_data, "preset", "ultrafast", 0);
    av_opt_set_int(m_codecCtx->priv_data, "delay", 0, 0);

    m_codecCtx->thread_count = 0; // let FFmpeg choose
    m_codecCtx->thread_type  = FF_THREAD_SLICE;

    if (avcodec_open2(m_codecCtx, codec, nullptr) < 0) {
        avcodec_free_context(&m_codecCtx);
        emit decoderError("Could not open H.264 codec context");
        return false;
    }

    // swscale context: YUV420p → RGBA
    m_swsCtx = sws_getContext(width, height, AV_PIX_FMT_YUV420P,
                               width, height, AV_PIX_FMT_RGBA,
                               SWS_BILINEAR, nullptr, nullptr, nullptr);
    if (!m_swsCtx) {
        avcodec_free_context(&m_codecCtx);
        emit decoderError("Could not init swscale context");
        return false;
    }

    return true;
}

void MediaStreamDecoder::destroyDecoder() {
    if (m_swsCtx)  { sws_freeContext(m_swsCtx);      m_swsCtx  = nullptr; }
    if (m_codecCtx){ avcodec_free_context(&m_codecCtx); m_codecCtx = nullptr; }
}

// ─────────────────────────────────────────────────────────────────────────────
// Streaming: accumulate bytes, parse scrcpy packet headers, decode
// ─────────────────────────────────────────────────────────────────────────────
void MediaStreamDecoder::onSocketReadyRead() {
    if (!m_socket || !m_running) return;
    QByteArray chunk = m_socket->readAll();
    parseStreamChunk(chunk);
}

void MediaStreamDecoder::parseStreamChunk(const QByteArray& chunk) {
    m_streamBuf.append(chunk);

    while (true) {
        if (!m_headerRead) {
            if (m_streamBuf.size() < SCRCPY_HDR) return;
            // PTS: 8 bytes big-endian
            const auto* raw = reinterpret_cast<const uchar*>(m_streamBuf.constData());
            m_pendingPts = 0;
            for (int i = 0; i < 8; ++i)
                m_pendingPts = (m_pendingPts << 8) | raw[i];
            // Size: 4 bytes big-endian
            m_pendingSize = (raw[8] << 24) | (raw[9] << 16) | (raw[10] << 8) | raw[11];
            m_streamBuf.remove(0, SCRCPY_HDR);
            m_headerRead = true;
        }
        if (m_streamBuf.size() < m_pendingSize) return;

        QByteArray pktData = m_streamBuf.left(m_pendingSize);
        m_streamBuf.remove(0, m_pendingSize);
        m_headerRead = false;

        feedData(pktData);
    }
}

void MediaStreamDecoder::feedData(const QByteArray& pktData) {
    AVPacket* pkt = av_packet_alloc();
    if (!pkt) return;

    pkt->data = reinterpret_cast<uint8_t*>(
        const_cast<char*>(pktData.constData()));
    pkt->size = pktData.size();
    pkt->pts  = m_pendingPts;

    decodePacket(pkt);
    av_packet_free(&pkt);
}

void MediaStreamDecoder::decodePacket(AVPacket* pkt) {
    if (avcodec_send_packet(m_codecCtx, pkt) < 0) return;

    AVFrame* frame = av_frame_alloc();
    while (avcodec_receive_frame(m_codecCtx, frame) == 0) {
        QImage img = convertFrame(frame);
        if (!img.isNull()) {
            DecodedFrame df;
            df.image  = std::move(img);
            df.ptsUs  = (frame->pts == AV_NOPTS_VALUE) ? 0 : frame->pts;
            df.width  = frame->width;
            df.height = frame->height;

            QMutexLocker lk(&m_queueMutex);
            // Drop oldest if queue is full
            while (m_frameQueue.size() >= m_queueDepth)
                m_frameQueue.dequeue();
            m_frameQueue.enqueue(std::move(df));
            lk.unlock();
            m_frameReady.release();

            if (m_frameCount == 0)
                emit firstFrameDecoded(frame->width, frame->height);
            ++m_frameCount;
        }
    }
    av_frame_free(&frame);
}

QImage MediaStreamDecoder::convertFrame(AVFrame* frame) {
    if (!m_swsCtx) return {};

    QImage img(frame->width, frame->height, QImage::Format_RGBA8888);
    uint8_t* dst[1]  = { img.bits() };
    int dstStride[1] = { static_cast<int>(img.bytesPerLine()) };

    sws_scale(m_swsCtx,
              frame->data, frame->linesize, 0, frame->height,
              dst, dstStride);
    return img;
}

// ─────────────────────────────────────────────────────────────────────────────
// Frame consumption
// ─────────────────────────────────────────────────────────────────────────────
bool MediaStreamDecoder::tryPopFrame(DecodedFrame& frame) {
    QMutexLocker lk(&m_queueMutex);
    if (m_frameQueue.isEmpty()) return false;
    frame = m_frameQueue.dequeue();
    return true;
}

bool MediaStreamDecoder::waitForFrame(DecodedFrame& frame, int timeoutMs) {
    if (!m_frameReady.tryAcquire(1, timeoutMs)) return false;
    return tryPopFrame(frame);
}

QSize MediaStreamDecoder::resolution() const {
    return {m_width.load(), m_height.load()};
}

#endif // VPP_FFMPEG_AVAILABLE
} // namespace VirtualPhonePro
