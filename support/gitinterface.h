#ifndef GITINTERFACE_H
#define GITINTERFACE_H


#include <QString>
#include <git2.h>

#include "databaseinterface.h"


class RemoteConnection;

class GitInterface : public DatabaseInterface
{
public:
    GitInterface();
    ~GitInterface();

    WP::err setTo(const QString &path, bool create = true);
    void unSet();
    QString path();

    WP::err setBranch(const QString &branch, bool createBranch = true);
    QString branch() const;

    WP::err write(const QString &path, const QByteArray &data);
    WP::err remove(const QString& path);
    WP::err commit();
    WP::err read(const QString &path, QByteArray &data) const;

    WP::err writeObject(const char *data, int size);
    WP::err writeFile(const QString& hash, const char *data, int size);

    QString getTip() const;
    WP::err updateTip(const QString &commit);

    QStringList listFiles(const QString &path) const;
    QStringList listDirectories(const QString &path) const;

    QString getLastSyncCommit(const QString remoteName, const QString &remoteBranch) const;
    WP::err updateLastSyncCommit(const QString remoteName, const QString &remoteBranch, const QString &uid) const;
    WP::err exportPack(QByteArray &pack, const QString &startCommit, const QString &endCommit, const QString &ignoreCommit, int format = -1) const;
    WP::err importPack(const QByteArray &pack, const QString &baseCommit, const QString &endCommit, int format = -1);

    WP::err getDiff(const QString &baseCommit, const QString &endCommit, DatabaseDiff &databaseDiff);

private:
    QStringList listDirectoryContent(const QString &path, int type = -1) const;

    git_commit *getTipCommit() const;
    git_tree *getTipTree() const;
    //! returns the tree for the given directory
    git_tree *getDirectoryTree(const QString &dirPath) const;

    QString removeFilename(QString &path) const;

    git_tree *getCommitTree(const QString &commitHash) const;

private:
    static bool sGitThreadsHaveBeeInit;

    QString repositoryPath;
    git_repository *repository;
    git_odb *objectDatabase;
    QString currentBranch;

    git_oid newRootTreeOid;
};

#endif // GITINTERFACE_H
