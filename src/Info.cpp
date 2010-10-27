/*
 * $Id$
 *
 * Info panel inplementation
 */

#include <QByteArray>
#include <QBuffer>
#include <QDataStream>
#include <QFile>
#include <QFileInfo>
#include <QLabel>

#include "EPUBParser.h"
#include "FB2Parser.h"
#include "LRFParser.h"
#include "Info.h"
#include "FPanel.h"
#include "unzip.h"

QLocale Info::_loc(QLocale::English, QLocale::UnitedStates);

////////////////////////////////////////////////////////////////////////
Info::Info(QWidget *parent) : QFrame(parent)
{
} // Info::Info

////////////////////////////////////////////////////////////////////////
Info::~Info()
{
} // Info::~Info

////////////////////////////////////////////////////////////////////////
void Info::disable()
{
    foreach (QList<QLabel*> labels, _labels)
    {
        foreach(QLabel* l, labels)
        {
            l->setEnabled(false);
        }
        labels[1]->setText(QString());
    }
} // Info::disable


////////////////////////////////////////////////////////////////////////
void Info::infoReq(FPanel *panel, const QString& dname, const QString& fname)
{
    disable();

    // Directory name
    QList<QLabel*> dl = _labels["path"];
    dl[0]->setEnabled(true);
    dl[1]->setEnabled(true);
    dl[1]->setText(dname);
    if (fname == QString(".."))
        return;

    // File name
    QList<QLabel*> fl = _labels["name"];
    fl[0]->setEnabled(true);
    fl[1]->setEnabled(true);
    fl[1]->setText(fname);

    BookData::BType  type = BookData::getType(fname);
    if (type == BookData::Unknown)
        return;

    QList<QLabel*> al = _labels["author"];
    al[0]->setEnabled(true);
    al[1]->setEnabled(true);
    al[1]->setText("???");
    QList<QLabel*> tl = _labels["title"];
    tl[0]->setEnabled(true);
    tl[1]->setEnabled(true);
    tl[1]->setText("???");

    BookData bd;
    if (getBookData(panel, type, dname + "/" + fname, &bd))
    {
        al[1]->setText(bd.author);
        if (bd.page != 0)
            tl[1]->setText(QString("%1&nbsp;&nbsp;&nbsp;&nbsp;<font color=#0000ff>"
                                   "Page: %2</font>")
                           .arg(bd.title).arg(bd.page));
        else
            tl[1]->setText(bd.title);
    }
} // Info::infoReq


////////////////////////////////////////////////////////////////////////
bool Info::getBookData(FPanel *panel, const BookData::BType type, const QString& fname,
                       BookData *bd)
{
    if (type == BookData::Unknown)
        return false;

    //qDebug("Look for \"%s\"", qPrintable(fname));
    // Look in cache first
    QMap<QString, BookData>::const_iterator it = panel->books_data.find(fname);
    if (it != panel->books_data.end())
    {
        //qDebug("--- Found");
        *bd = it.value();
        return true;
    }

    //qDebug("--- Not found");
    // Not found in cache - extract book data from the file
    bd->page = 0;
    bool rc;
    switch (type)
    {
    case BookData::FB2:
        rc = fillFB2Info(fname, bd);
        break;
    case BookData::LRF:
        rc = fillLRFInfo(fname, bd);
        break;
    case BookData::TXT:
        rc = fillTXTInfo(fname, bd);
        break;
    case BookData::PDF:
        rc = fillPDFInfo(fname, bd);
        break;
    case BookData::RTF:
        rc = fillRTFInfo(fname, bd);
        break;
    case BookData::EPUB:
        rc = fillEPUBInfo(fname, bd);
        break;
    default:
        rc = false;
    }

    //qDebug("--- Not found, \"%s\" / %d => %d", qPrintable(fname), type, rc);
    if (rc)
    {
        QFileInfo fi(fname);

        bd->type = type;
        bd->size = fi.size();
        QDateTime ft(fi.lastModified().isValid() ? fi.lastModified() : QDateTime::currentDateTime());
        bd->date = _loc.toString(ft.toUTC(), "ddd, d MMM yyyy hh:mm:ss UTC");
        bd->is_file = true;
        panel->books_data.insert(fname, *bd);
    }

    return rc;
} // Info::getBookData


////////////////////////////////////////////////////////////////////////
bool Info::fillFB2Info(const QString& fullname, BookData *bd)
{
    // Input file
    QFile i_file(fullname);
    if (!i_file.open(QIODevice::ReadOnly))
        return false;

    // Output buffer (used only if zipped format is detected
    QBuffer     outbuff;
    if (!outbuff.open(QIODevice::ReadWrite))
        return false;

    // Read all input file
    QByteArray  data = i_file.readAll();
    QDataStream inp_stream(&data, QIODevice::ReadOnly);
    inp_stream.setByteOrder(QDataStream::LittleEndian);
    qint32 head;
    inp_stream >> head;

    // Set  FB2 parser
    FB2Parser        handler;
    FB2ErrorHandler  errorHandler(fullname);
    QXmlInputSource  source;
    QXmlSimpleReader reader;
    reader.setErrorHandler(&errorHandler);
    reader.setContentHandler(&handler);

    if (head == ZIP_MAGIC)
    {
        UnZip            uz;
        QBuffer          *buff = new QBuffer(&data);
        UnZip::ErrorCode ec = uz.openArchive(buff);

        if (ec != UnZip::Ok)
            return false;
        QStringList      flist = uz.fileList();

        //foreach(QString f, flist) {
        //    qDebug("    %s", qPrintable(f));
        //}
        if (flist.size() < 1)
            return false;
        ec = uz.extractFile(flist[0], &outbuff, UnZip::SkipPaths);
        if (ec != UnZip::Ok)
            return false;

        source.setData(outbuff.data());  // Set data for FB2 parser
    }
    else
        source.setData(data);            // Set data for FB2 parser

    // Parse FB2
    if (!reader.parse(source))
        return false;

    bd->author = handler.author();
    bd->title  = handler.title();
    return true;
} // Info::fillFB2Info


////////////////////////////////////////////////////////////////////////
#if defined(_MSC_VER)
#   pragma pack (1)
#endif
struct LRFHeader {
    uint8_t  sign[8];       // 0x00
    uint16_t version;       // 0x08
    uint16_t pseudokey;     // 0x0A
    uint32_t rootObjectId;  // 0x0C
    uint64_t numOfObjects;  // 0x10
    uint64_t objIndexOffs;  // 0x18
    uint32_t unk1;          // 0x20
    uint8_t  bindDirection; // 0x24
    uint8_t  pad1;          // 0x25
    uint16_t dpi;           // 0x26
    uint16_t pad2;          // 0x28
    uint16_t scrWidth;      // 0x2A
    uint16_t scrHeight;     // 0x2C
    uint8_t  colorDepth;    // 0x2E
    uint8_t  pad3;          // 0x2F
    uint8_t  unk2[20];      // 0x30
    uint32_t tocObjID;      // 0x44
    uint32_t tocObjOffs;    // 0x48
    uint16_t docInfoCSize;  // 0x4C
}
#if !defined(_MSC_VER)
   __attribute__ ((__packed__))
#endif
;

bool Info::fillLRFInfo(const QString& fullname, BookData *bd)
{
    LRFHeader      hdr;

    // Input file and stream
    QFile i_file(fullname);
    if (!i_file.open(QIODevice::ReadOnly))
        return false;

    // Read header
    if (i_file.read((char *)&hdr, sizeof(LRFHeader)) != sizeof(LRFHeader))
        return false;

    uint16_t thumbType = 0;     // 0x4E
    uint32_t thumbSize = 0;     // 0x50
    uint32_t docInfoUSize = 0;  // 0x54 if version >= 800 or 0x4E otherwise
    if (hdr.version >= 800)
    {
        if (i_file.read((char *)&thumbType, sizeof(thumbType)) != sizeof(thumbType))
            return false;
        if (i_file.read((char *)&thumbSize, sizeof(thumbSize)) != sizeof(thumbSize))
            return false;
    }
    if (i_file.read((char *)&docInfoUSize, sizeof(docInfoUSize)) != sizeof(docInfoUSize))
        return false;

    // Some sanity check
    if (!docInfoUSize || !hdr.docInfoCSize)
        return false;
    if (docInfoUSize > MAX_SANE_U_SIZE  ||  hdr.docInfoCSize > MAX_SANE_C_SIZE)
        return false;

    // Read metainfo
    QByteArray cmetainfo = i_file.read(hdr.docInfoCSize);
    if (cmetainfo.size() != hdr.docInfoCSize)
        return false;


    // Uncompress metainfo
    QByteArray  pref_ba;
    QDataStream pref_ds(&pref_ba, QIODevice::WriteOnly);
    pref_ds.setByteOrder(QDataStream::BigEndian);
    pref_ds << (quint32) docInfoUSize;
    cmetainfo.prepend(pref_ba);
    QByteArray umetainfo = qUncompress(cmetainfo);
    if (umetainfo.isEmpty())
        return false;

    // Parse metainfo
    LRFParser        handler;
    LRFErrorHandler  errorHandler(fullname);
    QXmlInputSource  source;
    QXmlSimpleReader reader;
    reader.setErrorHandler(&errorHandler);
    reader.setContentHandler(&handler);
    source.setData(umetainfo);
    if (!reader.parse(source))
        return false;

    bd->author = handler.author();
    bd->title  = handler.title();
    return true;
} // Info::fillLRFInfo


////////////////////////////////////////////////////////////////////////
bool Info::fillEPUBInfo(const QString& fullname, BookData *bd)
{
    // Set  FB2 parser
    EPUBParser        handler;
    EPUBErrorHandler  errorHandler(fullname);

    // Archive
    UnZip            uz;
    UnZip::ErrorCode ec = uz.openArchive(fullname);
    if (ec != UnZip::Ok)
        return false;

    {
        QXmlInputSource   source;
        QXmlSimpleReader  reader;
        reader.setErrorHandler(&errorHandler);
        reader.setContentHandler(&handler);

        // Read container
        QBuffer     outcont;
        if (!outcont.open(QIODevice::ReadWrite))
            return false;
        ec = uz.extractFile("META-INF/container.xml", &outcont, UnZip::SkipPaths);
        if (ec != UnZip::Ok)
            return false;
        source.setData(outcont.data());
        errorHandler.setFname("META-INF/container.xml");
        if (!reader.parse(source))
            return false;
    }
    QString content_file = handler.rootfile();
    if (content_file.isEmpty())
        return false;
    {
        // We cannot reuse QXmlInputSource here despite source.reset() call -
        // on a second attempt QTXml parser chokes on BOM - every time. I guess
        // it resets itself too much :)

        QXmlInputSource   source;
        QXmlSimpleReader  reader;
        reader.setErrorHandler(&errorHandler);
        reader.setContentHandler(&handler);

        // Read content
        QBuffer     outopf;
        if (!outopf.open(QIODevice::ReadWrite))
            return false;
        ec = uz.extractFile(content_file, &outopf, UnZip::SkipPaths);
        if (ec != UnZip::Ok)
            return false;
        source.setData(outopf.data());
        errorHandler.setFname(content_file);
        if (!reader.parse(source))
            return false;
    }
    bd->author = handler.author();
    bd->title  = handler.title();
    return true;
}

/*
 * Not implemented yet!!!
 */
////////////////////////////////////////////////////////////////////////
bool Info::fillRTFInfo(const QString& fullname, BookData *bd)
{
    QFileInfo fi(fullname);

    bd->author = "Unknown";
    bd->title  = fi.baseName();
    return true;
} // Info::fillRTFInfo

////////////////////////////////////////////////////////////////////////
bool Info::fillTXTInfo(const QString& fullname, BookData *bd)
{
    QFileInfo fi(fullname);

    bd->author = "Unknown";
    bd->title  = fi.baseName();
    return true;
} // Info::fillTXTInfo

////////////////////////////////////////////////////////////////////////
bool Info::fillPDFInfo(const QString& fullname, BookData *bd)
{
    QFileInfo fi(fullname);

    bd->author = "Unknown";
    bd->title  = fi.baseName();
    return true;
} // Info::fillPDFInfo
