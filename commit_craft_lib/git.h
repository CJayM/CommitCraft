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
    void unstageFile(const QString &fileName);
    void commit(const QString &message, bool amend = false);

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

    // Remote operations
    void fetchRemote(const QString &remote);
    void pruneRemote(const QString &remote);
    void getRemoteUrl(const QString &remote);
    
signals:
    // Git operation results
    void statusReady(const QString &output);
    void diffReady(const QString &output);
    void commitHistoryReady(const QList<QList<QString>> &commits);
    void fileContentReady(const QString &content, bool staged);
    void commitReady(bool success, const QString &message);
    void addFileReady(bool success, const QString &message);
    void unstageFileReady(bool success, const QString &message);
    void error(const QString &error);

    // Branch modification results
    void checkoutReady(bool success, const QString &message);
    void branchCreated(bool success, const QString &message);
    void branchDeleted(bool success, const QString &message);
    void branchRenamed(bool success, const QString &message);

    // Remote signals
    void fetchReady(bool success, const QString &message);
    void pruneReady(bool success, const QString &message);
    void remoteUrlReady(const QString &remote, const QString &url);

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
    
    // Remote slots
    void onFetchFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onPruneFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onRemoteUrlFinished(int exitCode, QProcess::ExitStatus exitStatus);
    
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
    QProcess *m_branchModifyProcess; // Для create/delete/rename
    QProcess *m_remoteProcess;       // Для fetch/prune/url

    // Temporary storage for async operations
    QList<QString> m_currentBranchesList;
    QString m_currentRemoteName; // Для отслеживания remote в async вызовах

    // Git parser
    GitParser m_gitParser;
    
    // Helper methods
    QString getGitExecutable() const;
    void setupProcess(QProcess *process, const QStringList &arguments);
};

#endif // GIT_H