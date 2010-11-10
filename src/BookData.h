/*
 * $Id$
 *
 * BookData definitions
 */

#ifndef BOOKDATA_H
#define BOOKDATA_H

#include <QString>

class BookData {
public:
    BookData() : page(0), size(0), is_file(false), is_media(false) { }
    BookData(const QString& a, const QString& t) : author(a), title(t),
        page(0), size(0), is_file(false), is_media(false) { }

    enum BType { Unknown, FB2, LRF, PDF, RTF, TXT, EPUB };

    static BType getType(const QString& fname);
    static bool  isFB2(const QString& fname);
    const char   *mimeType();

    QString path;
    QString author;
    QString title;
    QString date;
    BType   type;
    QImage  cover;
    int     page;
    int     size;
    bool    is_file;
    bool    is_media;
};

#endif
