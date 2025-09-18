#include <QCoreApplication>
#include <QFileInfo>
#include <QLockFile>
#include <QProcess>
#include <QTemporaryDir>
#include <QtTest/QtTest>
#include <memory>

#include <iostream>

class TestFileLocking : public QObject
{
    Q_OBJECT

private Q_SLOTS: // ✅ canonical Qt style
    void initTestCase();
    void testExclusiveLockSameProcess();
    void testExclusiveLockOtherProcess();
    void testLockRelease();

private:
    QString lockDir;
    QString lockFilePath;
    std::unique_ptr<QTemporaryDir> tempDir; // keep alive for test lifetime
};

void TestFileLocking::initTestCase()
{
    // Check if user passed a custom directory (e.g. on NFS)
#if 0
    QStringList args = QCoreApplication::arguments();
    if (args.size() > 1 && args.at(1) != "--child") {
        lockDir = args.at(1);
#else
    QString path = qgetenv("TESTFILELOCKING_PATH");
    if (!path.isEmpty()) {
        lockDir = path;
#endif
}
else
{
    tempDir = std::make_unique<QTemporaryDir>();
    QVERIFY2(tempDir->isValid(), "Failed to create temporary directory");
    lockDir = tempDir->path();
}

QVERIFY2(QFileInfo::exists(lockDir), "Lock directory does not exist");
lockFilePath = QDir(lockDir).filePath("test.lock");
}

void TestFileLocking::testExclusiveLockSameProcess()
{
    QLockFile lock1(lockFilePath);
    QVERIFY2(lock1.tryLock(100), "First lock attempt should succeed");

    QLockFile lock2(lockFilePath);
    QVERIFY2(!lock2.tryLock(100), "Second lock attempt in same process should fail while first lock is held");
}

void TestFileLocking::testExclusiveLockOtherProcess()
{
    QLockFile lock1(lockFilePath);
    QVERIFY2(lock1.tryLock(100), "Parent process lock should succeed");

    // Spawn a child process that tries to acquire the lock
    QProcess proc;
    QString program = QCoreApplication::applicationFilePath();
    QStringList args;
    args << "--child" << lockFilePath;

    proc.start(program, args);
    QVERIFY2(proc.waitForFinished(5000), "Child process did not finish in time");

    QByteArray output = proc.readAllStandardOutput();
    qDebug() << "Child stdout:" << output;

    // Child should fail to acquire the lock
    QVERIFY2(output.contains("LOCK_FAILED"), "Child process unexpectedly acquired the lock");
}

void TestFileLocking::testLockRelease()
{
    {
        QLockFile lock1(lockFilePath);
        QVERIFY2(lock1.tryLock(100), "First lock attempt should succeed");
        // lock1 goes out of scope -> releases lock
    }

    QLockFile lock2(lockFilePath);
    QVERIFY2(lock2.tryLock(100), "Second lock attempt should succeed after release");
}

// ---- Entry point ----
int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);

    // Special mode: act as child lock tester
    QStringList args = app.arguments();
    if (args.contains("--child")) {
        int idx = args.indexOf("--child");
        if (idx + 1 < args.size()) {
            QString path = args.at(idx + 1);
            QLockFile childLock(path);
            if (childLock.tryLock(100)) {
                std::cout << "LOCK_ACQUIRED" << std::endl;
                return 0;
            } else {
                std::cout << "LOCK_FAILED" << std::endl;
                return 1;
            }
        }
        return 2;
    }

    // Normal test runner
    TestFileLocking tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "testfilelocking.moc"
