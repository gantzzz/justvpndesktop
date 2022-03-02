#include "server.h"
#include <QtNetwork/QNetworkDatagram>
#include "tundev.h"
#include <QtCore/qthread.h>
#include <netdb.h>
#include <QInputDialog>

std::mutex Server::m_Mutex;
Server* Server::m_pSelf = nullptr;

Server::Server() :
    m_pProcessForwardingThread(nullptr)
  , m_pSocket(nullptr)
  , m_bActive(false)
  , m_nTunnelFD(-1)
{
}
Server::~Server()
{
    
}

bool Server::handshake(QString serverAddress)
{
    if (m_sRootPassword.empty())
    {
        bool ok;
        QString text = QInputDialog::getText(nullptr, tr("root password"),
                                             tr("root password:"), QLineEdit::Password,
                                             "", &ok);
        if (ok)
        {
            m_sRootPassword = text.toStdString();
        }
    }
    if (m_pSocket)
    {
        m_pSocket->disconnectFromHost();

        StopProcessingThread();

        delete m_pSocket;

        tundev::unset_default_route(m_sAddress.toStdString());
    }

    m_sServerAddress = serverAddress;
    m_pSocket = new QUdpSocket(this);
    connect(m_pSocket, SIGNAL(readyRead()), this, SLOT(onSocketReadyRead()));
    
    std::string control = " action:connect";
    control[0] = 0;

    int nSent = m_pSocket->writeDatagram(control.c_str(), control.length() ,QHostAddress(serverAddress), 8811 );
    
    control = " action:getparameters";
    control[0] = 0;
    nSent = m_pSocket->writeDatagram(control.c_str(), control.length() ,QHostAddress(serverAddress), 8811 );
    emit onHandshakeComplete(serverAddress);
}

void Server::processControlMessage(const QString& sControlMsg)
{
    if (sControlMsg.contains("action:connected"))
    {
        if (m_nTunnelFD > 0)
        {
            close(m_nTunnelFD);
        }
        m_nTunnelFD = tundev::open_tun_socket();
    }
    else if (sControlMsg.contains("mtu") &&
             sControlMsg.contains("route"))
    {
        for (auto param : sControlMsg.split(";"))
        {
            auto p = param.split(":");
            if (p[0] == "mtu")
            {
                m_sMTU = p[1];
            }
            else if (p[0] == "address")
            {
                m_sAddress = p[1];
            }
            else if (p[0] == "dns")
            {
                m_sDNS = p[1];
            }
        }
        
        // now as we have all the parameters, configure tun interface
        tundev::protect(m_sServerAddress.toStdString());
        tundev::set_tun_address(m_sAddress.toStdString());
        tundev::set_default_route(m_sAddress.toStdString());
        tundev::set_ip_forward();
        tundev::set_iptables_masquerade();
        
        string control = " action:configured";
        control[0] = 0;
        m_pSocket->writeDatagram(control.c_str(), control.length() ,QHostAddress(m_sServerAddress), 8811 );
        
        control = " action:verifysubscription;token=debug";
        control[0] = 0;
        m_pSocket->writeDatagram(control.c_str(), control.length() ,QHostAddress(m_sServerAddress), 8811 );
        
        // start tun select thread
        StopProcessingThread();
        m_bActive = true;
        m_pProcessForwardingThread = new std::thread(&Server::processForwarding, this);
    }
    else if(sControlMsg.contains("keepalive"))
    {
        string control = " action:keepalive";
        control[0] = 0;
        m_pSocket->writeDatagram(control.c_str(), control.length() ,QHostAddress(m_sServerAddress), 8811 );
    }
}

void Server::processForwarding()
{
    char packet[1500] = { 0 };
    int length;

    fd_set fdset;
    struct timeval timeout;

    while (m_bActive)
    {
        FD_ZERO(&fdset);
        FD_SET(m_nTunnelFD, &fdset);

        memset(&timeout, 0, sizeof(struct timeval));
        timeout.tv_sec = 2;
        timeout.tv_usec = 0;

        int retval = select(m_nTunnelFD + 1, &fdset, NULL, NULL, &timeout);

        if (retval > 0)
        {
            if (FD_ISSET(m_nTunnelFD, &fdset))
            {
                length = read(m_nTunnelFD, packet, sizeof(packet));
                if (length > 0)
                {
                    m_pSocket->writeDatagram(packet, length ,QHostAddress(m_sServerAddress), 8811 );
                }
            }
        }
    }
}

void Server::StopProcessingThread()
{
    if (m_pProcessForwardingThread)
    {
        m_bActive = false;
        m_pProcessForwardingThread->join();
        delete m_pProcessForwardingThread;
        m_pProcessForwardingThread = nullptr;
    }
}

void Server::onSocketReadyRead()
{
    auto data = m_pSocket->receiveDatagram().data();
    if (data.size() > 0)
    {
        if (data.at(0) == 0) // this is a control message
        {
            QString sMessage;
            for (int i = 1; i< data.size(); i++)
            {
                sMessage.append(data.at(i));
            }
            processControlMessage(sMessage);
        }
        else
        {
            // to tunnel
            write(m_nTunnelFD, data, data.size());
        }
    }
}
