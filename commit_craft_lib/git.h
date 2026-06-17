#ifndef GIT_H
#define GIT_H

#include <QObject>
#include <QProcess>
#include <QList>
#include <QPair>
#include <QString>
#include "gitparser.h"

class Git : public QObject
{
    Q_OBJECT

public:
    explicit Git(QObject *parent = nullptr);
    
    // Git operations
    void setRepositoryPath(const QString &path);
    QString repositoryPath() const;
    
    void setGitPath(const QString &path);
    QString gitPath() const;
    
    // Git commands
    void getStatus();
    void getDiff(const QString &fileName);
    void getDiffStaged(const QString &fileName);
    void getDiffWorkingTree(const QString &fileName);
    void getCommitHistory();
    void getFileContent(const QString &fileName, bool staged);
    void addFile(const QString &fileName);
    void addFiles(const QStringList &fileNames);
    void unstageFile(const QString &fileName);
    void unstageFiles(const QStringList &fileNames);
    bool isFileTracked(const QString &fileName);
    void deleteFiles(const QStringList &fileNames);
    void commit(const QString &message, bool amend = false);

    bool applyPatchToIndex(const QString &patchFilePath);
    bool revertPatchInWorkingTree(const QString &patchFilePath);

    // Branch operations
    void getLocalBranches();
    void getRemotes();
    void getRemoteBranches(const QString &remote);
    void getTags();
    void getStashes();
    
    // Branch modification
    void checkoutBranch(const QString &branch, bool stashBeforeCheckout = false);
    void createBranch(const QString &name, const QString &fromRef = "");
    void deleteBranch(const QString &branch, bool force = false);
    void renameBranch(const QString &oldName, const QString &newName);
    void checkoutCommit(const QString &hash);
    void revertCommit(const QString &hash);
    void cherryPick(const QString &hash);
    void rebaseOnto(const QString &hash);
    void mergeCommit(const QString &hash, bool noFf = true);
    void mergeBranch(const QString &branch, bool noFf = true);

    // Remote operations
    void fetchRemote(const QString &remote);
    void pruneRemote(const QString &remote);
    void getRemoteUrl(const QString &remote);
    void pushRemote(const QString &remote = "", const QString &branch = "");
    void pullRemote(const QString &remote = "", const QString &branch = "");
    void addRemote(const QString &name, const QString &url);
    void removeRemote(const QString &name);
    void renameRemote(const QString &oldName, const QString &newName);

    // Stash operations
    void createStash(const QStringList &files, const QString &message);
    void applyStash(const QString &stashRef, bool drop = false);
    void dropStash(const QString &stashRef);
    void showStash(const QString &stashRef);
    
    // Clone operation
    void cloneRepository(const QString &url, const QString &destination);

    // Submodule operations
    void getSubmodules();
    void initSubmodule(const QString &path);
    void updateSubmodule(const QString &path, bool fetch = false);
    void syncSubmodule(const QString &path);
    void forEachSubmodule(const QString &command);
    
signals:
    // Git operation results
    void statusReady(const QString &output);
    void diffReady(const QString &output);
    void commitHistoryReady(const QList<QList<QString>> &commits);
    void fileContentReady(const QString &content, bool staged);
    void commitReady(bool success, const QString &message);
    void addFileReady(bool success, const QString &message);
    void unstageFileReady(bool success, const QString &message);
    void deleteFilesReady(bool success, const QString &message);
    void error(const QString &error);

    // Branch modification results
    void checkoutReady(bool success, const QString &message);
    void branchCreated(bool success, const QString &message);
    void branchDeleted(bool success, const QString &message);
    void branchRenamed(bool success, const QString &message);
    void checkoutCommitReady(bool success, const QString &message);
    void revertCommitReady(bool success, const QString &message);
    void cherryPickReady(bool success, const QString &message);
    void rebaseReady(bool success, const QString &message);
    void mergeReady(bool success, const QString &message);

    // Remote signals
    void fetchReady(bool success, const QString &message);
    void pruneReady(bool success, const QString &message);
    void remoteUrlReady(const QString &remote, const QString &url);
    void pushReady(bool success, const QString &message);
    void pullReady(bool success, const QString &message);
    void addRemoteReady(bool success, const QString &message);
    void removeRemoteReady(bool success, const QString &message);
    void renameRemoteReady(bool success, const QString &message);

    // Stash signals
    void stashCreated(bool success, const QString &message);
    void stashApplied(bool success, const QString &message);
    void stashDropped(bool success, const QString &message);
    void stashShown(const QString &diff);
    
    // Clone signal
    void cloneReady(bool success, const QString &message);

    // Submodule signals
    void submodulesReady(const QList<QStringList> &submodules); // name, path, url, branch, commit, head, dirty, uninitialized, missing
    void submoduleInitReady(bool success, const QString &message);
    void submoduleUpdateReady(bool success, const QString &message);
    void submoduleSyncReady(bool success, const QString &message);
    void submoduleForeachReady(bool success, const QString &output);

    // Branch operation results
    void localBranchesReady(const QList<QString> &branches, const QString &currentBranch);
    void remotesReady(const QList<QString> &remotes);
    void remoteBranchesReady(const QString &remote, const QList<QString> &branches);
    void tagsReady(const QList<QString> &tags);
    void stashesReady(const QList<QString> &stashes);
    
private slots:
    void onStatusFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onDiffFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onCommitHistoryFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onFileContentFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onAddFileFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onUnstageFileFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onDeleteFilesFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onCommitFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);

    // Branch operation slots
    void onLocalBranchesFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onCurrentBranchFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onRemotesFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onRemoteBranchesFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onTagsFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onStashesFinished(int exitCode, QProcess::ExitStatus exitStatus);

    // Branch modification slots
    void onCheckoutFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onCreateBranchFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onDeleteBranchFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onRenameBranchFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onCommitOpFinished(int exitCode, QProcess::ExitStatus exitStatus);
    
    // Remote slots
    void onFetchFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onPruneFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onRemoteUrlFinished(int exitCode, QProcess::ExitStatus exitStatus);
    
    // Stash slots
    void onCreateStashFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onApplyStashFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onDropStashFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onShowStashFinished(int exitCode, QProcess::ExitStatus exitStatus);
    
    // Clone slot
    void onCloneFinished(int exitCode, QProcess::ExitStatus exitStatus);

    // Submodule slots
    void onSubmodulesFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onSubmoduleInitFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onSubmoduleUpdateFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onSubmoduleSyncFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onSubmoduleForeachFinished(int exitCode, QProcess::ExitStatus exitStatus);
    
private:
    QString m_repositoryPath;
    QString m_gitPath;
    
    // Processes for different operations
    QProcess *m_statusProcess;
    QProcess *m_diffProcess;
    QProcess *m_commitHistoryProcess;
    QProcess *m_fileContentProcess;
    QProcess *m_addFileProcess;
    QProcess *m_unstageFileProcess;
    QProcess *m_deleteFilesProcess;
    QProcess *m_commitProcess;

    // Processes for branch operations
    QProcess *m_branchesProcess;
    QProcess *m_currentBranchProcess;
    QProcess *m_remotesProcess;
    QProcess *m_remoteBranchesProcess;
    QProcess *m_tagsProcess;
    QProcess *m_stashesProcess;

    // Process for branch modification
    QProcess *m_checkoutProcess;
    QProcess *m_commitOpProcess;     // Для commit операций (revert/cherry-pick/rebase/merge)
    QProcess *m_branchModifyProcess; // Для create/delete/rename
    QProcess *m_remoteProcess;       // Для fetch/prune/url
    QProcess *m_stashProcess;        // Для stash операций (apply/drop/show)
    QProcess *m_createStashProcess;  // Для создания stash
    QProcess *m_submoduleProcess;    // Для submodule операций
    QProcess *m_cloneProcess;        // Для clone операции

    // Temporary storage for async operations
    QList<QString> m_currentBranchesList;
    QString m_currentRemoteName; // Для отслеживания remote в async вызовах

    // Git parser
    GitParser m_gitParser;

public:
    // Helper methods
    QString getGitExecutable() const;

private:
    void setupProcess(QProcess *process, const QStringList &arguments);
};

#endif // GIT_H