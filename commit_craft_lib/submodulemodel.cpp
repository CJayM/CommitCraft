#include "submodulemodel.h"
#include <QFont>

SubmoduleModel::SubmoduleModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

int SubmoduleModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_submodules.size();
}

int SubmoduleModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return ColumnCount;
}

QVariant SubmoduleModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_submodules.size())
        return QVariant();

    const SubmoduleInfo &info = m_submodules.at(index.row());

    if (role == SubmoduleInfoRole) {
        return QVariant::fromValue(info);
    }

    if (role == StatusColorRole) {
        return getStatusBackgroundColor(info);
    }

    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        switch (index.column()) {
        case ColName:
            return info.name;
        case ColPath:
            return info.path;
        case ColBranch:
            return info.branch.isEmpty() ? QStringLiteral("-") : info.branch;
        case ColCommit:
            return info.commit.isEmpty() ? QStringLiteral("-") : info.commit.left(7);
        case ColStatus:
            return getStatusText(info);
        default:
            return QVariant();
        }
    }

    if (role == Qt::DecorationRole && index.column() == ColName) {
        // Иконка статуса
        QString symbol = getStatusSymbol(info);
        if (!symbol.isEmpty()) {
            return symbol;
        }
    }

    if (role == Qt::ForegroundRole && index.column() == ColStatus) {
        return getStatusBackgroundColor(info);
    }

    if (role == Qt::FontRole && index.column() == ColCommit) {
        QFont font;
        font.setFamily(QStringLiteral("Courier New"));
        font.setPointSize(10);
        return font;
    }

    return QVariant();
}

QVariant SubmoduleModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
        case ColName:
            return tr("Имя");
        case ColPath:
            return tr("Путь");
        case ColBranch:
            return tr("Ветка");
        case ColCommit:
            return tr("Коммит");
        case ColStatus:
            return tr("Статус");
        default:
            return QVariant();
        }
    }
    return QAbstractTableModel::headerData(section, orientation, role);
}

Qt::ItemFlags SubmoduleModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

void SubmoduleModel::setSubmodules(const QList<SubmoduleInfo> &submodules)
{
    beginResetModel();
    m_submodules = submodules;
    endResetModel();
}

SubmoduleInfo SubmoduleModel::getSubmodule(int row) const
{
    if (row >= 0 && row < m_submodules.size())
        return m_submodules.at(row);
    return SubmoduleInfo();
}

QString SubmoduleModel::getSubmoduleName(int row) const
{
    if (row >= 0 && row < m_submodules.size())
        return m_submodules.at(row).name;
    return QString();
}

QString SubmoduleModel::getSubmodulePath(int row) const
{
    if (row >= 0 && row < m_submodules.size())
        return m_submodules.at(row).path;
    return QString();
}

void SubmoduleModel::updateSubmoduleStatus(int row, const SubmoduleInfo &info)
{
    if (row >= 0 && row < m_submodules.size()) {
        m_submodules[row] = info;
        emit dataChanged(index(row, 0), index(row, ColumnCount - 1), {Qt::DisplayRole, StatusColorRole});
    }
}

QColor SubmoduleModel::getStatusBackgroundColor(const SubmoduleInfo &info) const
{
    if (info.isMissing)
        return QColor(255, 200, 200); // Красный для отсутствующих
    if (info.isUninitialized)
        return QColor(255, 230, 150); // Желтый для неинициализированных
    if (info.isDirty)
        return QColor(200, 230, 255); // Голубой для измененных
    return QColor(200, 255, 200); // Зеленый для нормальных
}

QString SubmoduleModel::getStatusText(const SubmoduleInfo &info) const
{
    if (info.isMissing)
        return tr("Отсутствует");
    if (info.isUninitialized)
        return tr("Не инициализирован");
    if (info.isDirty)
        return tr("Есть изменения");
    if (info.commit != info.headCommit && !info.commit.isEmpty() && !info.headCommit.isEmpty())
        return tr("Вне коммита");
    return tr("OK");
}

QString SubmoduleModel::getStatusSymbol(const SubmoduleInfo &info) const
{
    if (info.isMissing)
        return QStringLiteral("❌");
    if (info.isUninitialized)
        return QStringLiteral("⚠️");
    if (info.isDirty)
        return QStringLiteral("✏️");
    if (info.commit != info.headCommit && !info.commit.isEmpty() && !info.headCommit.isEmpty())
        return QStringLiteral("🔄");
    return QStringLiteral("✓");
}
