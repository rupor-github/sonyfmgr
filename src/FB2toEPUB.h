/*
 * $Id$
 *
 * FB2toEPUB definitions
 */

#ifndef FB2TOEPUB_H
#define FB2TOEPUB_H

#include <QString>
#include <QTextEdit>

bool FB2toEPUB(const QString& fb2Name, const QString& EPUBName,
               QTextEdit *txt);

#endif
