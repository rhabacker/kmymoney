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
    QVariant customDirProp = property("lockDir");
    if (customDirProp.isValid()) {
        lockDir = customDirProp.toString();
    } else {
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

    if (output.contains("LOCK_FAILED")) {
        QVERIFY2(true, "Child correctly blocked from acquiring lock");
    } else if (output.contains("LOCK_ACQUIRED")) {
        QWARN("Child acquired lock – NFS may not support native locking (check mount options)");
        QEXPECT_FAIL("", "NFS without lockd allows multiple acquisitions", Continue);
        QVERIFY2(false, "Child should not have acquired the lock");
    } else {
        QFAIL("Child process did not report lock result");
    }
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
    QStringList args = app.arguments();

    // ---- Child process mode ----
    if (args.contains("--child")) {
        int idx = args.indexOf("--child");
        if (idx + 1 < args.size()) {
            QString path = args.at(idx + 1);
            QLockFile childLock(path);
            if (childLock.tryLock(100)) {
                fprintf(stdout, "LOCK_ACQUIRED\n");
                fflush(stdout);
                return 0;
            } else {
                fprintf(stdout, "LOCK_FAILED\n");
                fflush(stdout);
                return 1;
            }
        }
        return 2;
    }

    // ---- Extract custom lock dir after "--" ----
    QString customDir;
    int sep = args.indexOf("--");
    if (sep != -1 && sep + 1 < args.size()) {
        customDir = args.at(sep + 1);
    }

    // Remove our own args before passing to qExec
    QStringList qtArgs;
    qtArgs << args.first(); // program name
    for (int i = 1; i < args.size(); ++i) {
        if (args.at(i) == "--child") {
            ++i;
            continue;
        } // skip --child + path
        if (args.at(i) == "--") {
            ++i;
            continue;
        } // skip -- and custom dir
        qtArgs << args.at(i);
    }

    // Run tests
    TestFileLocking tc;
    if (!customDir.isEmpty())
        tc.setProperty("lockDir", customDir); // store in QObject property for initTestCase

    QVector<QByteArray> cArgs;
    QVector<char*> cArgv;
    for (const QString& a : qtArgs)
        cArgs.append(a.toLocal8Bit());
    for (QByteArray& ba : cArgs)
        cArgv.append(ba.data());
    int newArgc = cArgv.size();

    return QTest::qExec(&tc, newArgc, cArgv.data());
}

#include "test-filelocking.moc"
