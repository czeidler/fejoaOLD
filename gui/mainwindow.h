#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProgressBar>

#include "infostatuswidget.h"
#include "messageview.h"
#include "profile.h"
#include "syncmanager.h"


namespace Ui {
class MainWindow;
}


class SyncManagerGuiAdapter : public QObject {
Q_OBJECT
public:
    void setTo(SyncManager *manager, InfoStatusWidget *infoStatusWidget);

private slots:
    void onSyncError();
    void onSyncStarted();
    void onSyncStopped();

private:
    SyncManager *syncManager;
    InfoStatusWidget *infoStatusWidget;
};


class SyncManager;

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(Profile *profile, QWidget *parent = 0);
    ~MainWindow();
    
public slots:
    void accountActionToggled(bool checked);
    void messageActionToggled(bool checked);
private:
    Ui::MainWindow *ui;
    QWidget *identityView;
    MessageView *messageView;
    Profile *profile;
    QProgressBar *progressBar;
    InfoStatusWidget *infoStatusWidget;

    SyncManager *syncManager;
    SyncManagerGuiAdapter syncManagerGuiAdapter;
};

#endif // MAINWINDOW_H
