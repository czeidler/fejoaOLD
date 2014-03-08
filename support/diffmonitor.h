#ifndef DIFFMONITOR_H
#define DIFFMONITOR_H

#include <QObject>

#include "databaseinterface.h"

class DiffMonitorWatcher {
public:
    virtual ~DiffMonitorWatcher() {}

    virtual void onNewDiffs(const DatabaseDiff &diff) = 0;
};

class DiffMonitor : public QObject
{
    Q_OBJECT
public:
    DiffMonitor(QObject *parent = 0);

    void startWatching(DatabaseInterface *database);

    void addWatcher(DiffMonitorWatcher *watcher);
    bool removeWatcher(DiffMonitorWatcher *watcher);

public slots:
    virtual void onNewCommits(const QString &startCommit, const QString &endCommit);

private:
    QList<DiffMonitorWatcher*> watchers;

    DatabaseInterface *database;
};

#endif // DIFFMONITOR_H
