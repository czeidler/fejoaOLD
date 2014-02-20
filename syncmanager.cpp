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
    fRemoteDataStorage(remoteDataStorage),
    fAuthentication(remoteDataStorage->getRemoteAuthentication()),
    fRemoteConnection(remoteDataStorage->getRemoteConnection()),
    fServerReply(NULL)
{
}

SyncManager::~SyncManager()
{
    stopWatching();
}

WP::err SyncManager::keepSynced(DatabaseInterface *branch)
{
    SyncEntry *entry = new SyncEntry(this, branch, this);
    fSyncEntries.append(entry);

    restartWatching();
    return WP::kOk;
}

void SyncManager::startWatching()
{
    if (fAuthentication->isVerified())
        remoteAuthenticated(WP::kOk);
    else {
        connect(fAuthentication, SIGNAL(authenticationAttemptFinished(WP::err)),
                this, SLOT(remoteAuthenticated(WP::err)));
        fAuthentication->login();
    }
}

void SyncManager::restartWatching()
{
    stopWatching();
    startWatching();
}

void SyncManager::stopWatching()
{
    if (fServerReply != NULL) {
        fServerReply->abort();
        fServerReply = NULL;
    }
}

void SyncManager::syncBranches(const QStringList &branches)
{
    foreach (SyncEntry *entry, fSyncEntries) {
        if (entry->isSyncing())
            continue;
        if (!branches.contains(entry->getDatabase()->branch()))
            continue;
        entry->sync();
    }
}

void SyncManager::remoteAuthenticated(WP::err error)
{
    if (error != WP::kOk) {
        emit connectionError();
        return;
    }

    QByteArray outData;
    ProtocolOutStream outStream(&outData);

    IqOutStanza *iqStanza = new IqOutStanza(kGet);
    outStream.pushStanza(iqStanza);

    OutStanza *watchStanza = new OutStanza(kWathcBranchesStanza);
    outStream.pushChildStanza(watchStanza);

    bool first = true;
    foreach (SyncEntry *entry, fSyncEntries) {
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

    if (fServerReply != NULL)
        fServerReply->disconnect();
    fServerReply = fRemoteConnection->send(outData);
    connect(fServerReply, SIGNAL(finished(WP::err)), this, SLOT(watchReply(WP::err)));
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
        emit connectionError();
        return;
    }
    if (fServerReply == NULL)
        return;

    QByteArray data = fServerReply->readAll();
    fServerReply = NULL;

    IqInStanzaHandler iqHandler(kResult);
    WatchHandler *watchHandler = new WatchHandler();
    WatchItemsChangedHandler *itemsChangedHandler = new WatchItemsChangedHandler();
    iqHandler.addChildHandler(watchHandler);
    watchHandler->addChildHandler(itemsChangedHandler);

    ProtocolInStream inStream(data);
    inStream.addHandler(&iqHandler);

    inStream.parse();

    if (!watchHandler->hasBeenHandled()) {
        emit connectionError();
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
    fSyncManager(syncManager),
    fDatabase(database),
    fRemoteSync(NULL)
{
}

WP::err SyncEntry::sync()
{
    if (isSyncing())
        return WP::kOk;
    fOldTip = fDatabase->getTip();
    fRemoteSync = new RemoteSync(fDatabase, fSyncManager->fRemoteDataStorage, this);
    connect(fRemoteSync, SIGNAL(syncFinished(WP::err)), this, SLOT(onSyncFinished(WP::err)));
    WP::err error = fRemoteSync->sync();
    if (error != WP::kOk) {
        delete fRemoteSync;
        fRemoteSync = NULL;
    }
    return error;
}

bool SyncEntry::isSyncing()
{
    return fRemoteSync != NULL ? true : false;
}

const DatabaseInterface *SyncEntry::getDatabase() const
{
    return fDatabase;
}

void SyncEntry::onSyncFinished(WP::err error)
{
    fRemoteSync->deleteLater();
    fRemoteSync = NULL;
    fOldTip = "";
    fSyncManager->restartWatching();
}
