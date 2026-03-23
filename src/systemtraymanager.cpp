#include "systemtraymanager.h"
#include <QApplication>
#include <QPainter>
#include <QPainterPath>

SystemTrayManager::SystemTrayManager(QObject *parent)
    : QObject(parent)
    , m_trayIcon(new QSystemTrayIcon(this))
    , m_menu(new QMenu)
    , m_statusAction(nullptr)
    , m_quickConnectAction(nullptr)
    , m_disconnectAction(nullptr)
{
    createIcons();
    buildMenu();
    m_trayIcon->setContextMenu(m_menu);
    m_trayIcon->setIcon(m_iconDisconnected);
    m_trayIcon->setToolTip(QStringLiteral("NordLayer - Disconnected"));

    connect(m_trayIcon, &QSystemTrayIcon::activated,
            this, &SystemTrayManager::onActivated);
}

void SystemTrayManager::show()
{
    m_trayIcon->show();
}

bool SystemTrayManager::isVisible() const
{
    return m_trayIcon->isVisible();
}

void SystemTrayManager::createIcons()
{
    auto makeIcon = [](const QColor &color) -> QIcon {
        QPixmap pm(22, 22);
        pm.fill(Qt::transparent);
        QPainter p(&pm);
        p.setRenderHint(QPainter::Antialiasing);
        p.setBrush(color);
        p.setPen(Qt::NoPen);
        p.drawEllipse(3, 3, 16, 16);

        // Shield outline
        p.setPen(QPen(Qt::white, 1.5));
        p.setBrush(Qt::NoBrush);
        QPainterPath shield;
        shield.moveTo(11, 5);
        shield.lineTo(7, 7);
        shield.lineTo(7, 12);
        shield.quadTo(7, 16, 11, 18);
        shield.quadTo(15, 16, 15, 12);
        shield.lineTo(15, 7);
        shield.closeSubpath();
        p.drawPath(shield);
        return QIcon(pm);
    };

    m_iconConnected = makeIcon(QColor(0x27, 0xAE, 0x60));    // green
    m_iconDisconnected = makeIcon(QColor(0x95, 0xA5, 0xA6)); // gray
    m_iconConnecting = makeIcon(QColor(0xF3, 0x9C, 0x12));   // amber
}

void SystemTrayManager::buildMenu()
{
    m_menu->clear();

    m_statusAction = m_menu->addAction(QStringLiteral("NordLayer: Unknown"));
    m_statusAction->setEnabled(false);

    m_menu->addSeparator();

    m_quickConnectAction = m_menu->addAction(QStringLiteral("Quick Connect"));
    m_quickConnectAction->setEnabled(false);
    connect(m_quickConnectAction, &QAction::triggered, this, [this]() {
        if (!m_lastGatewayId.isEmpty()) {
            emit connectRequested(m_lastGatewayId);
        }
    });

    m_disconnectAction = m_menu->addAction(QStringLiteral("Disconnect"));
    connect(m_disconnectAction, &QAction::triggered, this, &SystemTrayManager::disconnectRequested);

    m_menu->addSeparator();

    auto *showAction = m_menu->addAction(QStringLiteral("Show/Hide"));
    connect(showAction, &QAction::triggered, this, &SystemTrayManager::toggleWindowRequested);

    auto *quitAction = m_menu->addAction(QStringLiteral("Quit"));
    connect(quitAction, &QAction::triggered, this, &SystemTrayManager::quitRequested);
}

void SystemTrayManager::updateStatus(const StatusInfo &status)
{
    m_currentState = status.state;
    m_trayIcon->setIcon(iconForState(status.state));

    QString tooltip;
    switch (status.state) {
    case ConnectionState::Connected:
        tooltip = QStringLiteral("NordLayer - Connected");
        if (!status.gateway.isEmpty())
            tooltip += QStringLiteral(" to ") + status.gateway;
        m_statusAction->setText(tooltip);
        m_disconnectAction->setEnabled(true);
        break;
    case ConnectionState::Connecting:
        tooltip = QStringLiteral("NordLayer - Connecting...");
        m_statusAction->setText(tooltip);
        m_disconnectAction->setEnabled(false);
        break;
    case ConnectionState::Disconnected:
        tooltip = QStringLiteral("NordLayer - Disconnected");
        m_statusAction->setText(tooltip);
        m_disconnectAction->setEnabled(false);
        break;
    default:
        tooltip = QStringLiteral("NordLayer - Unknown");
        m_statusAction->setText(tooltip);
        m_disconnectAction->setEnabled(false);
        break;
    }
    m_trayIcon->setToolTip(tooltip);
}

void SystemTrayManager::setLastGateway(const QString &name, const QString &id)
{
    m_lastGatewayName = name;
    m_lastGatewayId = id;
    if (!name.isEmpty()) {
        m_quickConnectAction->setText(QStringLiteral("Connect to ") + name);
        m_quickConnectAction->setEnabled(m_currentState == ConnectionState::Disconnected);
    }
}

QIcon SystemTrayManager::iconForState(ConnectionState state) const
{
    switch (state) {
    case ConnectionState::Connected:
        return m_iconConnected;
    case ConnectionState::Connecting:
        return m_iconConnecting;
    default:
        return m_iconDisconnected;
    }
}

void SystemTrayManager::onActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger) {
        emit toggleWindowRequested();
    }
}
