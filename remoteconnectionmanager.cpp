#include "remoteconnectionmanager.h"


RemoteConnectionJob::RemoteConnectionJob(const RemoteConnectionInfo &info) :
    remoteConnectionInfo(info)
{

}

RemoteConnectionInfo &RemoteConnectionJob::getRemoteConnectionInfo()
{
    return remoteConnectionInfo;
}

RemoteConnectionJobQueue::RemoteConnectionJobQueue(RemoteConnection *connection) :
    remoteConnection(connection),
    idleJob(NULL),
    runningJob(NULL)
{

}

RemoteConnectionJobQueue::~RemoteConnectionJobQueue()
{
    delete idleJob;
}

void RemoteConnectionJobQueue::start()
{
    reschedule();
}

WP::err RemoteConnectionJobQueue::queue(RemoteConnectionJob *job)
{
    jobQueue.append(job);
    reschedule();
}

void RemoteConnectionJobQueue::setIdleJob(RemoteConnectionJob *job)
{
    idleJob = job;
    reschedule();
}

void RemoteConnectionJobQueue::reschedule()
{
    // try to run a job from the queue
    if (jobQueue.size() != 0) {
        if (runningJob == idleJob) {
            idleJob->abort();
        }
        startJob(jobQueue.takeFirst());
        return;
    }
    // if no jobs in the queue start the idle job
    if (runningJob == NULL && idleJob != NULL) {
        startJob(idleJob);
        return;
    }
}

RemoteConnection *RemoteConnectionJobQueue::getRemoteConnection() const
{
    return remoteConnection;
}

void RemoteConnectionJobQueue::setRemoteConnection(RemoteConnection *value)
{
    remoteConnection = value;
}

void RemoteConnectionJobQueue::onJobDone(WP::err error)
{
    if (runningJob != idleJob)
        delete runningJob;
    runningJob = NULL;
    reschedule();
}

void RemoteConnectionJobQueue::startJob(RemoteConnectionJob *job)
{
    runningJob = idleJob;
    runningJob->run(remoteConnection);
    connect(runningJob, SIGNAL(jobDone(WP::err)), this, SLOT(onJobDone(WP::err)));
}


ConnectionManager::~ConnectionManager()
{
    foreach (ConnectionEntry *entry, connectionList)
        delete entry;
}

RemoteConnectionJobQueue *ConnectionManager::getConnectionJobQueue(const RemoteConnectionInfo &info)
{
    foreach (ConnectionEntry *entry, connectionList) {
        if (entry->info == info)
            return &entry->jobQueue;
    }
    ConnectionEntry *entry = new ConnectionEntry(info);
    connectionList.append(entry);

    return &entry->jobQueue;
}

RemoteConnectionInfo ConnectionManager::getDefaultConnectionFor(const QUrl &url)
{
    return getHTTPConnectionFor(url);
}

RemoteConnectionInfo ConnectionManager::getHTTPConnectionFor(const QUrl &url)
{
    RemoteConnectionInfo info;
    info.setUrl(url);
    info.setType(RemoteConnectionInfo::kPlain);
    return info;
}

RemoteConnectionInfo ConnectionManager::getEncryptedPHPConnectionFor(const QUrl &url)
{
    RemoteConnectionInfo info;
    info.setUrl(url);
    info.setType(RemoteConnectionInfo::kSecure);
    return info;
}

ConnectionManager::ConnectionEntry::ConnectionEntry(const RemoteConnectionInfo &info)
{
    this->info = info;
    this->jobQueue.setRemoteConnection(createRemoteConnection(info));
}

RemoteConnection *ConnectionManager::ConnectionEntry::createRemoteConnection(const RemoteConnectionInfo &info)
{
    if (info.getType() == RemoteConnectionInfo::kPlain)
        return new HTTPConnection(info.getUrl());
    if (info.getType() == RemoteConnectionInfo::kSecure)
        return new EncryptedPHPConnection(info.getUrl());

    return NULL;
}
