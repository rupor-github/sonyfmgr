/*
 * $Id$
 *
 * Info panel definitions
 */

#ifndef INFO_H
#define INFO_H

#include <QFrame>
#include <QLabel>
#include <QList>
#include <QMap>
#include <QString>
#include <QLocale>
#include <stdint.h>

#include "BookData.h"

class FPanel;
class QLabel;
class QLocale;
class Info: public QFrame {
    Q_OBJECT

public:
    Info(QWidget *parent=0);
    ~Info();

    void setLabels(QMap<QString, QList<QLabel*> > labels) { _labels = labels; }
    void disable();
    static bool getBookData(FPanel *p, const QString& fname, BookData *bd)
        { return getBookData(p, BookData::getType(fname), fname, bd); }
    static bool getBookData(FPanel *p, const QString& dname, const QString& fname,
                            BookData *bd)
        { return getBookData(p, BookData::getType(fname), dname + "/" + fname, bd); }

    static const   qint32   ZIP_MAGIC=0x04034b50;

public slots:
    void infoReq(FPanel *panel, const QString& dirname, const QString& fname);

private:
    static QLocale _loc;

    static bool  getBookData(FPanel *panel, const BookData::BType type, const QString& fname,
                             BookData *bd);
    static bool  fillFB2Info (const QString& fullname, BookData *bd);
    static bool  fillLRFInfo (const QString& fullname, BookData *bd);
    static bool  fillEPUBInfo(const QString& fullname, BookData *bd);
    static bool  fillTXTInfo (const QString& fullname, BookData *bd);
    static bool  fillPDFInfo (const QString& fullname, BookData *bd);
    static bool  fillRTFInfo (const QString& fullname, BookData *bd);


    static const   uint32_t MAX_SANE_C_SIZE =  10240;
    static const   uint32_t MAX_SANE_U_SIZE = 102400;

    QLabel *_path_l;
    QLabel *_name_l;
    QLabel *_author_l;
    QLabel *_title_l;
    QLabel *_path;
    QLabel *_name;
    QLabel *_author;
    QLabel *_title;
    QMap<QString, QList<QLabel*> > _labels;

};

#endif
