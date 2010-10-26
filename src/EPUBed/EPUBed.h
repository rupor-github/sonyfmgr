/*
 * $Id$
 *
 * EPUBed main window
 */

#ifndef EPUBED_H
#define EPUBED_H

#include <QByteArray>
#include <QColor>
#include <QList>
#include <QMap>
#include <QString>
#include <QStringList>

#include "ui_EPUBed.h"

class QAction;
class QDirModel;
class QCloseEvent;
class QItemDelegate;
class QMenu;
class QPushButton;
class EPUBed: public QMainWindow {
    Q_OBJECT

public:
    EPUBed();
    ~EPUBed();

    Ui::MainWindow    _ui;

    virtual void show();

    static const char *_version;
    static const char *_company;
    static const char *_appName;
    static const char *mtype;
    static const QColor bg_color;
    static const QColor changed_color;
    static const QColor orig_color;

public slots:
    void         config();
    void         config_revert();
    void         config_import();
    void         config_export();
    void         about();
    void         open();
    void         save();
    void         sort();
    void         quit();
    void         restore();

protected:
    virtual void closeEvent(QCloseEvent *event);

private slots:
    void newItemSelected(QListWidgetItem *newItem, QListWidgetItem *oldItem);
    void editorChanged();
    void revert();

private:
    class Data
    {
    public:
        Data(const QByteArray& o, const QByteArray& n = QByteArray()) :
            modified(false), new_txt(n), org_txt(o) {}
        Data() : modified(false) {}
        ~Data() {}

        bool       modified;
        QByteArray new_txt;
        QByteArray org_txt;
    };

    void                         checkSaveAct();

    QAction                      *_saveAct;
    QString                      _dir;
    QString                      _fname;
    QStringList                  _orig_order;
    QMap<QString,Data>           _comps;
    QMap<QString,Data>::iterator _currData;
};

#endif
