/*
 * $Id$
 *
 * LRF parser definitions
 */

#ifndef LRFPARSER_H
#define LRFPARSER_H

#include <QString>
#include <QXmlErrorHandler>

class LRFErrorHandler: public QXmlErrorHandler
{
public:
    LRFErrorHandler(const QString fname): _fname(fname) { }
    virtual ~LRFErrorHandler()                          { }
    bool warning(const QXmlParseException & exc)        { return _error(exc);  }
    bool error(const QXmlParseException & exc)          { return _error(exc);  }
    bool fatalError(const QXmlParseException & exc)     { return _error(exc);  }
    QString errorString() const                         { return QString::null; }

private:
    bool _error(const QXmlParseException & exc)
    {
        qWarning("\nParse error in file \"%s\"; Line:%d Column:%d\n%s",
                 qPrintable(_fname), exc.lineNumber(), exc.columnNumber(),
                 qPrintable(exc.message()));
        return true;
    }
    QString _fname;
};


class LRFParser : public QXmlContentHandler
{
public:
    LRFParser()          { }
    virtual ~LRFParser() { }

    bool startElement(const QString& namespaceURI, const QString& localName,
                      const QString & qName, const QXmlAttributes& atts);
    bool endElement(const QString& namespaceURI,
                    const QString& localName, const QString& qName);
    bool characters(const QString &ch);
    bool startDocument();
    bool endDocument()                                         { return true; }
    bool startPrefixMapping(const QString&, const QString&)    { return true; }
    bool endPrefixMapping(const QString&)                      { return true; }
    void setDocumentLocator(QXmlLocator*)                      { return;      }
    bool ignorableWhitespace(const QString&)                   { return true; }
    bool processingInstruction(const QString&, const QString&) { return true; }
    bool skippedEntity(const QString&)                         { return true; }
    QString errorString() const                                { return _err; }
    QString author() const                                     { return _author; }
    QString title() const                                      { return _title;  }

private:
    bool    err(const QString& e) { _err = e; return false; }

    QString     _err;
    QString     _currText;
    QString     _author;
    QString     _title;
};

#endif
