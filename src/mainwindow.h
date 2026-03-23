#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "types.h"
#include "nordlayerclient.h"
#include "systemtraymanager.h"

#include <QMainWindow>
#include <QTreeWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QFormLayout>
#include <QTimer>
#include <QStatusBar>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onStatusUpdated(const StatusInfo &status);
    void onGatewaysLoaded(const QList<Gateway> &gateways);
    void onSettingsLoaded(const SettingsInfo &settings);
    void onConnectionStateChanged(ConnectionState state);
    void onError(const QString &message);
    void onBusyChanged(bool busy);

    void onConnectClicked();
    void onDisconnectClicked();
    void onFilterChanged(const QString &text);
    void onGatewayDoubleClicked(QTreeWidgetItem *item, int column);

private:
    void setupUi();
    QWidget *createStatusPanel();
    QWidget *createGatewayPanel();
    QWidget *createSettingsPanel();
    QWidget *createAboutPanel();
    void applyFilter(const QString &text);
    void updateConnectButtonState();
    void markConnectedGateway();

    NordLayerClient *m_client;
    SystemTrayManager *m_tray;
    QTimer *m_refreshTimer;

    // Status panel
    QLabel *m_connectionIcon;
    QLabel *m_connectionLabel;
    QLabel *m_userLabel;
    QLabel *m_networkLabel;
    QLabel *m_gatewayLabel;
    QPushButton *m_disconnectBtn;

    // Gateway panel
    QTreeWidget *m_gatewayTree;
    QLineEdit *m_filterEdit;
    QPushButton *m_connectBtn;

    // Settings panel
    QLabel *m_settVpnProtocol;
    QLabel *m_settAutoConnect;
    QLabel *m_settWebProtection;
    QLabel *m_settAlwaysOn;
    QLabel *m_settKillSwitch;
    QLabel *m_settLanAccess;

    // Connection tracking
    QString m_connectedGatewayId;
    QString m_lastConnectedGatewayId;
    QString m_lastConnectedGatewayName;
    ConnectionState m_currentState = ConnectionState::Unknown;
};

#endif // MAINWINDOW_H
