#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "useridentity.h"
#include "useridentityview.h"
#include "syncmanager.h"


void SyncManagerGuiAdapter::setTo(SyncManager *manager, InfoStatusWidget *infoStatusWidget)
{
    this->syncManager = manager;
    this->infoStatusWidget = infoStatusWidget;

    connect(syncManager, SIGNAL(connectionError()), this, SLOT(onSyncError()));
    connect(syncManager, SIGNAL(syncStarted()), this, SLOT(onSyncStarted()));
    connect(syncManager, SIGNAL(syncStopped()), this, SLOT(onSyncStopped()));
}

void SyncManagerGuiAdapter::onSyncError() {
    infoStatusWidget->info("sync error");
}

void SyncManagerGuiAdapter::onSyncStarted()
{
    infoStatusWidget->info("sync started");
}

void SyncManagerGuiAdapter::onSyncStopped()
{
    infoStatusWidget->info("sync stopped");
}

MainWindow::MainWindow(Profile *profile, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    profile(profile)
{
    ui->setupUi(this);

    ui->toolBar->setMovable(false);

    QAction *accountAction = ui->toolBar->addAction("Profile");
    accountAction->setCheckable(true);
    QAction *messageAction = ui->toolBar->addAction("Messages");
    messageAction->setCheckable(true);

    QActionGroup* actionGroup = new QActionGroup(this);
    actionGroup->addAction(accountAction);
    actionGroup->addAction(messageAction);

    identityView = new UserIdentityView(profile->getIdentityList(), this);
    messageView = new MessageView(profile, this);
    Mailbox *mailbox = profile->getMailboxAt(0);
    messageView->setMailbox(mailbox);

    ui->horizontalLayout->setMargin(0);
    ui->stackedWidget->addWidget(identityView);
    ui->stackedWidget->addWidget(messageView);

    connect(accountAction, SIGNAL(toggled(bool)), this, SLOT(accountActionToggled(bool)));
    connect(messageAction, SIGNAL(toggled(bool)), this, SLOT(messageActionToggled(bool)));

    progressBar = new QProgressBar(this);
    statusBar()->addWidget(progressBar);

    infoStatusWidget = new InfoStatusWidget(this);
    statusBar()->addWidget(infoStatusWidget);

    messageAction->setChecked(true);

    // init fejoa stuff
    DatabaseBranch *branch = NULL;
    QList<DatabaseBranch*> &branches = profile->getBranches();
    RemoteDataStorage *remoteStorage = branches.at(0)->getRemoteAt(0);
    SyncManagerRef syncManager(new SyncManager(remoteStorage, this));
    foreach (branch, branches)
        syncManager->keepSynced(branch->getDatabase());
    syncManagerGuiAdapter.setTo(syncManager.data(), infoStatusWidget);

    RemoteConnectionJobQueue *queue = ConnectionManager::get()->getConnectionJobQueue(
                remoteStorage->getRemoteConnectionInfo());
    queue->setIdleJob(syncManager);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::accountActionToggled(bool checked)
{
    if (checked)
        ui->stackedWidget->setCurrentWidget(identityView);
}

void MainWindow::messageActionToggled(bool checked)
{
    if (checked)
        ui->stackedWidget->setCurrentWidget(messageView);
}

