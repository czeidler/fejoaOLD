#ifndef USERIDENTITYVIEW_H
#define USERIDENTITYVIEW_H

#include <QModelIndex>
#include <QSplitter>

class QListView;
class QPushButton;
class QStackedWidget;

class UserIdentityDetailsView;
class IdentityListModel;

class UserIdentityView : public QSplitter
{
    Q_OBJECT
public:
    explicit UserIdentityView(IdentityListModel *listModel, QWidget *parent = 0);
    
signals:
    
public slots:
    void onIdentitySelected(QModelIndex index);

private:
    QListView* identityList;
    IdentityListModel *identityListModel;

    QStackedWidget *displayStack;
    UserIdentityDetailsView *detailView;
};

#endif // USERIDENTITYVIEW_H
