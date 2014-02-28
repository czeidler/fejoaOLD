#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "useridentity.h"
#include "useridentityview.h"


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

    messageAction->setChecked(true);
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
