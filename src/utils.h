/*
 * Misc utility functions
 */
#ifndef UTILS_H
#define UTILS_H

#if defined(WINDOWS)
#include <windows.h>
#endif

#include <QString>

QString    translit(const QString& in);
const char *translit(const QChar& in);
QString    humanReadableNumber(unsigned long long n);
void       setWidgetRecurseEnable(QObject*, bool flag);
QString    myBaseName(const QString &fileName);

#if defined(WINDOWS)
QString wideCharToQString(const WCHAR *wide);
WCHAR   *qStringToWideChar(const QString &str);
#endif

#endif
