#ifndef SYSTEM_TRAY_MANAGER_H
#define SYSTEM_TRAY_MANAGER_H

#include "types.h"
#include <QObject>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QIcon>

class SystemTrayManager : public QObject {
    Q_OBJECT

public:
    explicit SystemTrayManager(QObject *parent = nullptr);

    void show();
    bool isVisible() const;
    void updateStatus(const StatusInfo &status);

signals:
    void toggleWindowRequested();
    void connectRequested(const QString &gatewayId);
    void disconnectRequested();
    void quitRequested();

public slots:
    void setLastGateway(const QString &name, const QString &id);

private slots:
    void onActivated(QSystemTrayIcon::ActivationReason reason);

private:
    void buildMenu();
    QIcon iconForState(ConnectionState state) const;
    void createIcons();

    QSystemTrayIcon *m_trayIcon;
    QMenu *m_menu;
    QAction *m_statusAction;
    QAction *m_quickConnectAction;
    QAction *m_disconnectAction;

    QString m_lastGatewayName;
    QString m_lastGatewayId;
    ConnectionState m_currentState = ConnectionState::Unknown;

    QIcon m_iconConnected;
    QIcon m_iconDisconnected;
    QIcon m_iconConnecting;
};

#endif // SYSTEM_TRAY_MANAGER_H
