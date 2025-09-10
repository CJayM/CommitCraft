#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSettings>
#include <QProcess>
#include <QPoint>
#include <QTextCharFormat>
#include <QScrollBar>
#include <QList>
#include <QPair>
#include <QRegularExpression>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class SettingsDialog;
class CodeEditor;
class FileModel;

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
    void commitChanges();
    void onFileTableSelectionChanged();
    void onStagedFileTableSelectionChanged();
    void synchronizeZoom(int zoom);
    void navigateToNextHunk();
    void navigateToPrevHunk();

private:
    void saveSplitterState();
    void restoreSplitterState();
    void executeGitStatus();
    void parseGitStatusOutput(const QString &output);
    bool isGitRepository(const QString &path);
    bool isStaged(const QString &status);
    bool isUnstaged(const QString &status);
    void updateCommitButtonState();
    void updateDiffPanel(const QString &fileName);
    QString getFileContent(const QString &fileName, bool staged);
    void applyDiffHighlighting(const QString &fileName);
    void parseAndApplyDiffHighlighting(const QString &diffOutput);
    void synchronizeScroll();
    void extractHunkPositions(const QString &diffOutput);

    Ui::MainWindow *ui;
    QSettings *settings;
    SettingsDialog *settingsDialog;
    QProcess *gitProcess;
    QString repositoryPath;
    CodeEditor *stagedContentEditor;
    CodeEditor *currentContentEditor;
    FileModel *unstagedFilesModel;
    FileModel *stagedFilesModel;
    
    // Hunk navigation data
    QList<QPair<int, int>> hunkPositions; // Pair of (stagedLine, currentLine) for each hunk
    int currentHunkIndex;
};
#endif // MAINWINDOW_H
