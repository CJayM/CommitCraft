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
    , m_checkoutProcess(new QProcess(this))
    , m_branchModifyProcess(new QProcess(this))
    , m_remoteProcess(new QProcess(this))
    , m_stashProcess(new QProcess(this))
    , m_createStashProcess(new QProcess(this))
    , m_submoduleProcess(new QProcess(this))
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

    // Branch modification
    connect(m_checkoutProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &Git::onCheckoutFinished);
    connect(m_checkoutProcess, &QProcess::errorOccurred, this, &Git::onProcessError);

    connect(m_branchModifyProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &Git::onCreateBranchFinished); // Универсальный слот для всех операций модификации
    connect(m_branchModifyProcess, &QProcess::errorOccurred, this, &Git::onProcessError);

    // Remote operations
    connect(m_remoteProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &Git::onFetchFinished); // По умолчанию fetch, будем проверять property
    connect(m_remoteProcess, &QProcess::errorOccurred, this, &Git::onProcessError);

    // Stash operations
    connect(m_stashProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &Git::onApplyStashFinished); // По умолчанию apply, будем проверять property
    connect(m_stashProcess, &QProcess::errorOccurred, this, &Git::onProcessError);

    connect(m_createStashProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &Git::onCreateStashFinished);
    connect(m_createStashProcess, &QProcess::errorOccurred, this, &Git::onProcessError);

    // Submodule operations
    connect(m_submoduleProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &Git::onSubmodulesFinished); // По умолчанию getSubmodules
    connect(m_submoduleProcess, &QProcess::errorOccurred, this, &Git::onProcessError);
}