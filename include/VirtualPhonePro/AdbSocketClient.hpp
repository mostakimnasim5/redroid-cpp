#pragma once
/**
 * AdbSocketClient — Zero-external-exe ADB protocol implementation.
 *
 * Speaks the raw ADB wire protocol over a plain TCP socket so that
 * adb.exe is never required at runtime.  Supports:
 *   - CONNECT handshake
 *   - OPEN / OKAY / SEND / WRTE / CLSE message types
 *   - shell:, sync:, and tcp: service channels
 *   - File push (SYNC SEND) for uploading scrcpy-server.jar
 *   - Port forwarding setup (for the scrcpy control/video channels)
 */

#include <QObject>
#include <QTcpSocket>
#include <QTimer>
#include <QString>
#include <QByteArray>
#include <QMutex>
#include <QHash>
#include <functional>
#include <atomic>

namespace VirtualPhonePro {

// ── ADB wire-protocol constants ──────────────────────────────────────────────
static constexpr quint32 ADB_MAGIC_OFFS   = 0xFFFFFFFF;
static constexpr quint32 A_SYNC           = 0x434e5953;
static constexpr quint32 A_CNXN           = 0x4e584e43;
static constexpr quint32 A_AUTH           = 0x48545541;
static constexpr quint32 A_OPEN           = 0x4e45504f;
static constexpr quint32 A_OKAY           = 0x59414b4f;
static constexpr quint32 A_CLSE           = 0x45534c43;
static constexpr quint32 A_WRTE           = 0x45545257;

static constexpr quint32 ADB_VERSION      = 0x01000001;
static constexpr quint32 ADB_MAX_DATA     = 1024 * 1024; // 1 MB

struct AdbMessage {
    quint32 command;
    quint32 arg0;
    quint32 arg1;
    quint32 dataLen;
    quint32 dataCrc;
    quint32 magic;
    QByteArray data;
};

// ── Channel: one logical ADB service stream ──────────────────────────────────
struct AdbChannel {
    quint32 localId;
    quint32 remoteId;
    bool    open;
    QByteArray rxBuffer;
    std::function<void(const QByteArray&)> onData;
    std::function<void()>                  onClose;
};

class AdbSocketClient : public QObject {
    Q_OBJECT

public:
    explicit AdbSocketClient(QObject* parent = nullptr);
    ~AdbSocketClient();

    // ── Lifecycle ─────────────────────────────────────────────────────────────
    bool connectToDevice(const QString& host, quint16 port = 5555);
    void disconnect();
    bool isConnected() const { return m_connected.load(); }

    // ── High-level helpers ────────────────────────────────────────────────────

    /** Execute a shell command; returns its stdout as a byte array. */
    QByteArray shell(const QString& command, int timeoutMs = 10000);

    /** Push a local byte array to a remote path (mode e.g. 0755). */
    bool push(const QByteArray& data, const QString& remotePath, quint32 mode = 0644);

    /**
     * Set up a remote-to-local TCP forward so that connecting to
     * localPort on 127.0.0.1 is forwarded to remotePort inside the
     * container — used for the scrcpy video and control channels.
     */
    bool forward(quint16 localPort, quint16 remotePort);

    /**
     * Open a persistent shell channel and stream each output chunk
     * to the provided callback (called on the main/Qt thread).
     */
    quint32 openShellStream(const QString& command,
                            std::function<void(const QByteArray&)> onData,
                            std::function<void()> onClose = {});

    /** Close a channel opened with openShellStream(). */
    void closeChannel(quint32 localId);

    /** Open the scrcpy control channel (TCP port inside container).
     *  Must be called after the video channel is up.
     *  All input messages (touch/scroll/key) are sent through this socket. */
    bool openControlChannel(const QString& host, quint16 controlPort);
    void closeControlChannel();

    /** Adopt a pre-accepted QTcpSocket as the control channel.
     *  Used when the scrcpy server connects TO US (tunnel_forward=false)
     *  so no outbound connect is needed. Takes ownership of the socket. */
    void adoptControlSocket(QTcpSocket* socket);

    /** Send a raw scrcpy control-protocol message synchronously. */
    bool sendControlMsg(const QByteArray& msg);

signals:
    void connected();
    void disconnected();
    void error(const QString& message);

private slots:
    void onSocketReadyRead();
    void onSocketDisconnected();
    void onSocketError(QAbstractSocket::SocketError err);

private:
    // ── Message I/O ───────────────────────────────────────────────────────────
    bool      sendMessage(quint32 cmd, quint32 arg0, quint32 arg1,
                          const QByteArray& data = {});
    bool      readMessage(AdbMessage& out, int timeoutMs = 5000);
    quint32   crc32(const QByteArray& data) const;

    // ── Handshake ─────────────────────────────────────────────────────────────
    bool      doHandshake();

    // ── Channel management ────────────────────────────────────────────────────
    quint32   nextLocalId();
    AdbChannel* findChannel(quint32 localId);
    void      dispatchMessage(const AdbMessage& msg);

    // ── SYNC (file push) ──────────────────────────────────────────────────────
    bool      syncSend(quint32 channelLocalId,
                       const QByteArray& data,
                       const QString& remotePath,
                       quint32 mode);
    bool      syncQuit(quint32 channelLocalId);

    QTcpSocket*                m_socket;
    QTcpSocket*                m_controlSocket = nullptr; // scrcpy control channel
    QMutex                     m_controlMutex;            // guards m_controlSocket writes
    std::atomic<bool>          m_connected{false};
    std::atomic<quint32>       m_nextId{1};

    QHash<quint32, AdbChannel> m_channels;
    QMutex                     m_channelMutex;

    // Incoming byte buffer (partial messages accumulate here)
    QByteArray                 m_rxBuf;
};

} // namespace VirtualPhonePro
