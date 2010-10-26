/*
 * $Id$
 *
 * LogWidget definition
 */

#ifndef LOGWIDGET_H
#define LOGWIDGET_H

#include "ui_LogWidget.h"

#define LOG_CANCEL(l) (l->_ui.bbox->button(QDialogButtonBox::Cancel))
#define LOG_OK(l)     (l->_ui.bbox->button(QDialogButtonBox::Ok))

class LogWidget: public QMainWindow {
    Q_OBJECT

public:
    LogWidget(QWidget *par=0);
    ~LogWidget();

    Ui::LogWindow   _ui;

    bool         canceled() { return _canceled; }
    virtual void show();

private slots:
    void cancelReq() { _canceled = true; }

private:
    bool  _canceled;
};

#endif
