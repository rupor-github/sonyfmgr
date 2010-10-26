/*
 * $Id$
 *
 * LRF parser implementation
 */

#include "LRFParser.h"

////////////////////////////////////////////////////////////////////////
bool LRFParser::startDocument()
{
    _author.clear();
    _title.clear();
    return true;
} // LRFParser::startDocument


////////////////////////////////////////////////////////////////////////
bool LRFParser::startElement(const QString&, const QString&,
                               const QString & qName,
                               const QXmlAttributes& /*atts*/)
{

    _currText.clear();
    if (qName == QString("Title"))
        _title.clear();
    else if (qName == QString("Author"))
        _author.clear();
    return true;
}


////////////////////////////////////////////////////////////////////////
bool LRFParser::endElement(const QString&, const QString&,
                             const QString& qName)
{
    if (qName == QString("Title"))
        _title = _currText;
    else if (qName == QString("Author"))
        _author = _currText;
    return true;
}


////////////////////////////////////////////////////////////////////////
bool LRFParser::characters(const QString &ch)
{
    _currText += ch;
    return true;
} // LRFParser::characters
