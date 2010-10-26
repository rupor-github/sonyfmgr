/*
 * $Id$
 *
 * mngr505 main
 */
#include <QApplication>
#include <QLibraryInfo>
#include <QTranslator>

#include <stdio.h>

#include "mngr505.h"

#if !defined(_MSC_VER)
int main(int ac, char *av[])
{
    QApplication app(ac, av);
#else
#include <windows.h>
int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd ) 
{
    QApplication app(__argc, __argv);
#endif

    bool         rc;

    //qDebug("LOCALE:    \"%s\"", qPrintable(QLocale::system().name()));
    //qDebug("TPATH:     \"%s\"", qPrintable(QLibraryInfo::location(QLibraryInfo::TranslationsPath)));

    QTranslator qtTranslator;
    rc = qtTranslator.load("qt_" + QLocale::system().name(),
                           QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    app.installTranslator(&qtTranslator);
    //qDebug("LOAD QT:   %d", rc);

    QTranslator appTranslator;
    rc = appTranslator.load(QString(mngr505::_appName) + "_" + QLocale::system().name());
    app.installTranslator(&appTranslator);
    //qDebug("LOAD APP:  %d", rc);

    mngr505      m;
    m.show();
    return app.exec();
}
