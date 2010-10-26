/*
 * $Id$
 *
 * Config implementation
 */
#include <QCloseEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QLabel>
#include <QMessageBox>
#include <QSettings>

#include "Config.h"
#include "EPUBed.h"

// Defaults
#define BACKUP_EXT ".bak"

bool    Config::_init = false;
const QColor Config::invalid(255, 128, 128);
const QColor Config::valid(128, 255, 128);

bool Config::_backup_crt = true;
QString Config::_backup_ext = BACKUP_EXT;

////////////////////////////////////////////////////////////////////////
Config::Config(QWidget *par) : QDialog(par)
{
    _ui.setupUi(this);

    // Init
    if (!_init)
        init();

    connect(_ui.ok_b,      SIGNAL(pressed()),     this, SLOT(accept()));
    connect(_ui.cancel_b,  SIGNAL(pressed()),     this, SLOT(reject()));

    connect(_ui.backup_c, SIGNAL(clicked(bool)), this, SLOT(backup_c(bool)));
    _ui.backup_c->setChecked(_backup_crt);
    connect(_ui.backup_e, SIGNAL(textChanged(const QString&)), this, SLOT(backup_e(const QString&)));
    _ui.backup_e->setText(_backup_ext);

    backup_c(_backup_crt);

    // Restore configuration
    QSettings    st(EPUBed::_company, EPUBed::_appName);
    st.beginGroup("Config");
    _ui.tabs->setCurrentIndex(st.value("current_index", 0).toInt());
    st.endGroup();

} // Config::Config


////////////////////////////////////////////////////////////////////////
void Config::revert()
{
    _backup_crt = true;
    _backup_ext = BACKUP_EXT;
} // Config::revert


////////////////////////////////////////////////////////////////////////
void Config::reread()
{
    QSettings    st(EPUBed::_company, EPUBed::_appName);
    st.beginGroup("Config");
    _backup_crt = st.value("create_backups", true).toBool();
    _backup_ext = st.value("backup_extension", BACKUP_EXT).toString();
    st.endGroup();
} // Config::reread


////////////////////////////////////////////////////////////////////////
Config::~Config()
{
    QSettings    st(EPUBed::_company, EPUBed::_appName);
    st.beginGroup("Config");
    st.setValue("current_index", _ui.tabs->currentIndex());
    st.endGroup();
} // Config::~Config


////////////////////////////////////////////////////////////////////////
void Config::init()
{
    QSettings    st(EPUBed::_company, EPUBed::_appName);
    st.beginGroup("Config");
    _backup_crt = st.value("create_backups", true).toBool();
    _backup_ext = st.value("backup_extension", BACKUP_EXT).toString();
    st.endGroup();
    _init = true;
} // Config::init


////////////////////////////////////////////////////////////////////////
bool Config::parseReplacement(const QString& r, QString& from, QString& to)
{
    if (r.length() < 5)
        return false;
    if (r[0] != QChar('s'))
        return false;

    QChar del = r[1];
    if (!r.endsWith(del))
        return false;
    int dp = r.indexOf(del, 2);
    if (dp == -1)
        return false;
    if (dp == r.length() - 1)
        return false;
    from = r.mid(2, dp-2);
    to   = r.mid(dp+1);
    to   = to.left(to.length()-1);
    if (to.indexOf(del) != -1)
        return false;

    return true;
} //  Config::parseReplacement


////////////////////////////////////////////////////////////////////////
bool Config::repl_rules_general(const QString& r, QRegExp& from_re, QString& to,
                                QString& rules)
{
    QString from;
    _ui.errstr->setText("");

    if (r.isEmpty())
    {
        from_re.setPattern("");
        rules = r;
        return repl_valid(true);
    }
    if (!parseReplacement(r, from, to))
        return repl_valid(false);

    from_re.setPattern(from);
    if (!from_re.isValid()) {
        _ui.errstr->setText(QString("<b><font color=#ff0000>%1</font></b>")
                            .arg(from_re.errorString()));
        return repl_valid(false);
    }

    rules = r;
    return repl_valid(true);
} // Config::repl_rules


////////////////////////////////////////////////////////////////////////
bool Config::repl_valid(bool v)
{
    QBrush   br(v ? valid : invalid);
    QPalette pal = _ui.ok_b->palette();
    pal.setBrush(QPalette::Inactive, QPalette::Button, br);
    pal.setBrush(QPalette::Active,   QPalette::Button, br);
    pal.setBrush(QPalette::Disabled, QPalette::Button, br);
    _ui.ok_b->setPalette(pal);

    _ui.ok_b->setText(v ? "OK" : "Invalid");
    _ui.ok_b->setEnabled(v);
    return v;
} // Config::repl_valid


////////////////////////////////////////////////////////////////////////
void Config::save()
{
    QSettings    st(EPUBed::_company, EPUBed::_appName);
    st.beginGroup("Config");
    st.setValue("create_backups", _backup_crt);
    st.setValue("backup_extension", _backup_ext);
    st.endGroup();
} // Config::save


////////////////////////////////////////////////////////////////////////
void  Config::setFileExists(const QString& fname, QString& v, QLabel *l)
{
    QFileInfo fi(fname);
    bool      ok = fi.isDir() ? false : fi.isExecutable();

    QBrush    br(ok ? valid : invalid);
    QPalette  pal = l->palette();
    br.setStyle(Qt::SolidPattern);
    pal.setBrush(QPalette::Inactive, QPalette::Window, br);
    pal.setBrush(QPalette::Active, QPalette::Window, br);
    l->setPalette(pal);

    l->setText(ok ? "OK" : "NA");
    if (ok)
        v = fname;
    else
        v.clear();
} // Config::setFileExists


////////////////////////////////////////////////////////////////////////
void  Config::setDirExists(const QString& fname, QString& v, QLabel *l)
{
    QFileInfo fi(fname);
    bool      ok = fi.isDir();

    QBrush    br(ok ? valid : invalid);
    QPalette  pal = l->palette();
    br.setStyle(Qt::SolidPattern);
    pal.setBrush(QPalette::Inactive, QPalette::Window, br);
    pal.setBrush(QPalette::Active, QPalette::Window, br);
    l->setPalette(pal);

    l->setText(ok ? "OK" : "NA");
    if (ok)
        v = fname;
    else
        v.clear();
} // Config::setFileExists


////////////////////////////////////////////////////////////////////////
void Config::setFname(QLineEdit *l, QString& v, const QString& t)
{
    QFileDialog fd(this, tr("Select %1").arg(t));

    fd.setFileMode(QFileDialog::ExistingFile);
    if (fd.exec() == QDialog::Accepted)
    {
        QStringList fns = fd.selectedFiles();
        if (fns.size() >= 1)
        {
            l->setText(fns[0]);
            v = fns[0];
        }
    }
} // Config::setFname


////////////////////////////////////////////////////////////////////////
void Config::setDname(QLineEdit *l, QString& v, const QString& t)
{
    QFileDialog fd(this, tr("Select %1").arg(t));

    fd.setFileMode(QFileDialog::DirectoryOnly);
    if (fd.exec() == QDialog::Accepted)
    {
        QStringList fns = fd.selectedFiles();
        if (fns.size() >= 1)
        {
            l->setText(fns[0]);
            v = fns[0];
        }
    }
} // Config::setFname


////////////////////////////////////////////////////////////////////////
void Config::backup_c(bool checked)
{
    _backup_crt = checked;
    _ui.backup_e->setEnabled(checked);
    _ui.backup_l->setEnabled(checked);
} // Config::backup_c
