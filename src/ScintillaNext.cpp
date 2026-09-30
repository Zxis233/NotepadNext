/*
 * This file is part of Notepad Next.
 * Copyright 2019 Justin Dailey
 *
 * Notepad Next is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Notepad Next is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Notepad Next.  If not, see <https://www.gnu.org/licenses/>.
 */


#include "ScintillaNext.h"
#include "Finder.h"
#include "ScintillaCommenter.h"

#include "ByteArrayUtils.h"
#include <cinttypes>

#include <QDir>
#include <QMouseEvent>
#include <QSaveFile>
#include <QSignalBlocker>


static QFileDevice::FileError writeBytes(const QByteArray &data, const QString &path, QString &error)
{
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        error = file.errorString();
        return file.error();
    }
    if (file.write(data) != data.size()) {
        error = file.errorString();
        file.cancelWriting();
        return QFileDevice::WriteError;
    }
    if (!file.commit()) {
        error = file.errorString();
        return file.error();
    }
    error.clear();
    return QFileDevice::NoError;
}

static QFileDevice::FileError writeToDisk(const QByteArray &data, const QString &path,
                                         FileEncoding::Type encoding, QString &error)
{
    QByteArray encoded;
    if (!FileEncoding::encode(data, encoding, encoded, error))
        return QFileDevice::WriteError;
    return writeBytes(encoded, path, error);
}

static bool isNewlineCharacter(char c)
{
    return c == '\n' || c == '\r';
}

ScintillaNext::ScintillaNext(QString name, QWidget *parent) :
    ScintillaEdit(parent),
    name(name),
    indicatorResources(INDICATOR_MAX + 1)
{
    // Per the scintilla documentation, some parts of the range are not generally available
    setCodePage(SC_CP_UTF8);
    indicatorResources.disableRange(0, 7);
    indicatorResources.disableRange(INDICATOR_IME, INDICATOR_IME_MAX);
    indicatorResources.disableRange(INDICATOR_HISTORY_REVERTED_TO_ORIGIN_INSERTION, INDICATOR_HISTORY_REVERTED_TO_MODIFIED_DELETION);
}

ScintillaNext::~ScintillaNext()
{
}

ScintillaNext *ScintillaNext::fromFile(const QString &filePath, bool tryToCreate, FileEncoding::Type encoding)
{
    QFile file(filePath);
    ScintillaNext *editor = new ScintillaNext(file.fileName());

    if(tryToCreate && !file.exists()) {
        qInfo("Trying to create %s", qUtf8Printable(filePath));
        QDir d;
        d.mkpath(QFileInfo(file).path());

        QFile f(filePath);
        f.open(QIODevice::WriteOnly);
        f.close();
    }

    bool readSuccessful = editor->readFromDisk(file, encoding);

    if (!readSuccessful) {
        delete editor;
        return Q_NULLPTR;
    }

    editor->setFileInfo(filePath);

    return editor;
}

ScintillaNext *ScintillaNext::fromSessionFile(const QString &filePath)
{
    auto editor = new ScintillaNext(filePath);
    QFile file(filePath);
    if (!editor->readFromDisk(file, FileEncoding::Auto, true)) {
        delete editor;
        return nullptr;
    }
    editor->setFileInfo(filePath);
    return editor;
}

QString ScintillaNext::eolModeToString(int eolMode)
{
    if (eolMode == SC_EOL_CRLF)
        return QStringLiteral("crlf");
    else if (eolMode == SC_EOL_CR)
        return QStringLiteral("cr");
    else if (eolMode == SC_EOL_LF)
        return QStringLiteral("lf");
    else
        return QString(); // unknown
}

int ScintillaNext::stringToEolMode(QString eolMode)
{
    if (eolMode == QStringLiteral("crlf"))
        return SC_EOL_CRLF;
    else if (eolMode == QStringLiteral("cr"))
        return SC_EOL_CR;
    else if (eolMode == QStringLiteral("lf"))
        return SC_EOL_LF;
    else
        return -1;
}

int ScintillaNext::allocateIndicator(const QString &name)
{
    return indicatorResources.requestResource(name);
}

void ScintillaNext::goToRange(const Sci_CharacterRange &range)
{
    qInfo(Q_FUNC_INFO);

    if (isRangeValid(range)) {
        // Lines can be folded so make sure they are visible
        ensureVisible(lineFromPosition(range.cpMin));
        ensureVisible(lineFromPosition(range.cpMax));

        setSelection(range.cpMax, range.cpMin);
        scrollRange(range.cpMax, range.cpMin);
    }
}

QByteArray ScintillaNext::eolString() const
{
    const int eol = eOLMode();

    if (eol == SC_EOL_LF) return QByteArrayLiteral("\n");
    else if (eol == SC_EOL_CRLF) return QByteArrayLiteral("\r\n");
    else return QByteArrayLiteral("\r");
}

bool ScintillaNext::lineIsEmpty(int line)
{
    return (lineEndPosition(line) - positionFromLine(line)) == 0;
}

void ScintillaNext::deleteLine(int line)
{
    deleteRange(positionFromLine(line), lineLength(line));
}

void ScintillaNext::cutAllowLine()
{
    if (selectionEmpty()) {
        copyAllowLine();
        lineDelete();
    }
    else {
        cut();
    }
}

Sci_CharacterRange ScintillaNext::getContextText()
{
    if (!selectionEmpty()) {
        if (selections() == 1) {
            int start = selectionStart();
            int end = selectionEnd();

            if (end - start < 1024 &&
                lineFromPosition(start) == lineFromPosition(end)) {
                return { start, end };
            }
        }

        return { INVALID_POSITION, INVALID_POSITION };
    }

    return wordAtPosition(currentPos());
}

Sci_CharacterRange ScintillaNext::wordAtPosition(int pos)
{
    int start = wordStartPosition(pos, true);
    int end = wordEndPosition(pos, true);

    if (end > start) {
        return { start, end };
    }

    return { INVALID_POSITION, INVALID_POSITION };
}

void ScintillaNext::modifyFoldLevels(int level, int action)
{
    const int totalLines = lineCount();

    int line = 0;
    while (line < totalLines) {
        int foldFlags = foldLevel(line); // Even though its called fold level it contains several other flags
        bool isHeader = foldFlags & SC_FOLDLEVELHEADERFLAG;
        int actualLevel = (foldFlags & SC_FOLDLEVELNUMBERMASK) - SC_FOLDLEVELBASE;

        if (isHeader && actualLevel == level) {
            foldLine(line, action);
            line = lastChild(line, -1) + 1;
        }
        else {
            ++line;
        }
    }
}

void ScintillaNext::foldAllLevels(int level)
{
    modifyFoldLevels(level, SC_FOLDACTION_CONTRACT);
}

void ScintillaNext::unFoldAllLevels(int level)
{
    modifyFoldLevels(level, SC_FOLDACTION_EXPAND);
}

void ScintillaNext::deleteLeadingEmptyLines()
{
    while (lineCount() > 1 && lineIsEmpty(0)) {
        deleteLine(0);
    }
}

void ScintillaNext::deleteTrailingEmptyLines()
{
    const int docLength = length();
    int position = docLength;

    while (position > 0 && isNewlineCharacter(charAt(position - 1))) {
        position--;
    }

    deleteRange(position, docLength - position);
}

bool ScintillaNext::isSavedToDisk() const
{
    return !canSaveToDisk();
}

bool ScintillaNext::canSaveToDisk() const
{
    // The buffer can be saved if:
    // - It is marked as a temporary since as soon as it gets saved it is no longer a temporary buffer
    // - A modified file
    // - A missing file since as soon as it is saved it is no longer missing.
    return temporary || fileEncoding != diskEncoding ||
           (bufferType == ScintillaNext::New && modify()) ||
           (bufferType == ScintillaNext::File && modify()) ||
            (bufferType == ScintillaNext::FileMissing);
}

void ScintillaNext::setName(const QString &name)
{
    this->name = name;

    emit renamed();
}

bool ScintillaNext::isFile() const
{
    return bufferType == ScintillaNext::File || bufferType == ScintillaNext::FileMissing;
}

QFileInfo ScintillaNext::getFileInfo() const
{
    Q_ASSERT(isFile());

    return fileInfo;
}

QString ScintillaNext::getPath() const
{
    Q_ASSERT(isFile());

    return QDir::toNativeSeparators(fileInfo.canonicalPath());
}

QString ScintillaNext::getFilePath() const
{
    Q_ASSERT(isFile());

    return QDir::toNativeSeparators(fileInfo.canonicalFilePath());
}

void ScintillaNext::setFoldMarkers(const QString &type)
{
    QMap<QString, QList<int>> map{
        {"simple", {SC_MARK_MINUS, SC_MARK_PLUS, SC_MARK_EMPTY, SC_MARK_EMPTY, SC_MARK_EMPTY, SC_MARK_EMPTY, SC_MARK_EMPTY}},
        {"arrow",  {SC_MARK_ARROWDOWN, SC_MARK_ARROW, SC_MARK_EMPTY, SC_MARK_EMPTY, SC_MARK_EMPTY, SC_MARK_EMPTY, SC_MARK_EMPTY}},
        {"circle", {SC_MARK_CIRCLEMINUS, SC_MARK_CIRCLEPLUS, SC_MARK_VLINE, SC_MARK_LCORNERCURVE, SC_MARK_CIRCLEPLUSCONNECTED, SC_MARK_CIRCLEMINUSCONNECTED, SC_MARK_TCORNERCURVE }},
        {"box",    {SC_MARK_BOXMINUS, SC_MARK_BOXPLUS, SC_MARK_VLINE, SC_MARK_LCORNER, SC_MARK_BOXPLUSCONNECTED, SC_MARK_BOXMINUSCONNECTED, SC_MARK_TCORNER }},
    };

    if (!map.contains(type))
        return;

    const auto types = map[type];
    markerDefine(SC_MARKNUM_FOLDEROPEN, types[0]);
    markerDefine(SC_MARKNUM_FOLDER, types[1]);
    markerDefine(SC_MARKNUM_FOLDERSUB, types[2]);
    markerDefine(SC_MARKNUM_FOLDERTAIL, types[3]);
    markerDefine(SC_MARKNUM_FOLDEREND, types[4]);
    markerDefine(SC_MARKNUM_FOLDEROPENMID, types[5]);
    markerDefine(SC_MARKNUM_FOLDERMIDTAIL, types[6]);
}

void ScintillaNext::close()
{
    emit closed();

    deleteLater();
}

ScintillaNext::BomType ScintillaNext::bom() const
{
    switch (fileEncoding) {
    case FileEncoding::Utf8Bom: return BomType::Utf8;
    case FileEncoding::Utf16LE: return BomType::Utf16LE;
    case FileEncoding::Utf16BE: return BomType::Utf16BE;
    default: return BomType::None;
    }
}

void ScintillaNext::setEncoding(FileEncoding::Type encoding)
{
    if (!FileEncoding::isValid(encoding) || encoding == fileEncoding)
        return;
    fileEncoding = encoding;
    emit encodingChanged();
    emit savePointChanged(canSaveToDisk());
}

void ScintillaNext::restoreEncoding(FileEncoding::Type encoding, FileEncoding::Type savedEncoding)
{
    if (!FileEncoding::isValid(encoding) || !FileEncoding::isValid(savedEncoding))
        return;
    fileEncoding = encoding;
    diskEncoding = savedEncoding;
    emit encodingChanged();
    emit savePointChanged(canSaveToDisk());
}

QFileDevice::FileError ScintillaNext::save()
{
    qInfo(Q_FUNC_INFO);

    Q_ASSERT(isFile());

    emit aboutToSave();

    const QByteArray data = QByteArray::fromRawData((char*)characterPointer(), textLength());
    const QString path = fileInfo.filePath();
    QFileDevice::FileError writeSuccessful = writeToDisk(data, path, fileEncoding, lastFileError);

    if (writeSuccessful == QFileDevice::NoError) {
        updateTimestamp();
        diskEncoding = fileEncoding;
        setSavePoint();

        // If this was a temporary file, make sure it is not any more
        setTemporary(false);

        emit saved();
    }

    return writeSuccessful;
}

void ScintillaNext::reload()
{
    // An unsaved output-encoding change must not reinterpret the disk bytes.
    if (!reloadWithEncoding(diskEncoding))
        emit fileReadFailed(lastFileError);
}

bool ScintillaNext::reloadWithEncoding(FileEncoding::Type encoding)
{
    if (!isFile())
        return false;

    const int line = firstVisibleLine();
    const int caret = selectionNCaret(mainSelection());
    const int anchor = selectionNAnchor(mainSelection());

    QFile f(fileInfo.filePath());
    bool readSuccessful = readFromDisk(f, encoding);

    if (!readSuccessful) {
        return false;
    }

    updateTimestamp();
    setSavePoint();

    // If this was a temporary file, make sure it is not any more
    if (isTemporary())
        setTemporary(false);
    emit savePointChanged(canSaveToDisk());

    scrollVertical(line, 0);
    setSelection(caret, anchor);

    emit reloaded();
    emit encodingChanged();
    return true;
}

void ScintillaNext::omitModifications()
{
    // If file modifications will be omitted just update file timestamp
    // so pop-up will be displayed only once per file modifications.
    updateTimestamp();
    setTemporary(true);

    return;
}

QFileDevice::FileError ScintillaNext::saveAs(const QString &newFilePath)
{
    bool isRenamed = bufferType == ScintillaNext::New || fileInfo.canonicalFilePath() != newFilePath;

    emit aboutToSave();

    const QByteArray data = QByteArray::fromRawData((char*)characterPointer(), textLength());
    QFileDevice::FileError saveSuccessful = writeToDisk(data, newFilePath, fileEncoding, lastFileError);

    if (saveSuccessful == QFileDevice::NoError) {
        setFileInfo(newFilePath);
        diskEncoding = fileEncoding;
        setSavePoint();

        // If this was a temporary file, make sure it is not any more
        setTemporary(false);

        emit saved();

        if (isRenamed) {
            emit renamed();
        }
    }

    return saveSuccessful;
}

QFileDevice::FileError ScintillaNext::saveCopyAs(const QString &filePath)
{
    const QByteArray data = QByteArray::fromRawData((char*)characterPointer(), textLength());
    return writeToDisk(data, filePath, fileEncoding, lastFileError);
}

QFileDevice::FileError ScintillaNext::saveSessionCopy(const QString &path)
{
    // Session snapshots preserve the internal bytes, including undecoded input.
    // They must not lose unsaved characters unrepresentable in the output codec.
    const QByteArray data = QByteArray::fromRawData((char*)characterPointer(), textLength());
    return writeBytes(data, path, lastFileError);
}

bool ScintillaNext::rename(const QString &newFilePath)
{
    emit aboutToSave();

    // Write out the buffer to the new path
    if (saveCopyAs(newFilePath) == QFileDevice::NoError) {
        // Remove the old file
        const QString oldPath = fileInfo.canonicalFilePath();
        QFile::remove(oldPath);

        // Everything worked fine, so update the buffer's info
        setFileInfo(newFilePath);
        diskEncoding = fileEncoding;
        setSavePoint();

        // If this was a temporary file, make sure it is not any more
        setTemporary(false);

        emit saved();

        emit renamed();

        return true;
    }

    return false;
}

ScintillaNext::FileStateChange ScintillaNext::checkFileForStateChange()
{
    if (bufferType == BufferType::New) {
        return FileStateChange::NoChange;
    }
    else if (bufferType == BufferType::File) {
        // refresh else exists() fails to notice missing file
        fileInfo.refresh();

        if (!fileInfo.exists()) {
            bufferType = BufferType::FileMissing;

            emit savePointChanged(false);

            return FileStateChange::Deleted;
        }

        // See if the timestamp changed
        if (modifiedTime != fileTimestamp()) {
            return FileStateChange::Modified;
        }
        else {
            return FileStateChange::NoChange;
        }
    }
    else if (bufferType == BufferType::FileMissing) {
        // See if it reappeared
        fileInfo.refresh();

        if (fileInfo.exists()) {
            bufferType = BufferType::File;

            return FileStateChange::Restored;
        }
        else {
            return FileStateChange::NoChange;
        }
    }

    qInfo("type() = %d", bufferType);
    Q_ASSERT(false);

    return FileStateChange::NoChange;
}

bool ScintillaNext::moveToTrash()
{
    if (QFile::exists(fileInfo.canonicalFilePath())) {
        QFile f(fileInfo.canonicalFilePath());

        return f.moveToTrash();
    }

    return false;
}

void ScintillaNext::toggleCommentSelection()
{
    ScintillaCommenter sc(this);
    sc.toggleSelection();
}

void ScintillaNext::commentLineSelection()
{
    ScintillaCommenter sc(this);
    sc.commentSelection();
}

void ScintillaNext::uncommentLineSelection()
{
    ScintillaCommenter sc(this);
    sc.uncommentSelection();
}

void ScintillaNext::removeDuplicateLines()
{
    QByteArray data = QByteArray::fromRawData((char*) characterPointer(), textLength());
    const QByteArray delim = eolString();

    auto lines = ByteArrayUtils::split(data, delim);
    int originalLineCount = lines.length();
    ByteArrayUtils::removeDuplicates(lines);

    if (originalLineCount == lines.length()){
        return; // No lines were removed
    }

    QByteArray result = ByteArrayUtils::join(lines, delim);

    const UndoAction ua(this);
    setTargetRange(0, textLength());
    replaceTarget(result.length(), result.constData());
}

void ScintillaNext::removeConsecutiveDuplicateLines()
{
    QByteArray data = QByteArray::fromRawData((char*) characterPointer(), textLength());
    const QByteArray delim = eolString();

    auto lines = ByteArrayUtils::split(data, delim);
    int originalLineCount = lines.length();
    ByteArrayUtils::removeConsecutiveDuplicates(lines);
    QByteArray result = ByteArrayUtils::join(lines, delim);

    if (originalLineCount == lines.length()){
        return; // No lines were removed
    }

    const UndoAction ua(this);
    setTargetRange(0, textLength());
    replaceTarget(result.length(), result.constData());
}

void ScintillaNext::dragEnterEvent(QDragEnterEvent *event)
{
    // Ignore all drag and drop events with urls and let the main application handle it
    if (event->mimeData()->hasUrls()) {
        return;
    }

    ScintillaEdit::dragEnterEvent(event);
}

void ScintillaNext::dropEvent(QDropEvent *event)
{
    // Ignore all drag and drop events with urls and let the main application handle it
    if (event->mimeData()->hasUrls()) {
        return;
    }

    ScintillaEdit::dropEvent(event);
}

bool ScintillaNext::readFromDisk(QFile &file, FileEncoding::Type encoding, bool sessionSnapshot)
{
    if (!file.open(QIODevice::ReadOnly)) {
        lastFileError = file.errorString();
        return false;
    }
    const QByteArray bytes = file.readAll();
    if (file.error() != QFileDevice::NoError) {
        lastFileError = file.errorString();
        return false;
    }
    file.close();
    QByteArray utf8;
    FileEncoding::Type detected = FileEncoding::Utf8;
    if (sessionSnapshot) {
        utf8 = bytes;
        lastFileError.clear();
    }
    else if (!FileEncoding::decode(bytes, encoding, utf8, detected, lastFileError))
        return false;

    // Read and decode before replacing the document: a failed reopen must
    // preserve the current text, undo history, and encoding.
    {
        const QSignalBlocker blocker(this);
        setReadOnly(false);
        setUndoCollection(false);
        setCodePage(SC_CP_UTF8);
        clearAll();
        appendText(utf8.size(), utf8.constData());
        emptyUndoBuffer();
        setUndoCollection(true);
        fileEncoding = diskEncoding = detected;
        setReadOnly(!QFileInfo(file).isWritable());
    }
    return true;
}

QDateTime ScintillaNext::fileTimestamp()
{
    Q_ASSERT(bufferType != ScintillaNext::New);

    fileInfo.refresh();
    qInfo("%s last modified %s", qUtf8Printable(fileInfo.fileName()), qUtf8Printable(fileInfo.lastModified().toString()));
    return fileInfo.lastModified();
}

void ScintillaNext::updateTimestamp()
{
    modifiedTime = fileTimestamp();
}

void ScintillaNext::setFileInfo(const QString &filePath)
{
    fileInfo.setFile(filePath);
    fileInfo.makeAbsolute();

    Q_ASSERT(fileInfo.exists());

    name = fileInfo.fileName();
    bufferType = ScintillaNext::File;

    updateTimestamp();
}

void ScintillaNext::detachFileInfo(const QString &newName)
{
    setName(newName);

    bufferType = ScintillaNext::New;
}

void ScintillaNext::setTemporary(bool temp)
{
    temporary = temp;

    // Fake this signal
    emit savePointChanged(temporary);
}
