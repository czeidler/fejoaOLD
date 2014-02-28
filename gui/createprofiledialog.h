#ifndef CREATEPROFILEDIALOG_H
#define CREATEPROFILEDIALOG_H

#include <QDialog>

class QLabel;
class QLineEdit;
class QPushButton;
class Profile;

namespace Ui {
class CreateProfileDialog;
}

class CreateProfileDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CreateProfileDialog(Profile *profile, QWidget *parent = 0);
    ~CreateProfileDialog();

private slots:
    void onTextChanged(QString);
    void createProfile();

private:
    Ui::CreateProfileDialog *ui;

    Profile *profile;

    QPushButton *applyButton;
    QLineEdit *serverUserEdit;
    QLineEdit *serverEdit;
    QLineEdit *passwordEdit1;
    QLineEdit *passwordEdit2;
    QLabel *passwordLabel2;
};

#endif // CREATEPROFILEDIALOG_H
