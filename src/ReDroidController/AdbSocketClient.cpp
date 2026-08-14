#include "VirtualPhonePro/AdbSocketClient.hpp"
#include <QEventLoop>
#include <QTimer>
#include <QDataStream>
#include <QDebug>
#include <QtEndian>

namespace VirtualPhonePro {

// ── Helpers ─────────────────────────────────────────────────────────────────

static QByteArray buildConnectPayload() {
    return QByteArrayLiteral("host::features=shell_v2,cmd,stat_v2,ls_v2,fixed_push_mkdir");
}

static quint32 adbCrc32(const QByteArray& d) {
    quint32 sum = 0;
    for (unsigned char c : d) sum += c;
    return sum;
}

// Write a little-endian 24-byte ADB message header
static QByteArray packHeader(quint32 cmd, quint32 a0, quint32 a1,
                             const QByteArray& data) {
    QByteArray hdr(24, '\0');
    auto* p = reinterpret_cast<quint32*>(hdr.data());
    p[0] = qToLittleEndian(cmd);
    p[1] = qToLittleEndian(a0);
    p[2] = qToLittleEndian(a1);
    p[3] = qToLittleEndian(static_cast<quint32>(data.size()));
    p[4] = qToLittleEndian(adbCrc32(data));
    p[5] = qToLittleEndian(cmd ^ ADB_MAGIC_OFFS);
    return hdr;
}

// ── AdbSocketClient ──────────────────────────────────────────────────────────

AdbSocketClient::AdbSocketClient(QObject* parent)
    : QObject(parent)
    , m_socket(new QTcpSocket(this))
{
    connect(m_socket, &QTcpSocket::readyRead,
            this, &AdbSocketClient::onSocketReadyRead);
    connect(m_socket, &QTcpSocket::disconnected,
            this, &AdbSocketClient::onSocketDisconnected);
    connect(m_socket,
            QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred),
            this, &AdbSocketClient::onSocketError);
}

AdbSocketClient::~AdbSocketClient() {
    disconnect();
}

// ─────────────────────────────────────────────────────────────────────────────
// connectToDevice — TCP connect + ADB CNXN handshake
// ─────────────────────────────────────────────────────────────────────────────
bool AdbSocketClient::connectToDevice(const QString& host, quint16 port) {
    m_socket->connectToHost(host, port);
    if (!m_socket->waitForConnected(8000)) {
        emit error(QString("TCP connect failed: %1").arg(m_socket->errorString()));
        return false;
    }
    if (!doHandshake()) {
        emit error("ADB handshake failed");
        m_socket->disconnectFromHost();
        return false;
    }
    m_connected = true;
    emit connected();
    return true;
}

void AdbSocketClient::disconnect() {
    m_connected = false;
    if (m_socket->state() != QAbstractSocket::UnconnectedState)
        m_socket->disconnectFromHost();
}

// ─────────────────────────────────────────────────────────────────────────────
// ADB handshake: send CNXN → expect CNXN back
// ─────────────────────────────────────────────────────────────────────────────
bool AdbSocketClient::doHandshake() {
    QByteArray payload = buildConnectPayload();
    sendMessage(A_CNXN, ADB_VERSION, ADB_MAX_DATA, payload);

    AdbMessage reply;
    if (!readMessage(reply, 8000)) return false;
    if (reply.command != A_CNXN)  return false;

    qDebug() << "[ADB] Connected to device:" << reply.data;
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// shell() — synchronous one-shot shell command, returns stdout
// ─────────────────────────────────────────────────────────────────────────────
QByteArray AdbSocketClient::shell(const QString& command, int timeoutMs) {
    quint32 lid = nextLocalId();
    QByteArray result;
    bool done = false;
    QMutex mx;

    {
        QMutexLocker lk(&m_channelMutex);
        AdbChannel ch;
        ch.localId  = lid;
        ch.remoteId = 0;
        ch.open     = false;
        ch.onData   = [&](const QByteArray& d) { result.append(d); };
        ch.onClose  = [&]() { QMutexLocker l2(&mx); done = true; };
        m_channels[lid] = ch;
    }

    QByteArray svc = "shell:" + command.toUtf8();
    sendMessage(A_OPEN, lid, 0, svc);

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);

    // Poll until channel closes or timeout
    timer.start(timeoutMs);
    while (!done && timer.isActive()) {
        loop.processEvents(QEventLoop::AllEvents, 50);
    }

    {
        QMutexLocker lk(&m_channelMutex);
        m_channels.remove(lid);
    }
    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// push() — push bytes to a remote path via ADB SYNC protocol
// ─────────────────────────────────────────────────────────────────────────────
bool AdbSocketClient::push(const QByteArray& data,
                           const QString& remotePath,
                           quint32 mode) {
    quint32 lid = nextLocalId();
    {
        QMutexLocker lk(&m_channelMutex);
        AdbChannel ch;
        ch.localId  = lid;
        ch.remoteId = 0;
        ch.open     = false;
        m_channels[lid] = ch;
    }

    sendMessage(A_OPEN, lid, 0, QByteArrayLiteral("sync:"));

    // Wait for OKAY
    AdbMessage reply;
    for (int i = 0; i < 20; ++i) {
        if (readMessage(reply, 500) && reply.command == A_OKAY) break;
    }

    bool ok = syncSend(lid, data, remotePath, mode);
    syncQuit(lid);

    sendMessage(A_CLSE, lid, m_channels.value(lid).remoteId);
    {
        QMutexLocker lk(&m_channelMutex);
        m_channels.remove(lid);
    }
    return ok;
}

// ─────────────────────────────────────────────────────────────────────────────
// forward() — tcp:<remotePort> forwarding via shell socat/nc
//   scrcpy uses fixed video port 1234 and control port 1235 inside the container.
// ─────────────────────────────────────────────────────────────────────────────
bool AdbSocketClient::forward(quint16 localPort, quint16 remotePort) {
    // ADB TCP forward is set up via the "tcp:" service on the host daemon.
    // Since we are the daemon ourselves here, we open a tcp:remotePort channel
    // and relay it through a local server — simulated by the caller.
    // For simplicity we use the reverse-forward shell trick:
    QString cmd = QString("nohup sh -c 'while true; do nc -l -p %1 | nc 127.0.0.1 %2; done' >/dev/null 2>&1 &")
                      .arg(localPort).arg(remotePort);
    QByteArray out = shell(cmd, 3000);
    Q_UNUSED(out)
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// openShellStream() — persistent shell channel with async callbacks
// ─────────────────────────────────────────────────────────────────────────────
quint32 AdbSocketClient::openShellStream(const QString& command,
                                         std::function<void(const QByteArray&)> onData,
                                         std::function<void()> onClose) {
    quint32 lid = nextLocalId();
    {
        QMutexLocker lk(&m_channelMutex);
        AdbChannel ch;
        ch.localId  = lid;
        ch.remoteId = 0;
        ch.open     = false;
        ch.onData   = std::move(onData);
        ch.onClose  = std::move(onClose);
        m_channels[lid] = ch;
    }
    sendMessage(A_OPEN, lid, 0, ("shell:" + command).toUtf8());
    return lid;
}

void AdbSocketClient::closeChannel(quint32 localId) {
    quint32 rid = 0;
    {
        QMutexLocker lk(&m_channelMutex);
        if (m_channels.contains(localId))
            rid = m_channels[localId].remoteId;
        m_channels.remove(localId);
    }
    if (rid) sendMessage(A_CLSE, localId, rid);
}

// ─────────────────────────────────────────────────────────────────────────────
// Low-level I/O
// ─────────────────────────────────────────────────────────────────────────────
bool AdbSocketClient::sendMessage(quint32 cmd, quint32 a0, quint32 a1,
                                  const QByteArray& data) {
    QByteArray hdr = packHeader(cmd, a0, a1, data);
    if (m_socket->write(hdr) != hdr.size()) return false;
    if (!data.isEmpty() && m_socket->write(data) != data.size()) return false;
    m_socket->flush();
    return true;
}

bool AdbSocketClient::readMessage(AdbMessage& out, int timeoutMs) {
    // Header = 24 bytes
    while (m_socket->bytesAvailable() < 24) {
        if (!m_socket->waitForReadyRead(timeoutMs)) return false;
    }
    QByteArray hdr = m_socket->read(24);
    if (hdr.size() != 24) return false;
    auto* p = reinterpret_cast<const quint32*>(hdr.constData());
    out.command = qFromLittleEndian(p[0]);
    out.arg0    = qFromLittleEndian(p[1]);
    out.arg1    = qFromLittleEndian(p[2]);
    out.dataLen = qFromLittleEndian(p[3]);
    out.dataCrc = qFromLittleEndian(p[4]);
    out.magic   = qFromLittleEndian(p[5]);

    if (out.dataLen > 0) {
        while (m_socket->bytesAvailable() < (qint64)out.dataLen) {
            if (!m_socket->waitForReadyRead(timeoutMs)) return false;
        }
        out.data = m_socket->read(out.dataLen);
    }
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// Async dispatch — called from readyRead signal
// ─────────────────────────────────────────────────────────────────────────────
void AdbSocketClient::onSocketReadyRead() {
    m_rxBuf.append(m_socket->readAll());

    while (m_rxBuf.size() >= 24) {
        auto* p = reinterpret_cast<const quint32*>(m_rxBuf.constData());
        quint32 dataLen = qFromLittleEndian(p[3]);
        if (m_rxBuf.size() < (qint64)(24 + dataLen)) break;

        AdbMessage msg;
        msg.command = qFromLittleEndian(p[0]);
        msg.arg0    = qFromLittleEndian(p[1]);
        msg.arg1    = qFromLittleEndian(p[2]);
        msg.dataLen = dataLen;
        msg.dataCrc = qFromLittleEndian(p[4]);
        msg.magic   = qFromLittleEndian(p[5]);
        msg.data    = m_rxBuf.mid(24, dataLen);
        m_rxBuf.remove(0, 24 + dataLen);
        dispatchMessage(msg);
    }
}

void AdbSocketClient::dispatchMessage(const AdbMessage& msg) {
    QMutexLocker lk(&m_channelMutex);
    switch (msg.command) {
    case A_OKAY: {
        auto it = m_channels.find(msg.arg1);
        if (it != m_channels.end()) {
            it->remoteId = msg.arg0;
            it->open     = true;
            // Send OKAY back
            lk.unlock();
            sendMessage(A_OKAY, msg.arg1, msg.arg0);
        }
        break;
    }
    case A_WRTE: {
        auto it = m_channels.find(msg.arg1);
        if (it != m_channels.end() && it->onData)
            it->onData(msg.data);
        lk.unlock();
        sendMessage(A_OKAY, msg.arg1, msg.arg0);
        break;
    }
    case A_CLSE: {
        auto it = m_channels.find(msg.arg1);
        if (it != m_channels.end()) {
            auto cb = it->onClose;
            m_channels.erase(it);
            lk.unlock();
            if (cb) cb();
        }
        break;
    }
    default: break;
    }
}

void AdbSocketClient::onSocketDisconnected() {
    m_connected = false;
    emit disconnected();
}
void AdbSocketClient::onSocketError(QAbstractSocket::SocketError) {
    emit error(m_socket->errorString());
}

quint32 AdbSocketClient::nextLocalId() {
    return m_nextId.fetch_add(1, std::memory_order_relaxed);
}

// ─────────────────────────────────────────────────────────────────────────────
// SYNC SEND — push file data through an open sync: channel
// ─────────────────────────────────────────────────────────────────────────────
bool AdbSocketClient::syncSend(quint32 lid, const QByteArray& data,
                               const QString& remotePath, quint32 mode) {
    quint32 rid = m_channels.value(lid).remoteId;

    // SEND header: "SEND" + path,mode
    QString pathMode = remotePath + "," + QString::number(mode);
    QByteArray sendCmd;
    {
        QDataStream ds(&sendCmd, QIODevice::WriteOnly);
        ds.setByteOrder(QDataStream::LittleEndian);
        ds.writeRawData("SEND", 4);
        ds << static_cast<quint32>(pathMode.size());
    }
    sendCmd.append(pathMode.toUtf8());
    sendMessage(A_WRTE, lid, rid, sendCmd);

    // DATA chunks (max 64 KB each)
    const int chunkSize = 64 * 1024;
    for (int off = 0; off < data.size(); off += chunkSize) {
        QByteArray chunk;
        QDataStream ds(&chunk, QIODevice::WriteOnly);
        ds.setByteOrder(QDataStream::LittleEndian);
        ds.writeRawData("DATA", 4);
        QByteArray piece = data.mid(off, chunkSize);
        ds << static_cast<quint32>(piece.size());
        chunk.append(piece);
        sendMessage(A_WRTE, lid, rid, chunk);
    }

    // DONE (mtime = current epoch seconds)
    QByteArray done;
    {
        QDataStream ds(&done, QIODevice::WriteOnly);
        ds.setByteOrder(QDataStream::LittleEndian);
        ds.writeRawData("DONE", 4);
        ds << static_cast<quint32>(QDateTime::currentSecsSinceEpoch());
    }
    sendMessage(A_WRTE, lid, rid, done);
    return true;
}

bool AdbSocketClient::syncQuit(quint32 lid) {
    quint32 rid = m_channels.value(lid).remoteId;
    sendMessage(A_WRTE, lid, rid, QByteArrayLiteral("QUIT\x00\x00\x00\x00"));
    return true;
}

} // namespace VirtualPhonePro
