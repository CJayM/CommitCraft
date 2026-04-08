#ifndef FILEMODEL_H
#define FILEMODEL_H

#include <QAbstractTableModel>
#include <QColor>
#include <QPair>
#include <QList>

class FileModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit FileModel(QObject *parent = nullptr);

    // QAbstractItemModel interface
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Custom methods
    void setFiles(const QList<QPair<QString, QString>> &files);
    QString getFileName(int row) const;
    QString getFileStatus(int row) const;

private:
    QList<QPair<QString, QString>> m_files; // status, filename
    QColor getStatusBackgroundColor(const QString &status) const;
    QString getStatusSymbol(const QString &status) const;
};

#endif // FILEMODEL_H