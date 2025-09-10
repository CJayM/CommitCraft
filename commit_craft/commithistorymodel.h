#ifndef COMMITHISTORYMODEL_H
#define COMMITHISTORYMODEL_H

#include <QAbstractTableModel>
#include <QList>
#include <QPair>

class CommitHistoryModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit CommitHistoryModel(QObject *parent = nullptr);

    // QAbstractItemModel interface
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Custom methods
    void setCommits(const QList<QList<QString>> &commits);
    QString getCommitHash(int row) const;

private:
    QList<QList<QString>> m_commits; // Each commit: hash, author, date, message
};

#endif // COMMITHISTORYMODEL_H