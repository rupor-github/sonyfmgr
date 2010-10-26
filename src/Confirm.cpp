/*
 * $Id$
 *
 * Confirm implementation
 */
#include <QSettings>

#include "Confirm.h"
#include "mngr505.h"

////////////////////////////////////////////////////////////////////////
Confirm::Confirm(QWidget *par) : QDialog(par)
{
    _ui.setupUi(this);
} // Confirm::Confirm


////////////////////////////////////////////////////////////////////////
void Confirm::saveGeom()
{
    // Save geomtery
    QSettings    st(mngr505::_company, mngr505::_appName);
    st.beginGroup("Confirm");
    st.setValue("size", size());
    st.setValue("pos", pos());
    st.endGroup();
    //qDebug("Save geometry:    %dx%d+%d+%d", size().width(), size().height(), pos().x(), pos().y());
} // Confirm::saveGeom


////////////////////////////////////////////////////////////////////////
void Confirm::restoreGeom()
{
    QSettings    st(mngr505::_company, mngr505::_appName);
    QVariant     v;
    QString      geom;

    // Restore geometry
    st.beginGroup("Confirm");
    v = st.value("size");
    if (v.isValid())
    {
        resize(v.toSize());
        geom.sprintf("%dx%d", v.toSize().width(), v.toSize().height());
    }
    v = st.value("pos");
    if (v.isValid())
    {
        move(v.toPoint());
        geom += QString("+%1+%2").arg(v.toPoint().x()).arg(v.toPoint().y());
    }
    st.endGroup();
    //qDebug("Restore geometry: %s", qPrintable(geom));
} // Confirm::restoreGeom


////////////////////////////////////////////////////////////////////////
int Confirm::exec1()
{
    restoreGeom();
    return exec();
} //  Confirm::exec1


////////////////////////////////////////////////////////////////////////
void Confirm::reject()
{
    QDialog::reject();
    saveGeom();
} // Confirm::reject


////////////////////////////////////////////////////////////////////////
void Confirm::accept()
{
    QDialog::accept();
    saveGeom();
} // Confirm::accept
