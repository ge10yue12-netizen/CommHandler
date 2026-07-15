#include "MainWindow.h"

#include <QApplication>

// 进程入口：创建应用与主窗并进入事件循环。
int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("MachinePeer"));
    QApplication::setOrganizationName(QStringLiteral("MachinePeer"));

    MainWindow window;
    window.show();
    return app.exec();
}
