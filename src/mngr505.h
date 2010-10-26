/*
 * $Id$
 *
 * mngr505 main window
 */

#ifndef MNGR505_H
#define MNGR505_H
#include <stdint.h>

#include <QList>

#include "ui_mngr505.h"

class QAction;
class QDirModel;
class QCloseEvent;
class QItemDelegate;
class QMenu;
class QPushButton;
class mngr505: public QMainWindow {
    Q_OBJECT

public:
    mngr505();
    ~mngr505();

    static Ui::MainWindow    _ui;

    virtual void show();

    static const char *_version;
    static const char *_company;
    static const char *_appName;

public slots:
    static  void confExit();

    void         setSelection();
    void         clearSelection();
    void         config();
    void         config_revert();
    void         config_import();
    void         config_export();
    void         about();
    void         umount();

protected:
    virtual void closeEvent(QCloseEvent *event);

private slots:
    void findDevice();
    void moveUp();
    void moveDown();
    void saveOrder();
    void enumerate();
    void unEnumerate();
    void sort();
    void bsort();
    void asort();
    void F2();
    void F3();
    void F4();
    void F5();
    void F6();
    void F7();
    void F8();
    void F9();
    void F10();

private:
    void reread_media();

    QList<QPushButton*> *_fl;
};

#endif
