#include "git.h"
#include <QDir>
#include <QSettings>
#include <QStandardPaths>

Git::Git(QObject *parent)
    : QObject(parent)
    , m_repositoryPath(QDir::currentPath())
    , m_gitPath("")
    , m_statusProcess(new QProcess(this))
    , m_diffProcess(new QProcess(this))
    , m_commitHistoryProcess(new QProcess(this))
    , m_fileContentProcess(new QProcess(this))
    , m_addFileProcess(new QProcess(this))
    , m_unstageFileProcess(new QProcess(this))
    , m_commitProcess(new QProcess(this))
    , m_branchesProcess(new QProcess(this))
    , m_currentBranchProcess(new QProcess(this))
    , m_remotesProcess(new QProcess(this))
    , m_remoteBranchesProcess(new QProcess(this))
    , m_tagsProcess(new QProcess(this))
    , m_stashesProcess(new QProcess(this))
    , m_gitParser(this)
{
    // Connect process signals
    connect(m_statusProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &Git::onStatusFinished);
    connect(m_statusProcess, &QProcess::errorOccurred, this, &Git::onProcessError);
    
    connect(m_diffProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &Git::onDiffFinished);
    connect(m_diffProcess, &QProcess::errorOccurred, this, &Git::onProcessError);
    
    connect(m_commitHistoryProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &Git::onCommitHistoryFinished);
    connect(m_commitHistoryProcess, &QProcess::errorOccurred, this, &Git::onProcessError);
    
    connect(m_fileContentProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &Git::onFileContentFinished);
    connect(m_fileContentProcess, &QProcess::errorOccurred, this, &Git::onProcessError);
    
    connect(m_addFileProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &Git::onAddFileFinished);
    connect(m_addFileProcess, &QProcess::errorOccurred, this, &Git::onProcessError);
    
    connect(m_unstageFileProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &Git::onUnstageFileFinished);
    connect(m_unstageFileProcess, &QProcess::errorOccurred, this, &Git::onProcessError);
    
    connect(m_commitProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &Git::onCommitFinished);
    connect(m_commitProcess, &QProcess::errorOccurred, this, &Git::onProcessError);

    // Branch operations
    connect(m_branchesProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &Git::onLocalBranchesFinished);
    connect(m_branchesProcess, &QProcess::errorOccurred, this, &Git::onProcessError);

    connect(m_currentBranchProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &Git::onCurrentBranchFinished);
    connect(m_currentBranchProcess, &QProcess::errorOccurred, this, &Git::onProcessError);

    connect(m_remotesProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &Git::onRemotesFinished);
    connect(m_remotesProcess, &QProcess::errorOccurred, this, &Git::onProcessError);

    connect(m_remoteBranchesProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &Git::onRemoteBranchesFinished);
    connect(m_remoteBranchesProcess, &QProcess::errorOccurred, this, &Git::onProcessError);

    connect(m_tagsProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &Git::onTagsFinished);
    connect(m_tagsProcess, &QProcess::errorOccurred, this, &Git::onProcessError);

    connect(m_stashesProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &Git::onStashesFinished);
    connect(m_stashesProcess, &QProcess::errorOccurred, this, &Git::onProcessError);
}

// ... (rest of the implementation remains the same)

void Git::setRepositoryPath(const QString &path)
{
    m_repositoryPath = path;
}

QString Git::repositoryPath() const
{
    return m_repositoryPath;
}

void Git::setGitPath(const QString &path)
{
    m_gitPath = path;
}

QString Git::gitPath() const
{
    if (m_gitPath.isEmpty()) {
        return "git";
    }
    return m_gitPath;
}

QString Git::getGitExecutable() const
{
    if (!m_gitPath.isEmpty()) {
        return m_gitPath;
    }

    // Try to find git in settings
    QSettings gitSettings("CommitCraft", "Settings");
    QString gitPath = gitSettings.value("gitPath", "").toString();

    if (!gitPath.isEmpty()) {
        return gitPath;
    }

    // Default to "git" and let the system find it in PATH
    return "git";
}

void Git::setupProcess(QProcess *process, const QStringList &arguments)
{
    process->setProgram(getGitExecutable());
    process->setArguments(arguments);
    process->setWorkingDirectory(m_repositoryPath);
}

void Git::getStatus()
{
    setupProcess(m_statusProcess, QStringList() << "status" << "--porcelain");
    m_statusProcess->start();
}

void Git::getDiff(const QString &fileName)
{
    // Используем "git diff HEAD" чтобы получить diff между HEAD и рабочей директорией.
    // Это важно для staged файлов: обычный "git diff" показывает diff между индексом
    // и рабочей директорией, что для staged файлов может быть пустым.
    setupProcess(m_diffProcess, QStringList() << "diff" << "HEAD" << "--" << fileName);
    m_diffProcess->start();
}

void Git::getDiffStaged(const QString &fileName)
{
    // Diff между HEAD и staged (индексом): git diff --cached HEAD
    setupProcess(m_diffProcess, QStringList() << "diff" << "--cached" << "HEAD" << "--" << fileName);
    m_diffProcess->start();
}

void Git::getDiffWorkingTree(const QString &fileName)
{
    // Diff между staged (индексом) и working tree: git diff -- file
    setupProcess(m_diffProcess, QStringList() << "diff" << "--" << fileName);
    m_diffProcess->start();
}

void Git::getCommitHistory()
{
    setupProcess(m_commitHistoryProcess,
                 QStringList() << "log" << "--pretty=format:%H|%an|%ad|%s" << "--date=short");
    m_commitHistoryProcess->start();
}

void Git::getFileContent(const QString &fileName, bool staged)
{
    if (staged) {
        // For staged content, we need to get it from git
        setupProcess(m_fileContentProcess,
                     QStringList() << "show" << QString("HEAD:%1").arg(fileName));
    } else {
        // For current content, read from file system (this should be handled differently)
        // For now, we'll emit an empty signal and let the caller handle file system reading
        emit fileContentReady("", staged);
        return;
    }
    m_fileContentProcess->start();
}

void Git::addFile(const QString &fileName)
{
    QString absoluteFilePath = QDir(m_repositoryPath).absoluteFilePath(fileName);
    setupProcess(m_addFileProcess, QStringList() << "add" << absoluteFilePath);
    m_addFileProcess->start();
}

void Git::unstageFile(const QString &fileName)
{
    QString absoluteFilePath = QDir(m_repositoryPath).absoluteFilePath(fileName);
    setupProcess(m_unstageFileProcess, QStringList() << "reset" << "HEAD" << absoluteFilePath);
    m_unstageFileProcess->start();
}

void Git::commit(const QString &message, bool amend)
{
    QStringList args;
    if (amend) {
        args << "commit" << "--amend" << "-m" << message;
    } else {
        args << "commit" << "-m" << message;
    }
    setupProcess(m_commitProcess, args);
    m_commitProcess->start();
}

void Git::onStatusFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitStatus == QProcess::NormalExit && exitCode == 0) {
        QString output = m_statusProcess->readAllStandardOutput();
        emit statusReady(output);
    } else {
        QString msg = m_statusProcess->readAllStandardError();
        emit error(QString("Failed to execute git status: %1").arg(msg));
    }
}

void Git::onDiffFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitStatus == QProcess::NormalExit && exitCode == 0) {
        QString output = m_diffProcess->readAllStandardOutput();
        emit diffReady(output);
        
        // Example of how to use GitParser with the diff output
        // QList<Hunk> hunks = m_gitParser.parseDiffOutput(output);
        // You can now work with the parsed hunks
        
    } else {
        QString msg = m_diffProcess->readAllStandardError();
        emit error(QString("Failed to execute git diff: %1").arg(msg));
    }
}

void Git::onCommitHistoryFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitStatus == QProcess::NormalExit && exitCode == 0) {
        QString output = m_commitHistoryProcess->readAllStandardOutput();
        QStringList lines = output.split('\n', Qt::SkipEmptyParts);

        QList<QList<QString>> commits;
        for (const QString &line : lines) {
            QStringList parts = line.split('|');
            if (parts.size() >= 4) {
                // Format the hash to show only first 8 characters
                if (!parts[0].isEmpty()) {
                    parts[0] = parts[0].left(8);
                }
                commits.append(parts);
            }
        }
        emit commitHistoryReady(commits);
    } else {
        QString msg = m_commitHistoryProcess->readAllStandardError();
        emit error(QString("Failed to execute git log: %1").arg(msg));
    }
}

void Git::onFileContentFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitStatus == QProcess::NormalExit && exitCode == 0) {
        QString output = m_fileContentProcess->readAllStandardOutput();
        emit fileContentReady(output, true); // staged = true since we're getting from git show HEAD:
    } else {
        QString msg = m_fileContentProcess->readAllStandardError();
        emit error(QString("Failed to execute git show: %1").arg(msg));
    }
}

void Git::onAddFileFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitStatus == QProcess::NormalExit && exitCode == 0) {
        emit addFileReady(true, "File added successfully");
    } else {
        QString msg = m_addFileProcess->readAllStandardError();
        emit addFileReady(false, QString("Failed to execute git add: %1").arg(msg));
        emit error(QString("Failed to execute git add: %1").arg(msg));
    }
}

void Git::onUnstageFileFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitStatus == QProcess::NormalExit && exitCode == 0) {
        emit unstageFileReady(true, "File unstaged successfully");
    } else {
        QString msg = m_unstageFileProcess->readAllStandardError();
        emit unstageFileReady(false, QString("Failed to execute git reset: %1").arg(msg));
        emit error(QString("Failed to execute git reset: %1").arg(msg));
    }
}

void Git::onCommitFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitStatus == QProcess::NormalExit && exitCode == 0) {
        emit commitReady(true, "Commit successful");
    } else {
        QString error = m_commitProcess->readAllStandardError();
        emit commitReady(false, QString("Failed to execute git commit: %1").arg(error));
    }
}

// ==========================================
// Branch Operations Implementation
// ==========================================

void Git::getLocalBranches()
{
    if (m_repositoryPath.isEmpty()) {
        emit error("Repository path is empty");
        return;
    }

    QStringList args;
    args << "branch" << "--format=%(refname:short)";
    setupProcess(m_branchesProcess, args);
    m_branchesProcess->start(getGitExecutable(), args);
}

void Git::getRemotes()
{
    if (m_repositoryPath.isEmpty()) {
        emit error("Repository path is empty");
        return;
    }

    QStringList args;
    args << "remote";
    setupProcess(m_remotesProcess, args);
    m_remotesProcess->start(getGitExecutable(), args);
}

void Git::getRemoteBranches(const QString &remote)
{
    if (m_repositoryPath.isEmpty()) {
        emit error("Repository path is empty");
        return;
    }

    QStringList args;
    args << "branch" << "-r" << "--format=%(refname:short)";
    if (!remote.isEmpty()) {
        args << remote;
    }
    setupProcess(m_remoteBranchesProcess, args);
    m_remoteBranchesProcess->start(getGitExecutable(), args);
}

void Git::getTags()
{
    if (m_repositoryPath.isEmpty()) {
        emit error("Repository path is empty");
        return;
    }

    QStringList args;
    args << "tag" << "--list";
    setupProcess(m_tagsProcess, args);
    m_tagsProcess->start(getGitExecutable(), args);
}

void Git::getStashes()
{
    if (m_repositoryPath.isEmpty()) {
        emit error("Repository path is empty");
        return;
    }

    QStringList args;
    args << "stash" << "list" << "--format=%gd: %s";
    setupProcess(m_stashesProcess, args);
    m_stashesProcess->start(getGitExecutable(), args);
}

void Git::onLocalBranchesFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitStatus == QProcess::NormalExit && exitCode == 0) {
        QString output = m_branchesProcess->readAllStandardOutput();
        QList<QString> branches = output.split('\n', Qt::SkipEmptyParts);
        
        // Clean up branch names (remove * and whitespace)
        for (auto &branch : branches) {
            branch = branch.trimmed();
        }
        
        // Get current branch in parallel
        m_currentBranchProcess->start(getGitExecutable(), {"rev-parse", "--abbrev-ref", "HEAD"});
        
        // Store branches temporarily until we get current branch
        m_currentBranchesList = branches;
    } else {
        QString errorMsg = m_branchesProcess->readAllStandardError();
        emit localBranchesReady({}, "");
        emit error(QString("Failed to get local branches: %1").arg(errorMsg));
    }
}

void Git::onCurrentBranchFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitStatus == QProcess::NormalExit && exitCode == 0) {
        QString currentBranch = m_currentBranchProcess->readAllStandardOutput().trimmed();
        emit localBranchesReady(m_currentBranchesList, currentBranch);
    } else {
        // If we can't get current branch, still emit branches with empty current
        emit localBranchesReady(m_currentBranchesList, "");
    }
}

void Git::onRemotesFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitStatus == QProcess::NormalExit && exitCode == 0) {
        QString output = m_remotesProcess->readAllStandardOutput();
        QList<QString> remotes = output.split('\n', Qt::SkipEmptyParts);
        
        for (auto &remote : remotes) {
            remote = remote.trimmed();
        }
        
        emit remotesReady(remotes);
    } else {
        QString errorMsg = m_remotesProcess->readAllStandardError();
        emit remotesReady({});
        emit error(QString("Failed to get remotes: %1").arg(errorMsg));
    }
}

void Git::onRemoteBranchesFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitStatus == QProcess::NormalExit && exitCode == 0) {
        QString output = m_remoteBranchesProcess->readAllStandardOutput();
        QList<QString> branches = output.split('\n', Qt::SkipEmptyParts);
        
        for (auto &branch : branches) {
            branch = branch.trimmed();
        }
        
        emit remoteBranchesReady("", branches);
    } else {
        QString errorMsg = m_remoteBranchesProcess->readAllStandardError();
        emit remoteBranchesReady("", {});
        emit error(QString("Failed to get remote branches: %1").arg(errorMsg));
    }
}

void Git::onTagsFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitStatus == QProcess::NormalExit && exitCode == 0) {
        QString output = m_tagsProcess->readAllStandardOutput();
        QList<QString> tags = output.split('\n', Qt::SkipEmptyParts);
        
        for (auto &tag : tags) {
            tag = tag.trimmed();
        }
        
        emit tagsReady(tags);
    } else {
        QString errorMsg = m_tagsProcess->readAllStandardError();
        emit tagsReady({});
        emit error(QString("Failed to get tags: %1").arg(errorMsg));
    }
}

void Git::onStashesFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitStatus == QProcess::NormalExit && exitCode == 0) {
        QString output = m_stashesProcess->readAllStandardOutput();
        QList<QString> stashes = output.split('\n', Qt::SkipEmptyParts);
        
        for (auto &stash : stashes) {
            stash = stash.trimmed();
        }
        
        emit stashesReady(stashes);
    } else {
        QString errorMsg = m_stashesProcess->readAllStandardError();
        emit stashesReady({});
        emit error(QString("Failed to get stashes: %1").arg(errorMsg));
    }
}

void Git::onProcessError(QProcess::ProcessError err)
{
    QProcess *process = qobject_cast<QProcess *>(sender());
    if (!process)
        return;

    QString errorMessage;
    switch (err) {
    case QProcess::FailedToStart:
        errorMessage = "Failed to start Git process. Check Git path in settings.";
        break;
    case QProcess::Crashed:
        errorMessage = "Git process crashed.";
        break;
    case QProcess::Timedout:
        errorMessage = "Git process timed out.";
        break;
    case QProcess::WriteError:
        errorMessage = "Error writing to Git process.";
        break;
    case QProcess::ReadError:
        errorMessage = "Error reading from Git process.";
        break;
    default:
        errorMessage = "Unknown Git process error.";
        break;
    }

    emit error(errorMessage);
}
