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
    void setCliPath(const QString &path);

public slots:
    void refreshStatus();
    void refreshGateways();
    void refreshSettings();
    void connectToGateway(const QString &gatewayId);
    void disconnectVpn();

signals:
    void statusUpdated(const StatusInfo &status);
    void gatewaysLoaded(const QList<Gateway> &gateways);
    void settingsLoaded(const SettingsInfo &settings);
    void connectionStateChanged(ConnectionState state);
    void errorOccurred(const QString &message);
    void busyChanged(bool busy);

private:
    enum class CommandType {
        Status,
        Gateways,
        Settings,
        Connect,
        Disconnect
    };

    struct PendingCommand {
        CommandType type;
        QProcess *process;
    };

    void runCommand(CommandType type, const QStringList &args);
    void handleFinished(PendingCommand cmd, int exitCode, QProcess::ExitStatus exitStatus);

    QString m_cliPath;
    QList<PendingCommand> m_pending;
    bool m_connectInFlight = false;

    static constexpr int COMMAND_TIMEOUT_MS = 30000;
};

#endif // NORDLAYER_CLIENT_H
