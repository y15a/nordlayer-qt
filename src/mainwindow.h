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
#include <QComboBox>
#include <QFormLayout>
#include <QStackedWidget>
#include <QTabWidget>
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

    void onLoginClicked();
    void onLoginMethodsAvailable(const QList<LoginMethod> &methods);
    void onMethodSelectClicked();
    void onLoginWaitingForBrowser(const QString &url);
    void onLoginFinished(bool success);
    void onLogoutClicked();

private:
    void setupUi();
    QWidget *createStatusPanel();
    QWidget *createLoginPage();
    QWidget *createStatusPage();
    QWidget *createGatewayPanel();
    QWidget *createSettingsPanel();
    QWidget *createAboutPanel();
    void applyFilter(const QString &text);
    void updateConnectButtonState();
    void markConnectedGateway();
    void setLoggedIn(bool loggedIn);
    void resetLoginForm();

    NordLayerClient *m_client;
    SystemTrayManager *m_tray;
    QTimer *m_refreshTimer;
    QTabWidget *m_tabs;

    // Status panel (stacked: login form / normal status)
    QStackedWidget *m_statusStack;

    // Login page (page 0)
    QLineEdit *m_orgEdit;
    QPushButton *m_loginBtn;
    QLabel *m_loginStatusLabel;
    QComboBox *m_methodCombo;
    QPushButton *m_methodSelectBtn;
    QPushButton *m_cancelLoginBtn;

    // Status page (page 1)
    QLabel *m_connectionIcon;
    QLabel *m_connectionLabel;
    QLabel *m_userLabel;
    QLabel *m_networkLabel;
    QLabel *m_gatewayLabel;
    QPushButton *m_disconnectBtn;
    QPushButton *m_logoutBtn;

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
    bool m_isLoggedIn = false;
};

#endif // MAINWINDOW_H
