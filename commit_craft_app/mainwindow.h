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
#include <QFileSystemWatcher>
#include <QTimer>

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
class GitParser;
class DiffEditor;

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
    void toggleLeftPanel(bool visible);
    void toggleTopPanel(bool visible);
    void onAmendCheckBoxChanged(int state);
    void updateCommitButtonState();
    void synchronizeZoom(int zoom);
    void navigateToNextHunk();
    void navigateToPrevHunk();
    
    /// Обновляет состояние кнопок навигации (Prev/Next Hunk)
    void updateNavigationButtonsState();

private:
    void saveSplitterState();
    void restoreSplitterState();
    bool isGitRepository(const QString &path);
    QString getFileContent(const QString &fileName, bool staged);
    void updateDiffPanel(const QString &fileName);

    Ui::MainWindow *ui;
    QSettings *settings;
    SettingsDialog *settingsDialog;
    QString repositoryPath;
    DiffEditor *diffEditor;
    FileModel *unstagedFilesModel;
    FileModel *stagedFilesModel;
    CommitHistoryModel *commitHistoryModel;
    CommitItemDelegate *commitItemDelegate;
    Git *git;

    // Отслеживание последнего выбранного файла и источника
    QString m_lastSelectedFileName;
    enum class SelectionSource { Unstaged, Staged };
    SelectionSource m_lastSelectionSource = SelectionSource::Unstaged;

    // Настройки шрифта DiffEditor
    void saveFontSettings(const QString &fontFamily, int fontSize);
    QPair<QString, int> loadFontSettings();

    // Файловый watcher для автообновления
    QFileSystemWatcher *m_fsWatcher;
    QTimer *m_fsDebounceTimer;
    void setupFileSystemWatcher();
    void onFileSystemChanged();

    // Отложенная навигация для асинхронной загрузки
    enum class PendingNavigation { None, GoNext, GoPrev };
    PendingNavigation m_pendingNavigation = PendingNavigation::None;
    
    // Дополнительные действия контекстного меню
    void copyFilePath(const QString &fileName);
    void openFile(const QString &fileName);
    void openFolder(const QString &fileName);
    void deleteFile(const QString &fileName);
    void discardFileChanges(const QString &fileName);
    void showBlameStub();
    void runGitCommand(const QString &command, const QStringList &args, const QString &workingDir);
};
#endif // MAINWINDOW_H
