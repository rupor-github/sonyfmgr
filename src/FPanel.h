/*
 * $Id$
 *
 * FPanel definitions
 */

#ifndef FPANEL_H
#define FPANEL_H

#include <QColor>
#include <QList>
#include <QListWidget>
#include <QMap>
#include <QRegExp>
#include <QSet>
#include <QString>
#include <QStringList>

#include "BookData.h"

class QAction;
class QComboBox;
class QDir;
class QFocusEvent;
class QKeyEvent;
class QLabel;
class QLineEdit;
class QListWidgetItem;
class QMouseEvent;
class QTextEdit;
class QPushButton;
class LogWidget;
class Media;
class FPanelItem;
class NameEdit;

class FPanel: public QListWidget {
    Q_OBJECT

public:
    FPanel(QWidget *parent=0);
    ~FPanel();

    bool    cd(const char *dir = ".", bool force=false)
                                      { return cd(QString(dir), force); }
    bool    infocus()                 { return _infocus;                }
    QString pwd()                     { return _dirname;                }
    QString root()                    { return _root;                   }
    QString mediafname()              { return _mediafname;             }
    bool    order_changed()           { return _order_changed;          }
    void    setOrder_changed(bool c)  { _order_changed = c;             }
    QString getPath(const QString& fpath);
    void    addPossibleMounts();
    bool    setFreeSpace();
    void    setCWidgets(FPanel *other, QComboBox *fsel, QLabel *l,
                        QLabel *f, QAction *findDevice,
                        QPushButton *b_root, QPushButton *b_up,
                        QPushButton *b_revert,
                        QLabel *cname_l, NameEdit *cname,
                        QAction *umount, QAction *scan,
                        QList<QPushButton*> *fl,
                        int toOther, int fromOther);
    void    setSelection();
    void    clearSelection();
    void    recolor(const bool infocus);
    void    recolor(const int  row);
    void    rereadDir();
    void    notFound(const QString& newRoot);
    const char *getMode() {
        switch (_mode) {
        case PRS505:
            return "Internal memory";
        case SD:
            return "SD";
        case FileSystem:
            return "Filesystem";
        }
        return "???";
    }
    void F2();
    void F3();
    void F4();
    void F5();
    void F6();
    void F7();
    void F8();
    void F9();
    void F10();

    QMap<QString, BookData> books_data;
    QList<BookData>         books_order;

    static const char   *order_fname;
    static const char   *cname_fname;
    static const bool   mouse_selects;
    static const bool   insert_selects;

public slots:
    bool findDevice(bool *ok = 0);
    void saveOrder();
    void enumerate();
    void unEnumerate();
    void moveUp();
    void moveDown();
    void sort();
    void asort();
    void bsort();
    bool updateColl();
    bool cd(const QString& dir, bool force=false);
    void cdRoot()        { cd(_mode == FileSystem ? "/" :_root ); }
    void cdUp();
    void cname_changed() { orderSet(true);                        }

signals:
    void infoReq(FPanel *panel, const QString& dirname, const QString& fname);

protected:
    virtual void mousePressEvent(QMouseEvent *ev);
    virtual void keyPressEvent(QKeyEvent *ev);
    virtual void leaveEvent(QEvent *event);
    virtual void focusInEvent(QFocusEvent *ev);
    virtual void focusOutEvent(QFocusEvent *ev);
    virtual void closeEditor(QWidget *editor, QAbstractItemDelegate::EndEditHint hint);

private slots:
    void collRevert();
    void action(QListWidgetItem *item);
    void newCurent(QListWidgetItem *curr, QListWidgetItem *prev);
    void entered(const QModelIndex& ind);

private:
    enum ConfStatus { Yes_to_all, No_to_all, Init_stat  };
    enum Mode       { PRS505,     SD,        FileSystem };

    class Device {
    public:
        Device(const QString& n, const QString& f, const QString& m, const Mode& mod)
            : name(n), fname(f), mname(m), mode(mod) { }
        bool operator==(const Device& d)
            { return d.name == name && d.fname == fname && d.mname == mname && d.mode == mode; }
        QString name;
        QString fname;
        QString mname;
        Mode    mode;
    };

    static int         filesAmount(const QString& fname);
    static QStringList possibleMounts();

    void printBackTrace();
    void recolorItem(QListWidgetItem *item);
    void toggleMark(QListWidgetItem *item);
    void setMark(QListWidgetItem *item);
    void clearMark(QListWidgetItem *item);
    void changeMark(const QString& s, bool sel);
    void copyMarked(bool move = false, bool fb2epub = false);
    void copyMarked(const QList<FPanelItem*>& clist, bool move = false, bool fb2epub = false);
    bool copyFile(const QString& fname, const QString& dst,  LogWidget *log, int maxlen, bool move, bool fb2epub);
    bool copyDir(const QString& fname, const QString& dst, LogWidget *log, bool move, bool fb2epub);
    bool deleteFile(const QString& fname, LogWidget *log, int maxlen);
    bool deleteDir(const QString& fname, LogWidget *log, bool first_level = false);
    void deleteMarked();
    void deleteMarked(const QList<FPanelItem*>& clist);
    void moveUp(int row, FPanelItem *c);
    void moveDown(int row, FPanelItem *c);
    void makedir();
    void viewFile();
    void orderSet(const bool changed);
    void setMode(const Mode m);
    void sortDir(const QString& d);
    void reset_confirmations();
    bool ask_delete_dir(const QString& fname);
    bool ask_overwrite_dirf(const QString& fname, LogWidget *log);
    bool ask_overwrite_file(const QString& fname);

    QAction                *_umountAct;
    QAction                *_scanAct;
    QAction                *_findDevice;
    Media                  *_media;
    FPanel                 *_otherFPanel;
    bool                   _infocus;
    QDir                   *_dir;
    QString                _root;
    QString                _mediafname;
    QString                _dirname;
    QString                _prevSelection;
    QSet<QString>          _marked;
    QColor const           *_rfg;
    QColor const           *_rbg;
    QColor const           *_sfg;
    QColor const           *_sbg;
    QLabel                 *_lab;
    QLabel                 *_freel;
    QComboBox              *_fsel;
    QPushButton            *_b_root;
    QPushButton            *_b_up;
    QPushButton            *_b_revert;
    QLabel                 *_cname_l;
    NameEdit               *_cname;
    bool                   _order_changed;
    FPanelItem             *_renItem;
    QString                _oldName;
    static const QString   _pref;
    static const QString   _dpref;
    static const QString   _suff;
    ConfStatus             _file;
    ConfStatus             _dirf;
    ConfStatus             _ddir;
    Mode                   _mode;
    LogWidget              *_sortLog;
    QList<QPushButton*>    *_Flist;
    int                    _toOther;
    int                    _fromOther;
};

#endif
