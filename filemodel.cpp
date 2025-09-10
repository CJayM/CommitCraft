#include "filemodel.h"
#include <QColor>

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
    return 2; // status, filename
}

QVariant FileModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_files.size())
        return QVariant();

    const auto &fileInfo = m_files.at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        if (index.column() == 0) {
            return fileInfo.first; // status
        } else if (index.column() == 1) {
            return fileInfo.second; // filename
        }
        break;
    case Qt::BackgroundRole:        
            return getStatusBackgroundColor(fileInfo.first);        
        break;
    case Qt::TextAlignmentRole:
        if (index.column() == 0) {
            return Qt::AlignCenter;
        }
        break;
    }

    return QVariant();
}

QVariant FileModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        if (section == 0) {
            return QString("Статус");
        } else if (section == 1) {
            return QString("Файл");
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
