/*
 * $Id$
 *
 * EPUB definitions
 */

#ifndef EPUB_H
#define EPUB_H

#include <QList>
#include <QString>
#include <QStringList>

class QDomElement;
class QDomNode;
class QTextEdit;
class EPUB {
public:
    EPUB(QTextEdit *txt) : _txt(txt) {}
    ~EPUB() {}

    //enum TxtType { P, IMAGE, POEM, SUBTITLE, CITE, EMPTY_LINE, TABLE };
    //enum ElType  { TITLE, EPIGRAPH, IMAGE, ANNOTATION, TEXT };
    enum ElType  { UNSUPPORTED, TEXT, TITLE, TITLE_END, EMPTY_LINE, P,
                   STRONG, EMPHASIS };

    class BookElement {
    public:
        BookElement(const ElType typ, const QString& val = QString()) :
            _typ(typ), _val(val) {}
        QString elName();
        ElType  _typ;
        QString _val;
    };
    static const char *mtype;
    static const char *mdir;
    static const char *ddir;

    bool fb2parse(QDomDocument& dom);
    bool create(const QString& dir);

private:
    // General XML functions
    bool               findElement(QDomNode& root, const QString& fullpath, QDomElement& res);
    QList<QDomElement> findElements(QDomNode& root, const QString& fullpath);
    QString            elText(QDomNode& root, const QString& fullpath);
    QStringList        elTexts(QDomNode& root, const QString& fullpath);

    // FB2 parsing functions
    void               newChapt();
    void               addTextElement(QDomElement el);
    bool               processSection(QDomElement& b);

    // EPUB functions
    bool               container();
    bool               content();
    bool               chapters();
    bool               css();

    typedef QList<BookElement> Chapter;

    QTextEdit      *_txt;
    QString        _dir;
    QString        _title;
    QString        _author;
    QString        _lang;
    QStringList    _genres;
    Chapter        _currChapt;
    QList<Chapter> _chapters;
};

#endif
