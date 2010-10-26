/*
 * $Id$
 *
 * mngr505 main
 */
#include <QApplication>
#include <QLibraryInfo>
#include <QTranslator>

#include <stdio.h>

#include "EPUBed.h"

int main(int ac, char *av[])
{
    QApplication app(ac, av);
    EPUBed       m;
    bool         rc;

    //qDebug("LOCALE:    \"%s\"", qPrintable(QLocale::system().name()));
    //qDebug("TPATH:     \"%s\"", qPrintable(QLibraryInfo::location(QLibraryInfo::TranslationsPath)));

    QTranslator qtTranslator;
    rc = qtTranslator.load("qt_" + QLocale::system().name(),
                           QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    app.installTranslator(&qtTranslator);
    //qDebug("LOAD QT:   %d", rc);

    QTranslator appTranslator;
    rc = appTranslator.load(QString(EPUBed::_appName) + "_" + QLocale::system().name());
    app.installTranslator(&appTranslator);
    //qDebug("LOAD APP:  %d", rc);

    m.show();
    return app.exec();
}
