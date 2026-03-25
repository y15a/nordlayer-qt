#include "nordlayerparser.h"
#include <QRegularExpression>

QString NordLayerParser::stripAnsi(const QString &raw)
{
    static const QRegularExpression ansiRe(
        QStringLiteral("\\x1B\\[[0-9;]*[a-zA-Z]|\\x1B\\[\\?[0-9]*[a-zA-Z]|\\r"));
    static const QRegularExpression spinnerRe(
        QStringLiteral("[\\x{2800}-\\x{28FF}]|\\[[-|/\\\\]\\]"));

    QString result = raw;
    result.remove(ansiRe);
    result.remove(spinnerRe);
    return result;
}

StatusInfo NordLayerParser::parseStatus(const QString &output)
{
    StatusInfo info;
    const QString clean = stripAnsi(output);
    const QStringList lines = clean.split('\n', Qt::SkipEmptyParts);

    // Helper: find key in line, return value after it (handles prepended junk)
    auto extractAfter = [](const QString &line, const QString &key) -> QString {
        int idx = line.indexOf(key);
        if (idx < 0) return QString();
        return line.mid(idx + key.length()).trimmed();
    };

    for (const QString &rawLine : lines) {
        const QString line = rawLine.trimmed();
        int idx;

        if ((idx = line.indexOf("Login:")) >= 0) {
            QString value = line.mid(idx + 6).trimmed();
            if (value.contains("Logged in")) {
                info.loggedIn = true;
                // Format: "Logged in [email org]"
                static const QRegularExpression loginRe(
                    QStringLiteral("Logged in \\[(.+?)\\s+(\\S+)\\]"));
                auto match = loginRe.match(value);
                if (match.hasMatch()) {
                    info.email = match.captured(1);
                    info.organization = match.captured(2);
                }
            } else if (value.contains("Not logged in")) {
                info.loggedIn = false;
            }
        } else if (line.contains("VPN:")) {
            QString value = extractAfter(line, "VPN:");
            if (value == "Connected") {
                info.state = ConnectionState::Connected;
            } else if (value == "Not Connected") {
                info.state = ConnectionState::Disconnected;
            } else if (value.startsWith("Connecting")) {
                info.state = ConnectionState::Connecting;
            } else {
                info.state = ConnectionState::Unknown;
            }
        } else if (line.contains("Current network:")) {
            info.network = extractAfter(line, "Current network:");
        } else if (line.contains("Gateway:")) {
            info.gateway = extractAfter(line, "Gateway:");
        } else if (line.contains("Server IP:")) {
            info.serverIp = extractAfter(line, "Server IP:");
        }
    }

    return info;
}

QList<Gateway> NordLayerParser::parseGateways(const QString &output)
{
    QList<Gateway> gateways;
    const QString clean = stripAnsi(output);
    const QStringList lines = clean.split('\n', Qt::SkipEmptyParts);

    bool inPrivate = false;
    bool inShared = false;
    bool headerSeen = false;

    for (const QString &rawLine : lines) {
        const QString line = rawLine.trimmed();

        if (line.contains("Private gateways:")) {
            inPrivate = true;
            inShared = false;
            headerSeen = false;
            continue;
        }
        if (line.contains("Shared gateways:")) {
            inPrivate = false;
            inShared = true;
            headerSeen = false;
            continue;
        }

        // Skip table header and separator lines
        if (line.contains("Name/Country") || line.contains("ID")) {
            headerSeen = true;
            continue;
        }
        if (line.startsWith(QChar(0x2500)) || line.isEmpty()) { // ─ box drawing
            continue;
        }

        if (!headerSeen)
            continue;
        if (!inPrivate && !inShared)
            continue;

        // Parse table row: "Name │ ID │"
        // Split on │ (U+2502)
        QStringList parts = line.split(QChar(0x2502), Qt::KeepEmptyParts);
        if (parts.size() >= 2) {
            Gateway gw;
            gw.name = parts[0].trimmed();
            gw.id = parts[1].trimmed();
            gw.isPrivate = inPrivate;
            if (!gw.name.isEmpty() && !gw.id.isEmpty()) {
                gateways.append(gw);
            }
        }
    }

    return gateways;
}

SettingsInfo NordLayerParser::parseSettings(const QString &output)
{
    SettingsInfo info;
    const QString clean = stripAnsi(output);
    const QStringList lines = clean.split('\n', Qt::SkipEmptyParts);

    for (const QString &rawLine : lines) {
        const QString line = rawLine.trimmed();
        int colonPos = line.indexOf(':');
        if (colonPos < 0)
            continue;

        const QString key = line.left(colonPos).trimmed();
        const QString value = line.mid(colonPos + 1).trimmed();

        if (key.startsWith("Preferred VPN Protocol") || key == "VPN Protocol") {
            info.vpnProtocol = value;
        } else if (key == "Auto-connect") {
            info.autoConnect = value;
        } else if (key == "Web Protection") {
            info.webProtection = value;
        } else if (key == "Always On") {
            info.alwaysOn = value;
        } else if (key == "Kill switch" || key == "Kill Switch") {
            info.killSwitch = value;
        } else if (key.startsWith("Local network access") || key == "LAN Access") {
            info.lanAccess = value;
        }
    }

    return info;
}

QList<LoginMethod> NordLayerParser::parseLoginMethods(const QString &output)
{
    QList<LoginMethod> methods;
    const QString clean = stripAnsi(output);
    const QStringList lines = clean.split('\n', Qt::SkipEmptyParts);

    static const QRegularExpression methodRe(
        QStringLiteral("^\\s*(\\d+)\\s*:\\s*(.+)$"));

    for (const QString &rawLine : lines) {
        auto match = methodRe.match(rawLine);
        if (match.hasMatch()) {
            LoginMethod m;
            m.number = match.captured(1).toInt();
            m.name = match.captured(2).trimmed();
            methods.append(m);
        }
    }

    return methods;
}
