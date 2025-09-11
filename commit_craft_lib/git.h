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
    void getCommitHistory();
    void getFileContent(const QString &fileName, bool staged);
    void addFile(const QString &fileName);
    void unstageFile(const QString &fileName);
    void commit(const QString &message, bool amend = false);
    
signals:
    // Git operation results
    void statusReady(const QString &output);
    void diffReady(const QString &output);
    void commitHistoryReady(const QList<QList<QString>> &commits);
    void fileContentReady(const QString &content, bool staged);
    void commitReady(bool success, const QString &message);
    void error(const QString &error);
    
private slots:
    void onStatusFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onDiffFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onCommitHistoryFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onFileContentFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onAddFileFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onUnstageFileFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onCommitFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);
    
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
    
    // Git parser
    GitParser m_gitParser;
    
    // Helper methods
    QString getGitExecutable() const;
    void setupProcess(QProcess *process, const QStringList &arguments);
};

#endif // GIT_H