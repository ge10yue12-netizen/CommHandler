#include "MainWindow.h"

#include "ICommSession.h"
#include "SessionConfig.h"

#include <QApplication>
#include <QMetaType>
#include <QVector>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("CommunicationAssistant"));
    QApplication::setOrganizationName(QStringLiteral("DemoTest"));

    qRegisterMetaType<ca::CommRecord>("ca::CommRecord");
    qRegisterMetaType<ca::CommError>("ca::CommError");
    qRegisterMetaType<ca::SessionState>("ca::SessionState");
    qRegisterMetaType<ca::LegacyConfig>("ca::LegacyConfig");
    qRegisterMetaType<ca::LegacyConfig>("LegacyConfig");
    qRegisterMetaType<QVector<double>>("QVector<double>");

    MainWindow window;
    window.show();
    return app.exec();
}