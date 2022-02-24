#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QtNetwork/QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QRadioButton>
#include "serverslistitemdelegate.h"
#include "serverdatamodel.h"
#include "server.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_pTrayIcon(NULL)
    , m_pTrayIconMenu(NULL)
    , m_pNetworkAccessManager(NULL)
{
    ui->setupUi(this);

    createTrayIcon();
    ServersListItemDelegate* pDelegate = new ServersListItemDelegate();
    ui->listWidget->setItemDelegate(pDelegate);
    refreshServersList();
    connect(Server::Instance(), SIGNAL(onHandshakeComplete(QString)), this, SLOT(onServerHandshakeComplete(QString)));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::createTrayIcon()
{
    if (m_pTrayIcon)
    {
        m_pTrayIcon->hide();
        delete m_pTrayIcon;
    }
    if (m_pTrayIconMenu)
    {
        delete m_pTrayIconMenu;
    }
    m_pTrayIconMenu = new QMenu;
    m_pTrayIconMenu->addAction("Connect");
    m_pTrayIconMenu->addAction("Disconnect");
    m_pTrayIconMenu->addAction("Exit");
    
    m_pTrayIcon = new QSystemTrayIcon(this);
    m_pTrayIcon->setIcon(QIcon(":icons/images/app_icon.png"));
    m_pTrayIcon->setContextMenu(m_pTrayIconMenu);
    m_pTrayIcon->show();
}

void MainWindow::refreshServersList()
{
    if (!m_pNetworkAccessManager)
    {
        m_pNetworkAccessManager = new QNetworkAccessManager();

        QObject::connect(m_pNetworkAccessManager, SIGNAL(finished(QNetworkReply*)),this, SLOT(onHttpReply(QNetworkReply*)));
    }

    QNetworkRequest req(QUrl("http://justvpn.online/api/getservers"));
    m_pNetworkAccessManager->get(req);
}

void MainWindow::onRadioButtonClicked(bool bIsChecked)
{
    QObject* pSender = sender();
    for (size_t i = 0; i < ui->listWidget->count(); i++)
    {
        auto pItem = ui->listWidget->item(i);
        if (pSender == ui->listWidget->itemWidget(pItem))
        {
            QVariant v = pItem->data(Qt::UserRole + 1);
            ServerDataModel model = v.value<ServerDataModel>();
            ui->listWidget->setCurrentItem(pItem);
            
            Server::Instance()->handshake(model.get_server_address());
        }
    }
}

void MainWindow::onServerHandshakeComplete(QString sServer)
{
    
}

void MainWindow::on_updateServersButton_clicked()
{
    refreshServersList();
}

void MainWindow::onHttpReply(QNetworkReply *pReply)
{
    QString sUrl = pReply->url().toString();
    if (pReply->error())
    {
       // TODO: Handle error
       return;
    }

    if (sUrl.contains("api/getservers"))
    {
        // delete all items
        for (size_t i = 0; i < ui->listWidget->count(); i++)
        {
            auto pItem = ui->listWidget->item(i);
            ui->listWidget->itemWidget(pItem)->deleteLater(); // delete radio button
            delete pItem;
        }
        ui->listWidget->clear();

        QString answer = pReply->readAll();
        QJsonArray arr = QJsonDocument::fromJson(answer.toUtf8()).array();

        for (auto item : arr)
        {
            ServerDataModel model;
            model.set_server_id(item.toObject().value("id").toInt());
            model.set_server_address(item.toObject().value("ip").toString());
            model.set_server_country(item.toObject().value("country").toString());

            QListWidgetItem* pItem = new QListWidgetItem(ui->listWidget);
            QVariant v;
            v.setValue(model);
            pItem->setData(Qt::UserRole + 1, v);

            QRadioButton* pButton = new QRadioButton();
            connect(pButton, SIGNAL(clicked(bool)),this,SLOT(onRadioButtonClicked(bool)));
            ui->listWidget->setItemWidget(pItem, pButton);
            ui->listWidget->addItem(pItem);

            // Request number of connections for the server
            QNetworkRequest req(QUrl("http://justvpn.online/api/connections?serverid="+ QString::number(model.get_server_id())));
            m_pNetworkAccessManager->get(req);
        }
    }
    else if(sUrl.contains("api/connections"))
    {
        int nInd = sUrl.lastIndexOf("serverid=") + strlen("serverid=");
        int nServerId = sUrl.mid(nInd).toInt();
        QJsonObject obj = QJsonDocument::fromJson(pReply->readAll()).object();
        int numOfConnections = obj.value("connections").toInt(-1);

        // Update model
        for (size_t i = 0; i < ui->listWidget->count(); i++)
        {
            auto pItem = ui->listWidget->item(i);
            QVariant v = pItem->data(Qt::UserRole + 1);
            ServerDataModel model = v.value<ServerDataModel>();
            if (model.get_server_id() == nServerId)
            {
                if (numOfConnections >= 100)
                {
                    model.set_signal(eSIG_LOW);
                }
                else if (numOfConnections >= 70)
                {
                    model.set_signal(eSIG_MID);
                }
                else if (numOfConnections >= 50)
                {
                    model.set_signal(eSIG_GOOD);
                }
                else if (numOfConnections >= 0)
                {
                    model.set_signal(eSIG_BEST);
                }
            }
            v.setValue(model);
            pItem->setData(Qt::UserRole + 1, v);
        }
    }
}

