#include "commithistorymodel.h"
#include <QColor>
#include <QDebug>
#include <QSet>

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
            
        // Используем graphColumn из --graph, если он передан (элемент 6)
        if (commitData.size() > 6) {
            commit.graphColumn = commitData.at(6).toInt();
        } else {
            commit.graphColumn = 0;
        }

        // Парсим активные колонки из префикса графа (элемент 7)
        if (commitData.size() > 7) {
            QString graphPrefix = commitData.at(7);
            for (int pos = 0; pos < graphPrefix.length(); pos += 2) {
                QChar c0 = graphPrefix.at(pos);
                QChar c1 = (pos + 1 < graphPrefix.length())
                    ? graphPrefix.at(pos + 1) : ' ';
                if (c0 != ' ' || c1 != ' ')
                    commit.activeColumns.append(pos / 2);
            }
            if (!commit.activeColumns.contains(commit.graphColumn))
                commit.activeColumns.append(commit.graphColumn);
        } else {
            commit.activeColumns.append(commit.graphColumn);
        }

        m_commits.append(commit);
        m_hashToRow[commit.hash] = m_commits.size() - 1;
    }

    // Второй проход: вычисляем parentColumns для merge-соединений
    // и branchColor для каждого коммита
    for (auto &commit : m_commits) {
        commit.parentColumns.clear();
        for (const QString &parentHash : commit.parents) {
            int row = m_hashToRow.value(parentHash, -1);
            if (row >= 0 && row < m_commits.size()) {
                commit.parentColumns.append(m_commits[row].graphColumn);
            } else {
                commit.parentColumns.append(-1);
            }
        }

        commit.branchColor = generateBranchColor(commit.graphColumn);
    }
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
    // Идём от старых коммитов к новым, чтобы родители уже были обработаны
    QMap<QString, int> hashToColumn; // hash → назначенная колонка

    for (int i = m_commits.size() - 1; i >= 0; --i) {
        auto &commit = m_commits[i];

        int column = 0;

        if (!commit.parents.isEmpty()) {
            // Пытаемся найти колонку любого из родителей
            bool foundParent = false;
            for (const QString &parent : commit.parents) {
                if (hashToColumn.contains(parent)) {
                    column = hashToColumn[parent];
                    foundParent = true;
                    break;
                }
            }

            if (!foundParent) {
                // Все родители уже заняты другими ветками (разветвление)
                // Назначаем минимальную свободную колонку
                QSet<int> occupiedColumns;
                for (auto it = hashToColumn.constBegin(); it != hashToColumn.constEnd(); ++it)
                    occupiedColumns.insert(it.value());
                column = 0;
                while (occupiedColumns.contains(column))
                    column++;
            }
        }
        // Корневой коммит → колонка 0 (по умолчанию)

        // Удаляем родителей из отслеживания (они больше не активны)
        for (const QString &parent : commit.parents) {
            hashToColumn.remove(parent);
        }

        // Регистрируем текущий коммит
        hashToColumn[commit.hash] = column;
        commit.graphColumn = column;
        commit.branchColor = generateBranchColor(column);
    }

    // Второй проход: вычисляем parentColumns для каждого коммита
    for (auto &commit : m_commits) {
        commit.parentColumns.clear();
        for (const QString &parentHash : commit.parents) {
            int row = m_hashToRow.value(parentHash, -1);
            if (row >= 0 && row < m_commits.size()) {
                commit.parentColumns.append(m_commits[row].graphColumn);
            } else {
                commit.parentColumns.append(-1); // родитель вне списка
            }
        }
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
