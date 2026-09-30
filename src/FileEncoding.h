#pragma once

#include <QByteArray>
#include <QString>

namespace FileEncoding {
// Values are persisted in sessions; do not reorder them.
enum Type { Auto = -1, Utf8 = 0, Utf8Bom, Utf16LE, Utf16BE, Gbk, ShiftJis };

bool isValid(int value);
QString name(Type encoding);
bool decode(const QByteArray &bytes, Type requested, QByteArray &utf8,
            Type &detected, QString &error);
bool encode(const QByteArray &utf8, Type encoding, QByteArray &bytes, QString &error);
}
