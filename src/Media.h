/*
 * $Id$
 *
 * Media definitions
 */

#ifndef MEDIA_H
#define MEDIA_H

#include <QObject>
#include <QDomDocument>
#include <QMainWindow>
#include <QMap>
#include <QList>
#include <QString>
#include <QStringList>

////////////////////////////////////////////////////////////////////////
class Collection
{
public:
    Collection()  { }
    ~Collection() { }

    QList<int> ids;
};

////////////////////////////////////////////////////////////////////////
class QTextEdit;
class QPushButton;
class PreviewXml : public QMainWindow
{
    Q_OBJECT

public:
    PreviewXml(QWidget *par, QString *prev_data, bool *wasEdit);
    ~PreviewXml();

    virtual void show();

    QTextEdit   *te;
    QPushButton *ok;
    QPushButton *edit;
    QPushButton *cancel;
    bool        stopped;

private slots:
    void stopReq()     { stopped = true;  }
    void textChanged() { *_wasEdit = true; }
    void saveReq();
    void editReq();
    void revertReq();

private:
    QString   *_prev_data;
    bool      *_wasEdit;
};


////////////////////////////////////////////////////////////////////////
class QString;
class FPanel;
class LogWidget;
class Media : public QObject
{
    Q_OBJECT

public:
    Media(FPanel *parent);
    ~Media() {}

    bool readPRS(const QString& root, const QString& bookroot,
                 const QString& fname, QString& errText);
    bool readSD(const QString& root, const QString& bookroot,
                const QString& fname, QString& errText);
    bool updateColl();
    void syncThumbs( const QString& path );
    bool ok() { return _readok; }

private slots:
    void previewXml();
    void collectDebug();
    bool cancelXml();
    bool saveXml();

private:
    void     scanDir(const QString& nprefix, const QString& cprefix,
                     int dcnt, const char *dformat, const QString& dname);
    int      getID();
    int      getID(const QString& path, bool *ok);
    QString  getPath(const QString& fpath);
    void     collectIDs(QDomNode n);
    void     dbgScanDir(const QString& offs, const QString& dname);
    bool     deleteDir(const QString& dname);

    QDomDocument _dom;
    QDomNode     _books_parent;
    QDomNode     _first_text;
    FPanel       *_par;
    QString      _mediafname;
    QString      _root;
    QString      _bookroot;
    QString      _thumbroot;
    bool         _readok;
    LogWidget    *_log;
    int          _nextID;
    QList<int>   _IDs;
    QList<int>   _usedIDs;
    bool         _isSD;
    QString      _prev_data;
    bool         _wasEdit;
    QStringList  _dbgList;
    QMap<QString, Collection> _colls;
    QMap<int, QString>        _all_books;

    QString      _dom_1;
    QString      _dom_2;
    QString      _dom_3;
    QString      _dom_4;
    QString      _dom_5;
    QString      _dom_6;
    QString      _dom_7;
};

#endif
