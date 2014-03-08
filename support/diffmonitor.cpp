#include "diffmonitor.h"


DiffMonitor::DiffMonitor(QObject *parent) :
    QObject(parent),
    database(NULL)
{

}

void DiffMonitor::startWatching(DatabaseInterface *database)
{
    if (this->database != NULL)
        this->database->disconnect(this);
    this->database = database;
    connect(database, SIGNAL(newCommits(QString,QString)), this, SLOT(onNewCommits(QString,QString)));
}

void DiffMonitor::addWatcher(DiffMonitorWatcher *watcher)
{
    watchers.append(watcher);
}

bool DiffMonitor::removeWatcher(DiffMonitorWatcher *watcher)
{
    int index = watchers.indexOf(watcher);
    if (index < 0)
        return false;
    watchers.removeAt(index);
    return true;
}

void DiffMonitor::onNewCommits(const QString &startCommit, const QString &endCommit)
{
    DatabaseDiff diff;
    if (database->getDiff(startCommit, endCommit, diff) != WP::kOk)
        return;
    foreach (DiffMonitorWatcher *watcher, watchers)
        watcher->onNewDiffs(diff);
}
