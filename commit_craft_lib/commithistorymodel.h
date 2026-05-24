#ifndef COMMITHISTORYMODEL_H
#define COMMITHISTORYMODEL_H

#include <QAbstractItemModel>
#include <QList>
#include <QVariant>
#include <QColor>
#include <QMap>

struct CommitData {
    QString hash;
    QString shortHash;
    QString author;
    QString date;
    QString message;
    QStringList parents;
    QList<int> parentColumns;  // graphColumn каждого родителя
    QStringList refs;  // Ветки, теги
    int graphColumn;   // Позиция в графе
    QList<int> activeColumns;  // Все активные колонки в этой строке графа
    QColor branchColor;
};

class CommitHistoryModel : public QAbstractItemModel
{
    Q_OBJECT
    
public:
    explicit CommitHistoryModel(QObject *parent = nullptr);
    
    // QAbstractItemModel interface
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    
    // Custom methods
    void setCommits(const QList<QList<QString>> &commits);
    QString getCommitHash(int row) const;
    
private:
    QList<CommitData> m_commits;
    QMap<QString, int> m_hashToRow;
    
    void calculateGraphLayout();
    QColor generateBranchColor(int index);
};

#endif // COMMITHISTORYMODEL_H
