/*
 * $Id$
 *
 * Media implementation
 */
#if defined(LINUX) || defined (MACOSX)
#include <unistd.h>
#define DEBUG_INFO "/tmp/debug_info.zip"
#define FNAME_CASE Qt::CaseSensitive
#endif

#if defined(WINDOWS)
#include <io.h>
#define DEBUG_INFO "C:/TEMP/debug_info.zip"
#define FNAME_CASE Qt::CaseInsensitive
#endif

#define CONFIG_ROOT       (_isSD ? Config::rootSD() : Config::rootPRS())
#define CONFIG_ROOT_THUMB (_isSD ? Config::rootSDtmb() : Config::rootPRStmb() );

#include <QCloseEvent>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QScrollBar>
#include <QSettings>
#include <QTextCodec>
#include <QTextStream>

#include "Config.h"
#include "FPanel.h"
#include "Info.h"
#include "LogWidget.h"
#include "Media.h"
#include "mngr505.h"
#include "utils.h"
#include "zip.h"

#define H(s)       QString(s).replace(" ", "&nbsp;")
#define DEF_NEXTID 500

//#define DEBUG_COLL_REORDERING
#define TO_BOTTOM(w) do {                   \
    qApp->processEvents();                  \
    QScrollBar *s = w->verticalScrollBar(); \
    if (s)                                  \
        s->setValue(s->maximum());          \
} while(0)

#define PLAYLIST_T (_isSD ? "playlist"   : "cache:playlist")
#define TEXT_T     (_isSD ? "text"       : "cache:text")
#define IMAGE_T    (_isSD ? "image"      : "cache:image")
#define AUDIO_T    (_isSD ? "audio"      : "cache:audio")
#define ITEM_T     (_isSD ? "item"       : "cache:item")
#define NAME_T     (_isSD ? "cache.xml"  : "media.xml")
#define BNAME_T    (_isSD ? "cache"      : "media")
#define SOURCEID_T (_isSD ? "500"        : "1")
#define OTHERS_T   (_isSD ? Config::othersSD() : Config::othersPRS())

#define GEN_INFO_F  "general"
#define LOGWIDGET_F "logwidget"
#define FILENAMES_F "full_list"

////////////////////////////////////////////////////////////////////////
PreviewXml::PreviewXml(QWidget *par, QString *prev_data, bool *wasEdit)
    : QMainWindow(par), stopped(false), _prev_data(prev_data), _wasEdit(wasEdit)
{
    QWidget     *cwidget = new QWidget(this);
    QGridLayout *gl = new QGridLayout(cwidget);

    ok = new QPushButton(cwidget);
    ok->setText(tr("Stop"));
    QFont f = ok->font();
    f.setBold(true);
    ok->setFont(f);

    edit = new QPushButton(cwidget);
    edit->setText(tr("Edit"));
    edit->setEnabled(false);

    cancel = new QPushButton(cwidget);
    cancel->setText(tr("Cancel"));
    f = cancel->font();
    f.setBold(true);
    cancel->setFont(f);
    cancel->setEnabled(false);

    te = new QTextEdit(cwidget);
    te->setReadOnly(true);
    te->setUndoRedoEnabled(false);
    te->setLineWrapMode(QTextEdit::NoWrap);
    connect(te, SIGNAL(textChanged()), this, SLOT(textChanged()));

    gl->addWidget(te,     0, 0, 1, 3);
    gl->addWidget(edit,   1, 1, 1, 1, Qt::AlignHCenter);
    gl->addWidget(ok,     1, 2, 1, 1, Qt::AlignHCenter);
    gl->addWidget(cancel, 1, 0, 1, 1, Qt::AlignHCenter);

    setCentralWidget(cwidget);

    connect(ok,     SIGNAL(pressed()), this, SLOT(stopReq()));
    connect(edit,   SIGNAL(pressed()), this, SLOT(editReq()));
    connect(cancel, SIGNAL(pressed()), this, SLOT(deleteLater()));
    setWindowTitle(tr("media.xml file preview"));
} // PreviewXml::PreviewXml


////////////////////////////////////////////////////////////////////////
PreviewXml::~PreviewXml()
{
    QSettings st(mngr505::_company, mngr505::_appName);

    // Save geomtery
    st.beginGroup("PreviewXml");
    st.setValue("size", size());
    st.setValue("pos", pos());
    st.endGroup();
} // PreviewXml::~PreviewXml

////////////////////////////////////////////////////////////////////////
void PreviewXml::editReq()
{
    te->setReadOnly(false);
    te->setUndoRedoEnabled(true);
    edit->setText(tr("Revert"));
    disconnect(edit, SIGNAL(pressed()), 0, 0);
    connect(edit,    SIGNAL(pressed()), this, SLOT(revertReq()));
} // PreviewXml::editReq


////////////////////////////////////////////////////////////////////////
void PreviewXml::saveReq()
{
    if (*_wasEdit)
        *_prev_data = te->toPlainText();

    deleteLater();
} // PreviewXml::saveReq


////////////////////////////////////////////////////////////////////////
void PreviewXml::revertReq()
{
    *_wasEdit = false;
    te->setPlainText(*_prev_data);
} // PreviewXml::revertReq


////////////////////////////////////////////////////////////////////////
void PreviewXml::show()
{
    QSettings    st(mngr505::_company, mngr505::_appName);
    QVariant     v;

    // Restore geometry
    st.beginGroup("PreviewXml");
    v = st.value("size");
    if (v.isValid())
        resize(v.toSize());
    v = st.value("pos");
    if (v.isValid())
        move(v.toPoint());

    st.endGroup();
    QMainWindow::show();
} // PreviewXml::show


////////////////////////////////////////////////////////////////////////
Media::Media(FPanel *parent) : _par(parent), _readok(false), _wasEdit(false)
{
} // Media::Media


////////////////////////////////////////////////////////////////////////
int Media::getID()
{
    int rc;

    if (_isSD)
        // SD card mode
        while (_usedIDs.contains(rc = _nextID++)) ;
    else
    {
        // eBook mode
        if (_IDs.isEmpty())
            return _nextID++;
        rc = _IDs.first();
        _IDs.pop_front();
    }
    return rc;
} // Media::getID


////////////////////////////////////////////////////////////////////////
QString Media::getPath(const QString& fpath)
{
    if (!fpath.startsWith(_root))
        return fpath;

    QString rc = fpath.mid(_root.length());
    while (rc.startsWith("/"))
        rc = rc.mid(1);

    return rc;
} // Media::getPath


////////////////////////////////////////////////////////////////////////
bool Media::readPRS(const QString& root,  const QString& bookroot,
                    const QString& fname, const QString& fxname, QString& errText)
{
    _readok = false;

    {
       QFile        media_file(fname);
       int          errLine, errCol;
       QString      errMsg;
       if (!media_file.open(QIODevice::ReadOnly))
       {
           errText = tr("Can't open file %1: %2")
               .arg(fname).arg(media_file.errorString());
           return false;
       }
       if (!_dom.setContent(&media_file, &errMsg, &errLine, &errCol))
       {
           errText = tr("File: %1<br>Line: %2, Column: %3 -- %4")
               .arg(fname).arg(errLine).arg(errCol).arg(errMsg);
           return false;
       }
    }
    {
       QFile        media_file(fxname);
       int          errLine, errCol;
       QString      errMsg;
       if (!media_file.open(QIODevice::ReadOnly))
       {
           errText = tr("Can't open file %1: %2")
               .arg(fname).arg(media_file.errorString());
           return false;
       }
       if (!_dom_ext.setContent(&media_file, &errMsg, &errLine, &errCol))
       {
           errText = tr("File: %1<br>Line: %2, Column: %3 -- %4")
               .arg(fname).arg(errLine).arg(errCol).arg(errMsg);
           return false;
       }
    }

    // Get max ID
    QDomNodeList books = _dom.elementsByTagName("xdbLite");
    if (books.count() != 1)
    {
        errText = tr("Should be one \"xdbLite\" tag!");
        return false;
    }
    QDomNode nextID = books.item(0).attributes().namedItem("nextID");
    if (nextID.isNull())
    {
        errText = tr("Tag \"xdbLite\" should have \"nextID\" attribute.");
        return false;
    }
    _nextID = nextID.nodeValue().toInt();

    books = _dom.elementsByTagName("cache:text");
    if (books.count() < 1)
    {
        QDomNodeList records = _dom.elementsByTagName("records");
        if (records.count() != 1)
        {
            errText = tr("Should be one \"records\" tag.");
            return false;
        }
        _books_parent = records.item(0);
        _first_text = QDomNode();
    }
    else
    {
        _books_parent = books.item(0).parentNode();
        _first_text = books.item(0);
    }
    _par->books_data.clear();
    _par->books_order.clear();
    for (int i=0; i<books.count(); i++) {
        QDomNode         b      = books.item(i);
        QDomNode         path   = b.attributes().namedItem("path");
        QDomNode         author = b.attributes().namedItem("author");
        QDomNode         title  = b.attributes().namedItem("title");
        QDomNode         page   = b.attributes().namedItem("page");
        if (path.isNull())
        {
            errText = QString("File: %1<br>Line: %2 - missing path")
                .arg(fname).arg(b.lineNumber());
            return false;
        }
        QString a = author.isNull() ? QString("Unknown") : author.nodeValue();
        QString t = title.isNull() ? QString("Unknown") : title.nodeValue();

        // Author and title
        BookData bd(a, t);
        bd.path = path.nodeValue();

        // Page
        if (!page.isNull())
            bd.page = page.nodeValue().toInt();

        bd.is_media = true;
        //qDebug("+++ Book \"%s\" inserted from media.xml",
        //      qPrintable(root + "/" + path.nodeValue()));
        _par->books_data.insert(root + "/" + path.nodeValue(), bd);
        _par->books_order.append(bd);
    }

    _root = root;
    _bookroot = bookroot;
    if (_bookroot.startsWith(_root))
        _bookroot = _bookroot.mid(_root.length());
    if (_bookroot.startsWith("/"))
        _bookroot = _bookroot.mid(1);
    _mediafname = fname;
    _extfname = fxname;
    _readok = true;
    _IDs.clear();
    _isSD = false;
    _thumbroot = CONFIG_ROOT_THUMB;
    if (_thumbroot.startsWith("/"))
        _thumbroot = _thumbroot.mid(1);
    return true;
} // Media::read

////////////////////////////////////////////////////////////////////////
bool Media::readSD(const QString& root, const QString& bookroot, const QString& fname, const QString& fxname, QString& errText)
{
    _readok = false;

    {
       QFile        cache_file(fname);
       int          errLine, errCol;
       QString      errMsg;
       if (!cache_file.open(QIODevice::ReadOnly))
       {
           errText = QString("Can't open file %1: %2")
               .arg(fname).arg(cache_file.errorString());
           return false;
       }
       if (!_dom.setContent(&cache_file, &errMsg, &errLine, &errCol))
       {
           errText = QString("File: %1<br>Line: %2, Column: %3 -- %4")
               .arg(fname).arg(errLine).arg(errCol).arg(errMsg);
           return false;
       }
    }
    {
       QFile        cache_file(fxname);
       int          errLine, errCol;
       QString      errMsg;
       if (!cache_file.open(QIODevice::ReadOnly))
       {
           errText = QString("Can't open file %1: %2")
               .arg(fname).arg(cache_file.errorString());
           return false;
       }
       if (!_dom_ext.setContent(&cache_file, &errMsg, &errLine, &errCol))
       {
           errText = QString("File: %1<br>Line: %2, Column: %3 -- %4")
               .arg(fname).arg(errLine).arg(errCol).arg(errMsg);
           return false;
       }
    }

    // Get max ID
    QDomNodeList books = _dom.elementsByTagName("cache");
    if (books.count() != 1)
    {
        errText = "Should be one \"cache\" tag!";
        return false;
    }
    QDomNode nextID = books.item(0).attributes().namedItem("nextID");
    _nextID = nextID.isNull() ? DEF_NEXTID : nextID.nodeValue().toInt();

    // Go over all elements and collect used IDs
    collectIDs(_dom);

    books = _dom.elementsByTagName("text");
    if (books.count() < 1)
    {
        QDomNodeList records = _dom.elementsByTagName("cache");
        _books_parent = records.item(0);
        _first_text = QDomNode();
    }
    else
    {
        _books_parent = books.item(0).parentNode();
        _first_text = books.item(0);
    }

    _par->books_data.clear();
    for (int i=0; i<books.count(); i++) {
        QDomNode         b      = books.item(i);
        QDomNode         path   = b.attributes().namedItem("path");
        QDomNode         author = b.attributes().namedItem("author");
        QDomNode         title  = b.attributes().namedItem("title");
        QDomNode         page   = b.attributes().namedItem("page");
        if (path.isNull())
        {
            errText = QString("File: %1<br>Line: %2 - missing path")
                .arg(fname).arg(b.lineNumber());
            return false;
        }
        QString a = author.isNull() ? QString("Unknown") : author.nodeValue();
        QString t = title.isNull() ? QString("Unknown") : title.nodeValue();

        // Author and title
        BookData bd(a, t);
        bd.path = path.nodeValue();

        // Page
        if (!page.isNull())
            bd.page = page.nodeValue().toInt();

        bd.is_media = true;
        //qDebug("+++ Book \"%s\" page: %d/%d inserted from cache.xml",
        //      qPrintable(root + "/" + path.nodeValue()), bd.page, bd.pages);
        _par->books_data.insert(root + "/" + path.nodeValue(), bd);
    }

    _root = root;
    _bookroot = bookroot;
    if (_bookroot.startsWith(_root))
        _bookroot = _bookroot.mid(_root.length());
    if (_bookroot.startsWith("/"))
        _bookroot = _bookroot.mid(1);
    _mediafname = fname;
    _extfname = fxname;
    _readok = true;
    _IDs.clear();
    _isSD = true;
    _thumbroot = CONFIG_ROOT_THUMB;
    if (_thumbroot.startsWith("/"))
        _thumbroot = _thumbroot.mid(1);
    return true;
} // Media::readSD


////////////////////////////////////////////////////////////////////////
void Media::collectIDs(QDomNode n)
{
    if (n.isNull())
        return;

    QDomElement el = n.toElement();
    if (!el.isNull())
    {
        QString ids = el.attribute("id", "None");
        if (ids != QString("None"))
            _usedIDs += ids.toInt();
    }

    QDomNodeList children = n.childNodes();
    for (int i=0; i<children.size(); i++)
        collectIDs(children.item(i));
} // Media::collectIDs


////////////////////////////////////////////////////////////////////////
bool Media::updateColl()
{
    QDomNodeList books;

    if (!ok())
        return false;

    _log = new LogWidget(_par);
    _log->show();
    _log->_ui.txt->insertHtml(tr("<h3>Scan for books</h3><br>"));
    TO_BOTTOM(_log->_ui.txt);

    // Remove old collections
    _colls.clear();
    for (;;) {
        books = _dom.elementsByTagName(PLAYLIST_T);
        if (books.count() < 1)
            break;
        QDomNode         b      = books.item(0);
        if (_isSD)
            _usedIDs.removeAll(b.attributes().namedItem("id").nodeValue().toInt());
        else
            _IDs += b.attributes().namedItem("id").nodeValue().toInt();
        QDomNode         p      = b.parentNode();
        p.removeChild(b);
    }
    if (Config::collectDbg())
        _dom_1 = _dom.toString(4);

    // Remove all books not read from media
    for (QMap<QString, BookData>::iterator it = _par->books_data.begin(); it != _par->books_data.end(); )
        if (!it.value().is_media)
            it = _par->books_data.erase(it);
        else
            it++;
    if (Config::collectDbg())
        _dom_2 = _dom.toString(4);

    // Scan for books
    scanDir("", "", 0, "", _root);
    if (Config::collectDbg())
        _dom_3 = _dom.toString(4);

    // Remove nonexistent media (images)
    _log->_ui.txt->insertHtml(tr("<br><h3>Remove nonexistent media files</h3><br>"));
    TO_BOTTOM(_log->_ui.txt);
    for (int beg = 0; ; ) {
        books = _dom.elementsByTagName(IMAGE_T);
        if (books.count() < beg+1)
            break;
        QDomNode  b      = books.item(beg);
        QString   path   = b.attributes().namedItem("path").nodeValue();
        if (QFile::exists(_root + "/" + path))
            beg++;
        else
        {
            _log->_ui.txt->insertPlainText(QString("%1\n").arg(path));
            _IDs += b.attributes().namedItem("id").nodeValue().toInt();
            QDomNode p      = b.parentNode();
            p.removeChild(b);

            if( Config::mngThumbs() )
            {
               QDir thumb( _root + "/" + _thumbroot + "/" + path );

               if( thumb.exists() )
                  if( thumb.remove( "main_thumbnail.jpg" ) )
                     thumb.rmpath( thumb.canonicalPath() );
            }
        }
        TO_BOTTOM(_log->_ui.txt);
        if (_log->canceled())
            return false;
    }

    // Remove nonexistent media (audio)
    for (int beg = 0; ; ) {
        books = _dom.elementsByTagName(AUDIO_T);
        if (books.count() < beg+1)
            break;
        QDomNode  b      = books.item(beg);
        QString   path   = b.attributes().namedItem("path").nodeValue();
        if (QFile::exists(_root + "/" + path))
            beg++;
        else
        {
            _log->_ui.txt->insertPlainText(QString("%1\n").arg(path));
            _IDs += b.attributes().namedItem("id").nodeValue().toInt();
            QDomNode p      = b.parentNode();
            p.removeChild(b);

            if( Config::mngThumbs() )
            {
               QDir thumb( _root + "/" + _thumbroot + "/" + path );

               if( thumb.exists() )
                  if( thumb.remove( "main_thumbnail.jpg" ) )
                     thumb.rmpath( thumb.canonicalPath() );
            }
        }
        TO_BOTTOM(_log->_ui.txt);
        if (_log->canceled())
            return false;
    }

    // Remove nonexistent books
    _log->_ui.txt->insertHtml(tr("<br><h3>Remove nonexistent books</h3><br>"));
    TO_BOTTOM(_log->_ui.txt);
    for (int beg = 0; ; ) {
        books = _dom.elementsByTagName(TEXT_T);
        if (books.count() < beg+1)
            break;
        QDomNode  b      = books.item(beg);
        QString   path   = b.attributes().namedItem("path").nodeValue();
        QMap<QString, BookData>::iterator it = _par->books_data.find(_root + "/" + path);
        if (it != _par->books_data.end()  &&  it->is_file)
            beg++;
        else
        {
            _log->_ui.txt->insertPlainText(QString("%1\n").arg(path));
            _IDs += b.attributes().namedItem("id").nodeValue().toInt();
            QDomNode p      = b.parentNode();
            p.removeChild(b);

            if( Config::mngThumbs() )
            {
               QDir thumb( _root + "/" + _thumbroot + "/" + path );

               if( thumb.exists() )
                  if( thumb.remove( "main_thumbnail.jpg" ) )
                     thumb.rmpath( thumb.canonicalPath() );
            }
        }
        TO_BOTTOM(_log->_ui.txt);
        if (_log->canceled())
            return false;
    }
    if (Config::collectDbg())
        _dom_4 = _dom.toString(4);

    // Check if _first_text already invalid ?
    books = _dom.elementsByTagName(TEXT_T);
    if (books.count() < 1)
    {
        _log->_ui.txt->insertPlainText(tr("Error: no books - nothing to do"));
        TO_BOTTOM(_log->_ui.txt);
        return false;
    }
    _first_text = books.item(0);

    // Transliterate all books and store id/path map
    books = _dom.elementsByTagName(TEXT_T);

    for (int i=0; i<books.count(); i++) {
        QDomElement  b = books.item(i).toElement();

        // Store for "other" collection creation
        _all_books[b.attribute("id").toInt()] = b.attribute("path");

        if (Config::confTranslC())
        {
            // Clear transliteration
            QString a = b.attribute("author");
            if (a[1] == QChar(':')  ||  a[2] == QChar(' '))
                b.setAttribute("author", a.mid(3));
            b.removeAttribute("titleSorter");
        }
        else
        {
            // Transliterate (First letter of author)
            if (Config::confTranslA())
            {
                QString a = b.attribute("author");
                if (a[1] != QChar(':')  ||  a[2] != QChar(' '))
                {
                    const char *transl = translit(a[0]);
                    if (transl)
                        b.setAttribute("author", QString(transl) + QString(": ") + a);
                }
            }

            // Transliterate (titleSorter)
            if (Config::confTranslT())
                b.setAttribute("titleSorter", translit(b.attribute("title")));
        }
    }
    if (Config::collectDbg())
        _dom_5 = _dom.toString(4);

    // Create "others" collection
    if (Config::others())
    {
        Collection o_collection;
        for (QMap<int, QString>::const_iterator it = _all_books.constBegin();
             it != _all_books.constEnd(); ++it)
        {
            int  id = it.key();
            bool in_coll = false;
            for (QMap<QString, Collection>::const_iterator ct = _colls.constBegin();
                 ct != _colls.constEnd(); ++ct)
            {
                if (ct.value().ids.contains(id))
                {
                    in_coll = true;
                    break;
                }
            }
            if (!in_coll)
                o_collection.ids += id;
        }
        if (o_collection.ids.size() > 0)
            _colls.insert(OTHERS_T, o_collection);
    }

    // Write collections back to XML file
    for (QMap<QString, Collection>::const_iterator it = _colls.constBegin();
         it != _colls.constEnd(); ++it)
    {
        if (!Config::emptyColl()  &&  it.value().ids.size() < 1)
            continue;
        QDomElement new_collection = _dom.createElement(PLAYLIST_T);
        QString coll_name = it.key();

        if (Config::replTruscore())
            coll_name.replace(QRegExp("_"), " ");
        if (!Config::replFrom().isEmpty())
            coll_name.replace(Config::replFrom(), Config::replTo());

        new_collection.setAttribute("title", coll_name);
        new_collection.setAttribute("sourceid", "0");
        new_collection.setAttribute("id", QString("%1").arg(getID()));
        int i = 0;
        for (; i<it.value().ids.size(); i++)
        {
            QDomElement new_el = _dom.createElement(ITEM_T);
            new_el.setAttribute("id", it.value().ids[i]);
            new_collection.appendChild(new_el);
        }
        if (Config::dummyColl()  &&  i < 1)
        {
            // Empty collection
            QDomElement new_el = _dom.createElement(ITEM_T);
            new_el.setAttribute("id", _first_text.toElement().attribute("id"));
            new_collection.appendChild(new_el);
        }
        _books_parent.appendChild(new_collection);
    }
    if (Config::collectDbg())
        _dom_6 = _dom.toString(4);

    // Fix NextID
    if (!_isSD)
        _dom.elementsByTagName("xdbLite").item(0).attributes()
            .namedItem("nextID").setNodeValue(QString("%1").arg(_nextID));

    // Remove all comments on highest level
    QDomNodeList    children = _dom.childNodes();
    QList<QDomNode> comments;
    for (int i=0; i<children.size(); i++)
        if (children.item(i).isComment())
            comments.append(children.item(i));
    for (int i=0; i<comments.size(); i++)
        _dom.removeChild(comments[i]);

    // Creation comment
    QDomComment created(_dom.createComment(
                            QString(" Created %1 by %2 version %3 ")
                            .arg(QDateTime::currentDateTime().toString())
                            .arg(mngr505::_appName)
                            .arg(mngr505::_version)));
    _dom.insertBefore(created, _dom.documentElement());
    if (Config::collectDbg())
        _dom_7 = _dom.toString(4);

    // Fix LogWindow - add "previewXml" button,
    // connect OK to saveXml and set tooltips
    LOG_CANCEL(_log)->setEnabled(true);
    LOG_CANCEL(_log)->setToolTip(tr("Write nothing to eBook"));
    disconnect(LOG_CANCEL(_log), SIGNAL(pressed()), 0, 0);
    connect(LOG_CANCEL(_log), SIGNAL(pressed()), this, SLOT(cancelXml()));
    LOG_OK(_log)->setEnabled(true);
    LOG_OK(_log)->setDefault(true);
    LOG_OK(_log)->setToolTip(
        tr("Write all changes back to <b><font color=#0000ff>%1</font></b><br>"
           "Note that the backup of original file will be saved as:<br> <b><font color=#0000ff>%2</font></b>")
        .arg(_mediafname).arg(_mediafname + ".bak"));
    disconnect(LOG_OK(_log), SIGNAL(pressed()), 0, 0);
    connect(LOG_OK(_log), SIGNAL(pressed()), this, SLOT(saveXml()));

    QGridLayout *gl = qobject_cast<QGridLayout *>(_log->_ui.centralwidget->layout());
    if (gl) {
        QHBoxLayout *hbox = new QHBoxLayout(0);
        hbox->addStretch();

        QPushButton *debug_b = new QPushButton(_log);
        debug_b->setText(tr("Collect debug information"));
        connect(debug_b, SIGNAL(pressed()), this, SLOT(collectDebug()));
        hbox->addWidget(debug_b);
        hbox->addStretch();

        QPushButton *prev_b  = new QPushButton(_log);
        prev_b->setText(tr("Preview %1 file").arg(NAME_T));
        prev_b->setToolTip(tr("Preview %1 file, then you can write it "
                              "back or cancel and write nothing to eBook")
                           .arg(NAME_T));
        connect(prev_b, SIGNAL(pressed()), this, SLOT(previewXml()));
        hbox->addWidget(prev_b);
        hbox->addStretch();

        gl->addLayout(hbox, 2, 0);

        QFont f = _log->_ui.bbox->button(QDialogButtonBox::Ok)->font();
        f.setBold(true);
        _log->_ui.bbox->button(QDialogButtonBox::Ok)->setFont(f);
        f = _log->_ui.bbox->button(QDialogButtonBox::Cancel)->font();
        f.setBold(true);
        _log->_ui.bbox->button(QDialogButtonBox::Cancel)->setFont(f);
    }
    TO_BOTTOM(_log->_ui.txt);
    return true;
} // Media::updateColl


////////////////////////////////////////////////////////////////////////
void Media::scanDir(const QString& nprefix, const QString& cprefix,
                    int dcnt, const char *dformat, const QString& dname)
{
    //+++++ DEBUG
    //_log->_ui.txt->insertPlainText(QString("scanDir(\"%1\", \"%2\", %3, \"%4\", \"%5\")\n")
    //                               .arg(nprefix).arg(cprefix).arg(dcnt)
    //                               .arg(dformat).arg(getPath(dname)));
    //TO_BOTTOM(_log->_ui.txt);

    if (_log->canceled())
        return;

    if( Config::dbdirsColl() )
    {
       if( 0 == getPath( dname ).indexOf( _thumbroot ) )
          return;
       if( 0 == getPath( dname ).indexOf( "database/cache" ) )
          return;
       if( 0 == getPath( dname ).indexOf( "database/layout" ) )
          return;
       if( 0 == getPath( dname ).indexOf( "database/sync" ) )
          return;
       if( 0 == getPath( dname ).indexOf( "Digital Editions" ) )
          return;
       if( 0 == getPath( dname ).indexOf( "epub", Qt::CaseInsensitive ) )
          return;
    }

    QTextCodec        *codec = QTextCodec::codecForName("UTF-8");
    QDir              dir(dname);
    QDir::Filters     filt = QDir::AllEntries | QDir::Hidden | QDir::System;
    QDir::SortFlags   sort = QDir::DirsFirst;
    QFileInfoList     files = dir.entryInfoList(filt, sort);
    QFileInfoList     result_list, files_order, entries_list;

    _log->_ui.txt->insertPlainText(QString("%1\n").arg(dname));
    TO_BOTTOM(_log->_ui.txt);

    // Try read "index.order" file and create files_order list
    result_list.clear();
    QFile order_file(dname + "/" + FPanel::order_fname);
    if (order_file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream in(&order_file);
        in.setCodec(codec);
        while (!in.atEnd())
        {
            QString line = in.readLine().trimmed();
            int ind = 0;
            for (; ind < files.size(); ind++)
                if (files[ind].fileName() == line)
                    break;
            if (ind < files.size())
            {
                // Found
                result_list.append(files[ind]);
                files.removeAt(ind);
            }
        }
    }

    // Insert directories which are not in "index.order"
    for (int i = files.size()-1; i >= 0; i--)
        if (files[i].isDir())
            result_list.prepend(files[i]);

    // Insert files which are not in "index.order"
    foreach (QFileInfo finfo, files)
        if (!finfo.isDir())
            result_list.append(finfo);

    // result_list is list of dirs and files in correct order
    //qDebug("result_list -- %s", qPrintable(dname));
    //foreach (QFileInfo finfo, result_list)           //+++++
    //    qDebug("  result_list -- %s / \"%s\"",       //+++++
    //           qPrintable(finfo.absoluteFilePath()), //+++++
    //          qPrintable(finfo.fileName()));         //+++++
    //qDebug("%s", "");                                //+++++

    // Go over result_list and descend to directories and collect file info
    entries_list.clear();
    foreach (QFileInfo finfo, result_list)
    {
        if (_log->canceled())
            return;

        QString   f(finfo.fileName());

        if (f == QString(".")  ||  f == QString(".."))
            continue;

        QString d = finfo.canonicalPath();
        if (d.isEmpty())
            d = finfo.absolutePath();

        if (finfo.isDir())
            entries_list.append(finfo);
        else
        {
            QString  fpath = d + "/" + f;
            BookData bd;
            if (Info::getBookData(_par, fpath, &bd))
            {
                _par->books_data[d + "/" + f].is_file = true;
                entries_list.append(finfo);

                // Add new book
                if (bd.is_file && !bd.is_media)
                {
                    _log->_ui.txt->insertPlainText(QString("    new book       - %1\n").arg(fpath));
                    TO_BOTTOM(_log->_ui.txt);

                    QDomElement new_book = _dom.createElement(TEXT_T);
                    new_book.setAttribute("author", bd.author);
                    new_book.setAttribute("date", bd.date);
                    new_book.setAttribute("id", QString("%1").arg(getID()));
                    new_book.setAttribute("mime", bd.mimeType());
                    new_book.setAttribute("path", getPath(fpath));
                    new_book.setAttribute("size", QString("%1").arg(bd.size));
                    new_book.setAttribute("sourceid", SOURCEID_T);
                    if (!_isSD)
                        new_book.setAttribute("status", "0");
                    new_book.setAttribute("title", bd.title);

                    if (_first_text.isNull())
                        _books_parent.appendChild(new_book);
                    else
                        _books_parent.insertBefore(new_book, _first_text);

                    if( Config::mngThumbs() && ! bd.cover.isNull() )
                    {
                       QString thumb( _root + "/" + _thumbroot + "/" + getPath( fpath ) );

                       if( ! QDir( thumb ).exists() )
                       {
                          if( QDir().mkpath( thumb ) )
                          {
                             QImage image( bd.cover );

                             // grayscale
                             QVector<QRgb> ct = image.colorTable();
                             for( int ni = 0; ni < ct.size(); ++ni )
                             {
                                int gray = qGray( ct.at( ni ) );
                                ct[ ni ] = qRgb( gray, gray, gray );
                             }
                             image.setColorTable(  ct );

                             image.save( thumb + "/main_thumbnail.jpg", "JPG" );

                             QString num;
                             QDomElement book_ext  = _dom_ext.createElement( "text" );
                             book_ext.setAttribute( "path", getPath(fpath) );
                             QDomElement thumbnail = _dom_ext.createElement( "thumbnail" );
                             QDomText text = _dom_ext.createTextNode( "main_thumbnail.jpg" );
                             thumbnail.appendChild( text );
                             thumbnail.setAttribute( "width", num.setNum( image.width() ) );
                             thumbnail.setAttribute( "height", num.setNum( image.height() ) );
                             book_ext.appendChild( thumbnail );

                             _dom_ext.documentElement().appendChild( book_ext );
                          }
                       }
                    }
                }
            }
        }
    }

    //qDebug("++ \"%s\" entries_list: %d", qPrintable(dname),     //+++++
    //       entries_list.size());                                //+++++
    //for (int i = 0; i<entries_list.size(); i++)                 //+++++
    //    qDebug("++ entries_list: \"%s\"",                       //+++++
    //           qPrintable(entries_list[i].absoluteFilePath())); //+++++
    //qDebug("%s", "");                                           //+++++

    // Calculate values for current directory possible enumeration
    int dcount=0, damount=0;
    char _dformat[10];
    _dformat[0] = 0;
    QString coll_name;
    QString rpath = getPath(dname);
    QString root = CONFIG_ROOT;
    while (root.startsWith("/"))
        root = root.mid(1);

    //// DEBUG
    //_log->_ui.txt->insertPlainText(QString(" -- rpath: \"%1\", root: \"%2\", "
    //                                       "_bookroot: \"%3\" -> %4\n")
    //                               .arg(rpath).arg(root).arg(_bookroot)
    //                               .arg(rpath.startsWith(root)));
    //TO_BOTTOM(_log->_ui.txt);

    // Create collection
    if (rpath.startsWith(root)  ||  root.isEmpty())
    {
        QString     path = rpath.mid(_bookroot.length());
        QStringList sp   = path.split('/', QString::SkipEmptyParts);

        //// DEBUG
        //_log->_ui.txt->insertPlainText(QString(" ---- s.size()=%1 ").arg(sp.size()));
        //foreach (QString s, sp)
        //    _log->_ui.txt->insertPlainText(QString(" ---- %1 ").arg(s));
        //_log->_ui.txt->insertPlainText("\n");
        //TO_BOTTOM(_log->_ui.txt);

        for (int i = 0; i < entries_list.size(); i++)
            if (entries_list[i].isDir())
                damount++;
        QString d1(QString("%1").arg(damount+1));
        sprintf(_dformat, "%%0%dd", d1.length());

        if (sp.size() > 0)
        {
            // Current collection name
            coll_name = sp[sp.size()-1];
            QFile   cname_file(dname + "/" + FPanel::cname_fname);
            if (cname_file.open(QIODevice::ReadOnly | QIODevice::Text))
            {
                QTextStream in(&cname_file);
                in.setCodec(codec);
                if (!in.atEnd())
                    coll_name = in.readLine().trimmed();
            }

            // Possible concat with parent names
            if (Config::concat())
            {
                if (!cprefix.isEmpty())
                    coll_name.prepend(Config::concatSep());
                coll_name.prepend(cprefix);
            }


            // Possible replace
            if (Config::replTruscore())
                coll_name.replace(QRegExp("_"), " ");
            if (!Config::replFrom().isEmpty())
                coll_name.replace(Config::replFrom(), Config::replTo());

            // Possible enum
            if (Config::enumColl()  &&  *dformat)
            {
                QString   numb;
                numb.sprintf(dformat, dcnt);
                if (!nprefix.isEmpty())
                    numb = nprefix + "." + numb;
                coll_name = numb + " " + coll_name;
            }

            _log->_ui.txt->insertPlainText(QString("    new collection - %1\n").arg(coll_name));

            TO_BOTTOM(_log->_ui.txt);

            _colls.insert(coll_name, Collection());
        }
    }

    //qDebug("== \"%s\" entries_list: %d", qPrintable(dname),     //+++++
    //       entries_list.size());                                //+++++
    //for (int i = 0; i<entries_list.size(); i++)                 //+++++
    //    qDebug("== entries_list: \"%s\"",                       //+++++
    //           qPrintable(entries_list[i].absoluteFilePath())); //+++++
    //qDebug("%s", "");                                           //+++++

    // Now scan undelraying directories and files
    for (int i = 0; i < entries_list.size(); i++)
    {
        QFileInfo finfo(entries_list[i]);
        QString   f(finfo.fileName());
        QString   numb;

        QString d = finfo.canonicalPath();
        if (d.isEmpty())
            d = finfo.absolutePath();

        if (finfo.isDir())
        {
            QString d = finfo.canonicalPath();
            if (d.isEmpty())
                d = finfo.absolutePath();

            if (*dformat)
            {
                numb.sprintf(dformat, dcnt);
                if (!nprefix.isEmpty())
                    numb = nprefix + "." + numb;
            }

            QString c(coll_name);
            if (Config::enumColl())
                c.replace(QRegExp("^[0-9.]+\\s+(.*)$"), "\\1");

            scanDir(numb, c, ++dcount, _dformat, d + "/" + f);
        }
        else if (!coll_name.isEmpty())
        {
            bool     ok;
            QString  fpath = d + "/" + f;

            int      id = getID(getPath(fpath), &ok);
            if (ok)
                _colls[coll_name].ids += id;
        }
    }
} // Media::scanDir

////////////////////////////////////////////////////////////////////////
int Media::getID(const QString& path, bool *ok)
{
    *ok = false;
    QDomNodeList books = _dom.elementsByTagName(TEXT_T);

    for (int i=0; i<books.count(); i++) {
        QDomElement  b = books.item(i).toElement();
        if (b.attribute("path") == path)
        {
            *ok = true;
            return b.attribute("id").toInt();
        }
    }

    return 0;
} // Media::getId


////////////////////////////////////////////////////////////////////////
void Media::previewXml()
{
    // Save new media.xml file to bytearray
    QTextCodec  *codec = QTextCodec::codecForName("UTF-8");
    QByteArray  data;
    QTextStream out(&data, QIODevice::WriteOnly);
    out.setCodec(codec);
    _dom.save(out, 4);

    // Preview window
    PreviewXml *p = new PreviewXml(_log, &_prev_data, &_wasEdit);
    p->show();

    // Fill data
    QTextStream in(&data, QIODevice::ReadOnly);
    in.setCodec(codec);
    while (!in.atEnd()) {
        QString l = in.readLine();
        p->te->insertPlainText(l);
        p->te->insertPlainText("\n");
        qApp->processEvents();
        if (p->stopped)
            break;
    }
    _wasEdit = false;
    _prev_data = p->te->toPlainText();

    p->ok->setText(tr("Save"));
    disconnect(p->ok, SIGNAL(pressed()), 0, 0);
    connect(p->ok, SIGNAL(pressed()), p, SLOT(saveReq()));
    p->cancel->setEnabled(true);
    p->edit->setEnabled(true);
} // Media::previewXml


////////////////////////////////////////////////////////////////////////
bool Media::cancelXml()
{
    QString errText;
    QString n = _isSD ? "Cache" : "Media";

    bool  rc = _isSD ? readSD (_root, _bookroot, _mediafname, _extfname, errText)
                     : readPRS(_root, _bookroot, _mediafname, _extfname, errText);
    if (!rc)
    {
        QMessageBox::warning(0, tr("%1 file error").arg(n),
                             tr("<b>%1 file error:</b><br>"
                                "<font color=#ff0000><b>%2</b></font>")
                             .arg(n).arg(errText));
        return false;
    }
    _log->close();
    return true;
} // Media::cancelXml


////////////////////////////////////////////////////////////////////////
bool Media::saveXml()
{
   {
       QFile media(_mediafname);

       // Remove porevious backup
       if (QFile::exists(_mediafname + ".bak"))
       {
           QFile old(_mediafname + ".bak");
           if (!old.remove())
           {
               QMessageBox::critical(0, tr("Can't remove file"),
                                        tr("Can't remove<b><font color=#0000ff>%1"
                                           "</font><b>: <b><font color=#ff0000>"
                                           "%2</font><b>")
                                        .arg(_mediafname + ".bak")
                                        .arg(old.errorString()));
               return false;
           }
       }

       // Backup media file
       if (!media.copy(_mediafname + ".bak"))
       {
           QMessageBox::critical(0, tr("Can't backup file"),
                                    tr("Can't copy <b><font color=#0000ff>%1"
                                       "</font><b> to <b><font color=#0000ff>"
                                       "%2</font><b>: <b><font color=#ff0000>"
                                       "%3</font><b>")
                                    .arg(_mediafname)
                                    .arg(_mediafname + ".bak")
                                    .arg(media.errorString()));
           return false;
       }
       media.close();

       if (!media.open(QFile::WriteOnly | QFile::Truncate))
       {
           QMessageBox::critical(0, tr("Can't open file"),
                                    tr("Can't open <b><font color=#0000ff>%1"
                                       "</font><b>:<br> <b><font color=#ff0000>"
                                       "%2</font><b>")
                                    .arg(_mediafname)
                                    .arg(media.errorString()));
           return false;
       }

       // Write new XML to media file
       QTextStream out(&media);
       if (_wasEdit)
           out << _prev_data;
       else
           _dom.save(out, 4);

       // Flush the data
       if (!media.flush())
       {
           QMessageBox::critical(0, tr("Can't flush file"),
                                    tr("Can't flush data to<b><font color=#0000ff>%1"
                                       "</font><b>:<br> <b><font color=#ff0000>"
                                       "%2</font><b>")
                                    .arg(_mediafname)
                                    .arg(media.errorString()));
           return false;
       }
       int fd = media.handle();
       if (fd != -1)
       {
#if defined(LINUX) || defined (MACOSX)
           fsync(fd);
           sync(); sync();
#elif defined(WINDOWS)
           _commit(fd);
#endif
       }
       media.close();
   }

   if( Config::mngThumbs() )
   {
       QFile media(_extfname);

       // Remove porevious backup
       if (QFile::exists(_extfname + ".bak"))
       {
           QFile old(_extfname + ".bak");
           if (!old.remove())
           {
               QMessageBox::critical(0, tr("Can't remove file"),
                                        tr("Can't remove<b><font color=#0000ff>%1"
                                           "</font><b>: <b><font color=#ff0000>"
                                           "%2</font><b>")
                                        .arg(_extfname + ".bak")
                                        .arg(old.errorString()));
               return false;
           }
       }

       // Backup media file
       if (!media.copy(_extfname + ".bak"))
       {
           QMessageBox::critical(0, tr("Can't backup file"),
                                    tr("Can't copy <b><font color=#0000ff>%1"
                                       "</font><b> to <b><font color=#0000ff>"
                                       "%2</font><b>: <b><font color=#ff0000>"
                                       "%3</font><b>")
                                    .arg(_extfname)
                                    .arg(_extfname + ".bak")
                                    .arg(media.errorString()));
           return false;
       }
       media.close();

       if (!media.open(QFile::WriteOnly | QFile::Truncate))
       {
           QMessageBox::critical(0, tr("Can't open file"),
                                    tr("Can't open <b><font color=#0000ff>%1"
                                       "</font><b>:<br> <b><font color=#ff0000>"
                                       "%2</font><b>")
                                    .arg(_extfname)
                                    .arg(media.errorString()));
           return false;
       }

       // Write new XML to media file
       QTextStream out(&media);
       _dom_ext.save(out, 4);

       // Flush the data
       if (!media.flush())
       {
           QMessageBox::critical(0, tr("Can't flush file"),
                                    tr("Can't flush data to<b><font color=#0000ff>%1"
                                       "</font><b>:<br> <b><font color=#ff0000>"
                                       "%2</font><b>")
                                    .arg(_mediafname)
                                    .arg(media.errorString()));
           return false;
       }
       int fd = media.handle();
       if (fd != -1)
       {
#if defined(LINUX) || defined (MACOSX)
           fsync(fd);
           sync(); sync();
#elif defined(WINDOWS)
           _commit(fd);
#endif
       }
       media.close();
   }

   if( Config::mngThumbs() )
   {
        QMessageBox::information(0, "OK", tr("Files <b><font color=#0000ff>%1</font></b> and <b><font color=#0000ff>%2</font></b>"
                                              "<font color=#008000><br>saved successfully</font>")
                                         .arg(H(_mediafname)).arg(H(_extfname)));
   }
   else
   {
        QMessageBox::information(0, "OK", tr("File  <b><font color=#0000ff>%1"
                                             "</font> <font color=#008000><br>"
                                             "saved successfully</font></b>")
                                          .arg(H(_mediafname)));
   }
   _log->close();
   return true;
}


#define INTERM_XML(SUFF, STRING) do {                                      \
    QString outf(ddirname + "/" + BNAME_T + SUFF);                         \
    QFile outfile(outf);                                                   \
    if (!outfile.open(QFile::WriteOnly | QFile::Truncate))                 \
    {                                                                      \
        QMessageBox::critical(0, tr("Can't open file"),                    \
                              tr("Can't open <b><font color=#0000ff>%1"    \
                                 "</font><b>:<br> <b><font color=#ff0000>" \
                                 "%2</font><b>")                           \
                              .arg(outf).arg(outfile.errorString()));      \
            return;                                                        \
        }                                                                  \
    QTextStream out(&outfile);                                             \
    out.setCodec(QTextCodec::codecForName("UTF-8"));                       \
    out << STRING;                                                         \
} while (0)

////////////////////////////////////////////////////////////////////////
void Media::collectDebug()
{
    // Collect full recursive filenames,
    //         LogWidget content
    //         old media.xml file
    //         new media.xml file
    Zip uz;

    // Select result file
    QSettings    st(mngr505::_company, mngr505::_appName);

    st.beginGroup("DebugInfo");
    QString dir  = st.value("output_file", DEBUG_INFO).toString();

    QString fileName = QFileDialog::getSaveFileName(0, tr("Select file for debug info"),
                                                    dir,
                                                    tr("Zip file (*.zip)"));
    if (fileName.isNull())
        return;
    if (!fileName.endsWith(".zip"))
        fileName.append(".zip");
    st.setValue("output_file", fileName);
    st.endGroup();

    // Open destination archive
    Zip::ErrorCode ec = uz.createArchive(fileName);
    if (ec != Zip::Ok)
    {
        QMessageBox::critical(0, tr("Can't open archive"),
                                  tr("Can't open <b><font color=#0000ff>%1"
                                     "</font><b>:<br> <b><font color=#ff0000>"
                                     "%2</font><b>")
                                  .arg(fileName)
                                  .arg(uz.formatError(ec)));
        return;
    }

    // Destination directory
    QFileInfo fi(fileName);
    QString   sdirname = fi.absolutePath();
    QString   basename = fi.fileName();
    QString   ddirname = sdirname + "/" + fi.completeBaseName();

    // Clear destination directory
    if (QFile::exists(ddirname)  &&  !deleteDir(ddirname))
    {
        QMessageBox::warning(0, tr("Remove directory error"),
                             tr("Can't remove old content from %1 directory")
                             .arg(ddirname));
        return;
    }
    if (!QDir(sdirname).mkdir(fi.completeBaseName()))
    {
        QMessageBox::warning(0, tr("Create directory error"),
                             tr("Can't create %1/%2 directory")
                             .arg(sdirname).arg(fi.completeBaseName()));
        return;
    }

    // General info
    {
        QFile outfile(ddirname + "/" + GEN_INFO_F);
        if (!outfile.open(QFile::WriteOnly | QFile::Truncate))
        {
            QMessageBox::critical(0, tr("Can't open file"),
                                  tr("Can't open <b><font color=#0000ff>%1"
                                     "</font><b>:<br> <b><font color=#ff0000>"
                                     "%2</font><b>")
                                  .arg(sdirname + "/" +GEN_INFO_F)
                                  .arg(outfile.errorString()));
            return;
        }
        QTextStream out(&outfile);
        out.setCodec(QTextCodec::codecForName("UTF-8"));
        out << "Version:   " << mngr505::_version << "\n";
        out << "Date:      " << QDateTime::currentDateTime().toString() << "\n";
        out << "Mode:      " << _par->getMode() << "\n";
        out << "Parent:    " << _par->objectName() << "\n";
        out << "  root:    " << _par->root() << "\n";
        out << "  pwd:     " << _par->pwd() << "\n";
        out << "\nSettings:\n";

        QSettings    st(mngr505::_company, mngr505::_appName);
        foreach (QString k, st.allKeys())
            out << "    \"" << k << "\" = \"" << st.value(k).toString() << "\"\n";
    }

    // Full list of filenames
    {
        QFile outfile(ddirname + "/" + FILENAMES_F);
        if (!outfile.open(QFile::WriteOnly | QFile::Truncate))
        {
            QMessageBox::critical(0, tr("Can't open file"),
                                  tr("Can't open <b><font color=#0000ff>%1"
                                     "</font><b>:<br> <b><font color=#ff0000>"
                                     "%2</font><b>")
                                  .arg(sdirname + "/" + FILENAMES_F)
                                  .arg(outfile.errorString()));
            return;
        }
        QTextStream out(&outfile);
        out.setCodec(QTextCodec::codecForName("UTF-8"));

        _dbgList.clear();
        _dbgList.append(QString(_par->root()) + "\n");
        dbgScanDir("  ", _par->root());
        foreach (QString s, _dbgList)
            out << s;
    }

    // Log widget content
    {
        QFile outfile(ddirname + "/" + LOGWIDGET_F);
        if (!outfile.open(QFile::WriteOnly | QFile::Truncate))
        {
            QMessageBox::critical(0, tr("Can't open file"),
                                  tr("Can't open <b><font color=#0000ff>%1"
                                     "</font><b>:<br> <b><font color=#ff0000>"
                                     "%2</font><b>")
                                  .arg(sdirname + "/" + LOGWIDGET_F)
                                  .arg(outfile.errorString()));
            return;
        }
        QTextStream out(&outfile);
        out.setCodec(QTextCodec::codecForName("UTF-8"));
        out << _log->_ui.txt->toPlainText();
    }

    // Copy media files - Old file
    {
        QString src = _par->mediafname();
        QString dst = ddirname + "/" + QFileInfo(_par->mediafname()).fileName();
        dst.replace(".", "_");
        dst.append(".xml");
        QFile srcf(src);
        if (!srcf.copy(dst))
        {
            QMessageBox::critical(0, tr("Can't copy file"),
                                  tr("Can't copy <b><font color=#0000ff>%1</font><b><br> "
                                     "to <b><font color=#0000ff>%2</font><b>:<br>"
                                     "<b><font color=#ff0000>%3</font><b>")
                                  .arg(src).arg(dst)
                                  .arg(srcf.errorString()));
            return;
        }
    }

    // Copy media files - Backup file
    if (QFile::exists(_par->mediafname() + ".bak"))
    {
        QString src = _par->mediafname() + ".bak";
        QString dst = ddirname + "/" + QFileInfo(_par->mediafname()).fileName() + ".bak";
        dst.replace(".", "_");
        dst.append(".xml");
        QFile srcf(src);
        if (!srcf.copy(dst))
        {
            QMessageBox::critical(0, tr("Can't copy file"),
                                  tr("Can't copy <b><font color=#0000ff>%1</font><b><br> "
                                     "to <b><font color=#0000ff>%2</font><b>:<br>"
                                     "<b><font color=#ff0000>%3</font><b>")
                                  .arg(src).arg(dst)
                                  .arg(srcf.errorString()));
            return;
        }
    }

    // Copy media files - New file
    {
        QString outf(ddirname + "/new-" + QFileInfo(_par->mediafname()).fileName());
        outf.replace(".", "_");
        outf.append(".xml");
        QFile outfile(outf);
        if (!outfile.open(QFile::WriteOnly | QFile::Truncate))
        {
            QMessageBox::critical(0, tr("Can't open file"),
                                  tr("Can't open <b><font color=#0000ff>%1"
                                     "</font><b>:<br> <b><font color=#ff0000>"
                                     "%2</font><b>")
                                  .arg(outf).arg(outfile.errorString()));
            return;
        }

        QTextStream out(&outfile);
        _dom.save(out, 4);
    }

    // Create intermediate XML files
    if (Config::collectDbg())
    {
        INTERM_XML("-1-After_removing_old_collection.xml", _dom_1);
        INTERM_XML("-2-After_removing_all_nonmedia_books.xml", _dom_2);
        INTERM_XML("-3-After_directory_scan.xml", _dom_3);
        INTERM_XML("-4-After_removing_nonexistent_entries.xml", _dom_4);
        INTERM_XML("-5-After_transliteration.xml", _dom_5);
        INTERM_XML("-6-After_collection_creation.xml", _dom_6);
        INTERM_XML("-7-Final.xml", _dom_7);
    }

    ec = uz.addDirectory(ddirname);
    if (ec != Zip::Ok)
    {
        QMessageBox::critical(0, tr("Can't add directory"),
                              tr("Can't add directory <b><font color=#0000ff>%1"
                                 "</font><b> to <b><font color=#0000ff>%2"
                                 "</font><b>:<br> <b><font color=#ff0000>"
                                 "%3</font><b>")
                              .arg(ddirname).arg(fileName)
                              .arg(uz.formatError(ec)));
        return;
    }

    ec = uz.closeArchive();
    if (ec != Zip::Ok)
    {
        QMessageBox::critical(0, tr("Can't close archive"),
                              tr("Can't close <b><font color=#0000ff>%1"
                                 "</font><b>:<br> <b><font color=#ff0000>"
                                 "%2</font><b>")
                              .arg(fileName)
                              .arg(uz.formatError(ec)));
        return;
    }

    deleteDir(ddirname);
    QMessageBox::information(0, tr("Success"),
                             tr("Debug info archive: <b><font color=#0000ff>%1"
                                "</font><b><br><br> <b><font color=#00a000>"
                                "Created successfully</font><b>")
                             .arg(fileName));
} // Media::collectDebug


////////////////////////////////////////////////////////////////////////
void Media::dbgScanDir(const QString& offs, const QString& dname)
{
    QDir              dir(dname);
    QDir::Filters     filt = QDir::AllEntries | QDir::Hidden | QDir::System;
    QDir::SortFlags   sort = QDir::DirsLast;
    QFileInfoList     files = dir.entryInfoList(filt, sort);

    int maxFSize=0;
    foreach (QFileInfo finfo, files)
        maxFSize = finfo.size() > maxFSize ? finfo.size() : maxFSize;
    QString m(QString("%1").arg(maxFSize));
    char format[10];
    sprintf(format, "%%%dld", m.length());

    foreach (QFileInfo finfo, files)
    {
        QString f = finfo.fileName();
        if (f == QString(".")  ||  f == QString(".."))
            continue;

        if (finfo.isDir())
        {
            _dbgList.append(QString("\n%1%2\n").arg(offs).arg(dname + "/" + f));
            dbgScanDir(offs + QString("  "), dname + "/" + f);
        }
        else
        {
            QString n;
            n.sprintf(format, finfo.size());
            _dbgList.append(QString("%1%2 %3\n").arg(offs).arg(n).arg(f));
        }
    }
} // Media::dbgScanDir


////////////////////////////////////////////////////////////////////////
bool Media::deleteDir(const QString& dname)
{
    QDir              dir(dname);
    QDir::Filters     filt = QDir::AllEntries | QDir::Hidden | QDir::System;
    QDir::SortFlags   sort = QDir::DirsLast;
    QFileInfoList     files = dir.entryInfoList(filt, sort);

    foreach (QFileInfo finfo, files)
    {
        QString f = finfo.fileName();
        if (f == QString(".")  ||  f == QString(".."))
            continue;

        if (finfo.isDir())
        {
            if (!deleteDir(dname + "/" + f))
                return false;
            //qDebug("RmDir:  \"%s\"", qPrintable(dname + "/" + f));
            if (!dir.rmdir(f))
                return false;
        }
        else
        {
            //qDebug("Remove: \"%s\"", qPrintable(dname + "/" + f));
            if (!dir.remove(f))
                return false;
        }
    }

    //qDebug("RmDir:  \"%s\"", qPrintable(QFileInfo(dname).absolutePath() + "/"
    //           + QFileInfo(dname).fileName()));
    return QDir(QFileInfo(dname).absolutePath()).rmdir(QFileInfo(dname).fileName());
} // Media::deleteDir
