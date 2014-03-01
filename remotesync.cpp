#include "remotesync.h"

#include "protocolparser.h"
#include "remoteauthentication.h"
#include "remotestorage.h"


RemoteSync::RemoteSync(DatabaseInterface *database, RemoteDataStorage* remoteStorage, QObject *parent) :
    QObject(parent),
    database(database),
    remoteStorage(remoteStorage),
    authentication(remoteStorage->getRemoteAuthentication()),
    remoteConnection(remoteStorage->getRemoteAuthentication()->getConnection()),
    serverReply(NULL)
{
}

RemoteSync::~RemoteSync()
{
}

WP::err RemoteSync::sync()
{
    if (authentication->isVerified())
        syncConnected(WP::kOk);
    else {
        connect(authentication, SIGNAL(authenticationAttemptFinished(WP::err)),
                this, SLOT(syncConnected(WP::err)));
        authentication->login();
    }
    return WP::kOk;
}

void RemoteSync::syncConnected(WP::err code)
{
    if (code != WP::kOk)
        return;

    QString branch = database->branch();
    QString lastSyncCommit = database->getLastSyncCommit(remoteStorage->getUid(), branch);

    QByteArray outData;
    ProtocolOutStream outStream(&outData);

    IqOutStanza *iqStanza = new IqOutStanza(kGet);
    outStream.pushStanza(iqStanza);

    OutStanza *syncStanza = new OutStanza("sync_pull");
    syncStanza->addAttribute("branch", branch);
    syncStanza->addAttribute("base", lastSyncCommit);

    outStream.pushChildStanza(syncStanza);

    outStream.flush();

    if (serverReply != NULL)
        serverReply->disconnect();
    serverReply = remoteConnection->send(outData);
    connect(serverReply, SIGNAL(finished(WP::err)), this, SLOT(syncReply(WP::err)));
}


class SyncPullData {
public:
    QString branch;
    QString tip;
    QByteArray pack;
};


class SyncPullPackHandler : public InStanzaHandler {
public:
    SyncPullPackHandler(SyncPullData *d) :
        InStanzaHandler("pack", true),
        data(d)
    {
    }

    bool handleStanza(const QXmlStreamAttributes &attributes)
    {
        return true;
    }

    bool handleText(const QStringRef &text)
    {
        data->pack = QByteArray::fromBase64(text.toLatin1());
        return true;
    }

public:
    SyncPullData *data;
};


class SyncPullHandler : public InStanzaHandler {
public:
    SyncPullHandler(SyncPullData *d) :
        InStanzaHandler("sync_pull"),
        data(d)
    {
        syncPullPackHandler = new SyncPullPackHandler(data);
        addChildHandler(syncPullPackHandler);
    }

    bool handleStanza(const QXmlStreamAttributes &attributes)
    {
        if (!attributes.hasAttribute("branch"))
            return false;
        if (!attributes.hasAttribute("tip"))
            return false;

        data->branch = attributes.value("branch").toString();
        data->tip = attributes.value("tip").toString();
        return true;
    }

public:
    SyncPullPackHandler *syncPullPackHandler;
    SyncPullData *data;
};


void RemoteSync::syncReply(WP::err code)
{
    if (code != WP::kOk)
        return;

    QByteArray data = serverReply->readAll();
    serverReply = NULL;

    IqInStanzaHandler iqHandler(kResult);
    SyncPullData syncPullData;
    SyncPullHandler *syncPullHandler = new SyncPullHandler(&syncPullData);
    iqHandler.addChildHandler(syncPullHandler);

    ProtocolInStream inStream(data);
    inStream.addHandler(&iqHandler);

    inStream.parse();

    QString localBranch = database->branch();
    QString localTipCommit = database->getTip();
    QString lastSyncCommit = database->getLastSyncCommit(remoteStorage->getUid(), localBranch);

    if (!syncPullHandler->hasBeenHandled() || syncPullData.branch != localBranch) {
        // error occured, the server should at least send back the branch name
        // TODO better error message
        emit syncFinished(WP::kBadValue);
        return;
    }
    if (syncPullData.tip == localTipCommit) {
        // done
        emit syncFinished(WP::kOk);
        return;
    }
    // see if the server is ahead by checking if we got packages
    if (syncPullData.pack.size() != 0) {
        syncUid = syncPullData.tip;
        WP::err error = database->importPack(syncPullData.pack, lastSyncCommit,
                                              syncPullData.tip);
        if (error != WP::kOk) {
            emit syncFinished(error);
            return;
        }

        localTipCommit = database->getTip();
        // done? otherwise it was a merge and we have to push our merge
        if (localTipCommit == lastSyncCommit) {
            emit syncFinished(WP::kOk);
            return;
        }
    }

    // we are ahead of the server: push changes to the server
    QByteArray pack;
    WP::err error = database->exportPack(pack, lastSyncCommit, localTipCommit, syncUid);
    if (error != WP::kOk) {
        emit syncFinished(error);
        return;
    }
    syncUid = localTipCommit;

    QByteArray outData;
    ProtocolOutStream outStream(&outData);

    IqOutStanza *iqStanza = new IqOutStanza(kSet);
    outStream.pushStanza(iqStanza);
    OutStanza *pushStanza = new OutStanza("sync_push");
    pushStanza->addAttribute("branch", localBranch);
    pushStanza->addAttribute("start_commit", syncPullData.tip);
    pushStanza->addAttribute("last_commit", localTipCommit);
    outStream.pushChildStanza(pushStanza);
    OutStanza *pushPackStanza = new OutStanza("pack");
    pushPackStanza->setText(pack.toBase64());
    outStream.pushChildStanza(pushPackStanza);

    outStream.flush();

    serverReply = remoteConnection->send(outData);
    connect(serverReply, SIGNAL(finished(WP::err)), this, SLOT(syncPushReply(WP::err)));
}

void RemoteSync::syncPushReply(WP::err code)
{
    if (code != WP::kOk)
        return;

    QByteArray data = serverReply->readAll();

    database->updateLastSyncCommit(remoteStorage->getUid(), database->branch(), syncUid);
    emit syncFinished(WP::kOk);
}
