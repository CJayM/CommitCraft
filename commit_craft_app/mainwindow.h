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
#include <QFileSystemModel>
#include <QStandardItemModel>
#include <QAction>
#include <QListView>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>

class QTreeView;

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
class RepositoryDelegate;
class SubmoduleModel;

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
    void editGitignore();
    void openRepository();
    void onCloneRepository();
    void onCloneFinished(bool success, const QString &message);
    void refreshGitStatus();
    void onGitStatusFinished(const QString &output);
    void onGitDiffReady(const QString &output);
    void onGitCommitHistoryReady(const QList<QList<QString>> &commits);
    void onCreateBranch();
    void onStageSelectedPatch(const QString &fileName, const QString &patch);
    void onRevertSelectedPatch(const QString &fileName, const QString &patch);
    void onGitCommitFinished(bool success, const QString &message);
    void onGitError(const QString &error);
    void showFileContextMenu(const QPoint &pos);
    void showStagedFileContextMenu(const QPoint &pos);
    void showCommitHistoryContextMenu(const QPoint &pos);
    void showRepositoryContextMenu(const QPoint &pos);
    void addSelectedFile();
    void unstageSelectedFile();
    void unstageSelectedFiles(const QStringList &files);
    void stageSelectedFiles(const QStringList &files);
    void deleteSelectedFiles(const QStringList &files);
    void stashSelectedFiles(const QStringList &files);
    void commitChanges();
    void onFileTableSelectionChanged();
    void onStagedFileTableSelectionChanged();
    void toggleLeftPanel(bool visible);
    void toggleRepositoryPanel(bool visible);
    void toggleBranchesPanel(bool visible);
    void toggleFilesPanel(bool visible);
    void toggleTopPanel(bool visible);
    void toggleRightPanel(bool visible);
    void onAmendCheckBoxChanged(int state);
    void updateCommitButtonState();
    void synchronizeZoom(int zoom);
    void navigateToNextHunk();
    void navigateToPrevHunk();
    void selectNextFile();
    void selectPrevFile();
    void showAboutDialog();
    void stageSelectedFilesHotkey();
    void unstageSelectedFilesHotkey();
    void clearSelection();
    void onPushReady(bool success, const QString &message);
    void onPullReady(bool success, const QString &message);
    void onFetchReady(bool success, const QString &message);
    void onAddRemoteReady(bool success, const QString &message);
    void onRemoveRemoteReady(bool success, const QString &message);
    void onRenameRemoteReady(bool success, const QString &message);
    void onRemotesReady(const QList<QString> &remotes);
    void onCheckoutCommitReady(bool success, const QString &message);
    void onRevertCommitReady(bool success, const QString &message);
    void onCherryPickReady(bool success, const QString &message);
    void onRebaseReady(bool success, const QString &message);
    void onMergeReady(bool success, const QString &message);

    // Submodule slots
    void onSubmodulesReady(const QList<QStringList> &submodules);
    void onSubmoduleInitReady(bool success, const QString &message);
    void onSubmoduleUpdateReady(bool success, const QString &message);
    void onSubmoduleSyncReady(bool success, const QString &message);

    /// Обновляет состояние кнопок навигации (Prev/Next Hunk)
    void updateNavigationButtonsState();

    // Фильтрация по директории
    void onDirTreeSelectionChanged(const QModelIndex &current, const QModelIndex &previous);
    void showDirTreeContextMenu(const QPoint &pos);

private:
    void saveSplitterState();
    void restoreSplitterState();
    bool isGitRepository(const QString &path);
    QString getFileContent(const QString &fileName, bool staged);
    QByteArray getFileRawData(const QString &fileName, bool staged);
    void updateDiffPanel(const QString &fileName);

    Ui::MainWindow *ui;
    QSettings *settings;
    SettingsDialog *settingsDialog;
    QString repositoryPath;
    DiffEditor *diffEditor;
    FileModel *unstagedFilesModel;
    FileModel *stagedFilesModel;
    SubmoduleModel *submoduleModel;
    CommitHistoryModel *commitHistoryModel;
    CommitItemDelegate *commitItemDelegate;
    Git *git;

    // Отслеживание последнего выбранного файла и источника
    QString m_lastSelectedFileName;
    enum class SelectionSource { Unstaged, Staged };
    SelectionSource m_lastSelectionSource = SelectionSource::Unstaged;
    SelectionSource m_pushPullSource = SelectionSource::Unstaged;

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
    
    // Для повторного checkout с stash
    QString m_lastCheckoutBranch;

    // Для клонирования
    QString m_lastCloneDestination;
    
    // Дополнительные действия контекстного меню
    void copyFilePath(const QString &fileName);
    void openFile(const QString &fileName);
    void openFolder(const QString &fileName);
    void deleteFile(const QString &fileName);
    void discardFileChanges(const QString &fileName);
    void showBlameStub();
    void runGitCommand(const QString &command, const QStringList &args, const QString &workingDir);

    // Данные для фильтрации по директории
    QList<QPair<QString, QString>> m_allUnstagedFiles;
    QList<QPair<QString, QString>> m_allStagedFiles;
    QStringList m_remoteList;
    QString m_selectedDirectory;
    QStandardItemModel *m_repositoryModel;
    QSortFilterProxyModel *m_repositoryFilterModel;
    RepositoryDelegate *m_repositoryDelegate;
    QStandardItemModel *m_dirTreeModel;
    QTreeView *m_dirTreeView;
    void rebuildDirectoryTree();
    void applyDirectoryFilter();

    // Недавние репозитории
    void openRepositoryPath(const QString &path);
    void addRecentRepository(const QString &path);
    void updateRecentRepositoriesMenu();

    QStringList m_recentRepositories;
    QAction *m_recentSeparator = nullptr;
    QMenu *m_recentReposMenu = nullptr;
    void updateRepositoryList();
};
#endif // MAINWINDOW_H
