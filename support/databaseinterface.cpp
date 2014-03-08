#include "databaseinterface.h"

#include "gitinterface.h"


WP::err DatabaseFactory::open(const QString &databasePath, const QString &branch, DatabaseInterface **database)
{
    *database = NULL;
    DatabaseInterface *gitDatabase = new GitInterface;
    WP::err error = gitDatabase->setTo(databasePath);
    if (error != WP::kOk) {
        delete gitDatabase;
        return error;
    }
    error = gitDatabase->setBranch(branch);
    if (error != WP::kOk) {
        delete gitDatabase;
        return error;
    }
    *database = gitDatabase;
    return WP::kOk;
}

DatabaseInterface::DatabaseInterface(QObject *parent) :
     QObject(parent)
{

}

WP::err DatabaseInterface::write(const QString &path, const QString &data)
{
    return write(path, data.toLatin1());
}

WP::err DatabaseInterface::read(const QString &path, QString &data) const
{
    QByteArray dataArray;
    WP::err status = read(path, dataArray);
    data = dataArray;
    return status;
}


DatabaseDir::DatabaseDir(const QString &dirName) :
    directoryName(dirName)
{
}

DatabaseDir::~DatabaseDir()
{
    foreach (DatabaseDir *dir, directories)
        delete dir;
}

const DatabaseDir *DatabaseDir::findDirectory(const QString &path) const
{
    QStringList parts = path.split("/");
    const DatabaseDir *currentDir = this;
    foreach (const QString part, parts) {
        currentDir = currentDir->getChildDirectory(part);
        if (currentDir == NULL)
            return NULL;
    }
    return currentDir;
}

void DatabaseDir::addPath(const QString &path)
{
    QStringList parts = path.split("/");
    DatabaseDir *currentDir = this;
    for (int i = 0; i < parts.size(); i++) {
        const QString part = parts.at(i);
        if (i == parts.size() - 1) {
            currentDir->files.append(part);
            return;
        }

        DatabaseDir *child = currentDir->getChildDirectory(part);
        if (child == NULL) {
            child = new DatabaseDir(part);
            currentDir->directories.append(child);
        }
        currentDir = child;
    }
}

QStringList DatabaseDir::getChildDirectories() const
{
    QStringList directoryList;
    foreach (DatabaseDir *childDirectory, directories)
        directoryList.append(childDirectory->directoryName);
    return directoryList;
}

DatabaseDir *DatabaseDir::getChildDirectory(const QString dirName) const
{
    foreach (DatabaseDir *databaseDir, directories) {
        if (databaseDir->directoryName == dirName)
            return databaseDir;
    }
    return NULL;
}
