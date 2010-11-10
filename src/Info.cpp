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
#include <QScopedArrayPointer>

#include "Config.h"
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
        if( labels.size() > 1 )
           labels[1]->setText(QString());
        else
        {
           QPixmap pm;
           labels[0]->setPixmap( pm );
        }
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
    QList<QLabel*> cl = _labels["cover"];
    cl[0]->setEnabled(true);

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

        if( ! bd.cover.isNull() )
        {
           QPixmap pm;
           pm.convertFromImage( bd.cover );
           cl[0]->setPixmap( pm.scaledToHeight( cl[0]->height(), Qt::SmoothTransformation ) );
        }
    }
} // Info::infoReq


////////////////////////////////////////////////////////////////////////
bool Info::getBookData(FPanel *panel, const BookData::BType type, const QString& fname, BookData *bd)
{
    if (type == BookData::Unknown)
        return false;

    //qDebug("Look for \"%s\"", qPrintable(fname));
    // Look in cache first
    QMap<QString, BookData>::iterator it = panel->books_data.find( fname );
    if( (it != panel->books_data.end()) && !it.value().cover.isNull() )
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
    if( rc )
    {
       if( it != panel->books_data.end() )
       {
          it.value().cover = bd->cover;
       }
       else
       {
          QFileInfo fi(fname);

          bd->type = type;
          bd->size = fi.size();
          QDateTime ft(fi.lastModified().isValid() ? fi.lastModified() : QDateTime::currentDateTime());
          bd->date = _loc.toString(ft.toUTC(), "ddd, d MMM yyyy hh:mm:ss UTC");
          bd->is_file = true;
          panel->books_data.insert(fname, *bd);
       }
    }
    return rc;
}

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

    // Get image for cover page

    if( ! handler.cover().isEmpty() )
    {
       QImage src;

       src.loadFromData( handler.cover() );
       bd->cover = src.scaledToHeight( 217, Qt::SmoothTransformation );

       if( QImage::Format_Indexed8 != bd->cover.format() )
          bd->cover = bd->cover.convertToFormat( QImage::Format_Indexed8, Qt::AutoColor );
    }
    else
       bd->cover.load( ":/icons/Graphics/thumb.jpg" );

    return true;
} // Info::fillFB2Info


////////////////////////////////////////////////////////////////////////
#if defined(_MSC_VER)
#   pragma pack (push,1)
#endif
struct LRFHeader
{
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

struct LRFObjectRecord
{
    uint32_t id;            //object ID, must be unique
    uint32_t offset;        //LRF offset to object data
    uint32_t size;          //size of the object data
    uint32_t reserved;      //NULL in the file
}
#if !defined(_MSC_VER)
   __attribute__ ((__packed__))
#endif
;

#if defined(_MSC_VER)
#   pragma pack (pop)
#endif

uint32_t LRFTags[] =
{
   6, /* 00 */ 0, /* 01 */ 4, /* 02 */ 4, /* 03 */ 4, /* 04 */ 0, /* 05 */ 0, /* 06 */ 4, /* 07 */
   4, /* 08 */ 4, /* 09 */ 4, /* 0A */ 0, /* 0B */-1, /* 0C */ 0, /* 0D */ 2, /* 0E */-1, /* 0F */
  -1, /* 10 */ 2, /* 11 */ 2, /* 12 */ 2, /* 13 */ 2, /* 14 */ 2, /* 15 */ 0, /* 16 */ 4, /* 17 */
   4, /* 18 */ 2, /* 19 */ 2, /* 1A */ 2, /* 1B */ 2, /* 1C */ 2, /* 1D */ 2, /* 1E */-1, /* 1F */
  -1, /* 20 */ 2, /* 21 */ 2, /* 22 */ 2, /* 23 */ 2, /* 24 */ 2, /* 25 */ 2, /* 26 */ 2, /* 27 */
   2, /* 28 */ 6, /* 29 */ 2, /* 2A */ 2, /* 2B */ 2, /* 2C */ 4, /* 2D */ 2, /* 2E */-1, /* 2F */
  -1, /* 30 */ 2, /* 31 */ 2, /* 32 */ 2, /* 33 */ 4, /* 34 */ 2, /* 35 */ 2, /* 36 */ 4, /* 37 */
   2, /* 38 */ 2, /* 39 */ 2, /* 3A */-1, /* 3B */ 2, /* 3C */ 2, /* 3D */ 2, /* 3E */-1, /* 3F */
  -1, /* 40 */ 2, /* 41 */ 2, /* 42 */-1, /* 43 */ 4, /* 44 */ 4, /* 45 */ 2, /* 46 */ 2, /* 47 */
   2, /* 48 */ 8, /* 49 */ 8, /* 4A */ 4, /* 4B */ 4, /* 4C */ 0, /* 4D */12, /* 4E */-1, /* 4F */
  -1, /* 50 */ 2, /* 51 */ 2, /* 52 */ 4, /* 53 */ 2, /* 54 */ 0, /* 55 */ 0, /* 56 */ 2, /* 57 */
   2, /* 58 */ 0, /* 59 */ 0, /* 5A */ 4, /* 5B */ 0, /* 5C */ 0, /* 5D */ 2, /* 5E */-1, /* 5F */
  -1, /* 60 */ 2, /* 61 */ 0, /* 62 */ 0, /* 63 */ 0, /* 64 */ 0, /* 65 */ 0, /* 66 */ 0, /* 67 */
   0, /* 68 */ 0, /* 69 */ 0, /* 6A */ 0, /* 6B */ 8, /* 6C */ 0, /* 6D */ 0, /* 6E */-1, /* 6F */
  -1, /* 70 */ 0, /* 71 */ 0, /* 72 */10, /* 73 */-1, /* 74 */ 2, /* 75 */ 2, /* 76 */ 2, /* 77 */
   0, /* 78 */ 2, /* 79 */ 2, /* 7A */ 4, /* 7B */ 4, /* 7C */-1, /* 7D */-1, /* 7E */-1, /* 7F */
  -1, /* 80 */ 0, /* 81 */ 0, /* 82 */-1, /* 83 */-1, /* 84 */-1, /* 85 */-1, /* 86 */-1, /* 87 */
  -1, /* 88 */-1, /* 89 */-1, /* 8A */-1, /* 8B */-1, /* 8C */-1, /* 8D */-1, /* 8E */-1, /* 8F */
  -1, /* 90 */-1, /* 91 */-1, /* 92 */-1, /* 93 */-1, /* 94 */-1, /* 95 */-1, /* 96 */-1, /* 97 */
  -1, /* 98 */-1, /* 99 */-1, /* 9A */-1, /* 9B */-1, /* 9C */-1, /* 9D */-1, /* 9E */-1, /* 9F */
  -1, /* A0 */ 4, /* A1 */ 0, /* A2 */-1, /* A3 */-1, /* A4 */ 0, /* A5 */ 0, /* A6 */ 4, /* A7 */
   0, /* A8 */ 0, /* A9 */ 0, /* AA */ 0, /* AB */ 0, /* AC */ 0, /* AD */ 0, /* AE */-1, /* AF */
  -1, /* B0 */ 0, /* B1 */ 0, /* B2 */ 0, /* B3 */ 0, /* B4 */ 0, /* B5 */ 0, /* B6 */ 0, /* B7 */
   0, /* B8 */ 0, /* B9 */ 0, /* BA */ 0, /* BB */ 0, /* BC */ 0, /* BD */ 0, /* BE */-1, /* BF */
  -1, /* C0 */ 0, /* C1 */ 0, /* C2 */ 2, /* C3 */ 0, /* C4 */ 2, /* C5 */ 2, /* C6 */ 0, /* C7 */
   2, /* C8 */ 0, /* C9 */ 2, /* CA */ 0, /* CB */ 2, /* CC */-1, /* CD */-1, /* CE */-1, /* CF */
  -1, /* D0 */ 0, /* D1 */ 0, /* D2 */-1, /* D3 */ 2, /* D4 */-1, /* D5 */ 0, /* D6 */14, /* D7 */
   4, /* D8 */ 8, /* D9 */ 2, /* DA */ 2, /* DB */ 2, /* DC */ 2, /* DD */-1, /* DE */-1, /* DF */
  -1, /* E0 */-1, /* E1 */-1, /* E2 */-1, /* E3 */-1, /* E4 */-1, /* E5 */-1, /* E6 */-1, /* E7 */
  -1, /* E8 */-1, /* E9 */-1, /* EA */-1, /* EB */-1, /* EC */-1, /* ED */-1, /* EE */-1, /* EF */
  -1, /* F0 */ 2, /* F1 */ 4, /* F2 */ 4, /* F3 */ 2, /* F4 */ 4, /* F5 */ 4, /* F6 */ 4, /* F7 */
   4, /* F8 */ 6, /* F9 */-1, /* FA */-1, /* FB */-1, /* FC */-1, /* FD */-1, /* FE */-1  /* FF */
};

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

    // Get image for cover page
    QScopedArrayPointer<LRFObjectRecord> objects( new LRFObjectRecord[ hdr.numOfObjects ] );

    if( ! i_file.seek( hdr.objIndexOffs ) )
        return false;

    if( i_file.read( (char*)objects.data(), sizeof(LRFObjectRecord) * hdr.numOfObjects ) != sizeof(LRFObjectRecord) * hdr.numOfObjects )
        return false;

    for( uint32_t ni = 0; ni < hdr.numOfObjects; ++ni )
    {
       const LRFObjectRecord& obj = objects[ ni ];

       if( ! i_file.seek( obj.offset ) )
          continue;

       uint32_t left     = obj.size;
       bool     next_obj = true;
       uint16_t sflags   = 0;
       uint32_t ssize    = 0;

       while( left > 0 )
       {
          uint16_t tag = 0;

          if( i_file.read( (char*)&tag, sizeof(tag) ) != sizeof(tag) )
             break;

          left -= sizeof(tag);

          uint32_t param_size = LRFTags[ tag & 0xFF ];
          char     data[ 16 ];

          if( (tag >> 8 != 0xF5) || (param_size < 0) )
             break;

          if( i_file.read( (char*)data, param_size ) != param_size )
             break;

          left -= param_size;

          if( tag == 0xF500 )                     // object type
          {
             if( 0x11 != *(uint16_t*)(&data[4]) ) // if not image stream
                break;                            //    move to next obj
          }
          else if( tag == 0xF504 )                // stream size
          {
             ssize = *(uint32_t*)&data[0];
          }
          else if( tag == 0xF554 )                // stream flags
          {
             sflags = *(uint16_t*)&data[0];
          }
          else if( tag == 0xF505 )                // stream start
          {
             next_obj = false;
             left    -= ssize;

             QByteArray image = i_file.read( ssize );
             if( image.size() != ssize )
                break;

//           if( sflags & 0x100 ) { Unpack }

             if( sflags & 0x200 )
             {
                uint32_t len = image.size() > 0x400 ? 0x400 : image.size();
                uint32_t key = (uint16_t)((ssize % hdr.pseudokey) + 0x0F) & 0xFF;

                key = key << 8  | key;
                key = key << 16 | key;

                uint32_t* p = (uint32_t*)image.data();
                while( len >= sizeof(uint32_t) )
                {
                   (*p++) ^= key;
                   len -= 4;
                }

                unsigned char* p1 = (unsigned char*)p;
                while( len > 0 )
                {
                   (*p1++) ^= (key & 0xFF);
                   len--;
                }
             }

             QImage src;

             src.loadFromData( image );
             bd->cover = src.scaledToHeight( 217, Qt::SmoothTransformation );

             if( QImage::Format_Indexed8 != bd->cover.format() )
                bd->cover = bd->cover.convertToFormat( QImage::Format_Indexed8, Qt::AutoColor );

             break;
          }
       }
       if( ! next_obj )
          break;
    }
    if( bd->cover.isNull() )
       bd->cover.load( ":/icons/Graphics/thumb.jpg" );

    return true;
} // Info::fillLRFInfo

////////////////////////////////////////////////////////////////////////
bool Info::fillEPUBInfo(const QString& fullname, BookData *bd)
{
    // Set EPUB parser
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

    // Get image for cover page
    QString cover_file = handler.coverfile();

    if( cover_file.isEmpty() )
       return true;

    cover_file.prepend( content_file.section( '/', -2, 0, QString::SectionIncludeTrailingSep ) );

    QBuffer image;
    if( image.open( QIODevice::ReadWrite ) )
    {
       ec = uz.extractFile( cover_file, &image, UnZip::SkipPaths );
       if( ec == UnZip::Ok )
       {
          QImage src;

          src.loadFromData( image.buffer() );
          bd->cover = src.scaledToHeight( 217, Qt::SmoothTransformation );

          if( QImage::Format_Indexed8 != bd->cover.format() )
             bd->cover = bd->cover.convertToFormat( QImage::Format_Indexed8, Qt::AutoColor );
       }
    }

    if( bd->cover.isNull() )
       bd->cover.load( ":/icons/Graphics/thumb.jpg" );

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
