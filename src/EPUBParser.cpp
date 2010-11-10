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
    _covername.clear();
    _coverfile.clear();
    return true;
} // EPUBParser::startDocument


////////////////////////////////////////////////////////////////////////
bool EPUBParser::startElement(const QString&, const QString&,
                               const QString & qName,
                               const QXmlAttributes& atts)
{
    if (qName == QString(rootfile_tag))
       _rootfile = atts.value("full-path");
    else if (qName == QString("meta") && (atts.value( "name" ) == QString("cover")))
       _covername = atts.value( "content" );
    else if (qName == QString("item") && (atts.value( "id" ) == _covername))
       _coverfile = atts.value( "href" );
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
