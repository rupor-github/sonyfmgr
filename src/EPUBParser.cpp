/*
 * $Id$
 *
 * EPUB parser implementation
 */

#include "EPUBParser.h"

const char *author_tag   = ":creator";
const char *book_tag     = ":title";
const char *rootfile_tag = "rootfile";

////////////////////////////////////////////////////////////////////////
bool EPUBParser::startDocument()
{
    _author.clear();
    _title.clear();
    _rootfile.clear();
    return true;
} // EPUBParser::startDocument


////////////////////////////////////////////////////////////////////////
bool EPUBParser::startElement(const QString&, const QString&,
                               const QString & qName,
                               const QXmlAttributes& atts)
{
    if (qName == QString(rootfile_tag))
        _rootfile = atts.value("full-path");
    _currText.clear();
    return true;
} // EPUBParser::startElement


////////////////////////////////////////////////////////////////////////
bool EPUBParser::endElement(const QString&, const QString&,
                             const QString& qName)
{
    if (qName.endsWith(author_tag))
        _author = _currText;
    else if (qName.endsWith(book_tag))
        _title = _currText;
    return true;
} // EPUBParser::endElement


////////////////////////////////////////////////////////////////////////
bool EPUBParser::characters(const QString &ch)
{
    _currText += ch;
    return true;
} // EPUBParser::characters
