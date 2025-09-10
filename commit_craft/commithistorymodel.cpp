#include "commithistorymodel.h"
#include <QColor>

CommitHistoryModel::CommitHistoryModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

int CommitHistoryModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_commits.size();
}

int CommitHistoryModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return 4; // hash, author, date, message
}

QVariant CommitHistoryModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_commits.size())
        return QVariant();

    const auto &commit = m_commits.at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        if (index.column() >= 0 && index.column() < commit.size()) {
            return commit.at(index.column());
        }
        break;
    case Qt::TextAlignmentRole:
        if (index.column() == 0) { // hash column
            return Qt::AlignCenter;
        } else if (index.column() == 2) { // date column
            return Qt::AlignCenter;
        }
        break;
    }

    return QVariant();
}

QVariant CommitHistoryModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
        case 0:
            return QString("Hash");
        case 1:
            return QString("Author");
        case 2:
            return QString("Date");
        case 3:
            return QString("Message");
        }
    }
    return QVariant();
}

void CommitHistoryModel::setCommits(const QList<QList<QString>> &commits)
{
    beginResetModel();
    m_commits = commits;
    endResetModel();
}

QString CommitHistoryModel::getCommitHash(int row) const
{
    if (row >= 0 && row < m_commits.size()) {
        const auto &commit = m_commits.at(row);
        if (!commit.isEmpty()) {
            return commit.at(0); // hash is the first column
        }
    }
    return QString();
}