/*
 * $Id$
 *
 * Confirm definition
 */

#ifndef CONFIRM_H
#define CONFIRM_H

#include "ui_Confirm.h"

class Confirm: public QDialog {
    Q_OBJECT

public:
    Confirm(QWidget *par=0);
    ~Confirm() {}

    Ui_confirmDialog   _ui;

    virtual void reject();
    virtual void accept();

    int exec1();

private:
    void saveGeom();
    void restoreGeom();
};

#endif
