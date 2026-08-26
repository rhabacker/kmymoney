
#include <QApplication>
#include <QDebug>
#include <QElapsedTimer>
#include <QFile>
#include <QTextBrowser>
#include <qabstracttextdocumentlayout.h>

class HtmlPerformanceTest
{
public:
    static void run(const QString& fileName)
    {
        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly)) {
            qFatal("Unable to open %s", qPrintable(fileName));
        }

        const QByteArray html = file.readAll();

        qDebug() << "HTML size:" << html.size() << "bytes";

        QTextBrowser browser;
        browser.resize(1200, 800);

        QElapsedTimer timer;

        timer.start();
        browser.setHtml(QString::fromUtf8(html));
        qDebug() << "setHtml:" << timer.elapsed() << "ms";

        timer.restart();
        browser.show();
        qDebug() << "show:" << timer.elapsed() << "ms";

        timer.restart();
        QApplication::processEvents();
        qDebug() << "processEvents:" << timer.elapsed() << "ms";

        timer.restart();
        // Force document layout.
        const QSizeF size = browser.document()->documentLayout()->documentSize();

        qDebug() << "documentSize:" << size << "time:" << timer.elapsed() << "ms";

        // Keep the window around so that the actual widget can be inspected.
        QApplication::exec();
    }
};

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    const char* fileName = nullptr;
    if (argc == 1)
        fileName = "/home/ralf.habacker/src/kmymoney-master/kmymoney/plugins/views/reports/core/tests/test-qtextbrowser.html";
    else
        fileName = argv[1];

    qDebug() << "loading" << fileName;
    HtmlPerformanceTest::run(QString::fromLocal8Bit(fileName));
}