#include "FileEncoding.h"

#include <QObject>
#include <QTextCodec>

namespace {
QByteArray bom(FileEncoding::Type encoding)
{
    switch (encoding) {
    case FileEncoding::Utf8Bom: return QByteArray::fromHex("efbbbf");
    case FileEncoding::Utf16LE: return QByteArray::fromHex("fffe");
    case FileEncoding::Utf16BE: return QByteArray::fromHex("feff");
    default: return {};
    }
}

QTextCodec *codec(FileEncoding::Type encoding)
{
    switch (encoding) {
    case FileEncoding::Utf8:
    case FileEncoding::Utf8Bom: return QTextCodec::codecForName("UTF-8");
    case FileEncoding::Utf16LE: return QTextCodec::codecForName("UTF-16LE");
    case FileEncoding::Utf16BE: return QTextCodec::codecForName("UTF-16BE");
    case FileEncoding::Gbk: return QTextCodec::codecForName("GBK");
    case FileEncoding::ShiftJis: return QTextCodec::codecForName("Shift_JIS");
    default: return nullptr;
    }
}
}

bool FileEncoding::isValid(int value)
{
    return value >= Utf8 && value <= ShiftJis;
}

QString FileEncoding::name(Type encoding)
{
    switch (encoding) {
    case Utf8: return QStringLiteral("UTF-8");
    case Utf8Bom: return QStringLiteral("UTF-8 BOM");
    case Utf16LE: return QStringLiteral("UTF-16 LE BOM");
    case Utf16BE: return QStringLiteral("UTF-16 BE BOM");
    case Gbk: return QStringLiteral("GBK");
    case ShiftJis: return QStringLiteral("Shift_JIS");
    default: return QString();
    }
}

bool FileEncoding::decode(const QByteArray &bytes, Type requested, QByteArray &utf8,
                          Type &detected, QString &error)
{
    error.clear();
    // Reopening a BOM-less UTF-8 file does not add a BOM. That is a
    // conversion, and must go through the output-encoding/dirty-state path.
    detected = requested == Auto || requested == Utf8Bom ? Utf8 : requested;
    int offset = 0;
    // Reject UTF-32 before its LE signature can be mistaken for UTF-16.
    if (bytes.startsWith(QByteArray::fromHex("fffe0000")) ||
        bytes.startsWith(QByteArray::fromHex("0000feff"))) {
        error = QObject::tr("UTF-32 files are not supported.");
        return false;
    }
    for (Type type : {Utf8Bom, Utf16LE, Utf16BE}) {
        const QByteArray signature = bom(type);
        if (bytes.startsWith(signature)) {
            detected = type;
            offset = signature.size();
            break;
        }
    }
    QTextCodec *decoder = codec(detected);
    if (!decoder) {
        error = QObject::tr("The %1 codec is not available.").arg(name(detected));
        return false;
    }
    QTextCodec::ConverterState state(QTextCodec::IgnoreHeader);
    const QString text = decoder->toUnicode(bytes.constData() + offset, bytes.size() - offset, &state);
    if (state.invalidChars || state.remainingChars) {
        // Preserve unknown, BOM-less bytes on the initial open so the user can
        // select Reopen with Encoding. Never silently replace them on disk.
        if (requested == Auto && offset == 0) {
            utf8 = bytes;
            return true;
        }
        error = QObject::tr("The file contains invalid or incomplete %1 data.").arg(name(detected));
        return false;
    }
    utf8 = text.toUtf8();
    return true;
}

bool FileEncoding::encode(const QByteArray &utf8, Type encoding, QByteArray &bytes, QString &error)
{
    error.clear();
    QTextCodec *encoder = codec(encoding);
    QTextCodec *utf8Codec = codec(Utf8);
    if (!encoder || !utf8Codec) {
        error = QObject::tr("The %1 codec is not available.").arg(name(encoding));
        return false;
    }
    QTextCodec::ConverterState inputState(QTextCodec::IgnoreHeader);
    const QString text = utf8Codec->toUnicode(utf8.constData(), utf8.size(), &inputState);
    if (inputState.invalidChars || inputState.remainingChars) {
        error = QObject::tr("The document is not valid UTF-8. Use Encoding > Reopen with Encoding to select its original encoding before saving.");
        return false;
    }
    QTextCodec::ConverterState outputState(QTextCodec::IgnoreHeader);
    const QByteArray encoded = encoder->fromUnicode(text.constData(), text.size(), &outputState);
    // Also reject mappings which encode but change characters on reopening.
    QTextCodec::ConverterState roundTripState(QTextCodec::IgnoreHeader);
    const QString roundTrip = encoder->toUnicode(encoded.constData(), encoded.size(), &roundTripState);
    if (outputState.invalidChars || outputState.remainingChars ||
        roundTripState.invalidChars || roundTripState.remainingChars || roundTrip != text) {
        error = QObject::tr("Some characters cannot be saved losslessly as %1. Choose a Unicode encoding instead.").arg(name(encoding));
        return false;
    }
    bytes = bom(encoding) + encoded;
    return true;
}
