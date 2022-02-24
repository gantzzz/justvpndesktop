#ifndef SERVERDATAMODEL_H
#define SERVERDATAMODEL_H

#include <QString>
#include <QObject>

enum eSIG {eSIG_BEST, eSIG_GOOD, eSIG_MID, eSIG_LOW, eSIG_UNKNOWN};

class ServerDataModel
{
public:
    ServerDataModel()
    {
        m_eSignal = eSIG_UNKNOWN;
    }
    void set_server_id(const int& nId)
    {
        m_nServerId = nId;
    }
    void set_signal(eSIG signal)
    {
        m_eSignal = signal;
    }

    void set_server_address(const QString& sAddr)
    {
        m_sServerAddress = sAddr;
    }
    void set_server_country(const QString& sCountry)
    {
        m_sServerCountry = sCountry;
    }

    QString get_server_address()
    {
        return m_sServerAddress;
    }

    QString get_server_country()
    {
        return m_sServerCountry;
    }

    eSIG get_signal()
    {
        return m_eSignal;
    }

    int get_server_id()
    {
        return m_nServerId;
    }

private:
    QString m_sServerAddress;
    QString m_sServerCountry;
    eSIG m_eSignal;
    int m_nServerId;
};

Q_DECLARE_METATYPE(ServerDataModel);

#endif // SERVERDATAMODEL_H
