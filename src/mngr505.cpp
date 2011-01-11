/*
 * $Id$
 *
 * mngr505 main window
 */
#include <QCloseEvent>
#include <QDir>
#include <QDomDocument>
#include <QFile>
#include <QFileDialog>
#include <QMenu>
#include <QMessageBox>
#include <QSettings>
#include <QTextStream>
#include <QToolBar>

#include "Config.h"
#include "FPanel.h"
#include "Info.h"
#include "LogWidget.h"
#include "mngr505.h"

Ui::MainWindow    mngr505::_ui;
const char *mngr505::_version = "1.15";
const char *mngr505::_company = "HaraSoft";
const char *mngr505::_appName = "mngr650";

#define FCON(f) connect(_ui.f, SIGNAL(pressed()), this, SLOT(f())); \
    _fl->append(_ui.f);
#define FDEF(f) \
void mngr505::f() {                  \
    if (_ui.lFPanel->infocus())      \
        _ui.lFPanel->f();            \
    else if (_ui.rFPanel->infocus()) \
        _ui.rFPanel->f();            \
}

////////////////////////////////////////////////////////////////////////
mngr505::mngr505(): QMainWindow(0)
{
    _ui.setupUi(this);
    setWindowTitle("Manager650");
    qApp->setWindowIcon(QIcon(":/icons/Graphics/mngr505.png"));

    Config::init();

    /*
     * Menu
     * ----
     */
    QMenu    *menu;

    // ============================= File menu
    menu = menuBar()->addMenu(tr("&File"));
    menu->addAction(QIcon(":/icons/Graphics/select.png"),
                    tr("S&elect"), this, SLOT(setSelection()), tr("+"));
    menu->addAction(QIcon(":/icons/Graphics/unselect.png"),
                    tr("&UnSelect"), this, SLOT(clearSelection()), tr("-"));
    menu->addSeparator(); // -----------------------------

    menu->addAction(QIcon(":/icons/Graphics/exit.png"),
                    tr("&Quit"), this, SLOT(confExit()), tr("F10"));
    // =============================

    // ============================= Configuration menu
    menu = menuBar()->addMenu(tr("&Configuration"));
    menu->addAction(QIcon(":/icons/Graphics/config.png"),
                    tr("&Configure..."), this, SLOT(config()));
    menu->addAction(tr("&Revert to default"), this, SLOT(config_revert()));
    menu->addSeparator(); // -----------------------------

    menu->addAction(tr("&Import configuration"), this, SLOT(config_import()));
    menu->addAction(tr("&Export configuration"), this, SLOT(config_export()));
    // =============================

    // ============================= Actions menu
    menu = menuBar()->addMenu(tr("&Actions"));
    menu->addAction(QIcon(":/icons/Graphics/up.png"),
                    tr("Move item &Up"), this, SLOT(moveUp()), tr("Shift+Up"));
    menu->addAction(QIcon(":/icons/Graphics/down.png"),
                    tr("Move item &Down"), this, SLOT(moveDown()), tr("Shift+Down"));

    QAction *saveAction = new QAction(QIcon(":/icons/Graphics/save.png"),
                                      tr("Save &Order"), this);
    saveAction->setShortcut(tr("Ctrl+S"));
    saveAction->setShortcutContext(Qt::WidgetShortcut);
    connect(saveAction, SIGNAL(triggered()), this, SLOT(saveOrder()));
    menu->addAction(saveAction);
    menu->addSeparator(); // -----------------------------

    QAction *enumAction = new QAction(QIcon(":/icons/Graphics/enum.png"),
                                      tr("Enumerate"), this);
    enumAction->setShortcut(tr("Ctrl+E"));
    enumAction->setShortcutContext(Qt::WidgetShortcut);
    connect(enumAction, SIGNAL(triggered()), this, SLOT(enumerate()));
    menu->addAction(enumAction);
    QAction *unEnumAction = new QAction(QIcon(":/icons/Graphics/unenum.png"),
                                      tr("Un-enumerate"), this);
    unEnumAction->setShortcut(tr("Ctrl+W"));
    unEnumAction->setShortcutContext(Qt::WidgetShortcut);
    connect(unEnumAction, SIGNAL(triggered()), this, SLOT(unEnumerate()));
    menu->addAction(unEnumAction);
    menu->addSeparator(); // -----------------------------

    menu->addAction(QIcon(":/icons/Graphics/sort.png"),
                    tr("So&rt"), this, SLOT(sort()), tr("Ctrl+F3"));
    menu->addAction(QIcon(":/icons/Graphics/bsort.png"),
                    tr("Sort by &XML"), this, SLOT(bsort()), tr("Ctrl+F4"));
    menu->addAction(QIcon(":/icons/Graphics/asort.png"),
                    tr("Sort &all collections"), this, SLOT(asort()), tr("Ctrl+F5"));
    menu->addSeparator(); // -----------------------------

    QAction *lookDevice = new QAction(QIcon(":/icons/Graphics/bookshelf.png"),
                                     tr("Look for eBook or SD card"), this);
    connect(lookDevice, SIGNAL(triggered()), this, SLOT(findDevice()));
    menu->addAction(lookDevice);

    QAction *scanAct = new QAction(QIcon(":/icons/Graphics/books.png"),
                                   tr("&Scan and create colections"), this);
    scanAct->setEnabled(false);
    connect(scanAct, SIGNAL(triggered()), _ui.lFPanel, SLOT(updateColl()));
    menu->addAction(scanAct);

    QAction *umountAct = new QAction(QIcon(":/icons/Graphics/umount.png"),
                                     tr("U&mount"), this);
    umountAct->setEnabled(false);
    connect(umountAct, SIGNAL(triggered()), this, SLOT(umount()));
    menu->addAction(umountAct);
    // =============================

    // ============================= Help menu
    menu = menuBar()->addMenu(tr("&Help"));
    menu->addAction(QIcon(":/icons/Graphics/mngr505.png"), tr("&About"), this, SLOT(about()));
    menu->addAction(QIcon(":/icons/Graphics/qticon.png"), tr("About &Qt"), qApp, SLOT(aboutQt()));
    // =============================

    /*
     * Toolbar
     * -------
     */
    QToolBar *topBar = new QToolBar(tr("Tools"));
    addToolBar(Qt::TopToolBarArea, topBar);
    topBar->setMovable(false);
    topBar->addAction(QIcon(":/icons/Graphics/select.png"),  tr("Select"),   this, SLOT(setSelection()));
    topBar->addAction(QIcon(":/icons/Graphics/unselect.png"),tr("Unselect"),   this, SLOT(clearSelection()));
    topBar->addSeparator();
    topBar->addSeparator();
    topBar->addAction(QIcon(":/icons/Graphics/config.png"),  tr("Configure"), this, SLOT(config()));
    topBar->addSeparator();
    topBar->addSeparator();
    topBar->addAction(QIcon(":/icons/Graphics/up.png"),      tr("Move item up"),   this, SLOT(moveUp()));
    topBar->addAction(QIcon(":/icons/Graphics/down.png"),    tr("Move item down"), this, SLOT(moveDown()));
    topBar->addAction(QIcon(":/icons/Graphics/save.png"),    tr("Save order"),     this, SLOT(saveOrder()));
    topBar->addAction(QIcon(":/icons/Graphics/enum.png"),    tr("Enumerate"),      this, SLOT(enumerate()));
    topBar->addAction(QIcon(":/icons/Graphics/unenum.png"),  tr("Un-enumerate"),   this, SLOT(unEnumerate()));
    topBar->addSeparator();
    topBar->addSeparator();
    topBar->addAction(QIcon(":/icons/Graphics/sort.png"),    tr("Sort"), this, SLOT(sort()));
    topBar->addAction(QIcon(":/icons/Graphics/bsort.png"),   tr("Sort by XML"), this, SLOT(bsort()));
    topBar->addAction(QIcon(":/icons/Graphics/asort.png"),   tr("Sort all collections"), this, SLOT(asort()));
    topBar->addSeparator();
    topBar->addSeparator();
    topBar->addAction(lookDevice);
    topBar->addAction(scanAct);
    topBar->addAction(umountAct);

    // Pass labels to Info
    QMap<QString,  QList<QLabel*> > m;
    QList<QLabel*>                  l;

    l.clear();
    l.append(_ui.path_l);
    l.append(_ui.path);
    m.insert("path", l);
    l.clear();
    l.append(_ui.name_l);
    l.append(_ui.name);
    m.insert("name", l);
    l.clear();
    l.append(_ui.author_l);
    l.append(_ui.author);
    m.insert("author", l);
    l.clear();
    l.append(_ui.title_l);
    l.append(_ui.title);
    m.insert("title", l);
    l.clear();
    l.append(_ui.cover);
    m.insert("cover", l);
    _ui.info->setLabels(m);

    // Button list
    _fl = new QList<QPushButton*>;
    FCON(F2);
    FCON(F3);
    FCON(F4);
    FCON(F5);
    FCON(F6);
    FCON(F7);
    FCON(F8);
    FCON(F9);
    FCON(F10);

    // Pass control widgets to panels
    _ui.rFPanel->setCWidgets(_ui.lFPanel, _ui.rFselect, _ui.local_label,
                             _ui.local_free, lookDevice,
                             _ui.rb_root, _ui.rb_up, _ui.coll_revertr,
                             _ui.rcname_l, _ui.rcname,
                             umountAct, scanAct, _fl,
                             Qt::Key_Left, Qt::Key_Right);
    _ui.lFPanel->setCWidgets(_ui.rFPanel, _ui.lFselect, _ui.ebook_label,
                             _ui.ebook_free, lookDevice,
                             _ui.lb_root, _ui.lb_up, _ui.coll_revertl,
                             _ui.lcname_l, _ui.lcname,
                             umountAct, scanAct, _fl,
                             Qt::Key_Right, Qt::Key_Left);

    _ui.rFPanel->addPossibleMounts();

    _ui.info->disable();

    connect(_ui.lFPanel, SIGNAL(infoReq(FPanel *, const QString&, const QString&)),
            _ui.info,   SLOT(infoReq(FPanel *, const QString&, const QString&)));
    connect(_ui.rFPanel, SIGNAL(infoReq(FPanel *, const QString&, const QString&)),
            _ui.info,   SLOT(infoReq(FPanel *, const QString&, const QString&)));
} // mngr505::mngr505()


////////////////////////////////////////////////////////////////////////
mngr505::~mngr505()
{
    delete _fl;
} // mngr505::~mngr505


////////////////////////////////////////////////////////////////////////
void mngr505::about()
{
    QMessageBox::about(0, tr("Manager650"),
    tr("<h3><center>About program</center></h3>"
       "<p>Manager650 is a simple file manager for Sony eBook Reader PRS650. "
       "It provides basic two-panel file manager functionality as well "
       "as some Sony eBook Reader specific functions, like collection "
       "management</p>"
       "<p>Manager650 is aware about .lrf, .epub, .fb2 and .fb2.zip formats "
       "and shows author/title information for these types of files</p>"
       "<center>Version: %1</center>").arg(_version));
} // mngr505::about


////////////////////////////////////////////////////////////////////////
void mngr505::reread_media()
{
    // Reread SD or eBook
    _ui.lFPanel->findDevice();
} // mngr505::reread_media


////////////////////////////////////////////////////////////////////////
void mngr505::config()
{
    Config *c = new Config(this);

    if (c->exec1() == QDialog::Accepted)
    {
        c->save();
        reread_media();
    }
    else
    {
        c->reread();
        c->restore_dependecies();
    }
} // mngr505::config


////////////////////////////////////////////////////////////////////////
void mngr505::config_revert()
{
    Config::revert();
    Config::init();
    Config::save();
    reread_media();
} // mngr505::config_revert


////////////////////////////////////////////////////////////////////////
void mngr505::config_import()
{
    QString fileName = QFileDialog::getOpenFileName(0, tr("Select configuration file for import"),
                                                    ".",
                                                    tr("Configuration file (*.ini)"));
    if (fileName.isNull())
        return;

    {
        QSettings inp(fileName, QSettings::IniFormat);
        QSettings out(_company, _appName);
        foreach (QString k, inp.allKeys())
            out.setValue(k, inp.value(k));
    }
    Config::init();
    reread_media();
} // mngr505::config_import


////////////////////////////////////////////////////////////////////////
void mngr505::config_export()
{
    QString fileName = QFileDialog::getSaveFileName(0, tr("Select configuration file for export"),
                                                    ".",
                                                    tr("Configuration file (*.ini)"));
    if (fileName.isNull())
        return;

    QSettings inp(_company, _appName);
    QSettings out(fileName, QSettings::IniFormat);
    foreach (QString k, inp.allKeys())
        out.setValue(k, inp.value(k));
} // mngr505::config_export


////////////////////////////////////////////////////////////////////////
void mngr505::setSelection()
{
    if (_ui.rFPanel->infocus())
        _ui.rFPanel->setSelection();
    else if(_ui.lFPanel->infocus())
        _ui.lFPanel->setSelection();
} // mngr505::setSelection


////////////////////////////////////////////////////////////////////////
void mngr505::clearSelection()
{
    if (_ui.rFPanel->infocus())
        _ui.rFPanel->clearSelection();
    else if(_ui.lFPanel->infocus())
        _ui.lFPanel->clearSelection();
} // mngr505::clearSelection


////////////////////////////////////////////////////////////////////////
void mngr505::closeEvent(QCloseEvent *event)
{
    QSettings st(_company, _appName);

    // Save geomtery
    st.beginGroup("MainWindow");
    st.setValue("size", size());
    st.setValue("pos", pos());

    // Save splitter sizes
    QVariantList sl;
    foreach (int i, _ui.pSplitter->sizes())
        sl.append(i);
    st.setValue("splitters", sl);

    // Save current directories
    st.setValue("LocalDir", _ui.rFPanel->pwd());
    st.setValue("eBookDir", _ui.lFPanel->pwd());

    st.endGroup();
    event->accept();
} // mngr505::closeEvent


////////////////////////////////////////////////////////////////////////
void mngr505::show()
{

    QSettings    st(_company, _appName);
    QVariant     v;

    // Restore geometry
    st.beginGroup("MainWindow");
    v = st.value("size");
    if (v.isValid())
        resize(v.toSize());
    v = st.value("pos");
    if (v.isValid())
        move(v.toPoint());

    // Restore splitter sizes
    v = st.value("splitters");
    if (v.isValid())
    {
        QList<int>  il;
		QStringList sl = v.toStringList();
        for( int i = 0; i < sl.size(); ++i )
		   il.append( sl.at(i).toInt() );

        _ui.pSplitter->setSizes(il);
    }

    QMainWindow::show();

    // Restore current dirs
    bool ok;

    _ui.rFPanel->cd(st.value("LocalDir", QString()).toString());
    _ui.rFPanel->recolor(false);
    if (!_ui.lFPanel->findDevice(&ok))
    {
        QMessageBox::information(0, tr("Not found"), tr("Neither eBook nor SD-card found"));
        _ui.lFPanel->cd(st.value("eBookDir", QString("/")).toString());
        _ui.lFPanel->recolor(false);
    }
    else if (!ok)
    {
        _ui.lFPanel->cd(st.value("eBookDir", QString("/")).toString());
        _ui.lFPanel->recolor(false);
    }

    st.endGroup();
} // mngr505::show


////////////////////////////////////////////////////////////////////////
void mngr505::findDevice()
{
    if (_ui.lFPanel->order_changed())
    {
        if (QMessageBox::question(0, tr("New device confirmation"),
                                  tr("Some changes are exist. Do you\n"
                                     "want to discard them and continue\n"
                                     "to work with another device ?"),
                                  QMessageBox::Yes | QMessageBox::No,
                                  QMessageBox::No)
            != QMessageBox::Yes)
        return;
        _ui.lFPanel->setOrder_changed(false);
    }
    _ui.lFPanel->findDevice();
} // mngr505::findDevice


////////////////////////////////////////////////////////////////////////
void mngr505::confExit()
{
    if (_ui.lFPanel->order_changed() || _ui.rFPanel->order_changed())
    {
        if (QMessageBox::question(0, tr("Exit confirmation"),
                                  tr("Some changes are exist. Do you\n"
                                     "want to discard them and exit ?"),
                                  QMessageBox::Yes | QMessageBox::No,
                                  QMessageBox::No)
            == QMessageBox::Yes)
            qApp->closeAllWindows();
        else
            return;
    }

    if (!Config::confExit())
        qApp->closeAllWindows();
    else if (QMessageBox::question(0, tr("Exit confirmation"),
                                   tr("Do you really want to quit program?"),
                                   QMessageBox::Yes | QMessageBox::No,
                                   QMessageBox::Yes)
             == QMessageBox::Yes)
        qApp->closeAllWindows();
} // mngr505::confExit

FDEF(moveUp)
FDEF(moveDown)
FDEF(saveOrder)
FDEF(enumerate)
FDEF(unEnumerate)
FDEF(sort)
FDEF(bsort)
FDEF(asort)
FDEF(F2)
FDEF(F3)
FDEF(F4)
FDEF(F5)
FDEF(F6)
FDEF(F7)
FDEF(F8)
FDEF(F9)
FDEF(F10)
