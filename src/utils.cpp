/*
 * Misc utility functions
 */
#include <QChar>
#include <QMap>
#include <QObject>
#include <QWidget>
#include <QString>

#include "utils.h"

#define numb_el(x)  (sizeof(x)/sizeof(x[0]))

static const struct {
    const char *from;
    const char *to;
} utftransl[] =
{
    { "А", "A" },
    { "Б", "B" },
    { "В", "V" },
    { "Г", "G" },
    { "Д", "D" },
    { "Е", "E" },
    { "Ё", "YO" },
    { "Ж", "ZH" },
    { "З", "Z" },
    { "И", "I" },
    { "Й", "J" },
    { "К", "K" },
    { "Л", "L" },
    { "М", "M" },
    { "Н", "N" },
    { "О", "O" },
    { "П", "P" },
    { "Р", "R" },
    { "С", "S" },
    { "Т", "T" },
    { "У", "U" },
    { "Ф", "F" },
    { "Х", "H" },
    { "Ц", "C" },
    { "Ч", "CH" },
    { "Ш", "SH" },
    { "Щ", "SHC" },
    { "Ь", "'" },
    { "Ы", "Y" },
    { "Ъ", "'" },
    { "Э", "E" },
    { "Ю", "YU" },
    { "Я", "YA" },
    { "а", "a" },
    { "б", "b" },
    { "в", "v" },
    { "г", "g" },
    { "д", "d" },
    { "е", "e" },
    { "ё", "yo" },
    { "ж", "zh" },
    { "з", "z" },
    { "и", "i" },
    { "й", "j" },
    { "к", "k" },
    { "л", "l" },
    { "м", "m" },
    { "н", "n" },
    { "о", "o" },
    { "п", "p" },
    { "р", "r" },
    { "с", "s" },
    { "т", "t" },
    { "у", "u" },
    { "ф", "f" },
    { "х", "h" },
    { "ц", "c" },
    { "ч", "ch" },
    { "ш", "sh" },
    { "щ", "sch" },
    { "ь", "'" },
    { "ы", "y" },
    { "ъ", "'" },
    { "э", "e" },
    { "ю", "yu" },
    { "я", "ya" }
};

QMap<QChar, const char*> tr;

////////////////////////////////////////////////////////////////////////
static void init()
{
    tr.clear();
    for (unsigned i=0; i<numb_el(utftransl); i++)
        tr[QString::fromUtf8(utftransl[i].from).at(0)] = utftransl[i].to;
} // init


////////////////////////////////////////////////////////////////////////
QString translit(const QString& in)
{
    if (tr.isEmpty())
        init();

    QString rc;
    for (int i=0; i<in.size(); i++) {
        const char *t = translit(in.at(i));
        if (t)
            rc += t;
        else
            rc += in.at(i);
    }
    return rc;
} // translit


////////////////////////////////////////////////////////////////////////
const char *translit(const QChar& in)
{
    if (tr.isEmpty())
        init();

    QMap<QChar, const char *>::iterator it = tr.find(in);
    if (it == tr.end())
        return 0;
    else
        return it.value();
} // translit

////////////////////////////////////////////////////////////////////////
QString humanReadableNumber(unsigned long long n)
{
    const char *p = "<font color=#0000ff>";
    const char *s = "</font>";
    QString    rc;

    if (n > 1024ULL * 1024 * 1024 * 1024)
        rc.sprintf("%s%.1f%sT", p, (double)n / (1024ULL * 1024 * 1024 * 1024), s);
    else if (n > 1024 * 1024 * 1024)
        rc.sprintf("%s%.1f%sG", p, (double)n / (1024ULL * 1024 * 1024), s);
    else if (n > 1024 * 1024)
        rc.sprintf("%s%.1f%sM", p, (double)n / (1024 * 1024), s);
    else if (n > 1024)
        rc.sprintf("%s%.1f%sK", p, (double)n / 1024, s);
    else
        rc.sprintf("%s%llu%sB", p, n, s);

    return rc;
} // humanReadableNumber

////////////////////////////////////////////////////////////////////////
void setWidgetRecurseEnable(QObject* o, bool flag)
{
    QWidget *w = qobject_cast<QWidget *>(o);
    if (o->isWidgetType()  &&  w)
        w->setEnabled(flag);

    const QObjectList children = o->children();
    for (int i=0; i<children.size(); i++)
        setWidgetRecurseEnable(children[i], flag);

} // setWidgetRecurseEnable

////////////////////////////////////////////////////////////////////////
static const char *known_comresssion_suffixes[] = { ".zip", ".tgz", ".tbz", ".tar.gz", ".tar.bz2" };
QString myBaseName(const QString &fileName)
{
    QString rc = fileName;

     for (unsigned i=0; i<numb_el(known_comresssion_suffixes); i++)
         if (rc.endsWith(known_comresssion_suffixes[i], Qt::CaseInsensitive))
             rc = rc.left(rc.length() - sizeof(known_comresssion_suffixes[i]));

     int n = rc.lastIndexOf('.');
     if (n != -1)
         rc = rc.left(n);

     return rc;
} // myBaseName

#if defined(WINDOWS)
////////////////////////////////////////////////////////////////////////
QString wideCharToQString(const WCHAR *wide)
{
    QString result;
    result.setUtf16((ushort *)wide, lstrlenW(wide));
    return result;
} // wideCharToQString

////////////////////////////////////////////////////////////////////////
WCHAR *qStringToWideChar(const QString &str)
{
    if (str.isNull())
        return 0;
    WCHAR *result = new WCHAR[str.length() + 1];
    for (int i = 0; i < str.length(); ++i)
        result[i] = str[i].unicode();
    result[str.length()] = 0;
    return result;
} // qStringToWideChar

#endif
