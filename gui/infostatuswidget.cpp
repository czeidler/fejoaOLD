#include "infostatuswidget.h"

InfoStatusWidget::InfoStatusWidget(QWidget *parent) :
    QLabel(parent)
{
}

void InfoStatusWidget::info(const QString &info) {
    setText(info);
}
