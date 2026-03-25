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

bool NordLayerClient::isLoginInProgress() const
{
    return m_loginInFlight;
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

// --- Login flow (interactive, multi-step stdin) ---

void NordLayerClient::login(const QString &organization)
{
    if (m_loginInFlight) {
        emit errorOccurred(tr("Login is already in progress."));
        return;
    }
    if (m_connectInFlight) {
        emit errorOccurred(tr("A connection operation is already in progress."));
        return;
    }

    m_loginInFlight = true;
    m_loginMethodsSent = false;
    m_loginBuffer.clear();
    emit busyChanged(true);

    m_loginProcess = new QProcess(this);

    // Timeout for the entire login flow
    m_loginTimer = new QTimer(this);
    m_loginTimer->setSingleShot(true);
    connect(m_loginTimer, &QTimer::timeout, this, [this]() {
        emit errorOccurred(tr("Login timed out after %1 seconds.").arg(LOGIN_TIMEOUT_MS / 1000));
        cancelLogin();
    });

    connect(m_loginProcess, &QProcess::readyReadStandardOutput,
            this, &NordLayerClient::onLoginOutput);

    connect(m_loginProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int exitCode, QProcess::ExitStatus exitStatus) {
                bool success = (exitStatus == QProcess::NormalExit && exitCode == 0);
                cleanupLogin();
                emit loginFinished(success);
                if (!success) {
                    QString errMsg = NordLayerParser::stripAnsi(m_loginBuffer).trimmed();
                    if (!errMsg.isEmpty()) {
                        emit errorOccurred(tr("Login failed: %1").arg(errMsg));
                    } else {
                        emit errorOccurred(tr("Login failed (exit code %1).").arg(exitCode));
                    }
                }
                refreshStatus();
                refreshGateways();
                refreshSettings();
            });

    connect(m_loginProcess, &QProcess::errorOccurred,
            this, [this](QProcess::ProcessError error) {
                Q_UNUSED(error);
                QString msg;
                if (m_loginProcess && m_loginProcess->error() == QProcess::FailedToStart) {
                    msg = tr("Failed to start nordlayer CLI. Is it installed at %1?").arg(m_cliPath);
                } else {
                    msg = tr("Login process error: %1").arg(
                        m_loginProcess ? m_loginProcess->errorString() : QStringLiteral("unknown"));
                }
                emit errorOccurred(msg);
                cleanupLogin();
                emit loginFinished(false);
            });

    m_loginProcess->start(m_cliPath, {QStringLiteral("login")});
    m_loginTimer->start(LOGIN_TIMEOUT_MS);

    // Write organization name after process starts
    // The CLI prints "Organization: " prompt and waits for stdin
    QByteArray orgData = organization.toUtf8() + '\n';
    m_loginProcess->write(orgData);
}

void NordLayerClient::selectLoginMethod(int methodNumber)
{
    if (!m_loginProcess || !m_loginInFlight) {
        emit errorOccurred(tr("No login in progress."));
        return;
    }

    QByteArray data = QByteArray::number(methodNumber) + '\n';
    m_loginProcess->write(data);
    m_loginMethodsSent = true;
}

void NordLayerClient::cancelLogin()
{
    if (m_loginProcess) {
        m_loginProcess->kill();
        // cleanupLogin will be called from the finished signal
    }
}

void NordLayerClient::onLoginOutput()
{
    if (!m_loginProcess)
        return;

    QString newData = QString::fromUtf8(m_loginProcess->readAllStandardOutput());
    m_loginBuffer.append(newData);

    QString clean = NordLayerParser::stripAnsi(m_loginBuffer);

    // Check for login method selection prompt
    if (!m_loginMethodsSent && clean.contains(QStringLiteral("Select"))) {
        auto methods = NordLayerParser::parseLoginMethods(clean);
        if (!methods.isEmpty()) {
            emit loginMethodsAvailable(methods);
        }
    }

    // Check for browser URL after method selection
    if (m_loginMethodsSent) {
        const QStringList lines = clean.split('\n', Qt::SkipEmptyParts);
        for (const QString &line : lines) {
            QString trimmed = line.trimmed();
            if (trimmed.startsWith(QStringLiteral("http://")) ||
                trimmed.startsWith(QStringLiteral("https://"))) {
                emit loginWaitingForBrowser(trimmed);
                break;
            }
        }
    }
}

void NordLayerClient::cleanupLogin()
{
    if (m_loginTimer) {
        m_loginTimer->stop();
        m_loginTimer->deleteLater();
        m_loginTimer = nullptr;
    }
    if (m_loginProcess) {
        m_loginProcess->deleteLater();
        m_loginProcess = nullptr;
    }
    m_loginInFlight = false;
    m_loginBuffer.clear();
    emit busyChanged(false);
}

// --- Logout ---

void NordLayerClient::logout()
{
    if (m_connectInFlight || m_loginInFlight) {
        emit errorOccurred(tr("Another operation is already in progress."));
        return;
    }
    m_connectInFlight = true;
    emit busyChanged(true);
    runCommand(CommandType::Logout, {QStringLiteral("logout")});
}

// --- Standard command infrastructure ---

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
                if (cmd.type == CommandType::Connect || cmd.type == CommandType::Disconnect
                    || cmd.type == CommandType::Logout) {
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
    case CommandType::Logout:
        m_connectInFlight = false;
        emit busyChanged(false);
        emit logoutFinished();
        refreshStatus();
        refreshGateways();
        refreshSettings();
        break;
    }
}
