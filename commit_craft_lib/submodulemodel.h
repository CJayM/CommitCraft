#ifndef SUBMODULEMODEL_H
#define SUBMODULEMODEL_H

#include <QAbstractTableModel>
#include <QColor>
#include <QString>
#include <QList>

struct SubmoduleInfo {
    QString name;           // Имя сабмодуля
    QString path;           // Путь к сабмодулю
    QString url;            // URL репозитория
    QString branch;         // Ветка (если указана)
    QString commit;         // Текущий закоммиченный хэш
    QString headCommit;     // Текущий HEAD хэш
    bool isDirty;           // Есть ли изменения
    bool isUninitialized;   // Не инициализирован
    bool isMissing;         // Отсутствует на диске
    
    SubmoduleInfo() : isDirty(false), isUninitialized(false), isMissing(false) {}
};

class SubmoduleModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Column {
        ColName = 0,
        ColPath = 1,
        ColBranch = 2,
        ColCommit = 3,
        ColStatus = 4,
        ColumnCount
    };
    
    enum Role {
        SubmoduleInfoRole = Qt::UserRole + 1,
        StatusColorRole
    };

    explicit SubmoduleModel(QObject *parent = nullptr);

    // QAbstractItemModel interface
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    // Custom methods
    void setSubmodules(const QList<SubmoduleInfo> &submodules);
    SubmoduleInfo getSubmodule(int row) const;
    QString getSubmoduleName(int row) const;
    QString getSubmodulePath(int row) const;
    void updateSubmoduleStatus(int row, const SubmoduleInfo &info);
    
    // Получить все сабмодули
    QList<SubmoduleInfo> submodules() const { return m_submodules; }

private:
    QList<SubmoduleInfo> m_submodules;
    
    QColor getStatusBackgroundColor(const SubmoduleInfo &info) const;
    QString getStatusText(const SubmoduleInfo &info) const;
    QString getStatusSymbol(const SubmoduleInfo &info) const;
};

#endif // SUBMODULEMODEL_H
