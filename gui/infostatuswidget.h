#ifndef INFOSTATUSWIDGET_H
#define INFOSTATUSWIDGET_H

#include <QLabel>

class InfoStatusWidget : public QLabel
{
    Q_OBJECT
public:
    explicit InfoStatusWidget(QWidget *parent = 0);
    void info(const QString &info);

signals:

public slots:

};

#endif // INFOSTATUSWIDGET_H
