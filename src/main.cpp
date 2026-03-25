#include "mainwindow.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QIcon>
#include <QSystemTrayIcon>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("NordLayer"));
    app.setApplicationVersion(QStringLiteral(APP_VERSION));
    app.setOrganizationName(QStringLiteral("nordlayer-qt"));
    app.setWindowIcon(QIcon(QStringLiteral(":/app-icon.svg")));
    app.setQuitOnLastWindowClosed(false);

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Lightweight GUI for NordLayer CLI"));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption minimizedOption(
        QStringLiteral("minimized"),
        QStringLiteral("Start minimized to system tray."));
    parser.addOption(minimizedOption);
    parser.process(app);

    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        QMessageBox::critical(nullptr, QStringLiteral("NordLayer"),
                              QStringLiteral("System tray is not available on this system."));
        return 1;
    }

    MainWindow window;
    if (!parser.isSet(minimizedOption)) {
        window.show();
    }

    return app.exec();
}
