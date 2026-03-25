#ifndef NORDLAYER_TYPES_H
#define NORDLAYER_TYPES_H

#include <QString>
#include <QList>
#include <QMetaType>

enum class ConnectionState {
    Disconnected,
    Connected,
    Connecting,
    Unknown
};

struct StatusInfo {
    ConnectionState state = ConnectionState::Unknown;
    bool loggedIn = false;
    QString email;
    QString organization;
    QString network;
    QString gateway;
    QString serverIp;
};

struct LoginMethod {
    int number = 0;
    QString name;
};

struct Gateway {
    QString name;
    QString id;
    bool isPrivate = false;
};

struct SettingsInfo {
    QString vpnProtocol;
    QString autoConnect;
    QString webProtection;
    QString alwaysOn;
    QString killSwitch;
    QString lanAccess;
};

Q_DECLARE_METATYPE(StatusInfo)
Q_DECLARE_METATYPE(LoginMethod)
Q_DECLARE_METATYPE(Gateway)
Q_DECLARE_METATYPE(SettingsInfo)

#endif // NORDLAYER_TYPES_H
