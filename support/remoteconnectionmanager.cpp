#include "remoteconnectionmanager.h"


RemoteConnectionJob::RemoteConnectionJob(QObject *parent) :
    QObject(parent)
{

}

RemoteConnectionJobQueue::RemoteConnectionJobQueue(RemoteConnection *connection) :
    remoteConnection(connection),
    idleJob(NULL),
    runningJob(NULL)
{

}

RemoteConnectionJobQueue::~RemoteConnectionJobQueue()
{
    foreach (AuthenticationEntry *entry, authenticationList)
        delete entry;
}

void RemoteConnectionJobQueue::start()
{
    reschedule();
}

void RemoteConnectionJobQueue::queue(RemoteConnectionJobRef job)
{
    jobQueue.append(job);
    reschedule();
}

void RemoteConnectionJobQueue::setIdleJob(RemoteConnectionJobRef job)
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

RemoteAuthentication *RemoteConnectionJobQueue::getRemoteAuthentication(
        const RemoteAuthenticationInfo &info, KeyStoreFinder *keyStoreFinder)
{
    foreach (AuthenticationEntry *entry, authenticationList) {
        if (entry->authenticationInfo == info)
            return entry->remoteAuthentication;
    }
    AuthenticationEntry *entry = new AuthenticationEntry(info, keyStoreFinder, remoteConnection);
    authenticationList.append(entry);

    return entry->remoteAuthentication;
}

void RemoteConnectionJobQueue::onJobDone(WP::err error)
{
    if (runningJob != idleJob)
        delete runningJob;
    runningJob = NULL;
    reschedule();
}

void RemoteConnectionJobQueue::startJob(RemoteConnectionJobRef job)
{
    runningJob = job.data();
    runningJob->run(this);
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

static ConnectionManager *sConnectionManager = NULL;

ConnectionManager *ConnectionManager::get()
{
    if (sConnectionManager == NULL)
        sConnectionManager = new ConnectionManager;
    return sConnectionManager;
}

ConnectionManager::ConnectionManager()
{

}

ConnectionManager::ConnectionEntry::ConnectionEntry(const RemoteConnectionInfo &info)
{
    this->info = info;
    this->jobQueue.setRemoteConnection(createRemoteConnection(info));
}

RemoteConnection *ConnectionManager::ConnectionEntry::createRemoteConnection(
        const RemoteConnectionInfo &info)
{
    if (info.getType() == RemoteConnectionInfo::kPlain)
        return new HTTPConnection(info.getUrl());
    if (info.getType() == RemoteConnectionInfo::kSecure)
        return new EncryptedPHPConnection(info.getUrl());

    return NULL;
}


RemoteConnectionJobQueue::AuthenticationEntry::AuthenticationEntry(
        const RemoteAuthenticationInfo &info, KeyStoreFinder *keyStoreFinder, RemoteConnection *remoteConnection) :
    remoteAuthentication(NULL)
{
    authenticationInfo = info;
    if (authenticationInfo.getType() == RemoteAuthenticationInfo::kSignature)
        remoteAuthentication = new SignatureAuthentication(remoteConnection, keyStoreFinder, info);
}
