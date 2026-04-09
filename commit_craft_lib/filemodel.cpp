#include "filemodel.h"
#include <QColor>
#include <QFileInfo>

FileModel::FileModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

int FileModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_files.size();
}

int FileModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return 3; // status, relative_dir, filename
}

QVariant FileModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_files.size())
        return QVariant();

    const auto &fileInfo = m_files.at(index.row());
    QString relativePath = fileInfo.second;
    QString fileName = QFileInfo(relativePath).fileName();
    QString relativeDir = QFileInfo(relativePath).path();
    // Если путь равен "." или пустой, значит файл в корне репозитория
    if (relativeDir == "." || relativeDir.isEmpty()) relativeDir = "";

    switch (role) {
    case Qt::DisplayRole:
        if (index.column() == 0) {
            // Преобразуем букву статуса в юникод-символ
            return getStatusSymbol(fileInfo.first);
        } else if (index.column() == 1) {
            return fileName;
        } else if (index.column() == 2) {
            return relativeDir;
        }
        break;
    case Qt::ForegroundRole:
        if (index.column() == 2) {
            // Серый цвет для директории
            return QColor(128, 128, 128);
        }
        break;
    case Qt::BackgroundRole:
            return getStatusBackgroundColor(fileInfo.first);
        break;
    case Qt::TextAlignmentRole:
        if (index.column() == 0) {
            return Qt::AlignCenter;
        } else if (index.column() == 2) {
            return Qt::AlignLeft;
        } else if (index.column() == 1) {
            return Qt::AlignLeft;
        }
        break;
    }

    return QVariant();
}

QVariant FileModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        if (section == 0) {
            return QString(); // Пустой заголовок для столбца статуса
        } else if (section == 1) {
            return QString("Файл");
        } else if (section == 2) {
            return QString("Директория");
        }
    }
    return QVariant();
}

void FileModel::setFiles(const QList<QPair<QString, QString>> &files)
{
    beginResetModel();
    m_files = files;
    endResetModel();
}

QString FileModel::getFileName(int row) const
{
    if (row >= 0 && row < m_files.size()) {
        return m_files.at(row).second;
    }
    return QString();
}

QString FileModel::getFileStatus(int row) const
{
    if (row >= 0 && row < m_files.size()) {
        return m_files.at(row).first;
    }
    return QString();
}

QString FileModel::getRelativePath(int row) const
{
    if (row >= 0 && row < m_files.size()) {
        return m_files.at(row).second;
    }
    return QString();
}

QColor FileModel::getStatusBackgroundColor(const QString &status) const
{
    // Define colors for each status type:
    // 'M' - Modified (light blue)
    if (status == "M") return QColor(173, 216, 230); // Light blue
    // 'T' - Type changed (light purple)
    if (status == "T") return QColor(230, 173, 230); // Light purple
    // 'A' - Added (light green)
    if (status == "A") return QColor(144, 238, 144); // Light green
    // 'D' - Deleted (light pink)
    if (status == "D") return QColor(255, 182, 193); // Light pink
    // 'R' - Renamed (light yellow)
    if (status == "R") return QColor(255, 255, 224); // Light yellow
    // 'C' - Copied (light orange)
    if (status == "C") return QColor(255, 218, 185); // Light orange
    // 'U' - Unmerged (light gray)
    if (status == "U") return QColor(211, 211, 211); // Light gray
    // '?' - Untracked (default color)
    if (status == "?") return QColor(255, 255, 255); // White
    // Default color for any other status
    return QColor(255, 255, 255); // White
}

QString FileModel::getStatusSymbol(const QString &status) const
{
    // Преобразуем букву статуса Git в юникод-символ
    if (status.isEmpty()) return QString();
    
    QChar c = status.at(0);
    switch (c.toLatin1()) {
        case 'M': return QString(QChar(0x270F)); // ✏ карандаш (modified)
        case 'A': return QString(QChar(0x002B)); // + плюс (added)
        case 'D': return QString(QChar(0x2718)); // ✘ крестик (deleted)
        case 'R': return QString(QChar(0x2192)); // → стрелка (renamed)
        case 'C': return QString(QChar(0x2398)); // ⎘ копирование (copied)
        case 'U': return QString(QChar(0x26A0)); // ⚠ предупреждение (unmerged)
        case 'T': return QString(QChar(0x25C6)); // ◆ ромб (type changed)
        case '?': return QString(QChar(0x003F)); // ? вопрос (untracked)
        default:  return status;
    }
}
