/*
 * $Id$
 *
 * FB2toLRF via external FB2LRF utility: definitions
 */

#ifndef FB2TOLRF_H
#define FB2TOLRF_H

#include <QString>
#include <QTextEdit>

bool FB2toLRF(const QString& fb2Name, const QString& EPUBName,
              QTextEdit *txt);

#endif
