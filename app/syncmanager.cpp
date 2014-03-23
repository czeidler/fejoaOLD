#include "syncmanager.h"

#include "protocolparser.h"
#include "remoteauthentication.h"


const char *kWatchBranchesStanza = "watch_branches";


class WatchBranchesStanza : public OutStanza {
public:
    WatchBranchesStanza(const QList<DatabaseInterface*> &branches) :
        OutStanza(kWatchBranchesStanza)
    {

        foreach (DatabaseInterface *database, branches) {
            OutStanza *item = new OutStanza("branch");
            item->addAttribute("branch", database->branch());
            item->addAttribute("tip", database->getTip());
        }
    }
};


SyncManager::SyncManager(RemoteDataStorage *remoteDataStorage, QObject *parent) :
    RemoteConnectionJob(parent),
    remoteDataStorage(remoteDataStorage),
    authentication(NULL),
    remoteConnection(NULL),
    serverReply(NULL),
    jobQueue(NULL),
    watching(false)
{
}

SyncManager::~SyncManager()
{
    abort();
}

WP::err SyncManager::keepSynced(DatabaseInterface *branch)
{
    foreach (const RemoteSyncRef &ref, syncEntries) {
        if (ref->getDatabase() == branch)
            return WP::kOk;
    }

    RemoteSyncRef entry(new RemoteSync(branch, remoteDataStorage, this));
    syncEntries.append(entry);

    if (watching)
        restartWatching();
    return WP::kOk;
}

void SyncManager::run(RemoteConnectionJobQueue *jobQueue)
{
    this->jobQueue = jobQueue;
    authentication = jobQueue->getRemoteAuthentication(
                remoteDataStorage->getRemoteAuthenticationInfo(), remoteDataStorage->getKeyStoreFinder());
    remoteConnection = jobQueue->getRemoteConnection();

    startWatching();
}

void SyncManager::abort()
{
    watching = false;

    if (serverReply != NULL) {
        serverReply->abort();
        serverReply = NULL;
    }
}

void SyncManager::startWatching()
{
    if (watching)
        return;

    watching = true;
    emit syncStarted();

    if (authentication->isVerified())
        remoteAuthenticated(WP::kOk);
    else {
        connect(authentication, SIGNAL(authenticationAttemptFinished(WP::err)),
                this, SLOT(remoteAuthenticated(WP::err)));
        authentication->login();
    }
}

void SyncManager::restartWatching()
{
    abort();
    startWatching();
}

void SyncManager::syncBranches(const QStringList &branches)
{
    foreach (const RemoteSyncRef &entry, syncEntries) {
        if (!branches.contains(entry->getDatabase()->branch()))
            continue;
        jobQueue->queue(entry);
    }
}

void SyncManager::handleConnectionError(WP::err error)
{
    abort();

    authentication->logout();
    emit connectionError();

    serverReply = NULL;
}

void SyncManager::remoteAuthenticated(WP::err error)
{
    if (error != WP::kOk) {
        handleConnectionError(error);
        return;
    }

    QByteArray outData;
    ProtocolOutStream outStream(&outData);

    IqOutStanza *iqStanza = new IqOutStanza(kGet);
    outStream.pushStanza(iqStanza);

    OutStanza *watchStanza = new OutStanza(kWatchBranchesStanza);
    outStream.pushChildStanza(watchStanza);

    bool first = true;
    foreach (const RemoteSyncRef &entry, syncEntries) {
        OutStanza *item = new OutStanza("branch");
        item->addAttribute("branch", entry->getDatabase()->branch());
        item->addAttribute("tip", entry->getDatabase()->getTip());
        if (first) {
            outStream.pushChildStanza(item);
            first = false;
        } else
            outStream.pushStanza(item);
    }

    outStream.flush();

    if (serverReply != NULL)
        serverReply->disconnect();
    serverReply = remoteConnection->send(outData);
    connect(serverReply, SIGNAL(finished(WP::err)), this, SLOT(watchReply(WP::err)));
}


class WatchHandler : public InStanzaHandler {
public:
    WatchHandler() :
        InStanzaHandler(kWatchBranchesStanza)
    {
    }

    bool handleStanza(const QXmlStreamAttributes &attributes)
    {
        if (!attributes.hasAttribute("status"))
            return false;
        status = attributes.value("status").toString();
        return true;
    }

public:
    QString status;
};


class WatchItemsChangedHandler : public InStanzaHandler {
public:
    WatchItemsChangedHandler() :
        InStanzaHandler("branch", true)
    {
    }

    bool handleStanza(const QXmlStreamAttributes &attributes)
    {
        if (!attributes.hasAttribute("branch"))
            return false;

        branches.append(attributes.value("branch").toString());
        return true;
    }

public:
    QStringList branches;
};


void SyncManager::watchReply(WP::err error)
{
    if (error != WP::kOk) {
        handleConnectionError(error);
        return;
    }
    if (serverReply == NULL)
        return;

    QByteArray data = serverReply->readAll();
    serverReply = NULL;

    IqInStanzaHandler iqHandler(kResult);
    WatchHandler *watchHandler = new WatchHandler();
    WatchItemsChangedHandler *itemsChangedHandler = new WatchItemsChangedHandler();
    iqHandler.addChildHandler(watchHandler);
    watchHandler->addChildHandler(itemsChangedHandler);

    ProtocolInStream inStream(data);
    inStream.addHandler(&iqHandler);

    inStream.parse();

    if (!watchHandler->hasBeenHandled()) {
        handleConnectionError(WP::kError);
        return;
    }

    if (watchHandler->status == "server_timeout") {
        restartWatching();
        return;
    } else if (watchHandler->status == "update") {
        syncBranches(itemsChangedHandler->branches);
        return;
    } else // try again
        restartWatching();
}
