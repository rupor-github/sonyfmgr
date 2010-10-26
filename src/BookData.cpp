/*
 * $Id$
 *
 * BookData implementation
 */
#include <QString>
#include <QStringList>
#include <QRegExp>

#include "Config.h"
#include "BookData.h"

#define numb_el(x)  (sizeof(x)/sizeof(x[0]))

static const struct {
    const char * const    suff[3];
    const BookData::BType t;
} btypes[] = {
    { { ".lrf",  0 }, BookData::LRF  },
    { { ".pdf",  0 }, BookData::PDF  },
    { { ".rtf",  0 }, BookData::RTF  },
    { { ".txt",  0 }, BookData::TXT  },
    { { ".epub", 0 }, BookData::EPUB }
};
////////////////////////////////////////////////////////////////////////
bool  BookData::isFB2(const QString& fname)
{
    QStringList sl = Config::fb2Pattern().split(',', QString::SkipEmptyParts);
    foreach(QString s, sl) {
        QRegExp r(s, Qt::CaseInsensitive, QRegExp::Wildcard);
        if (r.isValid()  &&  r.exactMatch(fname))
            return true;
    }

    return false;
} // BookData::isFB2


////////////////////////////////////////////////////////////////////////
BookData::BType BookData::getType(const QString& fname)
{
    for (unsigned i=0; i<numb_el(btypes); i++)
        for (unsigned j=0; btypes[i].suff[j]; j++)
            if (fname.endsWith(btypes[i].suff[j]))
                return btypes[i].t;

    if (isFB2(fname))
        return BookData::FB2;

    return Unknown;
} // BookData::getType


////////////////////////////////////////////////////////////////////////
const char *BookData::mimeType()
{
    switch (type)
    {
    case LRF:
        return "application/x-sony-bbeb";
    case PDF:
        return "application/pdf";
    case RTF:
        return "application/rtf";
    case TXT:
        return "text/plain";
    case EPUB:
        return "application/epub+zip";
    case FB2:
    default:
        return "";
    }
} // BookData::mimeType
