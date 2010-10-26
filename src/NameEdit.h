/*
 * $Id$
 *
 * Name edit panel definitions
 */

#ifndef NAMEEDIT_H
#define NAMEEDIT_H

#include <QLineEdit>

class QKeyEvent;
class NameEdit: public QLineEdit {
    Q_OBJECT

public:
    NameEdit(QWidget *parent=0);
    ~NameEdit() {}

protected:
    virtual void keyPressEvent(QKeyEvent * ev);

signals:
    void saveName();

};

#endif
