// main.cpp — MachinePeer 试验机侧进程入口
#include "MainWindow.h"

#include <QApplication>

// 创建 QApplication 与主窗口并进入事件循环
int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("MachinePeer"));
    QApplication::setOrganizationName(QStringLiteral("CommHandler"));

    MainWindow window;
    window.show();
    return app.exec();
}
