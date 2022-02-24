#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QtNetwork/QNetworkAccessManager>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    void createTrayIcon();
    QSystemTrayIcon* m_pTrayIcon;
    QMenu* m_pTrayIconMenu;
    void refreshServersList();
    QNetworkAccessManager* m_pNetworkAccessManager;

private slots:
    void onRadioButtonClicked(bool bIsChecked);
    void on_updateServersButton_clicked();
    void onHttpReply(QNetworkReply* pReply);
    void onServerHandshakeComplete(QString sServer);
};
#endif // MAINWINDOW_H
