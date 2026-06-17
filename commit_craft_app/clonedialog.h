#ifndef CLONEDIALOG_H
#define CLONEDIALOG_H

#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui {
class CloneDialog;
}
QT_END_NAMESPACE

class CloneDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CloneDialog(QWidget *parent = nullptr);
    ~CloneDialog();

    QString getRepositoryUrl() const;
    QString getDestinationPath() const;

private slots:
    void onSelectDestinationClicked();
    void onUrlChanged(const QString &url);
    void onAccept();

private:
    void validateInputs();

    Ui::CloneDialog *ui;
};

#endif // CLONEDIALOG_H
