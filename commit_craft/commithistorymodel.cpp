#include "commithistorymodel.h"
#include <QColor>

CommitHistoryModel::CommitHistoryModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int CommitHistoryModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_commits.size();
}

QVariant CommitHistoryModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_commits.size())
        return QVariant();

    const auto &commit = m_commits.at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        if (commit.size() >= 4) {
            // Return a formatted string for display
            return QString("%1 - %2 (%3)")
                .arg(commit.at(2))  // date
                .arg(commit.at(3))  // message
                .arg(commit.at(1)); // author
        }
        break;
    case Qt::UserRole:
        // Return the raw commit data for the delegate
        return QVariant::fromValue(commit);
    case Qt::ToolTipRole:
        if (commit.size() >= 4) {
            return QString("Hash: %1\nAuthor: %2\nDate: %3\nMessage: %4")
                    .arg(commit.at(0))
                    .arg(commit.at(1))
                    .arg(commit.at(2))
                    .arg(commit.at(3));
        }
        break;
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
