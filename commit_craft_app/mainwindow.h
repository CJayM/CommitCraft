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
class CommitHistoryModel;
class CommitItemDelegate;
class Git;

// Forward declare FileModel from the library
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
    void onGitStatusFinished(const QString &output);
    void onGitDiffReady(const QString &output);
    void onGitCommitHistoryReady(const QList<QList<QString>> &commits);
    void onGitCommitFinished(bool success, const QString &message);
    void onGitError(const QString &error);
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
    void toggleLeftPanel(bool visible);
    void toggleTopPanel(bool visible);
    void onAmendCheckBoxChanged(int state);
    void updateCommitButtonState();

private:
    void saveSplitterState();
    void restoreSplitterState();
    void parseGitStatusOutput(const QString &output);
    bool isGitRepository(const QString &path);
    bool isStaged(const QString &status);
    bool isUnstaged(const QString &status);    
    void updateDiffPanel(const QString &fileName);
    QString getFileContent(const QString &fileName, bool staged);
    void parseAndApplyDiffHighlighting(const QString &diffOutput);
    void synchronizeScroll();
    void extractHunkPositions(const QString &diffOutput);
    void applyDiffHighlighting(const QString &fileName);

    Ui::MainWindow *ui;
    QSettings *settings;
    SettingsDialog *settingsDialog;
    QString repositoryPath;
    CodeEditor *stagedContentEditor;
    CodeEditor *currentContentEditor;
    FileModel *unstagedFilesModel;
    FileModel *stagedFilesModel;
    CommitHistoryModel *commitHistoryModel;
    CommitItemDelegate *commitItemDelegate;
    Git *git;
    
    // Hunk navigation data
    QList<QPair<int, int>> hunkPositions; // Pair of (stagedLine, currentLine) for each hunk
    int currentHunkIndex;
};
#endif // MAINWINDOW_H
