#include "MainWindow.h"
#include "SmokeRunner.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QIODevice>
#include <QString>

namespace {

// 将一次冒烟结果写入 smoke_result.txt。
bool writeSmokeResult(const QString& content)
{
    QFile rf(QStringLiteral("smoke_result.txt"));
    if (!rf.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning().noquote() << "无法写入 smoke_result.txt：" << rf.errorString();
        return false;
    }
    rf.write(content.toUtf8());
    rf.close();
    return true;
}

// 以 QCoreApplication 跑单项冒烟并回写结果文件。
int runOneSmoke(const char* tag, int (*fn)(QString*), int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QString detail;
    const int rc = fn(&detail);
    const QString line = QStringLiteral("[%1] %2 %3\n")
                             .arg(QString::fromLatin1(tag))
                             .arg((rc == 0) ? QStringLiteral("PASS") : QStringLiteral("FAIL"))
                             .arg(detail);
    if (!writeSmokeResult(line) && rc == 0) {
        return 98;
    }
    qInfo().noquote() << QString::fromLatin1(tag)
                      << ((rc == 0) ? "PASS" : "FAIL")
                      << detail;
    return rc;
}

} // namespace

// 入口：识别 --smoke* 则无头自检，否则启动 GUI。
int main(int argc, char* argv[])
{
    for (int i = 1; i < argc; ++i) {
        const QString a = QString::fromLocal8Bit(argv[i]);
        if (a == QLatin1String("--smoke")) {
            return runOneSmoke("CommLab --smoke", RunUdpSansiSmoke, argc, argv);
        }
        if (a == QLatin1String("--smoke-send")) {
            return runOneSmoke("CommLab --smoke-send", RunUdpJsonSendSmoke, argc, argv);
        }
        if (a == QLatin1String("--smoke-preview")) {
            return runOneSmoke("CommLab --smoke-preview", RunSerialPreviewSmoke, argc, argv);
        }
        if (a == QLatin1String("--smoke-all")) {
            QCoreApplication app(argc, argv);
            QString d1;
            QString d2;
            QString d3;
            const int r1 = RunUdpSansiSmoke(&d1);
            const int r2 = RunUdpJsonSendSmoke(&d2);
            const int r3 = RunSerialPreviewSmoke(&d3);
            const QString body =
                QStringLiteral("[--smoke] %1 %2\n[--smoke-send] %3 %4\n"
                               "[--smoke-preview] %5 %6\n")
                    .arg(r1 == 0 ? QStringLiteral("PASS") : QStringLiteral("FAIL"))
                    .arg(d1)
                    .arg(r2 == 0 ? QStringLiteral("PASS") : QStringLiteral("FAIL"))
                    .arg(d2)
                    .arg(r3 == 0 ? QStringLiteral("PASS") : QStringLiteral("FAIL"))
                    .arg(d3);
            const bool wrote = writeSmokeResult(body);
            qInfo().noquote() << "[--smoke]" << (r1 == 0 ? "PASS" : "FAIL") << d1;
            qInfo().noquote() << "[--smoke-send]" << (r2 == 0 ? "PASS" : "FAIL") << d2;
            qInfo().noquote() << "[--smoke-preview]" << (r3 == 0 ? "PASS" : "FAIL") << d3;
            if (!wrote) {
                return 98;
            }
            return (r1 == 0 && r2 == 0 && r3 == 0) ? 0 : 1;
        }
    }

    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("CommLab"));
    QApplication::setOrganizationName(QStringLiteral("CommHandler"));

    MainWindow window;
    window.show();
    return app.exec();
}
