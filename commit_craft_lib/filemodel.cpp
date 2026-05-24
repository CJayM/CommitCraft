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
    // Современные приглушённые цвета статусов (GitHub-inspired)
    if (status == "M") return QColor(213, 228, 255); // Modified — голубой
    if (status == "T") return QColor(229, 215, 240); // Type changed — сиреневый
    if (status == "A") return QColor(218, 251, 225); // Added — зелёный
    if (status == "D") return QColor(255, 235, 233); // Deleted — красный
    if (status == "R") return QColor(255, 245, 210); // Renamed — жёлтый
    if (status == "C") return QColor(255, 228, 200); // Copied — оранжевый
    if (status == "U") return QColor(246, 248, 250); // Unmerged — серый
    if (status == "?") return QColor(255, 255, 255); // Untracked — белый
    return QColor(255, 255, 255);
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
