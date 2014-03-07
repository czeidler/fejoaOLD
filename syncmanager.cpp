#include "syncmanager.h"

#include "protocolparser.h"
#include "remoteauthentication.h"


const char *kWathcBranchesStanza = "watch_branches";


class WatchBranchesStanza : public OutStanza {
public:
    WatchBranchesStanza(const QList<DatabaseInterface*> &branches) :
        OutStanza(kWathcBranchesStanza)
    {

        foreach (DatabaseInterface *database, branches) {
            OutStanza *item = new OutStanza("branch");
            item->addAttribute("branch", database->branch());
            item->addAttribute("tip", database->getTip());
        }
    }
};


SyncManager::SyncManager(RemoteDataStorage *remoteDataStorage, QObject *parent) :
    QObject(parent),
    remoteDataStorage(remoteDataStorage),
    authentication(remoteDataStorage->getRemoteAuthentication()),
    remoteConnection(remoteDataStorage->getRemoteConnection()),
    serverReply(NULL),
    watching(false)
{
}

SyncManager::~SyncManager()
{
    stopWatching();
}

WP::err SyncManager::keepSynced(DatabaseInterface *branch)
{
    SyncEntry *entry = new SyncEntry(this, branch, this);
    syncEntries.append(entry);

    if (watching)
        restartWatching();
    return WP::kOk;
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
    stopWatching();
    startWatching();
}

void SyncManager::stopWatching()
{
    watching = false;

    if (serverReply != NULL) {
        serverReply->abort();
        serverReply = NULL;
    }

    emit syncStopped();
}

void SyncManager::syncBranches(const QStringList &branches)
{
    foreach (SyncEntry *entry, syncEntries) {
        if (entry->isSyncing())
            continue;
        if (!branches.contains(entry->getDatabase()->branch()))
            continue;
        entry->sync();
    }
}

void SyncManager::handleConnectionError(WP::err error)
{
    stopWatching();

    authentication->logout();
    emit connectionError();
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

    OutStanza *watchStanza = new OutStanza(kWathcBranchesStanza);
    outStream.pushChildStanza(watchStanza);

    bool first = true;
    foreach (SyncEntry *entry, syncEntries) {
        if (entry->isSyncing())
            continue;
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
        InStanzaHandler(kWathcBranchesStanza)
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


SyncEntry::SyncEntry(SyncManager *syncManager, DatabaseInterface *database, QObject *parent) :
    QObject(parent),
    syncManager(syncManager),
    database(database),
    remoteSync(NULL)
{
}

WP::err SyncEntry::sync()
{
    if (isSyncing())
        return WP::kOk;
    oldTip = database->getTip();
    remoteSync = new RemoteSync(database, syncManager->remoteDataStorage, this);
    connect(remoteSync, SIGNAL(syncFinished(WP::err)), this, SLOT(onSyncFinished(WP::err)));
    WP::err error = remoteSync->sync();
    if (error != WP::kOk) {
        delete remoteSync;
        remoteSync = NULL;
    }
    return error;
}

bool SyncEntry::isSyncing()
{
    return remoteSync != NULL ? true : false;
}

const DatabaseInterface *SyncEntry::getDatabase() const
{
    return database;
}

void SyncEntry::onSyncFinished(WP::err error)
{
    remoteSync->deleteLater();
    remoteSync = NULL;
    oldTip = "";
    syncManager->restartWatching();
}
