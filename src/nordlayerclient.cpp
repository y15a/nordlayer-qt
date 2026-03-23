#include "nordlayerclient.h"
#include "nordlayerparser.h"
#include <QStandardPaths>

NordLayerClient::NordLayerClient(QObject *parent)
    : QObject(parent)
    , m_cliPath(QStringLiteral("/usr/bin/nordlayer"))
{
}

bool NordLayerClient::isBusy() const
{
    return m_connectInFlight;
}

void NordLayerClient::setCliPath(const QString &path)
{
    m_cliPath = path;
}

void NordLayerClient::refreshStatus()
{
    runCommand(CommandType::Status, {QStringLiteral("status")});
}

void NordLayerClient::refreshGateways()
{
    runCommand(CommandType::Gateways, {QStringLiteral("gateways")});
}

void NordLayerClient::refreshSettings()
{
    runCommand(CommandType::Settings, {QStringLiteral("settings"), QStringLiteral("get")});
}

void NordLayerClient::connectToGateway(const QString &gatewayId)
{
    if (m_connectInFlight) {
        emit errorOccurred(tr("A connection operation is already in progress."));
        return;
    }
    m_connectInFlight = true;
    emit busyChanged(true);
    emit connectionStateChanged(ConnectionState::Connecting);
    runCommand(CommandType::Connect, {QStringLiteral("connect"), gatewayId});
}

void NordLayerClient::disconnectVpn()
{
    if (m_connectInFlight) {
        emit errorOccurred(tr("A connection operation is already in progress."));
        return;
    }
    m_connectInFlight = true;
    emit busyChanged(true);
    runCommand(CommandType::Disconnect, {QStringLiteral("disconnect")});
}

void NordLayerClient::runCommand(CommandType type, const QStringList &args)
{
    auto *process = new QProcess(this);
    PendingCommand cmd{type, process};

    // Kill on timeout
    auto *timer = new QTimer(process);
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, process, [process, this]() {
        process->kill();
        emit errorOccurred(tr("Command timed out after %1 seconds.").arg(COMMAND_TIMEOUT_MS / 1000));
    });

    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, cmd, timer](int exitCode, QProcess::ExitStatus exitStatus) {
                timer->stop();
                handleFinished(cmd, exitCode, exitStatus);
                cmd.process->deleteLater();
            });

    connect(process, &QProcess::errorOccurred,
            this, [this, cmd](QProcess::ProcessError error) {
                Q_UNUSED(error);
                QString msg;
                if (cmd.process->error() == QProcess::FailedToStart) {
                    msg = tr("Failed to start nordlayer CLI. Is it installed at %1?").arg(m_cliPath);
                } else {
                    msg = tr("CLI process error: %1").arg(cmd.process->errorString());
                }
                emit errorOccurred(msg);
                if (cmd.type == CommandType::Connect || cmd.type == CommandType::Disconnect) {
                    m_connectInFlight = false;
                    emit busyChanged(false);
                }
                cmd.process->deleteLater();
            });

    process->start(m_cliPath, args);
    timer->start(COMMAND_TIMEOUT_MS);
}

void NordLayerClient::handleFinished(PendingCommand cmd, int exitCode, QProcess::ExitStatus exitStatus)
{
    const QString output = QString::fromUtf8(cmd.process->readAllStandardOutput());
    const QString errOutput = QString::fromUtf8(cmd.process->readAllStandardError());

    if (exitStatus == QProcess::CrashExit || exitCode != 0) {
        QString errMsg = NordLayerParser::stripAnsi(errOutput.isEmpty() ? output : errOutput).trimmed();
        if (errMsg.isEmpty()) {
            errMsg = tr("Command exited with code %1").arg(exitCode);
        }
        emit errorOccurred(errMsg);
    }

    switch (cmd.type) {
    case CommandType::Status: {
        auto status = NordLayerParser::parseStatus(output);
        emit statusUpdated(status);
        emit connectionStateChanged(status.state);
        break;
    }
    case CommandType::Gateways: {
        auto gateways = NordLayerParser::parseGateways(output);
        emit gatewaysLoaded(gateways);
        break;
    }
    case CommandType::Settings: {
        auto settings = NordLayerParser::parseSettings(output);
        emit settingsLoaded(settings);
        break;
    }
    case CommandType::Connect:
        m_connectInFlight = false;
        emit busyChanged(false);
        // Refresh status to get the actual connection result
        refreshStatus();
        break;
    case CommandType::Disconnect:
        m_connectInFlight = false;
        emit busyChanged(false);
        refreshStatus();
        break;
    }
}
