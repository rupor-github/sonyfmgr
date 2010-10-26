/*
 * $Id$
 *
 * FB2 parser implementation
 */

#include "FB2Parser.h"

const char *author_f = "FictionBook.description.title-info.author.first-name";
const char *author_m = "FictionBook.description.title-info.author.middle-name";
const char *author_l = "FictionBook.description.title-info.author.last-name";
const char *book_tit = "FictionBook.description.title-info.book-title";

////////////////////////////////////////////////////////////////////////
QString FullPref::name()
{
    if (size() < 1)
        return "";
    else if (size() < 2)
        return (*this)[0];
    else
    {
        QString rc = (*this)[0];
        for (int i=1; i<size(); i++)
            rc += "." + (*this)[i];
        return rc;
    }
} // FullPref::name


////////////////////////////////////////////////////////////////////////
bool FB2Parser::startDocument()
{
    _author.clear();
    _title.clear();
    _pref.clear();
    return true;
} // FB2Parser::startDocument


////////////////////////////////////////////////////////////////////////
bool FB2Parser::startElement(const QString&, const QString&,
                               const QString & qName,
                               const QXmlAttributes& /*atts*/)
{

    _pref.push(qName);
    _currText.clear();
    return true;
} // FB2Parser::startElement


////////////////////////////////////////////////////////////////////////
bool FB2Parser::endElement(const QString&, const QString&,
                             const QString& qName)
{
    if (_pref.top() == qName)
    {
        if (_pref.name() == QString(author_f))
            _author = _currText;
        else if (_pref.name() == QString(author_m)
                 || _pref.name() == QString(author_l))
            _author += " " + _currText;
        else if (_pref.name() == QString(book_tit))
            _title = _currText;

        _pref.pop();
    }
    else
        return err(QString("\nOverlapped tags? element ends: \"%1\", should be: \"%1\"")
                   .arg(qName).arg(_pref.top()));
    return true;
} // FB2Parser::endElement


////////////////////////////////////////////////////////////////////////
bool FB2Parser::characters(const QString &ch)
{
    _currText += ch;
    return true;
} // FB2Parser::characters
