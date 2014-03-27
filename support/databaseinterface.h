#ifndef DATABASEINTERFACE_H
#define DATABASEINTERFACE_H

#include <QObject>
#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QVector>

#include "error_codes.h"

class DatabaseDir {
public:
    DatabaseDir(const QString &dirName = "");
    ~DatabaseDir();

    bool isEmpty() const;

    const DatabaseDir *findDirectory(const QString &path) const;
    void addPath(const QString &path);

    QStringList getChildDirectories() const;
    DatabaseDir *getChildDirectory(const QString dirName) const;

    QString directoryName;
    QList<DatabaseDir*> directories;
    QStringList files;
};

class DatabaseDiff {
public:
    DatabaseDir added;
    DatabaseDir modified;
    DatabaseDir removed;
};

class DatabaseInterface : public QObject {
    Q_OBJECT
public:
    DatabaseInterface(QObject *parent = NULL);
    virtual ~DatabaseInterface() {}

    virtual WP::err setTo(const QString &path, bool create = true) = 0;
    virtual void unSet() = 0;
    virtual QString path() = 0;

    virtual WP::err setBranch(const QString &branch,
                              bool createBranch = true) = 0;
    virtual QString branch() const = 0;

    virtual WP::err write(const QString& path, const QByteArray& data) = 0;
    virtual WP::err write(const QString& path, const QString& data);
    virtual WP::err remove(const QString& path) = 0;
    virtual WP::err commit() = 0;

    virtual WP::err read(const QString& path, QByteArray& data) const = 0;
    virtual WP::err read(const QString& path, QString& data) const;

    virtual QStringList listFiles(const QString &path) const = 0;
    virtual QStringList listDirectories(const QString &path) const = 0;

    virtual QString getTip() const = 0;
    virtual WP::err updateTip(const QString &commit) = 0;

    // sync
    virtual QString getLastSyncCommit(const QString remoteName, const QString &remoteBranch) const = 0;
    virtual WP::err updateLastSyncCommit(const QString remoteName, const QString &remoteBranch, const QString &uid) const = 0;
    virtual WP::err exportPack(QByteArray &pack, const QString &startCommit, const QString &endCommit, const QString &ignoreCommit, int format = -1) const = 0;
    //! import pack, tries to merge and update the tip
    virtual WP::err importPack(const QByteArray &pack, const QString &baseCommit, const QString &endCommit, int format = -1) = 0;

    // diff
    virtual WP::err getDiff(const QString &baseCommit, const QString &endCommit, DatabaseDiff &diff) = 0;

signals:
    void newCommits(const QString &startCommit, const QString &endCommit);
};


class DatabaseFactory {
public:
    static WP::err open(const QString &databasePath, const QString &branch, DatabaseInterface **database);
};

#endif // DATABASEINTERFACE_H
