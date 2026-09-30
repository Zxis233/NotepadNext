#include "FileEncoding.h"

#include <QtTest>

class FileEncodingTest : public QObject
{
    Q_OBJECT
private slots:
    void knownBytes_data()
    {
        QTest::addColumn<int>("encoding");
        QTest::addColumn<QByteArray>("utf8");
        QTest::addColumn<QByteArray>("disk");
        // Chinese U+4E2D U+6587 and Japanese U+65E5 U+672C U+8A9E.
        QTest::newRow("gbk") << int(FileEncoding::Gbk)
            << QByteArray::fromHex("e4b8ade69687") << QByteArray::fromHex("d6d0cec4");
        QTest::newRow("shift-jis") << int(FileEncoding::ShiftJis)
            << QByteArray::fromHex("e697a5e69cace8aa9e") << QByteArray::fromHex("93fa967b8cea");
        QTest::newRow("utf8") << int(FileEncoding::Utf8)
            << QByteArray::fromHex("e4b8ad") << QByteArray::fromHex("e4b8ad");
        QTest::newRow("utf8-bom") << int(FileEncoding::Utf8Bom)
            << QByteArray::fromHex("e4b8ad") << QByteArray::fromHex("efbbbfe4b8ad");
        QTest::newRow("utf16-le") << int(FileEncoding::Utf16LE)
            << QByteArray::fromHex("e4b8ad") << QByteArray::fromHex("fffe2d4e");
        QTest::newRow("utf16-be") << int(FileEncoding::Utf16BE)
            << QByteArray::fromHex("e4b8ad") << QByteArray::fromHex("feff4e2d");
    }

    void knownBytes()
    {
        QFETCH(int, encoding);
        QFETCH(QByteArray, utf8);
        QFETCH(QByteArray, disk);
        const auto type = static_cast<FileEncoding::Type>(encoding);
        QByteArray encoded, decoded;
        QString error;
        FileEncoding::Type detected;
        QVERIFY2(FileEncoding::encode(utf8, type, encoded, error), qPrintable(error));
        QCOMPARE(encoded, disk);
        QVERIFY2(FileEncoding::decode(disk, type, decoded, detected, error), qPrintable(error));
        QCOMPARE(decoded, utf8);
        QCOMPARE(detected, type);
    }

    void rejectsLossyConversion()
    {
        for (auto type : {FileEncoding::Gbk, FileEncoding::ShiftJis}) {
            QByteArray output("unchanged");
            QString error;
            QVERIFY(!FileEncoding::encode(QByteArray::fromHex("f09f9880"), type, output, error));
            QCOMPARE(output, QByteArray("unchanged"));
            QVERIFY(!error.isEmpty());
        }
    }

    void incompleteInput()
    {
        for (auto type : {FileEncoding::Gbk, FileEncoding::ShiftJis, FileEncoding::Utf8}) {
            QByteArray output("unchanged");
            QString error;
            FileEncoding::Type detected;
            const auto input = type == FileEncoding::Utf8 ? QByteArray::fromHex("e4b8") : QByteArray::fromHex("81");
            QVERIFY(!FileEncoding::decode(input, type, output, detected, error));
            QCOMPARE(output, QByteArray("unchanged"));
            QVERIFY(!error.isEmpty());
        }
        QByteArray output;
        QString error;
        FileEncoding::Type detected;
        QVERIFY(!FileEncoding::decode(QByteArray::fromHex("fffe2d"), FileEncoding::Auto, output, detected, error));
    }

    void bomOverridesRequestedCodec()
    {
        QByteArray output;
        QString error;
        FileEncoding::Type detected;
        QVERIFY(FileEncoding::decode(QByteArray::fromHex("efbbbfe4b8ad"), FileEncoding::Gbk, output, detected, error));
        QCOMPARE(detected, FileEncoding::Utf8Bom);
        QCOMPARE(output, QByteArray::fromHex("e4b8ad"));
    }

    void unknownBytesArePreservedButCannotBeSaved()
    {
        const QByteArray input = QByteArray::fromHex("d6d0cec4");
        QByteArray output, encoded;
        QString error;
        FileEncoding::Type detected;
        QVERIFY(FileEncoding::decode(input, FileEncoding::Auto, output, detected, error));
        QCOMPARE(output, input);
        QVERIFY(!FileEncoding::encode(output, FileEncoding::Utf8, encoded, error));
    }

    void reopeningDoesNotInventUtf8Bom()
    {
        QByteArray output;
        QString error;
        FileEncoding::Type detected;
        QVERIFY(FileEncoding::decode("plain text", FileEncoding::Utf8Bom, output, detected, error));
        QCOMPARE(detected, FileEncoding::Utf8);
        QCOMPARE(output, QByteArray("plain text"));
    }

    void nulAndLeadingBomCharacterArePreserved()
    {
        const QByteArray input = QByteArray::fromHex("efbbbf410042");
        QByteArray encoded, decoded;
        QString error;
        FileEncoding::Type detected;
        QVERIFY(FileEncoding::encode(input, FileEncoding::Utf8Bom, encoded, error));
        QCOMPARE(encoded, QByteArray::fromHex("efbbbfefbbbf410042"));
        QVERIFY(FileEncoding::decode(encoded, FileEncoding::Auto, decoded, detected, error));
        QCOMPARE(decoded, input);
    }
};

QTEST_GUILESS_MAIN(FileEncodingTest)
#include "FileEncodingTest.moc"
