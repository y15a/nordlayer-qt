#include <QtTest>
#include "nordlayerparser.h"

class TestParser : public QObject {
    Q_OBJECT

private slots:
    void testStripAnsi();
    void testStripAnsi_noAnsi();
    void testParseStatus_connected();
    void testParseStatus_disconnected();
    void testParseGateways();
    void testParseGateways_empty();
    void testParseSettings();
    void testParseSettings_partial();
};

void TestParser::testStripAnsi()
{
    QString input = QStringLiteral("\x1B[2K\x1B[1m[-]\x1B[0m Getting status...\x1B[2K");
    QString result = NordLayerParser::stripAnsi(input);
    QVERIFY(!result.contains(QStringLiteral("\x1B[")));
}

void TestParser::testStripAnsi_noAnsi()
{
    QString input = QStringLiteral("plain text here");
    QCOMPARE(NordLayerParser::stripAnsi(input), input);
}

void TestParser::testParseStatus_connected()
{
    QString output = QStringLiteral(
        "Login: Logged in [user@example.com myorg]\n"
        "VPN: Connected\n"
        "Current network: office-wifi\n"
        "Gateway: Japan\n"
        "Server IP: 1.2.3.4\n");

    auto status = NordLayerParser::parseStatus(output);
    QCOMPARE(status.state, ConnectionState::Connected);
    QCOMPARE(status.email, QStringLiteral("user@example.com"));
    QCOMPARE(status.organization, QStringLiteral("myorg"));
    QCOMPARE(status.network, QStringLiteral("office-wifi"));
    QCOMPARE(status.gateway, QStringLiteral("Japan"));
    QCOMPARE(status.serverIp, QStringLiteral("1.2.3.4"));
}

void TestParser::testParseStatus_disconnected()
{
    // Simulate real CLI output with ANSI (each status line on its own line)
    QString output = QStringLiteral(
        "\x1B[2K[-] Getting status...\r\n"
        "\x1B[2KLogin: Logged in [alice@example.com acme-corp]\n"
        "VPN: Not Connected\n"
        "Current network: home-wifi-5g\n");

    auto status = NordLayerParser::parseStatus(output);
    QCOMPARE(status.state, ConnectionState::Disconnected);
    QCOMPARE(status.email, QStringLiteral("alice@example.com"));
    QCOMPARE(status.organization, QStringLiteral("acme-corp"));
    QCOMPARE(status.network, QStringLiteral("home-wifi-5g"));
    QVERIFY(status.gateway.isEmpty());
}

void TestParser::testParseGateways()
{
    // Simulated CLI gateway output (ANSI stripped for clarity, uses │ U+2502)
    QString output = QStringLiteral(
        "Private gateways:\n"
        "Name/Country \u2502                  ID \u2502\n"
        "\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u253C\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u253C\n"
        "  Banking IP \u2502 banking-ip-p6ghzNLU \u2502\n"
        "\n"
        "Shared gateways:\n"
        "        Name/Country \u2502 ID \u2502\n"
        "\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u253C\u2500\u2500\u2500\u2500\u253C\n"
        "               Japan \u2502 jp \u2502\n"
        "         South Korea \u2502 kr \u2502\n"
        "           Hong Kong \u2502 hk \u2502\n");

    auto gateways = NordLayerParser::parseGateways(output);
    QCOMPARE(gateways.size(), 4);

    // Private gateway
    QCOMPARE(gateways[0].name, QStringLiteral("Banking IP"));
    QCOMPARE(gateways[0].id, QStringLiteral("banking-ip-p6ghzNLU"));
    QVERIFY(gateways[0].isPrivate);

    // Shared gateways
    QCOMPARE(gateways[1].name, QStringLiteral("Japan"));
    QCOMPARE(gateways[1].id, QStringLiteral("jp"));
    QVERIFY(!gateways[1].isPrivate);

    QCOMPARE(gateways[2].name, QStringLiteral("South Korea"));
    QCOMPARE(gateways[2].id, QStringLiteral("kr"));
}

void TestParser::testParseGateways_empty()
{
    QString output = QStringLiteral("No gateways available.\n");
    auto gateways = NordLayerParser::parseGateways(output);
    QCOMPARE(gateways.size(), 0);
}

void TestParser::testParseSettings()
{
    QString output = QStringLiteral(
        "Preferred VPN Protocol: Automatic\n"
        "Auto-connect: Disabled\n"
        "Web Protection: Disabled\n"
        "Always On: Disabled\n"
        "Kill switch: Enabled\n"
        "Local network access: Disabled (managed by your organisation)\n");

    auto settings = NordLayerParser::parseSettings(output);
    QCOMPARE(settings.vpnProtocol, QStringLiteral("Automatic"));
    QCOMPARE(settings.autoConnect, QStringLiteral("Disabled"));
    QCOMPARE(settings.webProtection, QStringLiteral("Disabled"));
    QCOMPARE(settings.alwaysOn, QStringLiteral("Disabled"));
    QCOMPARE(settings.killSwitch, QStringLiteral("Enabled"));
    QCOMPARE(settings.lanAccess, QStringLiteral("Disabled (managed by your organisation)"));
}

void TestParser::testParseSettings_partial()
{
    QString output = QStringLiteral(
        "Kill switch: Enabled\n"
        "Auto-connect: Enabled\n");

    auto settings = NordLayerParser::parseSettings(output);
    QCOMPARE(settings.killSwitch, QStringLiteral("Enabled"));
    QCOMPARE(settings.autoConnect, QStringLiteral("Enabled"));
    QVERIFY(settings.vpnProtocol.isEmpty());
}

QTEST_MAIN(TestParser)
#include "test_parser.moc"
