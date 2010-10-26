/*
 * $Id$
 *
 * Name edit panel implementation
 */

#include <QAction>
#include <QKeyEvent>

#include "NameEdit.h"

////////////////////////////////////////////////////////////////////////
NameEdit::NameEdit(QWidget *parent) : QLineEdit(parent)
{
} // NameEdit::NameEdit


////////////////////////////////////////////////////////////////////////
void NameEdit::keyPressEvent(QKeyEvent * ev)
{
    if (ev->key() == Qt::Key_S  &&  (ev->modifiers() & Qt::ControlModifier))
        emit saveName();
    else
        QLineEdit::keyPressEvent(ev);
} // NameEdit::keyPressEvent
