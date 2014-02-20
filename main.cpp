#include <QApplication>
#if QT_VERSION < 0x050000
#include <QtDeclarative/QDeclarativeContext>
#include <QtDeclarative/QDeclarativeView>
#else
#include <QtGui/QGuiApplication>
#endif

#include <mainapplication.h>


int main(int argc, char *argv[])
{
    MainApplication app(argc, argv);
    return app.exec();
}
