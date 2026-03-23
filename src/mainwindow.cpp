#include "mainwindow.h"

#include <QApplication>
#include <QCloseEvent>
#include <QGroupBox>
#include <QTime>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QTabWidget>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_client(new NordLayerClient(this))
    , m_tray(new SystemTrayManager(this))
    , m_refreshTimer(new QTimer(this))
{
    setupUi();

    // Client signals
    connect(m_client, &NordLayerClient::statusUpdated, this, &MainWindow::onStatusUpdated);
    connect(m_client, &NordLayerClient::gatewaysLoaded, this, &MainWindow::onGatewaysLoaded);
    connect(m_client, &NordLayerClient::settingsLoaded, this, &MainWindow::onSettingsLoaded);
    connect(m_client, &NordLayerClient::connectionStateChanged, this, &MainWindow::onConnectionStateChanged);
    connect(m_client, &NordLayerClient::errorOccurred, this, &MainWindow::onError);
    connect(m_client, &NordLayerClient::busyChanged, this, &MainWindow::onBusyChanged);

    // Gateway selection changes → update Connect button
    connect(m_gatewayTree, &QTreeWidget::currentItemChanged, this,
            [this](QTreeWidgetItem *, QTreeWidgetItem *) {
                updateConnectButtonState();
            });

    // Tray signals
    connect(m_tray, &SystemTrayManager::toggleWindowRequested, this, [this]() {
        setVisible(!isVisible());
        if (isVisible()) {
            raise();
            activateWindow();
        }
    });
    connect(m_tray, &SystemTrayManager::connectRequested, m_client, &NordLayerClient::connectToGateway);
    connect(m_tray, &SystemTrayManager::disconnectRequested, m_client, &NordLayerClient::disconnectVpn);
    connect(m_tray, &SystemTrayManager::quitRequested, qApp, &QApplication::quit);

    // Refresh timer
    connect(m_refreshTimer, &QTimer::timeout, m_client, &NordLayerClient::refreshStatus);
    m_refreshTimer->start(5000);

    // Initial data load
    m_client->refreshStatus();
    m_client->refreshGateways();
    m_client->refreshSettings();

    m_tray->show();
}

void MainWindow::setupUi()
{
    setWindowTitle(QStringLiteral("NordLayer"));
    setMinimumSize(420, 480);
    resize(420, 540);

    auto *central = new QWidget;
    auto *mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(6);

    // Status panel at top
    mainLayout->addWidget(createStatusPanel());

    // Tabs for gateways and settings
    auto *tabs = new QTabWidget;
    tabs->addTab(createGatewayPanel(), QStringLiteral("Gateways"));
    tabs->addTab(createSettingsPanel(), QStringLiteral("Settings"));
    tabs->addTab(createAboutPanel(), QStringLiteral("About"));
    mainLayout->addWidget(tabs, 1);

    setCentralWidget(central);

    // Status bar for errors
    statusBar()->showMessage(QStringLiteral("Starting..."));
}

QWidget *MainWindow::createStatusPanel()
{
    auto *group = new QGroupBox;
    auto *layout = new QVBoxLayout(group);
    layout->setSpacing(2);

    // Top row: small status dot + connection label
    auto *statusRow = new QHBoxLayout;
    statusRow->setSpacing(6);

    m_connectionIcon = new QLabel;
    m_connectionIcon->setFixedSize(12, 12);
    m_connectionIcon->setStyleSheet(
        QStringLiteral("QLabel { background-color: #95a5a6; border-radius: 6px; }"));
    statusRow->addWidget(m_connectionIcon, 0, Qt::AlignVCenter);

    m_connectionLabel = new QLabel(QStringLiteral("Unknown"));
    auto font = m_connectionLabel->font();
    font.setPointSize(font.pointSize() + 2);
    font.setBold(true);
    m_connectionLabel->setFont(font);
    statusRow->addWidget(m_connectionLabel);
    statusRow->addStretch();

    layout->addLayout(statusRow);

    m_userLabel = new QLabel;
    layout->addWidget(m_userLabel);

    m_networkLabel = new QLabel;
    layout->addWidget(m_networkLabel);

    m_gatewayLabel = new QLabel;
    layout->addWidget(m_gatewayLabel);

    // Disconnect button row — right-aligned, hidden when disconnected
    auto *btnRow = new QHBoxLayout;
    btnRow->addStretch();

    m_disconnectBtn = new QPushButton(QStringLiteral("Disconnect"));
    m_disconnectBtn->setVisible(false);
    connect(m_disconnectBtn, &QPushButton::clicked, this, &MainWindow::onDisconnectClicked);
    btnRow->addWidget(m_disconnectBtn);

    layout->addLayout(btnRow);

    return group;
}

QWidget *MainWindow::createGatewayPanel()
{
    auto *widget = new QWidget;
    auto *layout = new QVBoxLayout(widget);
    layout->setContentsMargins(0, 4, 0, 0);

    m_filterEdit = new QLineEdit;
    m_filterEdit->setPlaceholderText(QStringLiteral("Filter gateways..."));
    m_filterEdit->setClearButtonEnabled(true);
    connect(m_filterEdit, &QLineEdit::textChanged, this, &MainWindow::onFilterChanged);
    layout->addWidget(m_filterEdit);

    m_gatewayTree = new QTreeWidget;
    m_gatewayTree->setHeaderLabels({QStringLiteral("Name"), QStringLiteral("ID")});
    m_gatewayTree->header()->setStretchLastSection(true);
    m_gatewayTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_gatewayTree->setRootIsDecorated(true);
    m_gatewayTree->setAlternatingRowColors(true);
    connect(m_gatewayTree, &QTreeWidget::itemDoubleClicked, this, &MainWindow::onGatewayDoubleClicked);
    layout->addWidget(m_gatewayTree, 1);

    auto *btnLayout = new QHBoxLayout;
    btnLayout->addStretch();

    m_connectBtn = new QPushButton(QStringLiteral("Connect"));
    m_connectBtn->setEnabled(false);
    connect(m_connectBtn, &QPushButton::clicked, this, &MainWindow::onConnectClicked);
    btnLayout->addWidget(m_connectBtn);

    layout->addLayout(btnLayout);
    return widget;
}

QWidget *MainWindow::createSettingsPanel()
{
    auto *widget = new QWidget;
    auto *form = new QFormLayout(widget);
    form->setContentsMargins(8, 8, 8, 8);

    m_settVpnProtocol = new QLabel(QStringLiteral("-"));
    m_settAutoConnect = new QLabel(QStringLiteral("-"));
    m_settWebProtection = new QLabel(QStringLiteral("-"));
    m_settAlwaysOn = new QLabel(QStringLiteral("-"));
    m_settKillSwitch = new QLabel(QStringLiteral("-"));
    m_settLanAccess = new QLabel(QStringLiteral("-"));

    form->addRow(QStringLiteral("VPN Protocol:"), m_settVpnProtocol);
    form->addRow(QStringLiteral("Auto-connect:"), m_settAutoConnect);
    form->addRow(QStringLiteral("Web Protection:"), m_settWebProtection);
    form->addRow(QStringLiteral("Always On:"), m_settAlwaysOn);
    form->addRow(QStringLiteral("Kill Switch:"), m_settKillSwitch);
    form->addRow(QStringLiteral("LAN Access:"), m_settLanAccess);

    return widget;
}

QWidget *MainWindow::createAboutPanel()
{
    auto *widget = new QWidget;
    auto *layout = new QVBoxLayout(widget);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(8);

    auto *title = new QLabel(QStringLiteral("nordlayer-qt"));
    auto titleFont = title->font();
    titleFont.setPointSize(titleFont.pointSize() + 4);
    titleFont.setBold(true);
    title->setFont(titleFont);
    layout->addWidget(title);

    auto *version = new QLabel(QStringLiteral("Version %1").arg(qApp->applicationVersion()));
    layout->addWidget(version);

    layout->addSpacing(4);

    auto *description = new QLabel(
        QStringLiteral("Lightweight Qt6 system-tray GUI for the NordLayer CLI."));
    description->setWordWrap(true);
    layout->addWidget(description);

    auto *disclaimer = new QLabel(
        QStringLiteral("This is an unofficial, community project. It is not affiliated "
                       "with, endorsed by, or supported by Nord Security."));
    disclaimer->setWordWrap(true);
    auto disclaimerFont = disclaimer->font();
    disclaimerFont.setItalic(true);
    disclaimer->setFont(disclaimerFont);
    layout->addWidget(disclaimer);

    layout->addSpacing(8);

    auto *repoLink = new QLabel(
        QStringLiteral("<a href=\"https://github.com/y15a/nordlayer-qt\">"
                       "github.com/y15a/nordlayer-qt</a>"));
    repoLink->setOpenExternalLinks(true);
    layout->addWidget(repoLink);

    auto *licenseLabel = new QLabel(QStringLiteral("Licensed under the MIT License."));
    layout->addWidget(licenseLabel);

    layout->addStretch();
    return widget;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_tray->isVisible()) {
        hide();
        event->ignore();
    } else {
        event->accept();
    }
}

void MainWindow::onStatusUpdated(const StatusInfo &status)
{
    m_currentState = status.state;

    if (!status.email.isEmpty()) {
        m_userLabel->setText(QStringLiteral("%1 [%2]").arg(status.email, status.organization));
    }
    if (!status.network.isEmpty()) {
        m_networkLabel->setText(status.network);
    }

    // Resolve connected gateway ID from gateway name
    QString previousGatewayId = m_connectedGatewayId;
    if (status.state == ConnectionState::Connected && !status.gateway.isEmpty()) {
        // Find ID by matching name in tree
        m_connectedGatewayId.clear();
        for (int i = 0; i < m_gatewayTree->topLevelItemCount(); ++i) {
            auto *group = m_gatewayTree->topLevelItem(i);
            for (int j = 0; j < group->childCount(); ++j) {
                auto *child = group->child(j);
                if (child->data(0, Qt::UserRole + 1).toString() == status.gateway
                    || child->text(0) == status.gateway) {
                    m_connectedGatewayId = child->data(0, Qt::UserRole).toString();
                    break;
                }
            }
            if (!m_connectedGatewayId.isEmpty())
                break;
        }
    } else {
        m_connectedGatewayId.clear();
    }

    // Connection state display
    switch (status.state) {
    case ConnectionState::Connected:
        m_connectionLabel->setText(QStringLiteral("Connected"));
        m_connectionIcon->setStyleSheet(
            QStringLiteral("QLabel { background-color: #27ae60; border-radius: 6px; }"));
        if (!status.gateway.isEmpty()) {
            m_gatewayLabel->setText(status.gateway);
            m_gatewayLabel->setVisible(true);
        }
        m_disconnectBtn->setVisible(true);
        m_disconnectBtn->setEnabled(true);
        break;
    case ConnectionState::Connecting:
        m_connectionLabel->setText(QStringLiteral("Connecting..."));
        m_connectionIcon->setStyleSheet(
            QStringLiteral("QLabel { background-color: #f39c12; border-radius: 6px; }"));
        m_gatewayLabel->setVisible(false);
        m_disconnectBtn->setVisible(true);
        m_disconnectBtn->setEnabled(false);
        break;
    case ConnectionState::Disconnected:
        m_connectionLabel->setText(QStringLiteral("Disconnected"));
        m_connectionIcon->setStyleSheet(
            QStringLiteral("QLabel { background-color: #95a5a6; border-radius: 6px; }"));
        m_gatewayLabel->setVisible(false);
        m_disconnectBtn->setVisible(false);
        break;
    default:
        m_connectionLabel->setText(QStringLiteral("Unknown"));
        m_connectionIcon->setStyleSheet(
            QStringLiteral("QLabel { background-color: #95a5a6; border-radius: 6px; }"));
        m_gatewayLabel->setVisible(false);
        m_disconnectBtn->setVisible(false);
        break;
    }

    // Update gateway tree to mark the connected gateway
    if (m_connectedGatewayId != previousGatewayId) {
        markConnectedGateway();
    }

    updateConnectButtonState();
    m_tray->updateStatus(status);
    statusBar()->showMessage(
        QStringLiteral("Last updated: %1").arg(QTime::currentTime().toString(QStringLiteral("HH:mm:ss"))));
}

void MainWindow::onGatewaysLoaded(const QList<Gateway> &gateways)
{
    m_gatewayTree->clear();

    auto *privateRoot = new QTreeWidgetItem(m_gatewayTree, {QStringLiteral("Private Gateways")});
    auto *sharedRoot = new QTreeWidgetItem(m_gatewayTree, {QStringLiteral("Shared Gateways")});

    auto boldFont = privateRoot->font(0);
    boldFont.setBold(true);
    privateRoot->setFont(0, boldFont);
    sharedRoot->setFont(0, boldFont);

    // Make group headers non-selectable
    privateRoot->setFlags(privateRoot->flags() & ~Qt::ItemIsSelectable);
    sharedRoot->setFlags(sharedRoot->flags() & ~Qt::ItemIsSelectable);

    for (const auto &gw : gateways) {
        auto *parent = gw.isPrivate ? privateRoot : sharedRoot;
        auto *item = new QTreeWidgetItem(parent, {gw.name, gw.id});
        item->setData(0, Qt::UserRole, gw.id);
        item->setData(0, Qt::UserRole + 1, gw.name);
    }

    privateRoot->setExpanded(true);
    sharedRoot->setExpanded(true);

    // Hide empty groups
    privateRoot->setHidden(privateRoot->childCount() == 0);

    markConnectedGateway();
    updateConnectButtonState();

    statusBar()->showMessage(
        QStringLiteral("Loaded %1 gateways").arg(gateways.size()), 3000);
}

void MainWindow::onSettingsLoaded(const SettingsInfo &settings)
{
    m_settVpnProtocol->setText(settings.vpnProtocol);
    m_settAutoConnect->setText(settings.autoConnect);
    m_settWebProtection->setText(settings.webProtection);
    m_settAlwaysOn->setText(settings.alwaysOn);
    m_settKillSwitch->setText(settings.killSwitch);
    m_settLanAccess->setText(settings.lanAccess);
}

void MainWindow::onConnectionStateChanged(ConnectionState state)
{
    Q_UNUSED(state);
    // Button states are handled in onStatusUpdated
}

void MainWindow::onError(const QString &message)
{
    statusBar()->showMessage(QStringLiteral("Error: %1").arg(message), 10000);
}

void MainWindow::onBusyChanged(bool busy)
{
    updateConnectButtonState();
    m_disconnectBtn->setEnabled(!busy && m_currentState == ConnectionState::Connected);
    if (busy) {
        statusBar()->showMessage(QStringLiteral("Operation in progress..."));
    }
}

void MainWindow::onConnectClicked()
{
    auto *item = m_gatewayTree->currentItem();
    if (!item)
        return;

    QString id = item->data(0, Qt::UserRole).toString();
    QString name = item->data(0, Qt::UserRole + 1).toString();
    if (id.isEmpty())
        return;

    m_lastConnectedGatewayId = id;
    m_lastConnectedGatewayName = name;
    m_tray->setLastGateway(name, id);
    m_client->connectToGateway(id);
}

void MainWindow::onDisconnectClicked()
{
    m_client->disconnectVpn();
}

void MainWindow::onFilterChanged(const QString &text)
{
    applyFilter(text);
}

void MainWindow::onGatewayDoubleClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);
    if (!item || item->data(0, Qt::UserRole).toString().isEmpty())
        return;

    // Double-click to connect
    QString id = item->data(0, Qt::UserRole).toString();
    QString name = item->data(0, Qt::UserRole + 1).toString();
    m_lastConnectedGatewayId = id;
    m_lastConnectedGatewayName = name;
    m_tray->setLastGateway(name, id);
    m_client->connectToGateway(id);
}

void MainWindow::updateConnectButtonState()
{
    if (m_client->isBusy()) {
        m_connectBtn->setEnabled(false);
        return;
    }

    auto *item = m_gatewayTree->currentItem();
    if (!item) {
        m_connectBtn->setEnabled(false);
        return;
    }

    QString selectedId = item->data(0, Qt::UserRole).toString();
    if (selectedId.isEmpty()) {
        // Group header selected
        m_connectBtn->setEnabled(false);
        return;
    }

    // When connected, only enable if selected gateway differs from current
    if (m_currentState == ConnectionState::Connected && selectedId == m_connectedGatewayId) {
        m_connectBtn->setEnabled(false);
        return;
    }

    m_connectBtn->setEnabled(true);
}

void MainWindow::markConnectedGateway()
{
    for (int i = 0; i < m_gatewayTree->topLevelItemCount(); ++i) {
        auto *group = m_gatewayTree->topLevelItem(i);
        for (int j = 0; j < group->childCount(); ++j) {
            auto *child = group->child(j);
            QString id = child->data(0, Qt::UserRole).toString();
            bool isCurrent = (!m_connectedGatewayId.isEmpty() && id == m_connectedGatewayId);

            auto font = child->font(0);
            font.setBold(isCurrent);
            child->setFont(0, font);
            child->setFont(1, font);

            // Show "(connected)" suffix on name column
            QString baseName = child->data(0, Qt::UserRole + 1).toString();
            child->setText(0, isCurrent ? baseName + QStringLiteral(" (connected)") : baseName);
        }
    }
}

void MainWindow::applyFilter(const QString &text)
{
    for (int i = 0; i < m_gatewayTree->topLevelItemCount(); ++i) {
        auto *group = m_gatewayTree->topLevelItem(i);
        int visibleChildren = 0;
        for (int j = 0; j < group->childCount(); ++j) {
            auto *child = group->child(j);
            bool matches = text.isEmpty()
                           || child->text(0).contains(text, Qt::CaseInsensitive)
                           || child->text(1).contains(text, Qt::CaseInsensitive);
            child->setHidden(!matches);
            if (matches)
                ++visibleChildren;
        }
        group->setHidden(visibleChildren == 0 && !text.isEmpty());
    }
}
