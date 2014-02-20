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
