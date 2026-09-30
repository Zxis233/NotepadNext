#include "FileEncodingHistory.h"

#include <QDir>
#include <QFile>
#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>

class FileEncodingHistoryTest : public QObject
{
    Q_OBJECT
    QTemporaryDir directory;

    QString filePath(const QString &name) const
    {
        return directory.path() + QStringLiteral("/files/") + name;
    }

private slots:
    void initTestCase()
    {
        QVERIFY(directory.isValid());
        QCoreApplication::setOrganizationName(QStringLiteral("NotepadNextTests"));
        QCoreApplication::setApplicationName(QStringLiteral("FileEncodingHistoryTest"));
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, directory.path());
        QSettings::setPath(QSettings::IniFormat, QSettings::SystemScope, directory.path());
        QVERIFY(QDir().mkpath(filePath("sub")));
        QFile file(filePath("same.txt"));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write("text"), qint64(4));
    }

    void init()
    {
        QSettings settings;
        settings.clear();
        settings.sync();
    }

    void persistsAndReplacesChoice()
    {
        const auto path = filePath("same.txt");
        QCOMPARE(FileEncodingHistory::lookup(path), FileEncoding::Auto);
        FileEncodingHistory::remember(path, FileEncoding::Gbk);
        QCOMPARE(FileEncodingHistory::lookup(path), FileEncoding::Gbk);
        // Each history call uses a fresh QSettings instance. Also verify the
        // persistent store can be read from a separate settings object.
        QSettings persisted;
        persisted.beginGroup("FileEncodings");
        QCOMPARE(persisted.childKeys().size(), 1);
        QCOMPARE(persisted.value(persisted.childKeys().first()).toInt(), int(FileEncoding::Gbk));
        FileEncodingHistory::remember(path, FileEncoding::Utf8);
        QCOMPARE(FileEncodingHistory::lookup(path), FileEncoding::Utf8);
    }

    void fullPathsKeepSameNamesIndependent()
    {
        FileEncodingHistory::remember(filePath("same.txt"), FileEncoding::Gbk);
        FileEncodingHistory::remember(filePath("sub/same.txt"), FileEncoding::ShiftJis);
        QCOMPARE(FileEncodingHistory::lookup(filePath("same.txt")), FileEncoding::Gbk);
        QCOMPARE(FileEncodingHistory::lookup(filePath("sub/same.txt")), FileEncoding::ShiftJis);
    }

    void normalizesPaths()
    {
        const auto path = filePath("same.txt");
        FileEncodingHistory::remember(path, FileEncoding::ShiftJis);
        QCOMPARE(FileEncodingHistory::lookup(filePath("sub/../same.txt")), FileEncoding::ShiftJis);
        QCOMPARE(FileEncodingHistory::lookup(QDir::current().relativeFilePath(path)), FileEncoding::ShiftJis);
#ifdef Q_OS_WIN
        QCOMPARE(FileEncodingHistory::lookup(QDir::toNativeSeparators(path).toUpper()), FileEncoding::ShiftJis);
#endif
    }

    void invalidAndEmptyChoicesAreIgnored()
    {
        const auto path = filePath("same.txt");
        FileEncodingHistory::remember(path, FileEncoding::Gbk);
        FileEncodingHistory::remember(path, FileEncoding::Auto);
        FileEncodingHistory::remember(path, static_cast<FileEncoding::Type>(6));
        FileEncodingHistory::remember(QString(), FileEncoding::ShiftJis);
        QCOMPARE(FileEncodingHistory::lookup(path), FileEncoding::Gbk);
        QCOMPARE(FileEncodingHistory::lookup(QString()), FileEncoding::Auto);
        QSettings settings;
        settings.beginGroup("FileEncodings");
        const auto keys = settings.childKeys();
        QCOMPARE(keys.size(), 1);
        settings.setValue(keys.first(), QStringLiteral("invalid"));
        settings.sync();
        QCOMPARE(FileEncodingHistory::lookup(path), FileEncoding::Auto);
    }

    void forgetOnlyAffectsOnePath()
    {
        FileEncodingHistory::remember(filePath("same.txt"), FileEncoding::Gbk);
        FileEncodingHistory::remember(filePath("sub/same.txt"), FileEncoding::ShiftJis);
        FileEncodingHistory::forget(filePath("same.txt"));
        QCOMPARE(FileEncodingHistory::lookup(filePath("same.txt")), FileEncoding::Auto);
        QCOMPARE(FileEncodingHistory::lookup(filePath("sub/same.txt")), FileEncoding::ShiftJis);
    }

    void bomOverridesHistory()
    {
        const auto path = filePath("same.txt");
        FileEncodingHistory::remember(path, FileEncoding::Gbk);
        QByteArray decoded;
        QString error;
        FileEncoding::Type detected;
        QVERIFY(FileEncoding::decode(QByteArray::fromHex("efbbbfe4b8ad"),
                                     FileEncodingHistory::lookup(path), decoded, detected, error));
        QCOMPARE(detected, FileEncoding::Utf8Bom);
        QCOMPARE(decoded, QByteArray::fromHex("e4b8ad"));
    }
};

QTEST_GUILESS_MAIN(FileEncodingHistoryTest)
#include "FileEncodingHistoryTest.moc"
