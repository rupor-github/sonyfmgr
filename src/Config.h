/*
 * $Id$
 *
 * Config definition
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <QColor>
#include <QColorDialog>
#include <QMainWindow>
#include <QString>
#include <QRegExp>

#include "ui_Config.h"

////////////////////////////////////////////////////////////////////////
class QTextEdit;
class QPushButton;
class Config;
class EditCSS : public QMainWindow
{
    Q_OBJECT

public:
    EditCSS(Config *par);
    ~EditCSS();

    virtual void show();

    QTextEdit   *te;
    QPushButton *save;
    QPushButton *cancel;
    QPushButton *revertCurrent;
    QPushButton *revertDefault;

private slots:
    void textChanged();
    void saveReq();
    void revertCReq();
    void revertDReq();

private:
    Config    *_par;
    bool      _wasEdit;
};


////////////////////////////////////////////////////////////////////////
class Config: public QDialog {
    Q_OBJECT

public:
    Config(QWidget *par=0);
    ~Config();

    enum Sorder { S_UNKNOWN, S_AUTHOR, S_TITLE, S_DATE };
    static Ui_Dialog   _ui;

    static const QColor valid;
    static const QColor invalid;

    static bool    plainText()  { return !_htmlReport; }
    static void    save();
    static void    init();
    static void    revert();
    static void    reread();

    static Sorder  sorder1();
    static Sorder  sorder2();
    static Sorder  sorder3();

    virtual void reject();
    virtual void accept();

    int     exec1();
    void    restore_dependecies();

    static QString CSS()                    { return _EPUBStyles;  }
    static void    setCSS(const QString& c) { _EPUBStyles = c;      }
    static QString DefCSS()                 { return  _default_css; }

    // _cmouseSel property definition
    public:
        static bool cmouse_sel() { return _cmouseSel; }
    private:
        static bool _cmouseSel;
    private slots:
        void cmousesel(bool checked) { _cmouseSel = checked; }

    // _coll_dummy property definition
    public:
        static bool dummyColl() { return _coll_dummy; }
    private:
        static bool _coll_dummy;
    private slots:
        void coll_dummy(bool checked) { _coll_dummy = checked; }

    // _coll_empty property definition
    public:
        static bool emptyColl() { return _coll_empty; }
    private:
        static bool _coll_empty;

    // _coll_enum property definition
    public:
        static bool enumColl() { return _coll_enum; }
    private:
        static bool _coll_enum;
    private slots:
        void coll_enum(bool checked) { _coll_enum = checked; }

    // _collectdbg property definition
    public:
        static bool collectDbg() { return _collectdbg; }
    private:
        static bool _collectdbg;
    private slots:
        void collectdbg(bool checked) { _collectdbg = checked; }

    // _concat property definition
    public:
        static bool concat() { return _concat; }
    private:
        static bool _concat;

    // _concatSep property definition
    public:
        static QString concatSep() { return _concatSep; }
    private:
        static QString _concatSep;
    private slots:
        void concat_s(const QString& t) { _concatSep = t; }

    // _confDelDir property definition
    public:
        static bool confDelDir() { return _confDelDir; }
    private:
        static bool _confDelDir;
    private slots:
        void conf_dir(bool checked) { _confDelDir = checked; }

    // _confExit property definition
    public:
        static bool confExit() { return _confExit; }
    private:
        static bool _confExit;
    private slots:
        void conf_exit(bool checked) { _confExit = checked; }

    // _confMax property definition
    public:
        static int confMax() { return _confMax; }
    private:
        static int _confMax;
    private slots:
        void conf_max(int t) { _confMax = t; }

    // _confOverWr property definition
    public:
        static bool confOverwr() { return _confOverWr; }
    private:
        static bool _confOverWr;
    private slots:
        void conf_over(bool checked) { _confOverWr = checked; }

    // _epub_tmp property definition
    public:
        static QString EPUBTmp() { return _epub_tmp; }
    private:
        static QString _epub_tmp;

    // _f9_lrf property definition
    public:
        static bool F9LRF() { return _f9_lrf; }
    private:
        static bool _f9_lrf;

    // _fb2Pattern property definition
    public:
        static QString fb2Pattern() { return _fb2Pattern; }
    private:
        static QString _fb2Pattern;
    private slots:
        void fb2_patt(const QString& t) { _fb2Pattern = t; }

    // _fb2lrf property definition
    public:
        static QString fb2LRF() { return _fb2lrf; }
    private:
        static QString _fb2lrf;

    // _fb2lrf_cmd property definition
    public:
        static QString fb2lrfCmd() { return _fb2lrf_cmd; }
    private:
        static QString _fb2lrf_cmd;

    // _fb2lrf_drv property definition
    public:
        static QString wineRoot() { return _fb2lrf_drv; }
    private:
        static QString _fb2lrf_drv;
    private slots:
        void fb2lrf_drv(const QString& t) { _fb2lrf_drv = t; }

    // _fb2lrf_env property definition
    public:
        static QString fb2lrfEnv() { return _fb2lrf_env; }
    private:
        static QString _fb2lrf_env;

    // _fb2lrf_err property definition
    public:
        static bool fb2lrfSErr() { return _fb2lrf_err; }
    private:
        static bool _fb2lrf_err;
    private slots:
        void fb2lrf_serr(bool checked) { _fb2lrf_err = checked; }

    // _fb2lrf_ovr property definition
    public:
        static bool fb2lrfOver() { return _fb2lrf_ovr; }
    private:
        static bool _fb2lrf_ovr;
    private slots:
        void fb2lrf_ovr(bool checked) { _fb2lrf_ovr = checked; }

    // _fb2lrf_scm property definition
    public:
        static bool fb2lrfSCmd() { return _fb2lrf_scm; }
    private:
        static bool _fb2lrf_scm;
    private slots:
        void fb2lrf_scmd(bool checked) { _fb2lrf_scm = checked; }

    // _fb2lrf_tmp property definition
    public:
        static QString fb2lrfTmp() { return _fb2lrf_tmp; }
    private:
        static QString _fb2lrf_tmp;

    // _fb2lrf_utmp property definition
    public:
        static bool fb2UseTmp() { return _fb2lrf_utmp; }
    private:
        static bool _fb2lrf_utmp;
    private slots:
        void fb2lrf_utmp(bool checked) { _fb2lrf_utmp = checked; }

    // _fb2styles property definition
    public:
        static QString fb2Styles() { return _fb2styles; }
    private:
        static QString _fb2styles;

    // _fb2viewer property definition
    public:
        static QString fb2Viewer() { return _fb2viewer; }
    private:
        static QString _fb2viewer;

    // _hide_files property definition
    public:
        static QString hideFiles() { return _hide_files; }
    private:
        static QString _hide_files;
    private slots:
        void hide_files(const QString& t) { _hide_files = t; }

    // _htmlReport property definition
    public:
        static bool htmlReport() { return _htmlReport; }
    private:
        static bool _htmlReport;
    private slots:
        void html_rep(bool checked) { _htmlReport = checked; }

    // _insertSel property definition
    public:
        static bool insert_sel() { return _insertSel; }
    private:
        static bool _insertSel;
    private slots:
        void insertsel(bool checked) { _insertSel = checked; }

    // _log_disapp property definition
    public:
        static bool logDisapp() { return _log_disapp; }
    private:
        static bool _log_disapp;
    private slots:
        void log_disapp(bool checked) { _log_disapp = checked; }

    // _lrfviewer property definition
    public:
        static QString lrfViewer() { return _lrfviewer; }
    private:
        static QString _lrfviewer;

    // _mouseSel property definition
    public:
        static bool mouse_sel() { return _mouseSel; }
    private:
        static bool _mouseSel;
    private slots:
        void rmousesel(bool checked) { _mouseSel = checked; }

    // _nf_reg_bg property definition
    public:
        static QColor *nf_reg_bg() { return &_nf_reg_bg; }
    private:
        static QColor _nf_reg_bg;
    private slots:
        void nf_reg_bg_slot()
        {
            QColor n = QColorDialog::getColor(_nf_reg_bg, this);
            if (n.isValid())
            {
                _nf_reg_bg = n;
                QBrush br(_nf_reg_bg);
                QPalette pal = _ui.nf_reg_bg->palette();
                pal.setBrush(QPalette::Inactive, QPalette::Button, br);
                pal.setBrush(QPalette::Active,   QPalette::Button, br);
                pal.setBrush(QPalette::Disabled, QPalette::Button, br);
                _ui.nf_reg_bg->setPalette(pal);
            }
        }

    // _nf_reg_fg property definition
    public:
        static QColor *nf_reg_fg() { return &_nf_reg_fg; }
    private:
        static QColor _nf_reg_fg;
    private slots:
        void nf_reg_fg_slot()
        {
            QColor n = QColorDialog::getColor(_nf_reg_fg, this);
            if (n.isValid())
            {
                _nf_reg_fg = n;
                QBrush br(_nf_reg_fg);
                QPalette pal = _ui.nf_reg_fg->palette();
                pal.setBrush(QPalette::Inactive, QPalette::Button, br);
                pal.setBrush(QPalette::Active,   QPalette::Button, br);
                pal.setBrush(QPalette::Disabled, QPalette::Button, br);
                _ui.nf_reg_fg->setPalette(pal);
            }
        }

    // _nf_sel_bg property definition
    public:
        static QColor *nf_sel_bg() { return &_nf_sel_bg; }
    private:
        static QColor _nf_sel_bg;
    private slots:
        void nf_sel_bg_slot()
        {
            QColor n = QColorDialog::getColor(_nf_sel_bg, this);
            if (n.isValid())
            {
                _nf_sel_bg = n;
                QBrush br(_nf_sel_bg);
                QPalette pal = _ui.nf_sel_bg->palette();
                pal.setBrush(QPalette::Inactive, QPalette::Button, br);
                pal.setBrush(QPalette::Active,   QPalette::Button, br);
                pal.setBrush(QPalette::Disabled, QPalette::Button, br);
                _ui.nf_sel_bg->setPalette(pal);
            }
        }

    // _nf_sel_fg property definition
    public:
        static QColor *nf_sel_fg() { return &_nf_sel_fg; }
    private:
        static QColor _nf_sel_fg;
    private slots:
        void nf_sel_fg_slot()
        {
            QColor n = QColorDialog::getColor(_nf_sel_fg, this);
            if (n.isValid())
            {
                _nf_sel_fg = n;
                QBrush br(_nf_sel_fg);
                QPalette pal = _ui.nf_sel_fg->palette();
                pal.setBrush(QPalette::Inactive, QPalette::Button, br);
                pal.setBrush(QPalette::Active,   QPalette::Button, br);
                pal.setBrush(QPalette::Disabled, QPalette::Button, br);
                _ui.nf_sel_fg->setPalette(pal);
            }
        }

    // _ord_changed property definition
    public:
        static QColor *ord_changed() { return &_ord_changed; }
    private:
        static QColor _ord_changed;
    private slots:
        void ord_changed_sl()
        {
            QColor n = QColorDialog::getColor(_ord_changed, this);
            if (n.isValid())
            {
                _ord_changed = n;
                QBrush br(_ord_changed);
                QPalette pal = _ui.ord_changed->palette();
                pal.setBrush(QPalette::Inactive, QPalette::Button, br);
                pal.setBrush(QPalette::Active,   QPalette::Button, br);
                pal.setBrush(QPalette::Disabled, QPalette::Button, br);
                _ui.ord_changed->setPalette(pal);
            }
        }

    // _ord_unchanged property definition
    public:
        static QColor *ord_unchanged() { return &_ord_unchanged; }
    private:
        static QColor _ord_unchanged;
    private slots:
        void ord_unchanged_sl()
        {
            QColor n = QColorDialog::getColor(_ord_unchanged, this);
            if (n.isValid())
            {
                _ord_unchanged = n;
                QBrush br(_ord_unchanged);
                QPalette pal = _ui.ord_unchanged->palette();
                pal.setBrush(QPalette::Inactive, QPalette::Button, br);
                pal.setBrush(QPalette::Active,   QPalette::Button, br);
                pal.setBrush(QPalette::Disabled, QPalette::Button, br);
                _ui.ord_unchanged->setPalette(pal);
            }
        }

    // _others property definition
    public:
        static bool others() { return _others; }
    private:
        static bool _others;

    // _othersPRS property definition
    public:
        static QString othersPRS() { return _othersPRS; }
    private:
        static QString _othersPRS;
    private slots:
        void others_prs(const QString& t) { _othersPRS = t; }

    // _othersSD property definition
    public:
        static QString othersSD() { return _othersSD; }
    private:
        static QString _othersSD;
    private slots:
        void others_sd(const QString& t) { _othersSD = t; }

    // _reg_bg property definition
    public:
        static QColor *reg_bg() { return &_reg_bg; }
    private:
        static QColor _reg_bg;
    private slots:
        void reg_bg_slot()
        {
            QColor n = QColorDialog::getColor(_reg_bg, this);
            if (n.isValid())
            {
                _reg_bg = n;
                QBrush br(_reg_bg);
                QPalette pal = _ui.reg_bg->palette();
                pal.setBrush(QPalette::Inactive, QPalette::Button, br);
                pal.setBrush(QPalette::Active,   QPalette::Button, br);
                pal.setBrush(QPalette::Disabled, QPalette::Button, br);
                _ui.reg_bg->setPalette(pal);
            }
        }

    // _reg_fg property definition
    public:
        static QColor *reg_fg() { return &_reg_fg; }
    private:
        static QColor _reg_fg;
    private slots:
        void reg_fg_slot()
        {
            QColor n = QColorDialog::getColor(_reg_fg, this);
            if (n.isValid())
            {
                _reg_fg = n;
                QBrush br(_reg_fg);
                QPalette pal = _ui.reg_fg->palette();
                pal.setBrush(QPalette::Inactive, QPalette::Button, br);
                pal.setBrush(QPalette::Active,   QPalette::Button, br);
                pal.setBrush(QPalette::Disabled, QPalette::Button, br);
                _ui.reg_fg->setPalette(pal);
            }
        }

    // _replCase property definition
    public:
        static bool replCase() { return _replCase; }
    private:
        static bool _replCase;
    private slots:
        void dir_case(bool checked) { _replCase = checked; }

    // _replGreedy property definition
    public:
        static bool replGreedy() { return _replGreedy; }
    private:
        static bool _replGreedy;
    private slots:
        void dir_greedy(bool checked) { _replGreedy = checked; }

    // _replTruscore property definition
    public:
        static bool replTruscore() { return _replTruscore; }
    private:
        static bool _replTruscore;
    private slots:
        void dir_truscore(bool checked) { _replTruscore = checked; }


    // _mngThumbs property definition
    public:
        static bool mngThumbs() { return _mngThumbs; }
    private:
        static bool _mngThumbs;

    // _rootPRS property definition
    public:
        static QString rootPRS() { return _rootPRS; }
    private:
        static QString _rootPRS;
    private slots:
        void root_prs(const QString& t) { _rootPRS = t; }

    // _rootPRStmb property definition
    public:
        static QString rootPRStmb() { return _rootPRStmb; }
    private:
        static QString _rootPRStmb;
    private slots:
        void root_prs_thumbs(const QString& t) { _rootPRStmb = t; }

    // _rootSD property definition
    public:
        static QString rootSD() { return _rootSD; }
    private:
        static QString _rootSD;
    private slots:
        void root_sd(const QString& t) { _rootSD = t; }

    // _rootSDtmb property definition
    public:
        static QString rootSDtmb() { return _rootSDtmb; }
    private:
        static QString _rootSDtmb;
    private slots:
        void root_sd_thumbs(const QString& t) { _rootSDtmb = t; }

    // _s1Case property definition
    public:
        static bool s1Case() { return _s1Case; }
    private:
        static bool _s1Case;
    private slots:
        void s1_case(bool checked) { _s1Case = checked; }

    // _s1Greedy property definition
    public:
        static bool s1Greedy() { return _s1Greedy; }
    private:
        static bool _s1Greedy;
    private slots:
        void s1_greedy(bool checked) { _s1Greedy = checked; }

    // _s2Case property definition
    public:
        static bool s2Case() { return _s2Case; }
    private:
        static bool _s2Case;
    private slots:
        void s2_case(bool checked) { _s2Case = checked; }

    // _s2Greedy property definition
    public:
        static bool s2Greedy() { return _s2Greedy; }
    private:
        static bool _s2Greedy;
    private slots:
        void s2_greedy(bool checked) { _s2Greedy = checked; }

    // _s3Case property definition
    public:
        static bool s3Case() { return _s3Case; }
    private:
        static bool _s3Case;
    private slots:
        void s3_case(bool checked) { _s3Case = checked; }

    // _s3Greedy property definition
    public:
        static bool s3Greedy() { return _s3Greedy; }
    private:
        static bool _s3Greedy;
    private slots:
        void s3_greedy(bool checked) { _s3Greedy = checked; }

    // _selDirs property definition
    public:
        static bool selectDirs() { return _selDirs; }
    private:
        static bool _selDirs;
    private slots:
        void sel_dirs(bool checked) { _selDirs = checked; }

    // _sel_bg property definition
    public:
        static QColor *sel_bg() { return &_sel_bg; }
    private:
        static QColor _sel_bg;
    private slots:
        void sel_bg_slot()
        {
            QColor n = QColorDialog::getColor(_sel_bg, this);
            if (n.isValid())
            {
                _sel_bg = n;
                QBrush br(_sel_bg);
                QPalette pal = _ui.sel_bg->palette();
                pal.setBrush(QPalette::Inactive, QPalette::Button, br);
                pal.setBrush(QPalette::Active,   QPalette::Button, br);
                pal.setBrush(QPalette::Disabled, QPalette::Button, br);
                _ui.sel_bg->setPalette(pal);
            }
        }

    // _sel_fg property definition
    public:
        static QColor *sel_fg() { return &_sel_fg; }
    private:
        static QColor _sel_fg;
    private slots:
        void sel_fg_slot()
        {
            QColor n = QColorDialog::getColor(_sel_fg, this);
            if (n.isValid())
            {
                _sel_fg = n;
                QBrush br(_sel_fg);
                QPalette pal = _ui.sel_fg->palette();
                pal.setBrush(QPalette::Inactive, QPalette::Button, br);
                pal.setBrush(QPalette::Active,   QPalette::Button, br);
                pal.setBrush(QPalette::Disabled, QPalette::Button, br);
                _ui.sel_fg->setPalette(pal);
            }
        }

    // _smouseSel property definition
    public:
        static bool smouse_sel() { return _smouseSel; }
    private:
        static bool _smouseSel;
    private slots:
        void smousesel(bool checked) { _smouseSel = checked; }

    // _sorder_a property definition
    public:
        static bool sortAsc() { return _sorder_a; }
    private:
        static bool _sorder_a;
    private slots:
        void sorder_a(bool checked) { _sorder_a = checked; }

    // _sort1_a property definition
    public:
        static bool sort1A() { return _sort1_a; }
    private:
        static bool _sort1_a;

    // _sort1_d property definition
    public:
        static bool sort1D() { return _sort1_d; }
    private:
        static bool _sort1_d;

    // _sort1_t property definition
    public:
        static bool sort1T() { return _sort1_t; }
    private:
        static bool _sort1_t;

    // _sort2_a property definition
    public:
        static bool sort2A() { return _sort2_a; }
    private:
        static bool _sort2_a;

    // _sort2_d property definition
    public:
        static bool sort2D() { return _sort2_d; }
    private:
        static bool _sort2_d;

    // _sort2_t property definition
    public:
        static bool sort2T() { return _sort2_t; }
    private:
        static bool _sort2_t;

    // _sort3_a property definition
    public:
        static bool sort3A() { return _sort3_a; }
    private:
        static bool _sort3_a;

    // _sort3_d property definition
    public:
        static bool sort3D() { return _sort3_d; }
    private:
        static bool _sort3_d;

    // _sort3_t property definition
    public:
        static bool sort3T() { return _sort3_t; }
    private:
        static bool _sort3_t;

    // _translAuth property definition
    public:
        static bool confTranslA() { return _translAuth; }
    private:
        static bool _translAuth;
    private slots:
        void tr_author(bool checked) { _translAuth = checked; }

    // _translClear property definition
    public:
        static bool confTranslC() { return _translClear; }
    private:
        static bool _translClear;

    // _translTitl property definition
    public:
        static bool confTranslT() { return _translTitl; }
    private:
        static bool _translTitl;
    private slots:
        void tr_title(bool checked) { _translTitl = checked; }

    // collReplacement property definition
    public:
        static QRegExp replFrom() { return _from; }
        static QString replTo() { return _to; }
    private:
        static QRegExp _from;
        static QString _to;
        static QString _replRules;
    private slots:
        void repl_rules(const QString& t) {
            repl_rules_general(t, _from, _to, _replRules);
        }

    // s1SortReplacement property definition
    public:
        static QRegExp s1ReplFrom() { return _s1_from; }
        static QString s1ReplTo() { return _s1_to; }
    private:
        static QRegExp _s1_from;
        static QString _s1_to;
        static QString _s1_rules;
    private slots:
        void s1_rules(const QString& t) {
            if (repl_rules_general(t, _s1_from, _s1_to, _s1_rules))
                setSort();
        }

    // s2SortReplacement property definition
    public:
        static QRegExp s2ReplFrom() { return _s2_from; }
        static QString s2ReplTo() { return _s2_to; }
    private:
        static QRegExp _s2_from;
        static QString _s2_to;
        static QString _s2_rules;
    private slots:
        void s2_rules(const QString& t) {
            if (repl_rules_general(t, _s2_from, _s2_to, _s2_rules))
                setSort();
        }

    // s3SortReplacement property definition
    public:
        static QRegExp s3ReplFrom() { return _s3_from; }
        static QString s3ReplTo() { return _s3_to; }
    private:
        static QRegExp _s3_from;
        static QString _s3_to;
        static QString _s3_rules;
    private slots:
        void s3_rules(const QString& t) {
            if (repl_rules_general(t, _s3_from, _s3_to, _s3_rules))
                setSort();
        }


private slots:
    void fb2lrf_cmd(const QString& t)  { setFB2CommandCheck(t, _fb2lrf_cmd, _ui.fb2lrf_cmd_ok); }
    void lrf_text(const QString& t)    { setFileExists(t, _lrfviewer, _ui.lrf_ok);      }
    void fb2_text(const QString& t)    { setFileExists(t, _fb2viewer, _ui.fb2_ok);      }
    void fb2lrf_text(const QString& t) { setFileExists(t, _fb2lrf,    _ui.fb2lrf_ok);   }
    void fb2styles_text(const QString& t) { setFileReadable(t, _fb2styles,    _ui.fb2styles_ok);   }
    void epub_tmp(const QString& t)    { setDirExists(t,  _epub_tmp,  _ui.epub_tmp_ok);     }
    void fb2lrf_tmp(const QString& t)  { setDirExists(t,  _fb2lrf_tmp,  _ui.fb2lrf_tmp_ok); }
    void lrf_browse()       { setFname(_ui.lrfviewer_l, _lrfviewer, "LRF Viewer"); }
    void fb2_browse()       { setFname(_ui.fb2viewer_l, _fb2viewer, "FB2 Viewer"); }
    void fb2lrf_browse()    { setFname(_ui.fb2lrf_l, _fb2lrf,    "FB2LRF convertor"); }
    void fb2styles_browse() { setFname(_ui.fb2styles_l, _fb2styles,    "FB2LRF styles file"); }
    void epub_tmp_br() { setDname(_ui.epub_tmp_l,  _epub_tmp,  "EPUB temp. directory"); }
    void fb2lrf_tmp_br() { setDname(_ui.fb2lrf_tmp_l, _fb2lrf_tmp,  "FB2LRF temp. directory"); }
    void dirHelp();
    void sort1_a(bool checked) { _sort1_a = checked; setSort(); }
    void sort1_t(bool checked) { _sort1_t = checked; setSort(); }
    void sort1_d(bool checked) { _sort1_d = checked; setSort(); }
    void sort2_a(bool checked) { _sort2_a = checked; setSort(); }
    void sort2_t(bool checked) { _sort2_t = checked; setSort(); }
    void sort2_d(bool checked) { _sort2_d = checked; setSort(); }
    void sort3_a(bool checked) { _sort3_a = checked; setSort(); }
    void sort3_t(bool checked) { _sort3_t = checked; setSort(); }
    void sort3_d(bool checked) { _sort3_d = checked; setSort(); }
    void concat_sl(bool checked);
    void others_sl(bool checked);
    void coll_empty(bool checked);
    void prs_thumbs(bool checked);   
    void tr_clear(bool checked);
    void f9_lrf(bool checked);
    void epub_editstyles();
    void fb2lrf_env();

private:
    static QString    _EPUBStyles;
    static const char *_default_css;

    void        saveGeom();
    void        restoreGeom();
    void        setFileExists(const QString& fname, QString& v, QLabel *l);
    void        setFileReadable(const QString& fname, QString& v, QLabel *l);
    void        setFB2CommandCheck(const QString& t, QString& v, QLabel *l);
    void        setDirExists(const QString& fname, QString& v, QLabel *l);
    void        setSort();
    void        setFname(QLineEdit *l, QString& v, const QString& t);
    void        setDname(QLineEdit *l, QString& v, const QString& t);
    bool        repl_valid(bool v);
    bool        repl_rules_general(const QString& r, QRegExp& from_re, QString& to,
                                   QString& rules);
    static bool parseReplacement(const QString& r, QString& from, QString& to);

    static void set_f9_text(bool checked);
    static bool _init;
};

#endif
