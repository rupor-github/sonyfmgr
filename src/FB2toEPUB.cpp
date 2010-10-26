/*
 * $Id$
 *
 * FB2toEPUB implementation
 */
#include <QBuffer>
#include <QByteArray>
#include <QDataStream>
#include <QDir>
#include <QDomDocument>
#include <QFile>
#include <QTextStream>

#include "Config.h"
#include "EPUB.h"
#include "FB2toEPUB.h"
#include "Info.h"
#include "unzip.h"
#include "zip.h"

#define numb_el(x)  (sizeof(x)/sizeof(x[0]))
#define tr QObject::tr
#define MSG(P, H) do {           \
    if (Config::plainText())     \
        txt->insertPlainText(P); \
    else                         \
        txt->insertHtml(H);      \
} while (0)
#define ERR(P, H) do {           \
    if (Config::plainText())     \
        txt->insertPlainText(P); \
    else                         \
        txt->insertHtml(H);      \
    return false;                \
} while (0)

////////////////////////////////////////////////////////////////////////
static bool deleteDir(const QString& dname)
{
    QDir              dir(dname);
    QDir::Filters     filt = QDir::NoDotAndDotDot | QDir::AllEntries | QDir::Hidden | QDir::System;
    QDir::SortFlags   sort = QDir::DirsLast;
    QFileInfoList     files = dir.entryInfoList(filt, sort);

    foreach (QFileInfo finfo, files)
    {
        QString f = finfo.fileName();

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
    //                                    + QFileInfo(dname).fileName()));
    return QDir(QFileInfo(dname).absolutePath()).rmdir(QFileInfo(dname).fileName());
} // dbgScanDir


static QDir         _dir;

////////////////////////////////////////////////////////////////////////
static bool deleteTempFiles(QTextEdit *txt)
{
    const char *files[] = { EPUB::mtype, EPUB::mdir, EPUB::ddir };
    for (unsigned i=0; i<numb_el(files); i++)
    {
        QFileInfo fi(Config::EPUBTmp() + "/" + files[i]);

        if (!fi.exists())
            continue;
        if (fi.isDir())
        {
            if (!deleteDir(fi.absoluteFilePath()))
                ERR(tr("\nCan't remove old content from %1 directory\n")
                    .arg(fi.absoluteFilePath()),
                    tr("<br>Can't remove old content from %1 directory<br>")
                    .arg(fi.absoluteFilePath()));
        }
        else
        {
            if (!_dir.remove(files[i]))
                ERR(tr("\nCan't remove old file: %1\n")
                    .arg(fi.absoluteFilePath()),
                    tr("<br>Can't remove old file: %1<br>")
                    .arg(fi.absoluteFilePath()));
        }
    }
    return true;
} // deleteTempFiles


////////////////////////////////////////////////////////////////////////
bool FB2toEPUB(const QString& fb2Name, const QString& EPUBName,
               QTextEdit *txt)
{
    //txt->insertHtml(QString("<br>&nbsp; +++++&nbsp; FB2/EPUB \"%1\" -> \"%2\"<br>")
    //                .arg(fb2Name).arg(EPUBName));
    QDomDocument _dom;


    /*
     * FB2 read and parse
     * ------------------
     */
    MSG(tr("\n    Parse FB2"),
        tr("<br>&nbsp;&nbsp;&nbsp;&nbsp;<b><font color=#0000ff>Parse FB2</font></b>"));
    bool         rc;
    int          errLine, errCol;
    QString      errMsg;

    // Input file
    QFile i_file(fb2Name);
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

    if (head == Info::ZIP_MAGIC)
    {
        UnZip            uz;
        QBuffer          *buff = new QBuffer(&data);
        UnZip::ErrorCode ec = uz.openArchive(buff);

        if (ec != UnZip::Ok)
            return false;
        QStringList      flist = uz.fileList();

        if (flist.size() < 1)
            return false;
        ec = uz.extractFile(flist[0], &outbuff, UnZip::SkipPaths);
        if (ec != UnZip::Ok)
            return false;

        rc = _dom.setContent(outbuff.buffer(), &errMsg, &errLine, &errCol);
    }
    else
        rc = _dom.setContent(data, &errMsg, &errLine, &errCol);

    if (!rc)
        ERR(tr("\nParse error -- Line: %1, Column: %2 -- %3\n")
            .arg(errLine).arg(errCol).arg(errMsg),
            tr("<br><b><font color=#ff0000>Parse error</font></b> -- "
               "Line: %1, Column: %2 -- %3<br>")
            .arg(errLine).arg(errCol).arg(errMsg));


    /*
     * Create EPUB files
     * -----------------
     */
    MSG(tr("    Create EPUB"),
        tr("&nbsp;&nbsp;&nbsp;&nbsp;<b><font color=#0000ff>Create EPUB</font></b>"));

    // cd to temp
    if (!_dir.cd(Config::EPUBTmp()))
        ERR(tr("\nTemporary directory \"%1\" doesn't exist.\n")
            .arg(Config::EPUBTmp()),
            tr("<br><b><font color=#ff0000>Temporary directory \"%1\" "
               "doesn't exist</font></b><br>")
            .arg(Config::EPUBTmp()));

    // Open destination archive
    QString fileName(Config::EPUBTmp() + "/" + QFileInfo(EPUBName).fileName());
    Zip zip;
    Zip::ErrorCode ec = zip.createArchive(fileName);
    if (ec != Zip::Ok)
        ERR(tr("\nCan't open %1: %2\n")
            .arg(fileName).arg(zip.formatError(ec)),
            tr("<br>Can't open <b><font color=#0000ff>%1"
               "</font><b>: <b><font color=#ff0000>%2</font><b><br>")
            .arg(fileName).arg(zip.formatError(ec)));

    // Clean all from previous conversion
    if (!deleteTempFiles(txt))
        return false;

    // Create mimetype
    {
        QString outf(Config::EPUBTmp() + "/" + EPUB::mtype);
        QFile outfile(outf);
        if (!outfile.open(QFile::WriteOnly | QFile::Truncate))
            ERR(tr("\nCan't open file %1: %2\n")
                .arg(outf).arg(outfile.errorString()),
                tr("<br>Can't open file %1: <b><font color=#ff0000>%2</font><b><br>")
                .arg(outf).arg(outfile.errorString()));
        QTextStream out(&outfile);
        out << "application/epub+zip";
    }

    // Create blank dirs
    const char *dirs[] = { EPUB::mdir, EPUB::ddir };
    for (unsigned i=0; i<numb_el(dirs); i++)
        if (!_dir.mkdir(dirs[i]))
            ERR(tr("\nCan't create directory %1\n")
                .arg(Config::EPUBTmp() + "/" + dirs[i]),
                tr("\nCan't create directory %1\n")
                .arg(Config::EPUBTmp() + "/" + dirs[i]));


    /*
     * EPUB datastruct creation
     * ------------------------
     */
    EPUB epub(txt);
    if (!epub.fb2parse(_dom))
        return false;
    if (!epub.create(Config::EPUBTmp()))
        return false;

    /*
     * Create zip archive
     * ------------------
     */
    ec = zip.addFile(Config::EPUBTmp() + "/" + EPUB::mtype, "", Zip::Store);
    if (ec != Zip::Ok)
        ERR(tr("\nCan't add %1: %2\n")
            .arg(Config::EPUBTmp() + "/" + EPUB::mtype).arg(zip.formatError(ec)),
            tr("<br>Can't add %1: <b><font color=#ff0000>%2</font><b><br>\n")
            .arg(Config::EPUBTmp() + "/" + EPUB::mtype).arg(zip.formatError(ec)));

    for (unsigned i=0; i<numb_el(dirs); i++)
    {
        ec = zip.addDirectory(Config::EPUBTmp() + "/" + dirs[i],
                              Zip::RelativePaths, Zip::Deflate9);
        if (ec != Zip::Ok)
            ERR(tr("\nCan't add %1: %2\n")
                .arg(Config::EPUBTmp() + "/" + dirs[i]).arg(zip.formatError(ec)),
                tr("<br>Can't add %1: <b><font color=#ff0000>%2</font><b><br>\n")
                .arg(Config::EPUBTmp() + "/" + dirs[i]).arg(zip.formatError(ec)));
    }

    ec = zip.closeArchive();
    if (ec != Zip::Ok)
        ERR(tr("\nCan't close archive %1: %2\n")
            .arg(fileName).arg(zip.formatError(ec)),
            tr("<br>Can't close archive %1: <b><font color=#ff0000>%2</font><b><br>\n")
            .arg(fileName).arg(zip.formatError(ec)));

    /*
     * Copy to destination
     * -------------------
     */
    MSG(tr("    Copy to dest."),
        tr("&nbsp;&nbsp;&nbsp;&nbsp;<b><font color=#0000ff>Copy to dest.</font></b>"));

    QFile     oldf(fileName);
    if (oldf.copy(EPUBName))
        MSG(tr("    OK\n"),
            tr("&nbsp;&nbsp;&nbsp;&nbsp;<b><font color=#00ff00>OK</font></b><br>"));
    else
        ERR(tr("\nCan't copy: %1\n").arg(oldf.errorString()),
            tr("<br>Can't copy: <b><font color=#ff0000>%1</font></b><br>")
            .arg(oldf.errorString()));

    if (!oldf.remove())
        ERR(tr("\nCan't remove %1: %2\n")
            .arg(fileName).arg(oldf.errorString()),
            tr("<br>Can't remove %1: <b><font color=#ff0000>%2</font></b><br>")
            .arg(fileName).arg(oldf.errorString()));


    // Clean all temporary files
    if (!deleteTempFiles(txt))
        return false;

    return true;
} // FB2toEPUB
