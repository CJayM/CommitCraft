#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSettings>
#include <QProcess>

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
    void refreshGitStatus();
    void onGitStatusFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onGitStatusError(QProcess::ProcessError error);

private:
    void saveSplitterState();
    void restoreSplitterState();
    void executeGitStatus();
    void parseGitStatusOutput(const QString &output);

    Ui::MainWindow *ui;
    QSettings *settings;
    SettingsDialog *settingsDialog;
    QProcess *gitProcess;
};
#endif // MAINWINDOW_H
