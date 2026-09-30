#include "FileEncodingHistory.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QSettings>

namespace {
QString keyForPath(const QString &path)
{
    if (path.isEmpty())
        return {};
    const QFileInfo info(path);
    QString normalized = info.canonicalFilePath();
    if (normalized.isEmpty())
        normalized = info.absoluteFilePath();
    normalized = QDir::cleanPath(QDir::fromNativeSeparators(normalized));
#ifdef Q_OS_WIN
    normalized = normalized.toCaseFolded();
#endif
    // Hashing avoids QSettings group separators and escaping in file names.
    const QByteArray hash = QCryptographicHash::hash(normalized.toUtf8(), QCryptographicHash::Sha256).toHex();
    return QStringLiteral("FileEncodings/") + QString::fromLatin1(hash);
}
}

FileEncoding::Type FileEncodingHistory::lookup(const QString &path)
{
    const QString key = keyForPath(path);
    if (key.isEmpty())
        return FileEncoding::Auto;
    QSettings settings;
    bool ok = false;
    const int value = settings.value(key).toInt(&ok);
    return ok && FileEncoding::isValid(value) ? static_cast<FileEncoding::Type>(value) : FileEncoding::Auto;
}

void FileEncodingHistory::remember(const QString &path, FileEncoding::Type encoding)
{
    const QString key = keyForPath(path);
    if (key.isEmpty() || !FileEncoding::isValid(encoding))
        return;
    QSettings settings;
    settings.setValue(key, static_cast<int>(encoding));
    settings.sync();
}

void FileEncodingHistory::forget(const QString &path)
{
    const QString key = keyForPath(path);
    if (key.isEmpty())
        return;
    QSettings settings;
    settings.remove(key);
    settings.sync();
}
