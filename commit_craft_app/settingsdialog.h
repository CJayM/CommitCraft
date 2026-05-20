#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QSettings>

QT_BEGIN_NAMESPACE
namespace Ui {
class SettingsDialog;
}
QT_END_NAMESPACE

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog();

    // Git path
    QString getGitPath() const;
    void setGitPath(const QString &gitPath);

    // Font settings
    QString getFontFamily() const;
    int getFontSize() const;
    void setFontFamily(const QString &fontFamily);
    void setFontSize(int fontSize);

    // File type settings
    QStringList getImageExtensions() const;
    QStringList getSyntaxExtensions() const;
    void setImageExtensions(const QStringList &extensions);
    void setSyntaxExtensions(const QStringList &extensions);

private slots:
    void onGitPathButtonClicked();

private:
    void loadSettings();
    void saveSettings();

    Ui::SettingsDialog *ui;
    QSettings *settings;
};

#endif // SETTINGSDIALOG_H
