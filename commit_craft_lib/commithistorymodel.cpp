#include "commithistorymodel.h"
#include <QColor>
#include <QDebug>

CommitHistoryModel::CommitHistoryModel(QObject *parent)
    : QAbstractItemModel(parent)
{
}

QModelIndex CommitHistoryModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid() || row < 0 || row >= m_commits.size() || column < 0 || column >= columnCount())
        return QModelIndex();
    return createIndex(row, column);
}

QModelIndex CommitHistoryModel::parent(const QModelIndex &index) const
{
    Q_UNUSED(index);
    return QModelIndex();
}

int CommitHistoryModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_commits.size();
}

int CommitHistoryModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 2; // Колонка 0: граф + хэш, Колонка 1: сообщение
}

QVariant CommitHistoryModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        if (section == 0)
            return tr("Commit");
        else if (section == 1)
            return tr("Message");
    }
    return QVariant();
}

QVariant CommitHistoryModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_commits.size())
        return QVariant();

    const auto &commit = m_commits.at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        // Для QTreeView с делегатом DisplayRole не используется напрямую
        // но возвращаем пустую строку чтобы избежать проблем
        return QString();
        
    case Qt::UserRole:
        // Возвращаем все данные коммита для делегата
        return QVariant::fromValue(commit);
        
    case Qt::UserRole + 1:
        // graphColumn
        return commit.graphColumn;
        
    case Qt::UserRole + 2:
        // branchColor
        return commit.branchColor;
        
    case Qt::UserRole + 3:
        // parents
        return commit.parents;
        
    case Qt::ToolTipRole:
        return QString("Hash: %1\nAuthor: %2\nDate: %3\nMessage: %4")
                .arg(commit.hash)
                .arg(commit.author)
                .arg(commit.date)
                .arg(commit.message);
    }

    return QVariant();
}

void CommitHistoryModel::setCommits(const QList<QList<QString>> &commits)
{
    beginResetModel();
    m_commits.clear();
    m_hashToRow.clear();
    
    for (const auto &commitData : commits) {
        if (commitData.size() < 4)
            continue;
            
        CommitData commit;
        commit.hash = commitData.at(0);
        commit.shortHash = commitData.at(0).left(8);
        commit.author = commitData.at(1);
        commit.date = commitData.at(2);
        commit.message = commitData.at(3);
        
        // Если есть дополнительные данные (родители, refs)
        if (commitData.size() > 4)
            commit.parents = commitData.at(4).split(' ', Qt::SkipEmptyParts);
        if (commitData.size() > 5)
            commit.refs = commitData.at(5).split(',', Qt::SkipEmptyParts);
            
        m_commits.append(commit);
        m_hashToRow[commit.hash] = m_commits.size() - 1;
    }
    
    // Вычисляем расположение графа
    calculateGraphLayout();
    
    endResetModel();
}

QString CommitHistoryModel::getCommitHash(int row) const
{
    if (row >= 0 && row < m_commits.size()) {
        return m_commits.at(row).hash;
    }
    return QString();
}

void CommitHistoryModel::calculateGraphLayout()
{
    // Алгоритм для вычисления позиции веток в графе
    // Отслеживаем активные ветки и их позиции
    QMap<QString, int> activeBranches; // hash -> column
    
    for (int i = 0; i < m_commits.size(); ++i) {
        auto &commit = m_commits[i];
        
        if (commit.parents.isEmpty()) {
            // Initial commit - всегда в колонке 0
            commit.graphColumn = 0;
            activeBranches[commit.hash] = 0;
        } else {
            // Находим позицию первого родителя
            QString primaryParent = commit.parents.first();
            int parentColumn = 0;
            
            // Ищем родителя в активных ветках
            if (activeBranches.contains(primaryParent)) {
                parentColumn = activeBranches[primaryParent];
            }
            
            // Проверяем, есть ли другие родители (merge)
            bool isMerge = commit.parents.size() > 1;
            
            if (isMerge) {
                // Для merge-коммита используем колонку первого родителя
                commit.graphColumn = parentColumn;
                
                // Удаляем все родительские ветки из активных (они объединяются)
                for (const QString &parentHash : commit.parents) {
                    if (activeBranches.contains(parentHash)) {
                        activeBranches.remove(parentHash);
                    }
                }
            } else {
                // Обычный коммит - продолжаем ветку родителя
                commit.graphColumn = parentColumn;
                
                // Удаляем родителя из активных веток
                if (activeBranches.contains(primaryParent)) {
                    activeBranches.remove(primaryParent);
                }
            }
            
            // Добавляем текущий коммит в активные ветки
            activeBranches[commit.hash] = commit.graphColumn;
        }
        
        commit.branchColor = generateBranchColor(commit.graphColumn);
    }
}

QColor CommitHistoryModel::generateBranchColor(int index)
{
    // Генерируем различные цвета для разных веток
    static const QVector<QColor> colors = {
        QColor(230, 57, 70),    // красный
        QColor(46, 204, 113),   // зеленый
        QColor(52, 152, 219),   // синий
        QColor(155, 89, 182),   // фиолетовый
        QColor(241, 196, 15),   // желтый
        QColor(230, 126, 34),   // оранжевый
        QColor(26, 188, 156),   // бирюзовый
        QColor(241, 128, 23),   // темно-оранжевый
    };
    
    return colors[index % colors.size()];
}
