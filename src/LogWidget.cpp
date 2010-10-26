/*
 * $Id$
 *
 * LogWidget implementation
 */
#include <QCloseEvent>
#include <QSettings>

#include "LogWidget.h"
#include "mngr505.h"

////////////////////////////////////////////////////////////////////////
LogWidget::LogWidget(QWidget *par) : QMainWindow(par), _canceled(false)
{
    _ui.setupUi(this);

    setWindowModality(Qt::ApplicationModal);

    _ui.txt->setReadOnly(true);
    _ui.txt->setUndoRedoEnabled(false);
    _ui.txt->setLineWrapMode(QTextEdit::NoWrap);
    QFont f = _ui.txt->font();
    f.setFixedPitch(true);
    _ui.txt->setFont(f);
    LOG_OK(this)->setEnabled(false);
    LOG_OK(this)->setDefault(true);

    connect(LOG_CANCEL(this), SIGNAL(pressed()), this, SLOT(cancelReq()));
    connect(LOG_OK(this), SIGNAL(pressed()), this, SLOT(deleteLater()));
} // LogWidget::LogWidget


////////////////////////////////////////////////////////////////////////
LogWidget::~LogWidget()
{
    QSettings st(mngr505::_company, mngr505::_appName);

    // Save geomtery
    st.beginGroup("LogWidget");
    st.setValue("size", size());
    st.setValue("pos", pos());
    st.endGroup();

} // LogWidget::~LogWidget


////////////////////////////////////////////////////////////////////////
void LogWidget::show()
{
    QSettings    st(mngr505::_company, mngr505::_appName);
    QVariant     v;

    // Restore geometry
    st.beginGroup("LogWidget");
    v = st.value("size");
    if (v.isValid())
        resize(v.toSize());
    v = st.value("pos");
    if (v.isValid())
        move(v.toPoint());

    st.endGroup();
    QMainWindow::show();
} //  LogWidget::show
