#include "useridentityview.h"

#include <QLineEdit>
#include <QListView>
#include <QPushButton>
#include <QSpacerItem>
#include <QStackedWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include "profile.h"
#include "useridentity.h"

class UserIdentityDetailsView : public QWidget
{
public:
    explicit UserIdentityDetailsView(QWidget *parent = 0);

    void setTo(UserIdentity *identity);
private:
    UserIdentity *identity;
    QLineEdit *serverUserName;
};

UserIdentityDetailsView::UserIdentityDetailsView(QWidget *parent) :
    QWidget(parent),
    identity(NULL)
{
    serverUserName = new QLineEdit(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    mainLayout->addWidget(serverUserName);
    mainLayout->addStretch();
}

void UserIdentityDetailsView::setTo(UserIdentity *identity)
{
    this->identity = identity;
    serverUserName->setText(identity->getMyself()->getServerUser());
}

UserIdentityView::UserIdentityView(IdentityListModel *listModel, QWidget *parent) :
    QSplitter(Qt::Horizontal, parent),
    identityListModel(listModel)
{
    // left identity list
    QWidget *identityListWidget = new QWidget(this);
    QVBoxLayout *identityListLayout = new QVBoxLayout();
    identityListWidget->setLayout(identityListLayout);

    identityList = new QListView(identityListWidget);
    identityList->setViewMode(QListView::ListMode);
    identityList->setModel(identityListModel);
    connect(identityList, SIGNAL(activated(QModelIndex)), this, SLOT(onIdentitySelected(QModelIndex)));

    identityListLayout->addWidget(identityList);

    // right side
    displayStack = new QStackedWidget(this);
    detailView = new UserIdentityDetailsView(displayStack);
    displayStack->addWidget(detailView);

    addWidget(identityListWidget);
    addWidget(displayStack);

    if (identityListModel->rowCount() > 0) {
        identityList->setCurrentIndex(identityListModel->index(0));
        onIdentitySelected(identityListModel->index(0));
    }
}

void UserIdentityView::onIdentitySelected(QModelIndex index)
{
    UserIdentity *identity = identityListModel->identityAt(index.row());
    detailView->setTo(identity);
}

