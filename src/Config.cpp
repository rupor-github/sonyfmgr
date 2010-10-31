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
#include <QTextEdit>
#include <QPushButton>

#include "Config.h"
#include "mngr505.h"
#include "utils.h"

// Defaults
// #define REPL_RULES  "s/_/ /"

#define SEPARATOR   "/"
#define REPL_RULES  "s%(\\w+)(?:\\W+\\w*)*(/(\\w+\\W*)+)*\\W*%\\1\\2%"
#define S1_RULES    ""
#define S2_RULES    ""
#define S3_RULES    ""
#define FB2_PATTERN "*.fb2,*.fb2.zip"
#define ROOT_PRS    "database/media/books"
#define ROOT_SD     "Sony Reader/database/media/books"
#define OTHERS_SD   "_Others_SD"
#define OTHERS_PRS  "_Others_PRS"
#define HIDE_FILES  ".*,*.tmp"
#define FB2LRF      ""
#define FB2STYLES   ""
#define FB2LRF_TMP  "C:\\Temp"
#define FB2LRF_ENV  "LANG=ru_RU.UTF-8\nLC_ALL=ru_RU.UTF-8"
#define REG_FG        QColor(  0,   0,   0)
#define REG_BG        QColor(255, 255, 255)
#define SEL_FG        QColor(255,   0,   0)
#define SEL_BG        QColor(255, 255, 255)
#define NF_REG_FG     QColor(128, 128, 128)
#define NF_REG_BG     QColor(196, 196, 196)
#define NF_SEL_FG     QColor(128,   0,   0)
#define NF_SEL_BG     QColor(196, 196, 196)
#define ORD_CHANGED   QColor(255, 128, 128)
#define ORD_UNCHANGED QColor(128, 255, 128)

#if defined(WINDOWS)
#define FB2_VIEWER  "C:\\Program Files\\FBReader\\FBReader.exe"
#define LRF_VIEWER  "C:\\Program Files\\calibre\\lrfviewer.exe"
#define EPUB_TMP    "C:\\Temp"
#define FB2LRF_CMD  "%FB2LRF -p -x %INP %OUT"
#define FB2LRF_DRV  ""
#else
#define FB2_VIEWER  ""
#define LRF_VIEWER  ""
#define EPUB_TMP    "/tmp"
#define FB2LRF_CMD  "wine %FB2LRF -s %STYLES -t %TEMP -i %INP -o %OUT"
#define FB2LRF_DRV  "Z:"
#endif

const char *Config::_default_css =
"body {\n"
"    margin-right: 0px;\n"
"    text-align:   justify;\n"
"    font-family:  'Swis721 BT';\n"
"}\n"
"p.title-p {\n"
"    margin:       0px 0px 0.5em 0px;\n"
"    font-family:  'Swis721 BT';\n"
"    font-size:    12pt;\n"
"}\n"
"p.p {\n"
"    margin:       0px;\n"
"    text-align:   justify;\n"
"    font-family:  'Swis721 BT';\n"
"    font-size:    10pt;\n"
"}\n"
"p.empty-line {\n"
"    height:       1em;\n"
"    margin:       0px;\n"
"}\n"
"@font-face {\n"
"    font-family: 'Swis721 BT';\n"
"    src: url(res:///opt/sony/ebook/FONT/tt0003m_.ttf);\n"
"}\n"
"@font-face {\n"
"    font-family: 'Dutch801 Rm BT';\n"
"    src: url(res:///opt/sony/ebook/FONT/tt0011m_.ttf);\n"
"}\n"
"@font-face {\n"
"    font-family: 'Courier10 Win95BT';\n"
"    src: url(res:///opt/sony/ebook/FONT/tt0419m_.ttf);\n"
"}\n"
;

Ui_Dialog    Config::_ui;
bool         Config::_init = false;
const QColor Config::invalid(255, 128, 128);
const QColor Config::valid(128, 255, 128);
QString      Config::_EPUBStyles;

bool Config::_cmouseSel = true;
bool Config::_coll_dummy = false;
bool Config::_coll_empty = false;
bool Config::_coll_enum = false;
bool Config::_collectdbg = false;
bool Config::_concat = false;
QString Config::_concatSep = SEPARATOR;
bool Config::_confDelDir = true;
bool Config::_confExit = true;
int Config::_confMax = 100;
bool Config::_confOverWr = true;
QString Config::_epub_tmp = EPUB_TMP;
bool Config::_f9_lrf = false;
QString Config::_fb2Pattern = FB2_PATTERN;
QString Config::_fb2lrf = FB2LRF;
QString Config::_fb2lrf_cmd = FB2LRF_CMD;
QString Config::_fb2lrf_drv = FB2LRF_DRV;
QString Config::_fb2lrf_env = FB2LRF_ENV;
bool Config::_fb2lrf_err = false;
bool Config::_fb2lrf_ovr = true;
bool Config::_fb2lrf_scm = true;
QString Config::_fb2lrf_tmp = FB2LRF_TMP;
bool Config::_fb2lrf_utmp = true;
QString Config::_fb2styles = FB2STYLES;
QString Config::_fb2viewer = FB2_VIEWER;
QString Config::_hide_files = HIDE_FILES;
bool Config::_htmlReport = false;
bool Config::_insertSel = true;
bool Config::_log_disapp = true;
QString Config::_lrfviewer = LRF_VIEWER;
bool Config::_mouseSel = true;
QColor Config::_nf_reg_bg = NF_REG_BG;
QColor Config::_nf_reg_fg = NF_REG_FG;
QColor Config::_nf_sel_bg = NF_SEL_BG;
QColor Config::_nf_sel_fg = NF_SEL_FG;
QColor Config::_ord_changed = ORD_CHANGED;
QColor Config::_ord_unchanged = ORD_UNCHANGED;
bool Config::_others = true;
QString Config::_othersPRS = OTHERS_PRS;
QString Config::_othersSD = OTHERS_PRS;
QColor Config::_reg_bg = REG_BG;
QColor Config::_reg_fg = REG_FG;
bool Config::_replCase = true;
bool Config::_replGreedy = true;
bool Config::_replTruscore = true;
QString Config::_rootPRS = ROOT_PRS;
QString Config::_rootSD = ROOT_SD;
bool Config::_s1Case = true;
bool Config::_s1Greedy = true;
bool Config::_s2Case = true;
bool Config::_s2Greedy = true;
bool Config::_s3Case = true;
bool Config::_s3Greedy = true;
bool Config::_selDirs = true;
QColor Config::_sel_bg = SEL_BG;
QColor Config::_sel_fg = SEL_FG;
bool Config::_smouseSel = true;
bool Config::_sorder_a = true;
bool Config::_sort1_a = true;
bool Config::_sort1_d = false;
bool Config::_sort1_t = false;
bool Config::_sort2_a = false;
bool Config::_sort2_d = false;
bool Config::_sort2_t = true;
bool Config::_sort3_a = false;
bool Config::_sort3_d = true;
bool Config::_sort3_t = false;
bool Config::_translAuth = false;
bool Config::_translClear = true;
bool Config::_translTitl = false;
QString Config::_replRules = REPL_RULES;
QRegExp Config::_from;
QString Config::_to;
QString Config::_s1_rules = S1_RULES;
QRegExp Config::_s1_from;
QString Config::_s1_to;
QString Config::_s2_rules = S2_RULES;
QRegExp Config::_s2_from;
QString Config::_s2_to;
QString Config::_s3_rules = S3_RULES;
QRegExp Config::_s3_from;
QString Config::_s3_to;

////////////////////////////////////////////////////////////////////////
static QString str2ini(const QString &s)
{
    QString    rc;
    QByteArray a = qCompress(s.toUtf8(), 9);
    for (int i=0; i<a.size(); i++)
        rc += QString().sprintf("%02X", a.at(i) & 0xff);
    return rc;
} // str2ini

////////////////////////////////////////////////////////////////////////
static QString ini2str(const QString &s)
{
    QByteArray a;
    a.reserve(s.size());
    for (const char *p = qPrintable(s); *p; p++)
    {
        unsigned i;
        if (*p >= '0' && *p <= '9')
            i = (*p - '0') << 4;
        else if (*p >= 'A' && *p <= 'F')
            i = (*p - 'A' + 10) << 4;
        else if (*p >= 'a' && *p <= 'f')
            i = (*p - 'a' + 10) << 4;
        else
            break;

        if (!*++p)
            break;

        if (*p >= '0' && *p <= '9')
            i |= (*p - '0') & 0x0f;
        else if (*p >= 'A' && *p <= 'F')
            i |= (*p - 'A' + 10) & 0x0f;
        else if (*p >= 'a' && *p <= 'f')
            i |= (*p - 'a' + 10) & 0x0f;
        else
            break;

        a.append((char)(i&0xff));
    }
    QByteArray b = qUncompress(a);
    return QString::fromUtf8(b.data(), b.size());
} // ini2str


////////////////////////////////////////////////////////////////////////
EditCSS::EditCSS(Config *par) : QMainWindow(par), _par(par), _wasEdit(false)
{
    QWidget     *cwidget = new QWidget(this);
    QGridLayout *gl = new QGridLayout(cwidget);

    save = new QPushButton(cwidget);
    save->setText(tr("Save"));
    save->setEnabled(false);

    cancel = new QPushButton(cwidget);
    cancel->setText(tr("Cancel"));

    revertCurrent = new QPushButton(cwidget);
    revertCurrent->setText(tr("Revert current editing"));
    revertCurrent->setEnabled(false);

    revertDefault = new QPushButton(cwidget);
    revertDefault->setText(tr("Revert to default"));

    te = new QTextEdit(cwidget);
    te->setReadOnly(false);
    te->setUndoRedoEnabled(true);
    te->setLineWrapMode(QTextEdit::NoWrap);
    te->setPlainText(par->CSS());
    connect(te, SIGNAL(textChanged()), this, SLOT(textChanged()));

    gl->addWidget(te,            0, 0, 1, 4);
    gl->addWidget(save,          1, 0, 1, 1, Qt::AlignHCenter);
    gl->addWidget(cancel,        1, 1, 1, 1, Qt::AlignHCenter);
    gl->addWidget(revertCurrent, 1, 2, 1, 1, Qt::AlignHCenter);
    gl->addWidget(revertDefault, 1, 3, 1, 1, Qt::AlignHCenter);

    setCentralWidget(cwidget);

    connect(save,          SIGNAL(pressed()), this, SLOT(saveReq()));
    connect(cancel,        SIGNAL(pressed()), this, SLOT(deleteLater()));
    connect(revertCurrent, SIGNAL(pressed()), this, SLOT(revertCReq()));
    connect(revertDefault, SIGNAL(pressed()), this, SLOT(revertDReq()));
    setWindowTitle(tr("media.xml file preview"));
} // EditCSS::EditCSS


////////////////////////////////////////////////////////////////////////
EditCSS::~EditCSS()
{
    QSettings st(mngr505::_company, mngr505::_appName);

    // Save geomtery
    st.beginGroup("EditCSS");
    st.setValue("size", size());
    st.setValue("pos", pos());
    st.endGroup();
} // EditCSS::~EditCSS

////////////////////////////////////////////////////////////////////////
void EditCSS::saveReq()
{
    if (_wasEdit)
        _par->setCSS(te->toPlainText());

    deleteLater();
} // EditCSS::saveReq

////////////////////////////////////////////////////////////////////////
void EditCSS::textChanged()
{
    _wasEdit = true;
    revertCurrent->setEnabled(true);
    save->setEnabled(true);
} // EditCSS::textChanged

////////////////////////////////////////////////////////////////////////
void EditCSS::revertCReq()
{
    _wasEdit = false;
    te->setPlainText(_par->CSS());
    revertCurrent->setEnabled(false);
    save->setEnabled(false);
} // EditCSS::revertCReq


////////////////////////////////////////////////////////////////////////
void EditCSS::revertDReq()
{
    _wasEdit = true;
    te->setPlainText(_par->DefCSS());
    revertCurrent->setEnabled(true);
    save->setEnabled(true);
} // EditCSS::revertCReq


////////////////////////////////////////////////////////////////////////
void EditCSS::show()
{
    QSettings    st(mngr505::_company, mngr505::_appName);
    QVariant     v;

    // Restore geometry
    st.beginGroup("EditCSS");
    v = st.value("size");
    if (v.isValid())
        resize(v.toSize());
    v = st.value("pos");
    if (v.isValid())
        move(v.toPoint());

    st.endGroup();
    QMainWindow::show();
} // EditCSS::show


////////////////////////////////////////////////////////////////////////
Config::Config(QWidget *par) : QDialog(par)
{
    _ui.setupUi(this);

    _ui.ok_b->setAutoFillBackground(true);
    _ui.fb2_ok->setAutoFillBackground(true);
    _ui.lrf_ok->setAutoFillBackground(true);
    _ui.epub_tmp_ok->setAutoFillBackground(true);
    _ui.fb2lrf_tmp_ok->setAutoFillBackground(true);
    _ui.fb2lrf_ok->setAutoFillBackground(true);
    _ui.fb2styles_ok->setAutoFillBackground(true);
    _ui.fb2lrf_cmd_ok->setAutoFillBackground(true);
    _ui.fb2lrf_env_ok->setAutoFillBackground(true);

#if defined(WINDOWS)
    setWidgetRecurseEnable(_ui.Wine_box1, false);
#endif

    // Init
    if (!_init)
        init();

    connect(_ui.ok_b,      SIGNAL(pressed()),     this, SLOT(accept()));
    connect(_ui.cancel_b,  SIGNAL(pressed()),     this, SLOT(reject()));
    connect(_ui.dir_help,  SIGNAL(pressed()),     this, SLOT(dirHelp()));
    connect(_ui.fb2viewer_browse,  SIGNAL(pressed()),
            this,                  SLOT(fb2_browse()));
    connect(_ui.lrfviewer_browse,  SIGNAL(pressed()),
            this,                  SLOT(lrf_browse()));
    connect(_ui.epub_tmp_browse,   SIGNAL(pressed()),
            this,                  SLOT(epub_tmp_br()));
    connect(_ui.fb2lrf_tmp_browse, SIGNAL(pressed()),
            this,                  SLOT(fb2lrf_tmp_br()));
    connect(_ui.fb2lrf_browse,     SIGNAL(pressed()),
            this,                  SLOT(fb2lrf_browse()));
    connect(_ui.fb2styles_browse,  SIGNAL(pressed()),
            this,                  SLOT(fb2styles_browse()));
    connect(_ui.epub_editstyles,   SIGNAL(pressed()),
            this,                  SLOT(epub_editstyles()));


    connect(_ui.cmouse_sel, SIGNAL(clicked(bool)), this, SLOT(cmousesel(bool)));
    _ui.cmouse_sel->setChecked(_cmouseSel);
    connect(_ui.coll_dummy, SIGNAL(clicked(bool)), this, SLOT(coll_dummy(bool)));
    _ui.coll_dummy->setChecked(_coll_dummy);
    connect(_ui.coll_empty, SIGNAL(clicked(bool)), this, SLOT(coll_empty(bool)));
    _ui.coll_empty->setChecked(_coll_empty);
    connect(_ui.coll_enum, SIGNAL(clicked(bool)), this, SLOT(coll_enum(bool)));
    _ui.coll_enum->setChecked(_coll_enum);
    connect(_ui.collect_dbg, SIGNAL(clicked(bool)), this, SLOT(collectdbg(bool)));
    _ui.collect_dbg->setChecked(_collectdbg);
    connect(_ui.concat, SIGNAL(clicked(bool)), this, SLOT(concat_sl(bool)));
    _ui.concat->setChecked(_concat);
    connect(_ui.concat_s, SIGNAL(textChanged(const QString&)), this, SLOT(concat_s(const QString&)));
    _ui.concat_s->setText(_concatSep);
    connect(_ui.conf_dir, SIGNAL(clicked(bool)), this, SLOT(conf_dir(bool)));
    _ui.conf_dir->setChecked(_confDelDir);
    connect(_ui.conf_exit, SIGNAL(clicked(bool)), this, SLOT(conf_exit(bool)));
    _ui.conf_exit->setChecked(_confExit);
    connect(_ui.conf_max, SIGNAL(valueChanged(int)), this, SLOT(conf_max(int)));
    _ui.conf_max->setValue(_confMax);
    connect(_ui.conf_over, SIGNAL(clicked(bool)), this, SLOT(conf_over(bool)));
    _ui.conf_over->setChecked(_confOverWr);
    connect(_ui.epub_tmp_l, SIGNAL(textChanged(const QString&)), this, SLOT(epub_tmp(const QString&)));
    _ui.epub_tmp_l->setText(_epub_tmp);
    connect(_ui.f9_lrf, SIGNAL(toggled(bool)), this, SLOT(f9_lrf(bool)));
    _ui.f9_lrf->setChecked(_f9_lrf);
    connect(_ui.fb2_patt, SIGNAL(textChanged(const QString&)), this, SLOT(fb2_patt(const QString&)));
    _ui.fb2_patt->setText(_fb2Pattern);
    connect(_ui.fb2lrf_l, SIGNAL(textChanged(const QString&)), this, SLOT(fb2lrf_text(const QString&)));
    _ui.fb2lrf_l->setText(_fb2lrf);
    connect(_ui.fb2lrf_cmd, SIGNAL(textChanged(const QString&)), this, SLOT(fb2lrf_cmd(const QString&)));
    _ui.fb2lrf_cmd->setText(_fb2lrf_cmd);
    connect(_ui.fb2lrf_drv, SIGNAL(textChanged(const QString&)), this, SLOT(fb2lrf_drv(const QString&)));
    _ui.fb2lrf_drv->setText(_fb2lrf_drv);
    connect(_ui.fb2lrf_env, SIGNAL(textChanged()), this, SLOT(fb2lrf_env()));
    _ui.fb2lrf_env->setPlainText(_fb2lrf_env);
    connect(_ui.fb2lrf_serr, SIGNAL(clicked(bool)), this, SLOT(fb2lrf_serr(bool)));
    _ui.fb2lrf_serr->setChecked(_fb2lrf_err);
    connect(_ui.fb2lrf_ovr, SIGNAL(clicked(bool)), this, SLOT(fb2lrf_ovr(bool)));
    _ui.fb2lrf_ovr->setChecked(_fb2lrf_ovr);
    connect(_ui.fb2lrf_scmd, SIGNAL(clicked(bool)), this, SLOT(fb2lrf_scmd(bool)));
    _ui.fb2lrf_scmd->setChecked(_fb2lrf_scm);
    connect(_ui.fb2lrf_tmp_l, SIGNAL(textChanged(const QString&)), this, SLOT(fb2lrf_tmp(const QString&)));
    _ui.fb2lrf_tmp_l->setText(_fb2lrf_tmp);
    connect(_ui.fb2lrf_utmp, SIGNAL(clicked(bool)), this, SLOT(fb2lrf_utmp(bool)));
    _ui.fb2lrf_utmp->setChecked(_fb2lrf_utmp);
    connect(_ui.fb2styles_l, SIGNAL(textChanged(const QString&)), this, SLOT(fb2styles_text(const QString&)));
    _ui.fb2styles_l->setText(_fb2styles);
    connect(_ui.fb2viewer_l, SIGNAL(textChanged(const QString&)), this, SLOT(fb2_text(const QString&)));
    _ui.fb2viewer_l->setText(_fb2viewer);
    connect(_ui.hide_files, SIGNAL(textChanged(const QString&)), this, SLOT(hide_files(const QString&)));
    _ui.hide_files->setText(_hide_files);
    connect(_ui.html_rep, SIGNAL(clicked(bool)), this, SLOT(html_rep(bool)));
    _ui.html_rep->setChecked(_htmlReport);
    connect(_ui.insert_sel, SIGNAL(clicked(bool)), this, SLOT(insertsel(bool)));
    _ui.insert_sel->setChecked(_insertSel);
    connect(_ui.log_disapp, SIGNAL(clicked(bool)), this, SLOT(log_disapp(bool)));
    _ui.log_disapp->setChecked(_log_disapp);
    connect(_ui.lrfviewer_l, SIGNAL(textChanged(const QString&)), this, SLOT(lrf_text(const QString&)));
    _ui.lrfviewer_l->setText(_lrfviewer);
    connect(_ui.rmouse_sel, SIGNAL(clicked(bool)), this, SLOT(rmousesel(bool)));
    _ui.rmouse_sel->setChecked(_mouseSel);
    connect(_ui.nf_reg_bg, SIGNAL(clicked()), this, SLOT(nf_reg_bg_slot()));
    {
        QBrush br(_nf_reg_bg);
        QPalette pal = _ui.nf_reg_bg->palette();
        pal.setBrush(QPalette::Inactive, QPalette::Button, br);
        pal.setBrush(QPalette::Active,   QPalette::Button, br);
        pal.setBrush(QPalette::Disabled, QPalette::Button, br);
        _ui.nf_reg_bg->setPalette(pal);
    }
    connect(_ui.nf_reg_fg, SIGNAL(clicked()), this, SLOT(nf_reg_fg_slot()));
    {
        QBrush br(_nf_reg_fg);
        QPalette pal = _ui.nf_reg_fg->palette();
        pal.setBrush(QPalette::Inactive, QPalette::Button, br);
        pal.setBrush(QPalette::Active,   QPalette::Button, br);
        pal.setBrush(QPalette::Disabled, QPalette::Button, br);
        _ui.nf_reg_fg->setPalette(pal);
    }
    connect(_ui.nf_sel_bg, SIGNAL(clicked()), this, SLOT(nf_sel_bg_slot()));
    {
        QBrush br(_nf_sel_bg);
        QPalette pal = _ui.nf_sel_bg->palette();
        pal.setBrush(QPalette::Inactive, QPalette::Button, br);
        pal.setBrush(QPalette::Active,   QPalette::Button, br);
        pal.setBrush(QPalette::Disabled, QPalette::Button, br);
        _ui.nf_sel_bg->setPalette(pal);
    }
    connect(_ui.nf_sel_fg, SIGNAL(clicked()), this, SLOT(nf_sel_fg_slot()));
    {
        QBrush br(_nf_sel_fg);
        QPalette pal = _ui.nf_sel_fg->palette();
        pal.setBrush(QPalette::Inactive, QPalette::Button, br);
        pal.setBrush(QPalette::Active,   QPalette::Button, br);
        pal.setBrush(QPalette::Disabled, QPalette::Button, br);
        _ui.nf_sel_fg->setPalette(pal);
    }
    connect(_ui.ord_changed, SIGNAL(clicked()), this, SLOT(ord_changed_sl()));
    {
        QBrush br(_ord_changed);
        QPalette pal = _ui.ord_changed->palette();
        pal.setBrush(QPalette::Inactive, QPalette::Button, br);
        pal.setBrush(QPalette::Active,   QPalette::Button, br);
        pal.setBrush(QPalette::Disabled, QPalette::Button, br);
        _ui.ord_changed->setPalette(pal);
    }
    connect(_ui.ord_unchanged, SIGNAL(clicked()), this, SLOT(ord_unchanged_sl()));
    {
        QBrush br(_ord_unchanged);
        QPalette pal = _ui.ord_unchanged->palette();
        pal.setBrush(QPalette::Inactive, QPalette::Button, br);
        pal.setBrush(QPalette::Active,   QPalette::Button, br);
        pal.setBrush(QPalette::Disabled, QPalette::Button, br);
        _ui.ord_unchanged->setPalette(pal);
    }
    connect(_ui.others_s, SIGNAL(clicked(bool)), this, SLOT(others_sl(bool)));
    _ui.others_s->setChecked(_others);
    connect(_ui.others_prs, SIGNAL(textChanged(const QString&)), this, SLOT(others_prs(const QString&)));
    _ui.others_prs->setText(_othersPRS);
    connect(_ui.others_sd, SIGNAL(textChanged(const QString&)), this, SLOT(others_sd(const QString&)));
    _ui.others_sd->setText(_othersSD);
    connect(_ui.reg_bg, SIGNAL(clicked()), this, SLOT(reg_bg_slot()));
    {
        QBrush br(_reg_bg);
        QPalette pal = _ui.reg_bg->palette();
        pal.setBrush(QPalette::Inactive, QPalette::Button, br);
        pal.setBrush(QPalette::Active,   QPalette::Button, br);
        pal.setBrush(QPalette::Disabled, QPalette::Button, br);
        _ui.reg_bg->setPalette(pal);
    }
    connect(_ui.reg_fg, SIGNAL(clicked()), this, SLOT(reg_fg_slot()));
    {
        QBrush br(_reg_fg);
        QPalette pal = _ui.reg_fg->palette();
        pal.setBrush(QPalette::Inactive, QPalette::Button, br);
        pal.setBrush(QPalette::Active,   QPalette::Button, br);
        pal.setBrush(QPalette::Disabled, QPalette::Button, br);
        _ui.reg_fg->setPalette(pal);
    }
    connect(_ui.dir_case, SIGNAL(clicked(bool)), this, SLOT(dir_case(bool)));
    _ui.dir_case->setChecked(_replCase);
    connect(_ui.dir_greedy, SIGNAL(clicked(bool)), this, SLOT(dir_greedy(bool)));
    _ui.dir_greedy->setChecked(_replGreedy);
    connect(_ui.dir_truscore, SIGNAL(clicked(bool)), this, SLOT(dir_truscore(bool)));
    _ui.dir_truscore->setChecked(_replTruscore);
    connect(_ui.root_prs, SIGNAL(textChanged(const QString&)), this, SLOT(root_prs(const QString&)));
    _ui.root_prs->setText(_rootPRS);
    connect(_ui.root_sd, SIGNAL(textChanged(const QString&)), this, SLOT(root_sd(const QString&)));
    _ui.root_sd->setText(_rootSD);
    connect(_ui.s1_cs, SIGNAL(clicked(bool)), this, SLOT(s1_case(bool)));
    _ui.s1_cs->setChecked(_s1Case);
    connect(_ui.s1_gr, SIGNAL(clicked(bool)), this, SLOT(s1_greedy(bool)));
    _ui.s1_gr->setChecked(_s1Greedy);
    connect(_ui.s2_cs, SIGNAL(clicked(bool)), this, SLOT(s2_case(bool)));
    _ui.s2_cs->setChecked(_s2Case);
    connect(_ui.s2_gr, SIGNAL(clicked(bool)), this, SLOT(s2_greedy(bool)));
    _ui.s2_gr->setChecked(_s2Greedy);
    connect(_ui.s3_cs, SIGNAL(clicked(bool)), this, SLOT(s3_case(bool)));
    _ui.s3_cs->setChecked(_s3Case);
    connect(_ui.s3_gr, SIGNAL(clicked(bool)), this, SLOT(s3_greedy(bool)));
    _ui.s3_gr->setChecked(_s3Greedy);
    connect(_ui.sel_dirs, SIGNAL(toggled(bool)), this, SLOT(sel_dirs(bool)));
    _ui.sel_dirs->setChecked(_selDirs);
    connect(_ui.sel_bg, SIGNAL(clicked()), this, SLOT(sel_bg_slot()));
    {
        QBrush br(_sel_bg);
        QPalette pal = _ui.sel_bg->palette();
        pal.setBrush(QPalette::Inactive, QPalette::Button, br);
        pal.setBrush(QPalette::Active,   QPalette::Button, br);
        pal.setBrush(QPalette::Disabled, QPalette::Button, br);
        _ui.sel_bg->setPalette(pal);
    }
    connect(_ui.sel_fg, SIGNAL(clicked()), this, SLOT(sel_fg_slot()));
    {
        QBrush br(_sel_fg);
        QPalette pal = _ui.sel_fg->palette();
        pal.setBrush(QPalette::Inactive, QPalette::Button, br);
        pal.setBrush(QPalette::Active,   QPalette::Button, br);
        pal.setBrush(QPalette::Disabled, QPalette::Button, br);
        _ui.sel_fg->setPalette(pal);
    }
    connect(_ui.smouse_sel, SIGNAL(clicked(bool)), this, SLOT(smousesel(bool)));
    _ui.smouse_sel->setChecked(_smouseSel);
    connect(_ui.sorder_a, SIGNAL(toggled(bool)), this, SLOT(sorder_a(bool)));
    _ui.sorder_a->setChecked(_sorder_a);
    connect(_ui.s1_a, SIGNAL(toggled(bool)), this, SLOT(sort1_a(bool)));
    _ui.s1_a->setChecked(_sort1_a);
    connect(_ui.s1_d, SIGNAL(toggled(bool)), this, SLOT(sort1_d(bool)));
    _ui.s1_d->setChecked(_sort1_d);
    connect(_ui.s1_t, SIGNAL(toggled(bool)), this, SLOT(sort1_t(bool)));
    _ui.s1_t->setChecked(_sort1_t);
    connect(_ui.s2_a, SIGNAL(toggled(bool)), this, SLOT(sort2_a(bool)));
    _ui.s2_a->setChecked(_sort2_a);
    connect(_ui.s2_d, SIGNAL(toggled(bool)), this, SLOT(sort2_d(bool)));
    _ui.s2_d->setChecked(_sort2_d);
    connect(_ui.s2_t, SIGNAL(toggled(bool)), this, SLOT(sort2_t(bool)));
    _ui.s2_t->setChecked(_sort2_t);
    connect(_ui.s3_a, SIGNAL(toggled(bool)), this, SLOT(sort3_a(bool)));
    _ui.s3_a->setChecked(_sort3_a);
    connect(_ui.s3_d, SIGNAL(toggled(bool)), this, SLOT(sort3_d(bool)));
    _ui.s3_d->setChecked(_sort3_d);
    connect(_ui.s3_t, SIGNAL(toggled(bool)), this, SLOT(sort3_t(bool)));
    _ui.s3_t->setChecked(_sort3_t);
    connect(_ui.tr_author, SIGNAL(clicked(bool)), this, SLOT(tr_author(bool)));
    _ui.tr_author->setChecked(_translAuth);
    connect(_ui.tr_clear, SIGNAL(clicked(bool)), this, SLOT(tr_clear(bool)));
    _ui.tr_clear->setChecked(_translClear);
    connect(_ui.tr_title, SIGNAL(clicked(bool)), this, SLOT(tr_title(bool)));
    _ui.tr_title->setChecked(_translTitl);
    connect(_ui.dir_rules, SIGNAL(textChanged(const QString&)), this, SLOT(repl_rules(const QString&)));
    _ui.dir_rules->setText(_replRules);
    connect(_ui.s1_re, SIGNAL(textChanged(const QString&)), this, SLOT(s1_rules(const QString&)));
    _ui.s1_re->setText(_s1_rules);
    connect(_ui.s2_re, SIGNAL(textChanged(const QString&)), this, SLOT(s2_rules(const QString&)));
    _ui.s2_re->setText(_s2_rules);
    connect(_ui.s3_re, SIGNAL(textChanged(const QString&)), this, SLOT(s3_rules(const QString&)));
    _ui.s3_re->setText(_s3_rules);

    restore_dependecies();
    QSettings    st(mngr505::_company, mngr505::_appName);
    st.beginGroup("Config");
    _ui.tabs->setCurrentIndex(st.value("current_index", 0).toInt());
    _EPUBStyles = ini2str(st.value("EPUB_styles", str2ini(_default_css)).toString());
    st.endGroup();

} // Config::Config


////////////////////////////////////////////////////////////////////////
void Config::restore_dependecies()
{
    _ui.sel_files->setChecked(!_selDirs);
    _ui.sorder_d->setChecked(!_sorder_a);
    concat_sl(_concat);
    others_sl(_others);
    coll_empty(_coll_empty);
    tr_clear(_translClear);
    _ui.f9_epub->setChecked(!_f9_lrf);
    f9_lrf(_f9_lrf);

    setFileExists(_fb2viewer, _fb2viewer, _ui.fb2_ok);
    setFileExists(_lrfviewer, _lrfviewer, _ui.lrf_ok);
    setFileExists(_fb2lrf,    _fb2lrf,    _ui.fb2lrf_ok);
    setDirExists(_epub_tmp,  _epub_tmp,  _ui.epub_tmp_ok);
    setDirExists(_fb2lrf_tmp,  _fb2lrf_tmp,  _ui.fb2lrf_tmp_ok);
    setFB2CommandCheck(_fb2lrf_cmd, _fb2lrf_cmd, _ui.fb2lrf_cmd_ok);
    setFileReadable(_fb2styles, _fb2styles, _ui.fb2styles_ok);
    fb2lrf_env();
} // Config::restore_dependecies


////////////////////////////////////////////////////////////////////////
void Config::saveGeom()
{
    // Save geomtery
    QSettings    st(mngr505::_company, mngr505::_appName);
    st.beginGroup("Config");
    st.setValue("size", size());
    st.setValue("pos", pos());
    st.endGroup();
    //qDebug("Config/Save geometry:    %dx%d+%d+%d", size().width(), size().height(), pos().x(), pos().y());
} // Config::saveGeom


////////////////////////////////////////////////////////////////////////
void Config::restoreGeom()
{
    QSettings    st(mngr505::_company, mngr505::_appName);
    QVariant     v;
    QString      geom;

    // Restore geometry
    st.beginGroup("Config");
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
    //qDebug("Config/Restore geometry: %s", qPrintable(geom));
} // Config::restoreGeom


////////////////////////////////////////////////////////////////////////
int Config::exec1()
{
    restoreGeom();
    return exec();
} //  Config::exec1


////////////////////////////////////////////////////////////////////////
void Config::reject()
{
    saveGeom();
    QDialog::reject();
} // Config::reject


////////////////////////////////////////////////////////////////////////
void Config::accept()
{
    saveGeom();
    QDialog::accept();
} // Config::accept


////////////////////////////////////////////////////////////////////////
void Config::revert()
{
    _cmouseSel = true;
    _coll_dummy = false;
    _coll_empty = false;
    _coll_enum = false;
    _collectdbg = false;
    _concat = false;
    _concatSep = SEPARATOR;
    _confDelDir = true;
    _confExit = true;
    _confMax = 100;
    _confOverWr = true;
    _epub_tmp = EPUB_TMP;
    _f9_lrf = false;
    _fb2Pattern = FB2_PATTERN;
    _fb2lrf = FB2LRF;
    _fb2lrf_cmd = FB2LRF_CMD;
    _fb2lrf_drv = FB2LRF_DRV;
    _fb2lrf_env = FB2LRF_ENV;
    _fb2lrf_err = false;
    _fb2lrf_ovr = true;
    _fb2lrf_scm = true;
    _fb2lrf_tmp = FB2LRF_TMP;
    _fb2lrf_utmp = true;
    _fb2styles = FB2STYLES;
    _fb2viewer = FB2_VIEWER;
    _hide_files = HIDE_FILES;
    _htmlReport = false;
    _insertSel = true;
    _log_disapp = true;
    _lrfviewer = LRF_VIEWER;
    _mouseSel = true;
    _nf_reg_bg = NF_REG_BG;
    _nf_reg_fg = NF_REG_FG;
    _nf_sel_bg = NF_SEL_BG;
    _nf_sel_fg = NF_SEL_FG;
    _ord_changed = ORD_CHANGED;
    _ord_unchanged = ORD_UNCHANGED;
    _others = true;
    _othersPRS = OTHERS_PRS;
    _othersSD = OTHERS_PRS;
    _reg_bg = REG_BG;
    _reg_fg = REG_FG;
    _replCase = true;
    _replGreedy = true;
    _replTruscore = true;
    _rootPRS = ROOT_PRS;
    _rootSD = ROOT_SD;
    _s1Case = true;
    _s1Greedy = true;
    _s2Case = true;
    _s2Greedy = true;
    _s3Case = true;
    _s3Greedy = true;
    _selDirs = true;
    _sel_bg = SEL_BG;
    _sel_fg = SEL_FG;
    _smouseSel = true;
    _sorder_a = true;
    _sort1_a = true;
    _sort1_d = false;
    _sort1_t = false;
    _sort2_a = false;
    _sort2_d = false;
    _sort2_t = true;
    _sort3_a = false;
    _sort3_d = true;
    _sort3_t = false;
    _translAuth = false;
    _translClear = true;
    _translTitl = false;
    _replRules = REPL_RULES;
    _from = QRegExp();
    _to.clear();
    _s1_rules = S1_RULES;
    _s1_from = QRegExp();
    _s1_to.clear();
    _s2_rules = S2_RULES;
    _s2_from = QRegExp();
    _s2_to.clear();
    _s3_rules = S3_RULES;
    _s3_from = QRegExp();
    _s3_to.clear();
    _EPUBStyles = _default_css;
} // Config::revert


////////////////////////////////////////////////////////////////////////
void Config::reread()
{
    QSettings    st(mngr505::_company, mngr505::_appName);
    st.beginGroup("Config");
    _cmouseSel = st.value("cmouse_selects", true).toBool();
    _coll_dummy = st.value("dummy_books_in_empty_collections", false).toBool();
    _coll_empty = st.value("create_empty_collections", false).toBool();
    _coll_enum = st.value("enumerate_collections", false).toBool();
    _collectdbg = st.value("collect_debug", false).toBool();
    _concat = st.value("concatenate", false).toBool();
    _concatSep = st.value("concatenate_separator", SEPARATOR).toString();
    _confDelDir = st.value("conf_non_empty_dirs", true).toBool();
    _confExit = st.value("conf_exit", true).toBool();
    _confMax = st.value("conf_max_files", 100).toInt();
    _confOverWr = st.value("conf_overwrite", true).toBool();
    _epub_tmp = st.value("epub_tmp_dir", EPUB_TMP).toString();
    _f9_lrf = st.value("f9_lrf", false).toBool();
    _fb2Pattern = st.value("fb2_pattern", FB2_PATTERN).toString();
    _fb2lrf = st.value("fb2lrf", FB2LRF).toString();
    _fb2lrf_cmd = st.value("fb2lrf_cmd", FB2LRF_CMD).toString();
    _fb2lrf_drv = st.value("fb2lrf_virt_drive", FB2LRF_DRV).toString();
    _fb2lrf_env = st.value("fb2lrf_env", FB2LRF_ENV).toStringList().join("\n");
    _fb2lrf_err = st.value("fb2lrf_show_on_error", false).toBool();
    _fb2lrf_ovr = st.value("fb2lrf_override_env", true).toBool();
    _fb2lrf_scm = st.value("fb2lrf_show_cmd", true).toBool();
    _fb2lrf_tmp = st.value("fb2lrf_tmp_dir", FB2LRF_TMP).toString();
    _fb2lrf_utmp = st.value("fb2lrf_usetmp", true).toBool();
    _fb2styles = st.value("fb2lrf_styles_file", FB2STYLES).toString();
    _fb2viewer = st.value("fb2_viewer", FB2_VIEWER).toString();
    _hide_files = st.value("hide_files", HIDE_FILES).toString();
    _htmlReport = st.value("html_report", false).toBool();
    _insertSel = st.value("insert_selects", true).toBool();
    _log_disapp = st.value("log_disappears", true).toBool();
    _lrfviewer = st.value("lrf_viewer", LRF_VIEWER).toString();
    _mouseSel = st.value("rmouse_selects", true).toBool();
    _nf_reg_bg = st.value("nf_reg_bg_color", NF_REG_BG).value<QColor>();
    _nf_reg_fg = st.value("nf_reg_fg_color", NF_REG_FG).value<QColor>();
    _nf_sel_bg = st.value("nf_sel_bg_color", NF_SEL_BG).value<QColor>();
    _nf_sel_fg = st.value("nf_sel_fg_color", NF_SEL_FG).value<QColor>();
    _ord_changed = st.value("ord_changed_color", ORD_CHANGED).value<QColor>();
    _ord_unchanged = st.value("ord_unchanged_color", ORD_UNCHANGED).value<QColor>();
    _others = st.value("create_others", true).toBool();
    _othersPRS = st.value("others_prsname", OTHERS_PRS).toString();
    _othersSD = st.value("others_sdname", OTHERS_PRS).toString();
    _reg_bg = st.value("reg_bg_color", REG_BG).value<QColor>();
    _reg_fg = st.value("reg_fg_color", REG_FG).value<QColor>();
    _replCase = st.value("repl_casesensitive", true).toBool();
    _replGreedy = st.value("repl_greedy", true).toBool();
    _replTruscore = st.value("repl_truscore", true).toBool();
    _rootPRS = st.value("root_prs", ROOT_PRS).toString();
    _rootSD = st.value("root_sd", ROOT_SD).toString();
    _s1Case = st.value("s1_casesensitive", true).toBool();
    _s1Greedy = st.value("s1_greedy", true).toBool();
    _s2Case = st.value("s2_casesensitive", true).toBool();
    _s2Greedy = st.value("s2_greedy", true).toBool();
    _s3Case = st.value("s3_casesensitive", true).toBool();
    _s3Greedy = st.value("s3_greedy", true).toBool();
    _selDirs = st.value("select_directories", true).toBool();
    _sel_bg = st.value("sel_bg_color", SEL_BG).value<QColor>();
    _sel_fg = st.value("sel_fg_color", SEL_FG).value<QColor>();
    _smouseSel = st.value("smouse_selects", true).toBool();
    _sorder_a = st.value("sort_order_asc", true).toBool();
    _sort1_a = st.value("sort1_author", true).toBool();
    _sort1_d = st.value("sort1_date", false).toBool();
    _sort1_t = st.value("sort1_title", false).toBool();
    _sort2_a = st.value("sort2_author", false).toBool();
    _sort2_d = st.value("sort2_date", false).toBool();
    _sort2_t = st.value("sort2_title", true).toBool();
    _sort3_a = st.value("sort3_author", false).toBool();
    _sort3_d = st.value("sort3_date", true).toBool();
    _sort3_t = st.value("sort3_title", false).toBool();
    _translAuth = st.value("translit_author", false).toBool();
    _translClear = st.value("translit_clear", true).toBool();
    _translTitl = st.value("translit_title", false).toBool();
    _replRules = st.value("coll_names_replacement", REPL_RULES).toString();
    _s1_rules = st.value("s1_sort_replacement", S1_RULES).toString();
    _s2_rules = st.value("s2_sort_replacement", S2_RULES).toString();
    _s3_rules = st.value("s3_sort_replacement", S3_RULES).toString();
    _EPUBStyles = ini2str(st.value("EPUB_styles", str2ini(_default_css)).toString());
    st.endGroup();
} // Config::reread


////////////////////////////////////////////////////////////////////////
Config::~Config()
{
    QSettings    st(mngr505::_company, mngr505::_appName);
    st.beginGroup("Config");
    st.setValue("current_index", _ui.tabs->currentIndex());
    st.endGroup();
} // Config::~Config


////////////////////////////////////////////////////////////////////////
void Config::init()
{
    QSettings    st(mngr505::_company, mngr505::_appName);
    st.beginGroup("Config");
    _cmouseSel = st.value("cmouse_selects", true).toBool();
    _coll_dummy = st.value("dummy_books_in_empty_collections", false).toBool();
    _coll_empty = st.value("create_empty_collections", false).toBool();
    _coll_enum = st.value("enumerate_collections", false).toBool();
    _collectdbg = st.value("collect_debug", false).toBool();
    _concat = st.value("concatenate", false).toBool();
    _concatSep = st.value("concatenate_separator", SEPARATOR).toString();
    _confDelDir = st.value("conf_non_empty_dirs", true).toBool();
    _confExit = st.value("conf_exit", true).toBool();
    _confMax = st.value("conf_max_files", 100).toInt();
    _confOverWr = st.value("conf_overwrite", true).toBool();
    _epub_tmp = st.value("epub_tmp_dir", EPUB_TMP).toString();
    _f9_lrf = st.value("f9_lrf", false).toBool();
    _fb2Pattern = st.value("fb2_pattern", FB2_PATTERN).toString();
    _fb2lrf = st.value("fb2lrf", FB2LRF).toString();
    _fb2lrf_cmd = st.value("fb2lrf_cmd", FB2LRF_CMD).toString();
    _fb2lrf_drv = st.value("fb2lrf_virt_drive", FB2LRF_DRV).toString();
    _fb2lrf_env = st.value("fb2lrf_env", FB2LRF_ENV).toStringList().join("\n");
    _fb2lrf_err = st.value("fb2lrf_show_on_error", false).toBool();
    _fb2lrf_ovr = st.value("fb2lrf_override_env", true).toBool();
    _fb2lrf_scm = st.value("fb2lrf_show_cmd", true).toBool();
    _fb2lrf_tmp = st.value("fb2lrf_tmp_dir", FB2LRF_TMP).toString();
    _fb2lrf_utmp = st.value("fb2lrf_usetmp", true).toBool();
    _fb2styles = st.value("fb2lrf_styles_file", FB2STYLES).toString();
    _fb2viewer = st.value("fb2_viewer", FB2_VIEWER).toString();
    _hide_files = st.value("hide_files", HIDE_FILES).toString();
    _htmlReport = st.value("html_report", false).toBool();
    _insertSel = st.value("insert_selects", true).toBool();
    _log_disapp = st.value("log_disappears", true).toBool();
    _lrfviewer = st.value("lrf_viewer", LRF_VIEWER).toString();
    _mouseSel = st.value("rmouse_selects", true).toBool();
    _nf_reg_bg = st.value("nf_reg_bg_color", NF_REG_BG).value<QColor>();
    _nf_reg_fg = st.value("nf_reg_fg_color", NF_REG_FG).value<QColor>();
    _nf_sel_bg = st.value("nf_sel_bg_color", NF_SEL_BG).value<QColor>();
    _nf_sel_fg = st.value("nf_sel_fg_color", NF_SEL_FG).value<QColor>();
    _ord_changed = st.value("ord_changed_color", ORD_CHANGED).value<QColor>();
    _ord_unchanged = st.value("ord_unchanged_color", ORD_UNCHANGED).value<QColor>();
    _others = st.value("create_others", true).toBool();
    _othersPRS = st.value("others_prsname", OTHERS_PRS).toString();
    _othersSD = st.value("others_sdname", OTHERS_PRS).toString();
    _reg_bg = st.value("reg_bg_color", REG_BG).value<QColor>();
    _reg_fg = st.value("reg_fg_color", REG_FG).value<QColor>();
    _replCase = st.value("repl_casesensitive", true).toBool();
    _replGreedy = st.value("repl_greedy", true).toBool();
    _replTruscore = st.value("repl_truscore", true).toBool();
    _rootPRS = st.value("root_prs", ROOT_PRS).toString();
    _rootSD = st.value("root_sd", ROOT_SD).toString();
    _s1Case = st.value("s1_casesensitive", true).toBool();
    _s1Greedy = st.value("s1_greedy", true).toBool();
    _s2Case = st.value("s2_casesensitive", true).toBool();
    _s2Greedy = st.value("s2_greedy", true).toBool();
    _s3Case = st.value("s3_casesensitive", true).toBool();
    _s3Greedy = st.value("s3_greedy", true).toBool();
    _selDirs = st.value("select_directories", true).toBool();
    _sel_bg = st.value("sel_bg_color", SEL_BG).value<QColor>();
    _sel_fg = st.value("sel_fg_color", SEL_FG).value<QColor>();
    _smouseSel = st.value("smouse_selects", true).toBool();
    _sorder_a = st.value("sort_order_asc", true).toBool();
    _sort1_a = st.value("sort1_author", true).toBool();
    _sort1_d = st.value("sort1_date", false).toBool();
    _sort1_t = st.value("sort1_title", false).toBool();
    _sort2_a = st.value("sort2_author", false).toBool();
    _sort2_d = st.value("sort2_date", false).toBool();
    _sort2_t = st.value("sort2_title", true).toBool();
    _sort3_a = st.value("sort3_author", false).toBool();
    _sort3_d = st.value("sort3_date", true).toBool();
    _sort3_t = st.value("sort3_title", false).toBool();
    _translAuth = st.value("translit_author", false).toBool();
    _translClear = st.value("translit_clear", true).toBool();
    _translTitl = st.value("translit_title", false).toBool();
    _replRules = st.value("coll_names_replacement", REPL_RULES).toString();
    {
        QString from;
        if (parseReplacement(_replRules, from, _to))
        {
            _from.setPattern(from);
            _from.setCaseSensitivity(_replCase ? Qt::CaseSensitive : Qt::CaseInsensitive);
            _from.setMinimal(_replGreedy ? false : true);
        }
    }
    _s1_rules = st.value("s1_sort_replacement", S1_RULES).toString();
    {
        QString from;
        if (parseReplacement(_s1_rules, from, _s1_to))
        {
            _s1_from.setPattern(from);
            _s1_from.setCaseSensitivity(_s1Case ? Qt::CaseSensitive : Qt::CaseInsensitive);
            _s1_from.setMinimal(_s1Greedy ? false : true);
        }
    }
    _s2_rules = st.value("s2_sort_replacement", S2_RULES).toString();
    {
        QString from;
        if (parseReplacement(_s2_rules, from, _s2_to))
        {
            _s2_from.setPattern(from);
            _s2_from.setCaseSensitivity(_s2Case ? Qt::CaseSensitive : Qt::CaseInsensitive);
            _s2_from.setMinimal(_s2Greedy ? false : true);
        }
    }
    _s3_rules = st.value("s3_sort_replacement", S3_RULES).toString();
    {
        QString from;
        if (parseReplacement(_s3_rules, from, _s3_to))
        {
            _s3_from.setPattern(from);
            _s3_from.setCaseSensitivity(_s3Case ? Qt::CaseSensitive : Qt::CaseInsensitive);
            _s3_from.setMinimal(_s3Greedy ? false : true);
        }
    }
    _EPUBStyles = ini2str(st.value("EPUB_styles", str2ini(_default_css)).toString());
    st.endGroup();
    set_f9_text(_f9_lrf);
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
void Config::dirHelp()
{
    static const char *txt=
        "<h3><center>\"Directory\" => \"collection name\" replacement</center></h3>"
        "<p>If you leave this field empty collection names will exactly reflect "
        "corresponding directory names.</p>"

        "<p>But you can replace some character from directory name to some "
        "other characters in collection names. The replacement syntax is:<br><br> "
        "<b>s/<font color=#0000FF>regexp</font>/<font color=#0000FF>replacement</font>/</b></p>"

        "<p>Any character may be used as a delimiter instead of \"/\". "
        "The syntax of regular expression is pretty much perl/sed like, "
        "so I won't explain it here. The only difference is usage of <font color=#0000FF>\\1</font> "
        "like patterns as part "
        "of replcement string instead of <font color=#0000FF>$1</font></p>"

        "<p>More about Qt regular expressions - <a href=\"http://doc.trolltech.com/4.4/qregexp.html\">here</a></p>"
        "Examples:<br>"
        "<b>s/Title_([^)]*)/\\1/</b> - Change Title_SOMETHING to SOMETHING<br>"
        "<b>s%(\\w+)(?:\\W+\\w*)*(/(\\w+\\W*)+)*\\W*%\\1\\2%</b> - Shorten collection name (pick first word from top level directory<br>"


        ;
    QMessageBox::information(0, "dir. name replacement", tr(txt));
} // Config::dirHelp

////////////////////////////////////////////////////////////////////////
void Config::save()
{
    QSettings    st(mngr505::_company, mngr505::_appName);
    st.beginGroup("Config");
    st.setValue("cmouse_selects", _cmouseSel);
    st.setValue("dummy_books_in_empty_collections", _coll_dummy);
    st.setValue("create_empty_collections", _coll_empty);
    st.setValue("enumerate_collections", _coll_enum);
    st.setValue("collect_debug", _collectdbg);
    st.setValue("concatenate", _concat);
    st.setValue("concatenate_separator", _concatSep);
    st.setValue("conf_non_empty_dirs", _confDelDir);
    st.setValue("conf_exit", _confExit);
    st.setValue("conf_max_files", _confMax);
    st.setValue("conf_overwrite", _confOverWr);
    st.setValue("epub_tmp_dir", _epub_tmp);
    st.setValue("f9_lrf", _f9_lrf);
    st.setValue("fb2_pattern", _fb2Pattern);
    st.setValue("fb2lrf", _fb2lrf);
    st.setValue("fb2lrf_cmd", _fb2lrf_cmd);
    st.setValue("fb2lrf_virt_drive", _fb2lrf_drv);
    st.setValue("fb2lrf_env", _fb2lrf_env.split('\n', QString::SkipEmptyParts));
    st.setValue("fb2lrf_show_on_error", _fb2lrf_err);
    st.setValue("fb2lrf_override_env", _fb2lrf_ovr);
    st.setValue("fb2lrf_show_cmd", _fb2lrf_scm);
    st.setValue("fb2lrf_tmp_dir", _fb2lrf_tmp);
    st.setValue("fb2lrf_usetmp", _fb2lrf_utmp);
    st.setValue("fb2lrf_styles_file", _fb2styles);
    st.setValue("fb2_viewer", _fb2viewer);
    st.setValue("hide_files", _hide_files);
    st.setValue("html_report", _htmlReport);
    st.setValue("insert_selects", _insertSel);
    st.setValue("log_disappears", _log_disapp);
    st.setValue("lrf_viewer", _lrfviewer);
    st.setValue("rmouse_selects", _mouseSel);
    st.setValue("nf_reg_bg_color", _nf_reg_bg);
    st.setValue("nf_reg_fg_color", _nf_reg_fg);
    st.setValue("nf_sel_bg_color", _nf_sel_bg);
    st.setValue("nf_sel_fg_color", _nf_sel_fg);
    st.setValue("ord_changed_color", _ord_changed);
    st.setValue("ord_unchanged_color", _ord_unchanged);
    st.setValue("create_others", _others);
    st.setValue("others_prsname", _othersPRS);
    st.setValue("others_sdname", _othersSD);
    st.setValue("reg_bg_color", _reg_bg);
    st.setValue("reg_fg_color", _reg_fg);
    st.setValue("repl_casesensitive", _replCase);
    st.setValue("repl_greedy", _replGreedy);
    st.setValue("repl_truscore", _replTruscore);
    st.setValue("root_prs", _rootPRS);
    st.setValue("root_sd", _rootSD);
    st.setValue("s1_casesensitive", _s1Case);
    st.setValue("s1_greedy", _s1Greedy);
    st.setValue("s2_casesensitive", _s2Case);
    st.setValue("s2_greedy", _s2Greedy);
    st.setValue("s3_casesensitive", _s3Case);
    st.setValue("s3_greedy", _s3Greedy);
    st.setValue("select_directories", _selDirs);
    st.setValue("sel_bg_color", _sel_bg);
    st.setValue("sel_fg_color", _sel_fg);
    st.setValue("smouse_selects", _smouseSel);
    st.setValue("sort_order_asc", _sorder_a);
    st.setValue("sort1_author", _sort1_a);
    st.setValue("sort1_date", _sort1_d);
    st.setValue("sort1_title", _sort1_t);
    st.setValue("sort2_author", _sort2_a);
    st.setValue("sort2_date", _sort2_d);
    st.setValue("sort2_title", _sort2_t);
    st.setValue("sort3_author", _sort3_a);
    st.setValue("sort3_date", _sort3_d);
    st.setValue("sort3_title", _sort3_t);
    st.setValue("translit_author", _translAuth);
    st.setValue("translit_clear", _translClear);
    st.setValue("translit_title", _translTitl);
    st.setValue("coll_names_replacement", _replRules);
    st.setValue("s1_sort_replacement", _s1_rules);
    st.setValue("s2_sort_replacement", _s2_rules);
    st.setValue("s3_sort_replacement", _s3_rules);
    st.setValue("EPUB_styles", str2ini(_EPUBStyles));
    st.setValue("current_index", _ui.tabs->currentIndex());
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
void  Config::setFileReadable(const QString& fname, QString& v, QLabel *l)
{
    QFileInfo fi(fname);
    bool      ok = fi.isDir() ? false : fi.isReadable();

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
void Config::setFB2CommandCheck(const QString& t, QString& v, QLabel *l)
{
    bool      ok = t.contains("%INP") && t.contains("%OUT");

    QBrush    br(ok ? valid : invalid);
    QPalette  pal = l->palette();
    br.setStyle(Qt::SolidPattern);
    pal.setBrush(QPalette::Inactive, QPalette::Window, br);
    pal.setBrush(QPalette::Active, QPalette::Window, br);
    l->setPalette(pal);

    l->setText(ok ? "OK" : "NA");
    if (ok)
        v = t;
    else
        v.clear();
} // Config::setFB2CommandCheck


////////////////////////////////////////////////////////////////////////
void  Config::setSort()
{
    bool ok = false;
    int  sa = (int)_sort1_a + (int)_sort2_a + (int)_sort3_a;
    int  st = (int)_sort1_t + (int)_sort2_t + (int)_sort3_t;
    int  sd = (int)_sort1_d + (int)_sort2_d + (int)_sort3_d;
    _ui.errstr->clear();
    if (sa == 1  &&  st == 1 &&  sd == 1)
        ok = true;
    else
        _ui.errstr->setText(tr("<b><font color=#ff0000>auhtor/title/data "
                               "should appear only once</font></b>"));

    _ui.s1_l->setDisabled(_sort1_d);
    _ui.s1_re->setDisabled(_sort1_d);
    _ui.s1_cs->setDisabled(_sort1_d);
    _ui.s1_gr->setDisabled(_sort1_d);

    _ui.s2_l->setDisabled(_sort2_d);
    _ui.s2_re->setDisabled(_sort2_d);
    _ui.s2_cs->setDisabled(_sort2_d);
    _ui.s2_gr->setDisabled(_sort2_d);

    _ui.s3_l->setDisabled(_sort3_d);
    _ui.s3_re->setDisabled(_sort3_d);
    _ui.s3_cs->setDisabled(_sort3_d);
    _ui.s3_gr->setDisabled(_sort3_d);

    QBrush   br(ok ? valid : invalid);
    QPalette pal = _ui.ok_b->palette();
    pal.setBrush(QPalette::Inactive, QPalette::Button, br);
    pal.setBrush(QPalette::Active,   QPalette::Button, br);
    pal.setBrush(QPalette::Disabled, QPalette::Button, br);
    _ui.ok_b->setPalette(pal);

    _ui.ok_b->setText(ok ? "OK" : "Invalid");
    _ui.ok_b->setEnabled(ok);
} // Config::setSort


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
void Config::concat_sl(bool checked)
{
    _ui.concat_l->setEnabled(checked);
    _ui.concat_s->setEnabled(checked);
    _concat = checked;
} // Config::concat_slot


////////////////////////////////////////////////////////////////////////
void Config::others_sl(bool checked)
{
    _ui.others_lprs->setEnabled(checked);
    _ui.others_prs->setEnabled(checked);
    _ui.others_lsd->setEnabled(checked);
    _ui.others_sd->setEnabled(checked);
    _others = checked;
} // Config::others_slot

////////////////////////////////////////////////////////////////////////
void Config::coll_empty(bool checked)
{
    _coll_empty = checked;
    _ui.coll_dummy->setEnabled(checked);
} // Config::coll_empty

////////////////////////////////////////////////////////////////////////
Config::Sorder Config::sorder1()
{
    if (sort1A())
        return S_AUTHOR;
    else if (sort1T())
        return S_TITLE;
    else if (sort1D())
        return S_DATE;
    return S_UNKNOWN;
} //  Config::sorder1

////////////////////////////////////////////////////////////////////////
Config::Sorder Config::sorder2()
{
    if (sort2A())
        return S_AUTHOR;
    else if (sort2T())
        return S_TITLE;
    else if (sort2D())
        return S_DATE;
    return S_UNKNOWN;
} //  Config::sorder2

////////////////////////////////////////////////////////////////////////
Config::Sorder Config::sorder3()
{
    if (sort3A())
        return S_AUTHOR;
    else if (sort3T())
        return S_TITLE;
    else if (sort3D())
        return S_DATE;
    return S_UNKNOWN;
} //  Config::sorder3

////////////////////////////////////////////////////////////////////////
void Config::tr_clear(bool checked)
{
    _ui.tr_author->setDisabled(checked);
    _ui.tr_title->setDisabled(checked);
} // Config::tr_clear

////////////////////////////////////////////////////////////////////////
void Config::f9_lrf(bool checked)
{
    set_f9_text(checked);
    _f9_lrf = checked;
} // Config::f9_lrf

////////////////////////////////////////////////////////////////////////
void Config::set_f9_text(bool checked)
{
    mngr505::_ui.F9->setText(checked ? "F9 - FB2LRF" : "F9 - FB2/EPUB");
} // Config::set_f9_text

////////////////////////////////////////////////////////////////////////
void Config::epub_editstyles()
{
    EditCSS *e = new EditCSS(this);
    e->show();
} // Config::epub_editstyles

////////////////////////////////////////////////////////////////////////
void Config::fb2lrf_env()
{
    QString     new_cont = _ui.fb2lrf_env->toPlainText();
    QStringList el(new_cont.split('\n', QString::SkipEmptyParts));

    // Validation
    bool        ok = true;
    for (int i=0; i<el.size(); i++)
        if (el[i].indexOf('=') == -1)
        {
            ok = false;
            break;
        }

    QBrush   br(ok ? valid : invalid);
    QPalette pal = _ui.fb2lrf_env_ok->palette();
    pal.setBrush(QPalette::Inactive, QPalette::Button, br);
    pal.setBrush(QPalette::Active,   QPalette::Button, br);
    pal.setBrush(QPalette::Disabled, QPalette::Button, br);
    pal.setBrush(QPalette::Inactive, QPalette::Window, br);
    pal.setBrush(QPalette::Active,   QPalette::Window, br);
    pal.setBrush(QPalette::Disabled, QPalette::Window, br);
    _ui.fb2lrf_env_ok->setPalette(pal);
    _ui.fb2lrf_env_ok->setText(ok ? "OK" : "NA");
    _ui.ok_b->setPalette(pal);
    _ui.ok_b->setText(ok ? "OK" : "Invalid");
    _ui.ok_b->setEnabled(ok);

    _ui.errstr->clear();
    if (ok)
        _fb2lrf_env = new_cont;
    else
        _ui.errstr->setText(tr("<b><font color=#ff0000>Invalid environment syntax: should be VAR=VALUE pair</font></b>"));
} // Config::fb2lrf_env
#if 0
////////////////////////////////////////////////////////////////////////
QString Config::join(const QStringList& sl, const QChar& del)
{
    if (sl.size() < 1)
        return QString();

    QString rc = sl[0];
    for (int i=1; i<sl.size(); i++)
    {
        rc += del;
        rc += sl[i];
    }
    return rc;
} // Config::join
#endif
