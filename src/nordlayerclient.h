#ifndef NORDLAYER_CLIENT_H
#define NORDLAYER_CLIENT_H

#include "types.h"
#include <QObject>
#include <QProcess>
#include <QTimer>

class NordLayerClient : public QObject {
    Q_OBJECT

public:
    explicit NordLayerClient(QObject *parent = nullptr);

    bool isBusy() const;
    bool isLoginInProgress() const;
    void setCliPath(const QString &path);

public slots:
    void refreshStatus();
    void refreshGateways();
    void refreshSettings();
    void connectToGateway(const QString &gatewayId);
    void disconnectVpn();

    void login(const QString &organization);
    void selectLoginMethod(int methodNumber);
    void cancelLogin();
    void logout();

signals:
    void statusUpdated(const StatusInfo &status);
    void gatewaysLoaded(const QList<Gateway> &gateways);
    void settingsLoaded(const SettingsInfo &settings);
    void connectionStateChanged(ConnectionState state);
    void errorOccurred(const QString &message);
    void busyChanged(bool busy);

    void loginMethodsAvailable(const QList<LoginMethod> &methods);
    void loginWaitingForBrowser(const QString &url);
    void loginFinished(bool success);
    void logoutFinished();

private:
    enum class CommandType {
        Status,
        Gateways,
        Settings,
        Connect,
        Disconnect,
        Logout
    };

    struct PendingCommand {
        CommandType type;
        QProcess *process;
    };

    void runCommand(CommandType type, const QStringList &args);
    void handleFinished(PendingCommand cmd, int exitCode, QProcess::ExitStatus exitStatus);
    void onLoginOutput();
    void cleanupLogin();

    QString m_cliPath;
    QList<PendingCommand> m_pending;
    bool m_connectInFlight = false;

    // Login-specific state
    QProcess *m_loginProcess = nullptr;
    QTimer *m_loginTimer = nullptr;
    QString m_loginBuffer;
    bool m_loginInFlight = false;
    bool m_loginMethodsSent = false;

    static constexpr int COMMAND_TIMEOUT_MS = 30000;
    static constexpr int LOGIN_TIMEOUT_MS = 300000; // 5 minutes for browser auth
};

#endif // NORDLAYER_CLIENT_H
