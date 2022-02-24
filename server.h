#ifndef server_h
#define server_h

#include <mutex>
#include <QObject>
#include <QString>
#include <QtNetwork/QUdpSocket>
#include <thread>

class Server : public QObject
{
    Q_OBJECT
    
public:
    static Server* Instance()
    {
        if (!m_pSelf)
        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            if (!m_pSelf)
            {
                m_pSelf = new Server();
            }
        }
        return m_pSelf;
    }
    bool handshake(QString serverAddress);
private:
    Server();
    ~Server();
    static Server* m_pSelf;
    static std::mutex m_Mutex;
    QUdpSocket* m_pSocket;
    void processControlMessage(const QString& sControl);
    QString m_sTunInterface;
    int m_nTunnelFD;
    QString m_sMTU;
    QString m_sAddress;
    QString m_sDNS;
    QString m_sServerAddress;
    std::thread* m_pProcessForwardingThread;
    void processForwarding();
    bool m_bActive;
    void StopProcessingThread();

private slots:
    void onSocketReadyRead();

signals:
    void onHandshakeComplete(QString);
    
};

#endif /* server_hpp */
