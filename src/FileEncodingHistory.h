#pragma once

#include "FileEncoding.h"

namespace FileEncodingHistory {
// The history describes bytes on disk, never an unsaved conversion choice.
FileEncoding::Type lookup(const QString &path);
void remember(const QString &path, FileEncoding::Type encoding);
void forget(const QString &path);
}
