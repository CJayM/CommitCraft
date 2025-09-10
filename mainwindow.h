#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSettings>
#include <QProcess>
#include <QPoint>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class SettingsDialog;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void openSettingsDialog();
    void openRepository();
    void refreshGitStatus();
    void onGitStatusFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onGitStatusError(QProcess::ProcessError error);
    void showFileContextMenu(const QPoint &pos);
    void showStagedFileContextMenu(const QPoint &pos);
    void addSelectedFile();
    void unstageSelectedFile();

private:
    void saveSplitterState();
    void restoreSplitterState();
    void executeGitStatus();
    void parseGitStatusOutput(const QString &output);
    bool isGitRepository(const QString &path);
    bool isStaged(const QString &status);

    Ui::MainWindow *ui;
    QSettings *settings;
    SettingsDialog *settingsDialog;
    QProcess *gitProcess;
    QString repositoryPath;
};
#endif // MAINWINDOW_H
