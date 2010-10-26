/*
 * $Id$
 *
 * EPUB implementation
 */

#include <QDir>
#include <QDomDocument>
#include <QFile>
#include <QFileInfo>
#include <QTextCodec>
#include <QTextEdit>
#include <QTextStream>

#include "Config.h"
#include "EPUB.h"

#define numb_el(x)  (sizeof(x)/sizeof(x[0]))
#define tr QObject::tr
#define MSG(P, H) do {            \
    if (Config::plainText())      \
        _txt->insertPlainText(P); \
    else                          \
        _txt->insertHtml(H);      \
} while (0)
#define ERR(P, H) do {            \
    if (Config::plainText())      \
        _txt->insertPlainText(P); \
    else                          \
        _txt->insertHtml(H);      \
    return false;                 \
} while (0)
#define ADDTEXT(n, t) do {              \
    QDomText c = dom.createTextNode(t); \
    n.appendChild(c);                   \
} while (0)

const char *EPUB::mtype = "mimetype";
const char *EPUB::mdir  = "META-INF";
const char *EPUB::ddir  = "OEBPS";

////////////////////////////////////////////////////////////////////////
QString EPUB::BookElement::elName()
{
    switch (_typ)
    {
    case UNSUPPORTED: return "Unsupported";
    case TEXT:        return "Text";
    case TITLE:       return "Title";
    case TITLE_END:   return "TitleEnd";
    case P:           return "P";
    case EMPTY_LINE:  return "EmptyLine";
    case EMPHASIS:    return "Emphasis";
    case STRONG:      return "Strong";
    }
    return "Unsupported";
} // EPUB::BookElement::elName


////////////////////////////////////////////////////////////////////////
bool EPUB::findElement(QDomNode& root, const QString& fullpath, QDomElement& res)
{
    if (root.isNull())
        return false;

    QDomNode n = root;
    QStringList pl = fullpath.split('.', QString::SkipEmptyParts);
    for (int i=0; i<pl.size(); i++)
    {
        QDomNodeList children = n.childNodes();
        int j=0;
        for (; j<children.size(); j++)
        {
            res = children.item(j).toElement();
            if (res.isNull())
                continue;
            if (res.tagName() == pl[i])
                break;
        }
        if (j >= children.size())
            return false;
        n = res;
    }
    return true;
} // EPUB::findElement

////////////////////////////////////////////////////////////////////////
QList<QDomElement> EPUB::findElements(QDomNode& root, const QString& fullpath)
{
    QList<QDomElement> res;

    if (root.isNull())
        return res;

    QDomNode    n = root;
    QDomElement el;
    QStringList pl = fullpath.split('.', QString::SkipEmptyParts);
    for (int i=0; i<pl.size(); i++)
    {
        QDomNodeList children = n.childNodes();
        int j=0;
        for (; j<children.size(); j++)
        {
            el = children.item(j).toElement();
            if (el.isNull())
                continue;
            if (el.tagName() == pl[i])
            {
                if (i == pl.size()-1)
                    res += el;
                else
                    break;
            }
        }
        if (i < pl.size()-1  &&  j >= children.size())
            return res;
        n = el;
    }
    return res;
} // EPUB::findElement


////////////////////////////////////////////////////////////////////////
QString EPUB::elText(QDomNode& root, const QString& fullpath)
{
    QString     rc;
    QDomElement el;

    if (findElement(root, fullpath, el))
        rc = el.text();
    return rc;
} // EPUB::elText

////////////////////////////////////////////////////////////////////////
QStringList EPUB::elTexts(QDomNode& root, const QString& fullpath)
{
    QStringList        rc;
    QList<QDomElement> el = findElements(root, fullpath);

    foreach(QDomElement e, el)
        rc += e.text();
    return rc;
} // EPUB::elsText


////////////////////////////////////////////////////////////////////////
bool EPUB::processSection(QDomElement& b)
{
    newChapt();
    QDomNodeList children = b.childNodes();
    for (int j=0; j<children.size(); j++)
    {
        QDomElement el = children.item(j).toElement();
        if (el.isNull())
            continue;
        if (el.tagName() == QString("section"))
            processSection(el);
        else
            addTextElement(el);
    }

    return true;
} // EPUB::processSection


////////////////////////////////////////////////////////////////////////
void EPUB::addTextElement(QDomElement el)
{
    if (el.isNull())
        return;

    QDomNodeList children;
    if (el.tagName() == QString("title"))
    {
        _currChapt.append(BookElement(TITLE));
        children = el.childNodes();
        for (int j=0; j<children.size(); j++)
            addTextElement(children.item(j).toElement());
        _currChapt.append(BookElement(TITLE_END));
    }
    else if (el.tagName() == QString("empty-line"))
        _currChapt.append(BookElement(EMPTY_LINE));
    //else if (el.tagName() == QString("epigraph"))
    //    eType = EPIGRAPH;
    //else if (el.tagName() == QString("image"))
    //    eType = IMAGE;
    //else if (el.tagName() == QString("annotation"))
    //    eType = ANNOTATION;
    else if (el.tagName() == QString("strong"))
        _currChapt.append(BookElement(STRONG, el.text()));
    else if (el.tagName() == QString("emphasis"))
        _currChapt.append(BookElement(EMPHASIS));
    else if (el.tagName() == QString("p"))
    {
        children = el.childNodes();
        for (int j=0; j<children.size(); j++)
        {
            switch (children.item(j).nodeType())
            {
            case QDomNode::ElementNode:
                addTextElement(children.item(j).toElement());
                break;
            case QDomNode::TextNode:
                _currChapt.append(BookElement(TEXT, children.item(j).toText().data()));
                break;
            default:
                qDebug("    type not handled: %d", children.item(j).nodeType());
                break;
            }
        }
    }
} // EPUB::addTextElement


////////////////////////////////////////////////////////////////////////
void EPUB::newChapt()
{
    if (!_currChapt.isEmpty())
        _chapters.append(_currChapt);
    _currChapt.clear();
} // EPUB::newChapt


////////////////////////////////////////////////////////////////////////
bool EPUB::fb2parse(QDomDocument& dom)
{
#if 0 /* elText/findElement function DEBUG */
    const char *p[] = { "FictionBook.description.title-info.author.last-name",
                        "FictionBook.description.title-info.book-title",
                        "FictionBook.description.title-info.lang",
                        "FictionBook.description.document-info.author",
                        "FictionBook.description.document-info.id"
    };

    for (unsigned i=0; i<numb_el(p); i++)
        qDebug("\"%s\" = \"%s\"", p[i], qPrintable(elText(dom, p[i])));

    const char *ps[] = { "FictionBook.description.title-info.genre" };
    for (unsigned i=0; i<numb_el(ps); i++)
    {
        QStringList res = elTexts(dom, ps[i]);
        qDebug("\"%s\" = %d: ", ps[i], res.size());
        for(int j=0; j<res.size(); j++)
            qDebug("    \"%s\" / %d: \"%s\"", ps[i], j, qPrintable(res[j]));
    }
#endif

    /*
     * Header information
     * ------------------
     */
    _title  = elText(dom, "FictionBook.description.title-info.book-title");
    _author = elText(dom, "FictionBook.description.title-info.author.first-name") +
        " " + elText(dom, "FictionBook.description.title-info.author.last-name");
    _lang   = elText(dom, "FictionBook.description.title-info.lang");
    _genres = elTexts(dom, "FictionBook.description.title-info.genre");

    /*
     * Body (may be a few)
     * -------------------
     */
    QList<QDomElement> bodies = findElements(dom, "FictionBook.body");
    if (bodies.size() < 1)
        ERR(tr("\nAt least one \"body\" tag should exist\n"),
            tr("<br><b><font color=#ff0000>At least one \"body\" tag should exist</font><b><br>"));
    foreach (QDomElement b, bodies)
        if (!processSection(b))
            return false;

    newChapt();
    return true;
} // EPUB::fb2parse


////////////////////////////////////////////////////////////////////////
bool EPUB::create(const QString& dir)
{
    _dir = dir;
    if (!container())
        return false;

    // ++++++DEBUG
#if 0
    QString outf("a.txt");
    QFile outfile(outf);
    if (!outfile.open(QFile::WriteOnly | QFile::Truncate))
        ERR(tr("\nCan't open file %1: %2\n")
            .arg(outf).arg(outfile.errorString()),
            tr("<br>Can't open file %1: <b><font color=#ff0000>%2</font><b><br>")
            .arg(outf).arg(outfile.errorString()));
    QTextStream out(&outfile);
    out.setCodec(QTextCodec::codecForName("UTF-8"));
    foreach(Chapter ch, _chapters)
    {
        out << endl << "Chapter:" << endl;
        foreach(BookElement be, ch)
        {
            out << "EL: " << be.elName() << " \"" << be._val << "\"" << endl;
        }
    }
#endif
    // ------DEBUG

    if (!content())
        return false;

    if (!chapters())
        return false;

    if (!css())
        return false;

    return true;
} // EPUB::create


////////////////////////////////////////////////////////////////////////
bool EPUB::container()
{
    QDomDocument dom;

    QDomElement root = dom.createElement("rootfile");
    root.setAttribute("full-path", "OEBPS/content.opf");
    root.setAttribute("media-type", "application/oebps-package+xml");

    QDomElement root1 = dom.createElement("rootfiles");
    root1.appendChild(root);

    QDomElement cont = dom.createElement("container");
    cont.setAttribute("version", "1.0");
    cont.setAttribute("xmlns", "urn:oasis:names:tc:opendocument:xmlns:container");
    cont.appendChild(root1);
    dom.appendChild(cont);

    QString outf(_dir + "/" + mdir + "/container.xml");
    QFile outfile(outf);
    if (!outfile.open(QFile::WriteOnly | QFile::Truncate))
        ERR(tr("\nCan't open file %1: %2\n")
            .arg(outf).arg(outfile.errorString()),
            tr("<br>Can't open file %1: <b><font color=#ff0000>%2</font><b><br>")
            .arg(outf).arg(outfile.errorString()));
    QTextStream out(&outfile);
    out.setCodec(QTextCodec::codecForName("UTF-8"));
    dom.save(out, 4, QDomNode::EncodingFromTextStream);

    return true;
} // EPUB::container


////////////////////////////////////////////////////////////////////////
bool EPUB::content()
{
    QDomDocument dom;

    QDomElement root = dom.createElement("package");
    root.setAttribute("version", "2.0");
    root.setAttribute("xmlns:dc", "http://purl.org/dc/elements/1.1/");
    root.setAttribute("xmlns:opf", "http://www.idpf.org/2007/opf");
    root.setAttribute("unique-identifier", "bookid");

    // Metadata
    QDomElement mdata = dom.createElement("metadata");

    QDomElement e = dom.createElement("dc:title");
    ADDTEXT(e, _title);
    mdata.appendChild(e);

    e = dom.createElement("dc:creator");
    e.setAttribute("opf:role", "aut");
    ADDTEXT(e, _author);
    mdata.appendChild(e);

    if (!_lang.isEmpty())
    {
        e = dom.createElement("dc:language");
        ADDTEXT(e, _lang);
        mdata.appendChild(e);
    }

    if (_genres.size() > 0)
    {
        QString t;
        e = dom.createElement("dc:type");
        for (int i=0; i<_genres.size(); i++)
            t += _genres[i] + QString((i<_genres.size()-1) ? ", " : "");
        ADDTEXT(e, t);
        mdata.appendChild(e);
    }

    e = dom.createElement("dc:identifier");
    e.setAttribute("id", "bookid");
    ADDTEXT(e, "123456");
    mdata.appendChild(e);

    // Manifest
    QDomElement man = dom.createElement("manifest");
    for (int i=0; i<_chapters.size(); i++)
    {
        e = dom.createElement("item");
        e.setAttribute("href", QString("ch%1.xhtml").arg(i));
        e.setAttribute("id", QString("id%1").arg(i));
        e.setAttribute("media-type", "application/xhtml+xml");
        man.appendChild(e);
    }
    e = dom.createElement("item");
    e.setAttribute("href", QString("style.css"));
    e.setAttribute("id", QString("id_css"));
    e.setAttribute("media-type", "text/css");
    man.appendChild(e);

    // Spine
    QDomElement spine = dom.createElement("spine");
    for (int i=0; i<_chapters.size(); i++)
    {
        e = dom.createElement("itemref");
        e.setAttribute("idref", QString("id%1").arg(i));
        spine.appendChild(e);
    }

    // Whole document
    root.appendChild(mdata);
    root.appendChild(man);
    root.appendChild(spine);
    dom.appendChild(root);

    // Save
    QString outf(_dir + "/" + ddir + "/content.opf");
    QFile outfile(outf);
    if (!outfile.open(QFile::WriteOnly | QFile::Truncate))
        ERR(tr("\nCan't open file %1: %2\n")
            .arg(outf).arg(outfile.errorString()),
            tr("<br>Can't open file %1: <b><font color=#ff0000>%2</font><b><br>")
            .arg(outf).arg(outfile.errorString()));
    QTextStream out(&outfile);
    out.setCodec(QTextCodec::codecForName("UTF-8"));
    dom.save(out, 4, QDomNode::EncodingFromTextStream);
    return true;
} // EPUB::content


////////////////////////////////////////////////////////////////////////
bool EPUB::chapters()
{
    for (int i=0; i<_chapters.size(); i++)
    {
        QDomDocument dom;
        Chapter      ch = _chapters[i];

        QDomElement html = dom.createElement("html");
        html.setAttribute("xmlns", "http://www.w3.org/1999/xhtml");

        QDomElement head = dom.createElement("head");
        QDomElement titl = dom.createElement("title");
        QDomElement link = dom.createElement("link");
        link.setAttribute("rel", "stylesheet");
        link.setAttribute("href", "style.css");
        link.setAttribute("type", "text/css");
        head.appendChild(titl);
        head.appendChild(link);
        html.appendChild(head);

        QDomElement body = dom.createElement("body");
        QDomElement empty_p = dom.createElement("p");
        empty_p.setAttribute("class", "p");
        bool title = false;
        foreach(BookElement be, ch)
        {
            switch (be._typ)
            {
            case TEXT:
            {
                QDomElement p = dom.createElement("p");
                p.setAttribute("class", title ? "title-p" : "p");
                ADDTEXT(p, be._val);
                body.appendChild(p);
                break;
            }
            case TITLE:
                title = true;
                break;
            case TITLE_END:
                title = false;
                break;
            case EMPTY_LINE:
                //body.appendChild(dom.createElement("br"));
                body.appendChild(empty_p);
                break;
            default:
                break;
            }
        }
        html.appendChild(body);
        dom.appendChild(html);

        // Save
        QString outf(_dir + "/" + ddir + "/" + QString("ch%1.xhtml").arg(i));
        QFile outfile(outf);
        if (!outfile.open(QFile::WriteOnly | QFile::Truncate))
            ERR(tr("\nCan't open file %1: %2\n")
                .arg(outf).arg(outfile.errorString()),
                tr("<br>Can't open file %1: <b><font color=#ff0000>%2</font><b><br>")
                .arg(outf).arg(outfile.errorString()));
        QTextStream out(&outfile);
        out.setCodec(QTextCodec::codecForName("UTF-8"));
        dom.save(out, 4, QDomNode::EncodingFromTextStream);
    }
    return true;
} // EPUB::chapters


////////////////////////////////////////////////////////////////////////
bool EPUB::css()
{
    QString outf(_dir + "/" + ddir + "/style.css");
    QFile outfile(outf);
    if (!outfile.open(QFile::WriteOnly | QFile::Truncate))
        ERR(tr("\nCan't open file %1: %2\n")
            .arg(outf).arg(outfile.errorString()),
            tr("<br>Can't open file %1: <b><font color=#ff0000>%2</font><b><br>")
            .arg(outf).arg(outfile.errorString()));
    QTextStream out(&outfile);
    out << Config::CSS() << endl;
    return true;
} // EPUB::css
