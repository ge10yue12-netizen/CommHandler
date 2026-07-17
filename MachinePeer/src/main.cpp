// main.cpp — MachinePeer 试验机测试工具入口（不调 CommHandler）
#include "MainWindow.h"

#include <QApplication>

// 创建 QApplication 与主窗口并进入事件循环
int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("MachinePeer"));
    QApplication::setOrganizationName(QStringLiteral("DemoTest"));

    MainWindow window;
    window.show();
    return app.exec();
}
