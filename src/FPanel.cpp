/*
 * $Id$
 *
 * FPanel implementation
 */

#if defined(LINUX) || defined (MACOSX)
#include <execinfo.h>
#include <stdio.h>
#define FNAME_CASE Qt::CaseSensitive
#endif

#if defined(LINUX)
#include <mntent.h>
#include <sys/statfs.h>
#elif defined(MACOSX)
#include <sys/mount.h>
#include <sys/param.h>
#endif

#if defined(WINDOWS)
#include <windows.h>
#include <winbase.h>
#define FNAME_CASE Qt::CaseInsensitive
#endif

#include <QBrush>
#include <QComboBox>
#include <QDateTime>
#include <QDir>
#include <QFileIconProvider>
#include <QFileInfoList>
#include <QFocusEvent>
#include <QInputDialog>
#include <QKeyEvent>
#include <QMessageBox>
#include <QMouseEvent>
#include <QProcess>
#include <QPushButton>
#include <QRegExp>
#include <QScrollBar>
#include <QTextCodec>
#include <QTextEdit>
#include <QTextStream>
#include <QToolTip>

#include "Config.h"
#include "Confirm.h"
#include "DeviceSelect.h"
#include "FB2toEPUB.h"
#include "FB2toLRF.h"
#include "FPanel.h"
#include "FPanelItem.h"
#include "Info.h"
#include "LogWidget.h"
#include "Media.h"
#include "NameEdit.h"
#include "Viewer.h"
#include "mngr505.h"
#include "utils.h"

#define numb_el(x)  (sizeof(x)/sizeof(x[0]))

// QFileIconProvider works too slow on Windows!!!!
#if defined(WINDOWS)
#define NO_FANCY_ICONS
#endif

#ifdef NO_FANCY_ICONS
#define MY_ICON finfo.isDir() ? dir_icon : fil_icon
#else
#define MY_ICON ficons.icon(finfo)
#endif

#define TO_BOTTOM(w) do {                   \
    qApp->processEvents();                  \
    QScrollBar *s = w->verticalScrollBar(); \
    if (s)                                  \
        s->setValue(s->maximum());          \
} while(0)

#define H(s) QString(s).replace(" ", "&nbsp;")

static const char   *media_files[] = { "database/cache/media.xml" };
static const char   *cache_files[] = { "Sony Reader/database/cache.xml" };

const char    *FPanel::order_fname = "index.order";
const char    *FPanel::cname_fname = "index.name";
const QString FPanel::_pref("&nbsp;&nbsp;&nbsp;&nbsp;<b><font color=#0000ff>");
const QString FPanel::_dpref("&nbsp;&nbsp;&nbsp;&nbsp;<b><font color=#a00000>");
const QString FPanel::_suff("</font></b><br>");

////////////////////////////////////////////////////////////////////////
FPanel::FPanel(QWidget *parent) : QListWidget(parent),
                                  _media(0),              _otherFPanel(0),
                                  _infocus(false),        _dir(0),
                                  _rfg(Config::reg_fg()), _rbg(Config::reg_bg()),
                                  _sfg(Config::sel_fg()), _sbg(Config::sel_bg()),
                                  _order_changed(false),  _renItem(0),
                                  _mode(FileSystem),      _Flist(0)
{
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setSelectionMode(QAbstractItemView::NoSelection);
    setSortingEnabled(false);

    connect(this, SIGNAL(itemActivated(QListWidgetItem *)),
            this, SLOT(action(QListWidgetItem *)));
    connect(this, SIGNAL(currentItemChanged(QListWidgetItem *, QListWidgetItem *)),
            this, SLOT(newCurent(QListWidgetItem *, QListWidgetItem *)));
    connect(this, SIGNAL(entered(const QModelIndex&)),
            this, SLOT(entered(const QModelIndex&)));

    _dir = new QDir();
} // FPanel::FPanel


////////////////////////////////////////////////////////////////////////
FPanel::~FPanel()
{
    delete _dir;
    delete _media;
} // FPanel::~FPanel


////////////////////////////////////////////////////////////////////////
void FPanel::setCWidgets(FPanel *other, QComboBox *fsel, QLabel *l,
                         QLabel *f, QAction *findDevice,
                         QPushButton *b_root, QPushButton *b_up,
                         QPushButton *b_revert,
                         QLabel *cname_l, NameEdit *cname,
                         QAction *umount, QAction *scan,
                         QList<QPushButton*> *fl,
                         int toOther, int fromOther)
{
    _lab          = l;
    _freel        = f;
    _fsel         = fsel;
    _b_root       = b_root;
    _b_up         = b_up;
    _b_revert     = b_revert;
    _otherFPanel  = other;
    _umountAct    = umount;
    _scanAct      = scan;
    _findDevice   = findDevice;
    _cname_l      = cname_l;
    _cname        = cname;
    _toOther      = toOther;
    _fromOther    = fromOther;
    _Flist        = fl;

    _lab->setAutoFillBackground(true);
    orderSet(false);

    _fsel->setInsertPolicy(QComboBox::NoInsert);
    _b_revert->setIcon(QIcon(":/icons/Graphics/revert.png"));

    setMode(FileSystem);

    connect(_b_revert, SIGNAL(pressed()), this, SLOT(collRevert()));
    connect(_b_root,   SIGNAL(pressed()), this, SLOT(cdRoot()));
    connect(_b_up,     SIGNAL(pressed()), this, SLOT(cdUp()));
    connect(_fsel,     SIGNAL(activated(const QString &)),
            this,      SLOT(cd(const QString &)));
    connect(_fsel,     SIGNAL(editTextChanged(const QString &)),
            this,      SLOT(cd(const QString &)));
    connect(_cname,    SIGNAL(textChanged(const QString&)),
            this,      SLOT(cname_changed()));
    connect(_cname,    SIGNAL(saveName()),
            this,      SLOT(saveOrder()));

} // FPanel::setCWidgets



////////////////////////////////////////////////////////////////////////
void FPanel::addPossibleMounts()
{
    QStringList poss_mounts = possibleMounts();

    _fsel->blockSignals(true);
    foreach (QString m, poss_mounts)
        _fsel->addItem(m);
    _fsel->blockSignals(false);
} //FPanel::addPossibleMounts


////////////////////////////////////////////////////////////////////////
QStringList FPanel::possibleMounts()
{
    QStringList         rc;

#if defined (MACOSX)

    struct statfs* sfs;

    int count = getmntinfo (&sfs, 0);
    if (count == 0)
        return rc;

    for (int i=0; i<count;i++)
    {
        QString fsname(sfs[i].f_mntfromname);

        if (fsname.startsWith("/dev/disk"))
            rc += sfs[i].f_mntonname;
    }

#elif defined(LINUX)

    static const char   *mtab[] = { "/etc/mtab", "/proc/mounts" };
    FILE                *mount_table=0;
    struct mntent       *mount_entry=0;

    for (unsigned i=0; i<numb_el(mtab); i++)
        if ( (mount_table = setmntent(mtab[i], "r")) != 0)
            break;
    if (!mount_table)
        return rc;

    while ((mount_entry = getmntent(mount_table)) != 0)
    {
        QString fsname(mount_entry->mnt_fsname);

        if (fsname.startsWith("/dev/sd") || fsname.startsWith("/dev/mmc"))
            rc += mount_entry->mnt_dir;
    }

    endmntent(mount_table);

#elif defined(WINDOWS)

    char         letter = 'A';
    DWORD        drives = GetLogicalDrives();

    for (; drives; drives >>= 1)
    {
        if (drives & 0x01)
            rc += QString("%1:").arg(letter);
        letter++;
    }
#endif

    return rc;
} // FPanel::addMounts


////////////////////////////////////////////////////////////////////////
bool FPanel::setFreeSpace()
{
    unsigned long long total=0, free=0;
    QString  device;
    QString  root = _dirname;

#if defined(MACOSX)
    struct statfs      *sfs;

    int count = getmntinfo (&sfs, 0);

    if (count == 0)
        return false;

    unsigned matched=0;
    unsigned index=0;
    for (int i=0; i<count;i++)
    {
        if (root.startsWith(sfs[i].f_mntonname))
        {
            if (strlen(sfs[i].f_mntonname) > matched)
            {
                matched = strlen(sfs[i].f_mntonname);
                device =  sfs[i].f_mntfromname;
                index = i;
            }
        }
    }

    total = sfs[index].f_blocks * (unsigned long long)sfs[index].f_bsize;
    free  = sfs[index].f_bavail * (unsigned long long)sfs[index].f_bsize;

#elif defined(LINUX)
    static const char   *mtab[] = { "/etc/mtab", "/proc/mounts" };
    FILE                *mount_table=0;
    struct mntent       *mount_entry=0;
    struct statfs       st;

    for (unsigned i=0; i<numb_el(mtab); i++)
        if ( (mount_table = setmntent(mtab[i], "r")) != 0)
            break;
    if (!mount_table)
        return false;

    unsigned matched=0;
    while ((mount_entry = getmntent(mount_table)) != 0)
    {
        if (root.startsWith(mount_entry->mnt_dir))
        {
            if (strlen(mount_entry->mnt_dir) > matched)
            {
                matched = strlen(mount_entry->mnt_dir);
                device =  mount_entry->mnt_fsname;
            }
        }
    }

    endmntent(mount_table);

    if (statfs(qPrintable(root), &st) < 0)
        return false;
    total = st.f_blocks * (unsigned long long)st.f_bsize;
    free  = st.f_bavail * (unsigned long long)st.f_bsize;

#elif defined(WINDOWS)
    __int64 lpFreeBytesAvailable, lpTotalNumberOfBytes;
    WCHAR    *myRoot = qStringToWideChar(root);
    bool rc = GetDiskFreeSpaceEx(myRoot,
                                 (PULARGE_INTEGER)&lpFreeBytesAvailable,
                                 (PULARGE_INTEGER)&lpTotalNumberOfBytes,
                                 0);
    delete [] myRoot;
    if (rc)
    {
        device = root.left(2);
        total = lpTotalNumberOfBytes;
        free  = lpFreeBytesAvailable;
    }

#else
    return false;
#endif

    _freel->setText(QString("%1 %2/%3").arg(device)
                    .arg(humanReadableNumber(free))
                    .arg(humanReadableNumber(total)));
    return true;
} // FPanel::getFreeSpace


////////////////////////////////////////////////////////////////////////
bool FPanel::findDevice(bool *ok)
{
    QList<Device> devs;
    Device        *dev=0;

    if (ok)
        *ok = false;

    // Create possible device list
    QStringList poss_mounts = possibleMounts();
    foreach (QString m, poss_mounts)
    {
        unsigned i;

        // Look for media.xml
        for (i=0; i<numb_el(media_files); i++)
        {
            QString name(m + "/" + media_files[i]);
            if (QFile::exists(name))
            {
                devs += Device(m, name, media_files[i], PRS505);
                break;
            }
        }
        if (i < numb_el(media_files))
            continue;

        // Look for cache.xml
        for (i=0; i<numb_el(cache_files); i++)
        {
            QString name(m + "/" + cache_files[i]);
            if (QFile::exists(name))
            {
                devs += Device(m, name, cache_files[i], SD);
                break;
            }
        }
    }

    if (devs.size() < 1)
        return false;
    if (devs.size() == 1)
        dev = &devs[0];
    else
    {
        DeviceSelect ds(this);
        foreach (Device d, devs)
            ds.addDevice(d.mode==PRS505
                         ? QIcon(":/icons/Graphics/mngr505.png")
                         : QIcon(":/icons/Graphics/SD.png"),
                         d.name);
        if (ds.exec() != QDialog::Accepted)
            return true;
        for (int i=0; i<devs.size(); i++)
            if (ds.selected() == devs[i].name)
                dev = &devs[i];
        if (!dev)
            return true;
    }

    // Look for start directory
    QString t(tr("%1 found on <b><font color=#0000ff>%2</font></b>, "
                 "media file is <b><font color=#0000ff>%3</font></b><br>"
                 "Start in <b><font color=#0000ff>%4</b><br>%5"));

    // Try root directory
    QString modeName(dev->mode==PRS505 ? "PRS505" : "SD card");
    QString rootDirName(dev->mode==PRS505 ? Config::rootPRS() :  Config::rootSD());
    QString dname(dev->name + "/" + rootDirName);
    while (dname.endsWith("/"))
        dname = dname.left(dname.length()-1);
    _root = dev->name;
    _mediafname = dev->fname;
    if (!cd(dname))
    {
        t = tr("%1 found on <b><font color=#0000ff>%2</font></b>, "
               "media file is <b><font color=#0000ff>%3</font></b> "
               "but directory <b><font color=#ff0000>%4</b> not found "
               "inside the book<br><br>"
               "Please create the directory or go to <b>Options/Collections</b> "
               "configuration screen and define another root directory "
               "for PRS505.")
            .arg(H(modeName)).arg(H(dev->name)).arg(H(dev->mname)).arg(H(rootDirName));
        QMessageBox::information(0, "Directory not found", t);
        _umountAct->setEnabled(false);
        _scanAct->setEnabled(false);
        return false;
    }

    // Read media file
    bool mediaOK = false;
    QString errText;
    delete _media;
    _media = 0;
    _media = new Media(this);
    if (dev->mode == PRS505)
    {
        if (_media->readPRS(_root, dname, _mediafname, errText))
        {
            t = t.arg(H(modeName)).arg(H(dev->name)).arg(H(dev->mname)).arg(H(dname))
                .arg(tr("<b><font color=#008000>"
                        "Media file has been read and parsed OK</font></b>"));
            mediaOK = true;
        }
        else
            t = t.arg(H(modeName)).arg(H(dev->name)).arg(H(dev->mname)).arg(H(dname))
                .arg(tr("<font color=#ff0000><b>"
                        "Media file error:</b><br>%1").arg(errText));
        _umountAct->setText(tr("U&mount Sony eBook PRS505"));
        _umountAct->setIcon(QIcon(":/icons/Graphics/mngr505.png"));
    }
    else
    {
        if (_media->readSD(_root, dname, _mediafname, errText))
        {
            t = t.arg(H(modeName)).arg(H(dev->name)).arg(H(dev->mname)).arg(H(dname))
                .arg(tr("<b><font color=#008000>"
                        "Cache file has been read and parsed OK</font></b>"));
            mediaOK = true;
        }
        else
            t = t.arg(H(modeName)).arg(H(dev->name)).arg(H(dev->mname)).arg(H(dname))
                .arg(tr("<font color=#ff0000><b>"
                        "Cache file error:</b><br>%1").arg(errText));
        _umountAct->setText("U&mount SD card");
        _umountAct->setIcon(QIcon(":/icons/Graphics/SD.png"));
    }
    if (mediaOK)
    {
        _umountAct->setEnabled(true);
        _scanAct->setEnabled(true);
        if (ok)
            *ok = true;
        QMessageBox::information(0, "Found", t);
    }
    else
    {
        _umountAct->setEnabled(false);
        _scanAct->setEnabled(false);
    }

    return true;
} // FPanel::findDevice


////////////////////////////////////////////////////////////////////////
void FPanel::notFound(const QString& newRoot)
{
    delete _media;
    _media = 0;
    _mediafname.clear();
    _root = newRoot;
    setMode(FileSystem);
    _umountAct->setText("U&mount");
    _umountAct->setEnabled(false);
    _scanAct->setEnabled(false);
} // FPanel::notFound


////////////////////////////////////////////////////////////////////////
bool FPanel::cd(const QString& dir, bool force)
{
    QDir    tmpdir(*_dir);
    //qDebug("Object %s: CD to \"%s\" (root:\"%s\")", qPrintable(objectName()),
    //       qPrintable(dir), qPrintable(_root));

    if (_order_changed)
    {
        if (QMessageBox::question(0, tr("cd confirmation"),
                                  tr("Trying to cd from <br><b><font color=#0000ff>%1</font></b><br>"
                                     "to <br><b><font color=#0000ff>%2</font></b><br><br>"
                                     "Some changes in the old directory are exist.<br>"
                                     "Do you want to discard them and continue ?")
                                  .arg(_dir->canonicalPath()).arg(dir),
                                  QMessageBox::Yes | QMessageBox::No,
                                  QMessageBox::No)
            != QMessageBox::Yes)
            return false;
        else
            _order_changed = false;
    }

    if (!tmpdir.cd(dir))
        return false;

    // Detect new directory name
    QString dname = tmpdir.canonicalPath();
    if (dname.isEmpty())
        dname = tmpdir.absolutePath();

    if (_dirname == dname)
        return true;   // Goto same directory

    if (!force  &&  !dname.startsWith(_root, FNAME_CASE))
        return false; // Goto above root

    // Real CD
    if (!_dir->cd(dir))
        return false;
    setFocus(Qt::TabFocusReason);

    _dirname = dname;
    QFileInfo dirinfo(_dirname);

    // Clear old contents
    _marked.clear();

    rereadDir();

    // Read special file in order to set collection name
    QString cname = dirinfo.fileName();
    QFile cname_file(_dirname + "/" + cname_fname);
    if (cname_file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream in(&cname_file);
        in.setCodec(QTextCodec::codecForName("UTF-8"));
        if (!in.atEnd())
            cname = in.readLine().trimmed();
    }
    _cname->setText(cname);

    // Disable users name if directory isn't writable
    if (dirinfo.isWritable())
    {
        _cname_l->setEnabled(true);
        _cname->setEnabled(true);
    }
    else
    {
        _cname_l->setEnabled(false);
        _cname->setEnabled(false);
    }

    orderSet(false);
    return true;
} // FPanel::cd


////////////////////////////////////////////////////////////////////////
void FPanel::cdUp()
{
    QString oldDir(QFileInfo(_dirname).fileName());

    cd("..");

    for (int i=0; i<count(); i++)
    {
        FPanelItem *c  = dynamic_cast<FPanelItem*>(item(i));
        if (c  && c->text() == oldDir)
        {
            setCurrentItem(c);
            recolorItem(c);
            break;
        }
    }
} // FPanel::cdUp

void FPanel::collRevert()
{
} // FPanel::collRevert


////////////////////////////////////////////////////////////////////////
void FPanel::rereadDir()
{
    // Fill new contents
    QDir::Filters     f = QDir::AllEntries | QDir::Hidden | QDir::System;
    QDir::SortFlags   s = QDir::DirsFirst;
    QFileInfoList     files = _dir->entryInfoList(f, s);
    QFileInfoList     files1;
    QFileIconProvider ficons;
#ifdef NO_FANCY_ICONS
    QIcon             dir_icon(ficons.icon(QFileIconProvider::Folder));
    QIcon             fil_icon(ficons.icon(QFileIconProvider::File));
#endif

    clear();

    // Try read "index.order" file and insert files accordingly
    files1.clear();
    QFile order_file(_dirname + "/" + order_fname);
    if (order_file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream in(&order_file);
        in.setCodec(QTextCodec::codecForName("UTF-8"));
        while (!in.atEnd())
        {
            QString line = in.readLine().trimmed();
            if (line == QString(order_fname))
                continue;
            if (line == QString(cname_fname))
                continue;
            int ind = 0;
            for (; ind < files.size(); ind++)
                if (files[ind].fileName() == line)
                    break;
            if (ind < files.size())
            {
                // Found
                files1.append(files[ind]);
                files.removeAt(ind);
                //QFileInfo finfo = files[ind];
                //files.removeAt(ind);
                //new FPanelItem(MY_ICON, finfo.fileName(), this, finfo);
            }
        }
    }

    // Insert all rest of directories, except of special
    foreach (QFileInfo finfo, files)
    {
        if (!finfo.isDir())
            continue;
        QString fn = finfo.fileName();
        if (_dir->isRoot())
        {
            if (fn == QString(".") || fn == QString(".."))
                continue;
        }
        else
        {
            if (fn == QString("."))
                continue;
        }
        new FPanelItem(MY_ICON, fn, this, finfo);
    }

    // Insert ordered files
    foreach (QFileInfo finfo, files1)
        new FPanelItem(MY_ICON, finfo.fileName(), this, finfo);

    // Insert all rest of files, except of "index.order/index.name"
    QStringList sl = Config::hideFiles().split(',', QString::SkipEmptyParts);
    foreach (QFileInfo finfo, files)
    {
        if (finfo.isDir())
            continue;
        QString fn = finfo.fileName();
        if (fn == QString(order_fname))
            continue;
        if (fn == QString(cname_fname))
            continue;
        bool isSpecial = false;
        foreach(QString s, sl)
        {
            QRegExp r(s, FNAME_CASE, QRegExp::Wildcard);
            if (r.isValid()  &&  r.exactMatch(fn))
            {
                isSpecial = true;
                break;
            }
        }
        if (isSpecial)
            continue;

        new FPanelItem(MY_ICON, fn, this, finfo);
    }

    // Set first item as current
    QListWidgetItem *i = item(0);
    if (i)
        setCurrentItem(i);

    // Set enabled/disbled status of buttons
    if (_mode == FileSystem)
    {
        _b_root->setDisabled(_dir->isRoot());
        _b_up->setDisabled(_dir->isRoot());
    }
    else
    {
        _b_root->setDisabled(_dirname == _root);
        _b_up->setDisabled(_dirname == _root);
    }

    // Store this directory in combobox
    if (_fsel->findText(_dirname) == -1)
        _fsel->addItem(_dirname);
    _fsel->setEditText(_dirname);

    // Refresh list of marked items
    QSet<QString> new_marked;
    for (int i=0; i<count(); i++)
    {
        FPanelItem *c  = dynamic_cast<FPanelItem*>(item(i));
        if (_marked.contains(c->text()))
            new_marked.insert(c->text());
    }
    _marked = new_marked;

    recolor(-1);

    setFreeSpace();
} // FPanel::rereadDir


////////////////////////////////////////////////////////////////////////
void FPanel::action(QListWidgetItem *it)
{
    QString oldDir(QFileInfo(_dirname).fileName());
    QString newDir = it->text();

    if (cd(_dir->absoluteFilePath(it->text()))  &&  newDir == QString(".."))
    {
        // Make "old current directory" as current if possible
        for (int i=0; i<count(); i++)
        {
            FPanelItem *c  = dynamic_cast<FPanelItem*>(item(i));
            if (c  && c->text() == oldDir)
            {
                setCurrentItem(c);
                recolorItem(c);
                break;
            }
        }
    }
} // FPanel::action


////////////////////////////////////////////////////////////////////////
void FPanel::leaveEvent(QEvent *event)
{
    QListWidget::leaveEvent(event);
    setToolTip(QString());
    QToolTip::hideText();
} // FPanel::leaveEvent


////////////////////////////////////////////////////////////////////////
void FPanel::focusInEvent(QFocusEvent *ev)
{
    QListWidget::focusInEvent(ev);
    if (_Flist)
        foreach (QPushButton *b, *_Flist)
            b->setEnabled(true);
    recolor(true);
} // FPanel::focusInEvent


////////////////////////////////////////////////////////////////////////
void FPanel::focusOutEvent(QFocusEvent *ev)
{
    QListWidget::focusOutEvent(ev);
    if (_Flist)
        foreach (QPushButton *b, *_Flist)
            b->setEnabled(false);
    recolor(false);
} // FPanel::focusOutEvent


////////////////////////////////////////////////////////////////////////
void FPanel::recolor(bool infocus)
{
    _infocus = infocus;
    QPalette pal = palette();
    if (_infocus)
    {
        pal.setColor(QPalette::Base, *Config::reg_bg());
        _rfg = Config::reg_fg();
        _rbg = Config::reg_bg();
        _sfg = Config::sel_fg();
        _sbg = Config::sel_bg();
    }
    else
    {
        pal.setColor(QPalette::Base, *Config::nf_reg_bg());
        _rfg = Config::nf_reg_fg();
        _rbg = Config::nf_reg_bg();
        _sfg = Config::nf_sel_fg();
        _sbg = Config::nf_sel_bg();
    }

    setPalette(pal);
    recolor(-1);
} // FPanel::recolor


////////////////////////////////////////////////////////////////////////
void FPanel::recolor(const int row)
{
    if (row == -1)
        for (int i=0; i<count(); i++)
            recolorItem(item(i));
    else
    {
        QListWidgetItem *i = item(row);
        if (i)
            recolorItem(i);
    }
} // FPanel::recolor


////////////////////////////////////////////////////////////////////////
void FPanel::recolorItem(QListWidgetItem *i)
{
    QColor const *fg = _marked.contains(i->text()) ? _sfg : _rfg;
    QColor const *bg = _marked.contains(i->text()) ? _sbg : _rbg;

    if (i == currentItem())
    {
        QColor const *tmp = fg;
        fg = bg;
        bg = tmp;
    }

    i->setData(Qt::TextColorRole, *fg);
    i->setData(Qt::BackgroundColorRole, *bg);
} // FPanel::recolorItem


////////////////////////////////////////////////////////////////////////
void FPanel::entered(const QModelIndex& ind)
{
    setToolTip(QString());
    QToolTip::hideText();

    QListWidgetItem *c = item(ind.row());
    if (c)
    {
        BookData bd;
        if (Info::getBookData(this, _dirname, c->text(), &bd))
        {
            if (bd.page != 0  &&  bd.pages != 0)
                setToolTip(QString("%1\n%2\nPage: %3/%4").arg(bd.author).arg(bd.title)
                           .arg(bd.page).arg(bd.pages));
            else
                setToolTip(QString("%1\n%2").arg(bd.author).arg(bd.title));
        }
    }
} // FPanel::entered


////////////////////////////////////////////////////////////////////////
void FPanel::newCurent(QListWidgetItem *curr, QListWidgetItem *prev)
{
    if (curr != prev)
    {
        if (prev)
        {
            if (_marked.contains(prev->text()))
            {
                prev->setData(Qt::TextColorRole, *Config::sel_fg());
                prev->setData(Qt::BackgroundColorRole, *Config::sel_bg());
            }
            else
            {
                prev->setData(Qt::TextColorRole, *Config::reg_fg());
                prev->setData(Qt::BackgroundColorRole, *Config::reg_bg());
            }
        }
        if (curr)
        {
            if (_marked.contains(curr->text()))
            {
                curr->setData(Qt::TextColorRole, *Config::sel_bg());
                curr->setData(Qt::BackgroundColorRole, *Config::sel_fg());
            }
            else
            {
                curr->setData(Qt::TextColorRole, *Config::reg_bg());
                curr->setData(Qt::BackgroundColorRole, *Config::reg_fg());
            }
        }
    }

    if (curr)
        emit infoReq(this, _dirname, curr->text());
} // FPanel::newCurent


////////////////////////////////////////////////////////////////////////
void FPanel::mousePressEvent(QMouseEvent *ev)
{
    if (Config::mouse_sel()  &&  ev->button() == Qt::RightButton)
    {
        QPoint      pos = ev->pos();
        QModelIndex ind = indexAt(pos);
        QListWidgetItem *c = item(ind.row());
        if (c)
        {
            toggleMark(c);
            recolorItem(c);
        }
    }
    else if (Config::cmouse_sel()  &&  (ev->modifiers() & Qt::ControlModifier)
             && ev->button() == Qt::LeftButton)
    {
        QPoint      pos = ev->pos();
        QModelIndex ind = indexAt(pos);

        FPanelItem  *c = dynamic_cast<FPanelItem*>(item(ind.row()));
        if (c)
        {
            toggleMark(c);
            recolorItem(c);
            setCurrentItem(c);
        }
    }
    else if (Config::smouse_sel()  && (ev->modifiers() & Qt::ShiftModifier)
             && ev->button() == Qt::LeftButton)
    {
        QPoint      pos = ev->pos();
        QModelIndex ind = indexAt(pos);

        int         row1 = currentRow() < ind.row() ? currentRow() : ind.row();
        int         row2 = currentRow() > ind.row() ? currentRow() : ind.row();
        for (int i = row1; i <= row2; i++)
        {
            FPanelItem  *c = dynamic_cast<FPanelItem*>(item(i));
            if (c)
            {
                setMark(c);
                recolorItem(c);
                setCurrentItem(c);
            }
        }
    }
    else
        QAbstractItemView::mousePressEvent(ev);
} // FPanel::mousePressEvent


////////////////////////////////////////////////////////////////////////
void FPanel::keyPressEvent(QKeyEvent *ev)
{
    FPanelItem *c  = dynamic_cast<FPanelItem*>(currentItem());

    // Select item
    if (Config::insert_sel()  &&  c  &&  ev->key() == Qt::Key_Insert)
    {
        int        row = currentRow();

        toggleMark(c);
        if (row < count()-1)
            setCurrentRow(row+1);
        else
            recolor(row);
        return;
    }

    // Go one directory up
    else if (c && ev->key() == Qt::Key_Backspace) {
        if (!_dir->isRoot())
            cdUp();
    }

    // Move item Up
    else if (c && ev->key() == Qt::Key_Up && (ev->modifiers() & Qt::ShiftModifier)) {
        moveUp();
        return;
    }

    // Move item Down
    else if (c  &&  ev->key() == Qt::Key_Down && (ev->modifiers() & Qt::ShiftModifier)) {
        moveDown();
        return;
    }

    // Save order
    else if (ev->key() == Qt::Key_S  &&  (ev->modifiers() & Qt::ControlModifier)) {
        saveOrder();
        return;
    }

    // Enumerate
    else if (ev->key() == Qt::Key_E  &&  (ev->modifiers() & Qt::ControlModifier)) {
        enumerate();
        return;
    }

    // Un enumerate
    else if (ev->key() == Qt::Key_W  &&  (ev->modifiers() & Qt::ControlModifier)) {
        unEnumerate();
        return;
    }

    // Select
    else if (ev->key() == Qt::Key_Plus) {
        setSelection();
        return;
    }

    // Unselect
    else if (ev->key() == Qt::Key_Minus) {
        clearSelection();
        return;
    }

    // Refresh
    else if ((ev->key() == Qt::Key_F2)
             || (ev->key() == Qt::Key_R  &&  (ev->modifiers() & Qt::ControlModifier))) {
        F2();
        return;
    }

    else if (ev->key() == Qt::Key_F3) {
        // View
        F3();
        return;
    } else if (ev->key() == Qt::Key_F4) {
        // Rename
        F4();
        return;
    } else if (ev->key() == Qt::Key_F5) {
        // Copy
        F5();
        return;
    } else if (ev->key() == Qt::Key_F6) {
        // Move
        F6();
        return;
    } else if (ev->key() == Qt::Key_F7) {
        // Mkdir
        F7();
        return;
    } else if (ev->key() == Qt::Key_F8  ||  ev->key() == Qt::Key_Delete) {
        // Delete
        F8();
        return;
    } else if (ev->key() == Qt::Key_F9) {
        // Copy with FB2 to EPUB conversion
        F9();
        return;
    } else if (ev->key() == Qt::Key_F10) {
        // Delete
        F10();
        return;
    } else if (c && ev->key() == _toOther  &&  (ev->modifiers() & Qt::ControlModifier)) {
        // Force other panel goto current directory
        if (QFileInfo(_dirname + "/" + c->text()).isDir())
            _otherFPanel->cd(_dirname + "/" + c->text());
        else
            _otherFPanel->cd(_dirname);
        setFocus(Qt::TabFocusReason);
    } else if (ev->key() == _fromOther  &&  (ev->modifiers() & Qt::ControlModifier)) {
        // Goto directory from other panel
        cd(_otherFPanel->pwd());
    }
#if defined (MACOSX)
    else if (ev->key() == Qt::Key_Down & currentRow() < count()-1 )
    {
        setCurrentRow(currentRow() + 1);
        return;
    }
    if (ev->key() == Qt::Key_Up & currentRow() > 0)
    {
        setCurrentRow(currentRow() - 1);
        return;
    }
#endif


    QAbstractItemView::keyPressEvent(ev);
} // FPanel::keyPressEvent


////////////////////////////////////////////////////////////////////////
void FPanel::setSelection()
{
    bool    ok;
    QString s = QInputDialog::getText(this, tr("Select"), tr("Add selection: "),
                                      QLineEdit::Normal, _prevSelection, &ok);

    if (ok)
        changeMark(s, true);
} // FPanel::setSelection


////////////////////////////////////////////////////////////////////////
void FPanel::clearSelection()
{
    bool    ok;
    QString s = QInputDialog::getText(this, tr("UnSelect"), tr("Clear selection: "),
                                      QLineEdit::Normal, _prevSelection, &ok);

    if (ok)
        changeMark(s, false);
    return;
} // FPanel::clearSelection


////////////////////////////////////////////////////////////////////////
void FPanel::changeMark(const QString &s, bool sel)
{
    QRegExp r(s, Qt::CaseSensitive, QRegExp::Wildcard);
    if (!r.isValid())
    {
        QMessageBox::warning(this, tr("Selection error"),
                             tr("Expression: <b>%1</b><br>"
                                "<center><b><font color=#ff0000>%2</font></b></center>")
                             .arg(s).arg(r.errorString()));
        return;
    }

    for (int i=0; i<count(); i++)
    {
        FPanelItem *c  = dynamic_cast<FPanelItem*>(item(i));
        if (r.exactMatch(c->text()))
            sel ? setMark(c) : clearMark(c);
    }
    _prevSelection = s;
} // FPanel::changeMark


////////////////////////////////////////////////////////////////////////
void FPanel::setMark(QListWidgetItem *item)
{
    FPanelItem *c  = dynamic_cast<FPanelItem*>(item);
    if (!c)
        return;

    if (c->text() == QString(".")  ||  c->text() == QString(".."))
        return;
    if (!Config::selectDirs()  &&  c->isDir())
        return;
    if (!_marked.contains(c->text()))
    {
        _marked.insert(c->text());
        recolorItem(c);
    }
} // FPanel::setMark


////////////////////////////////////////////////////////////////////////
void FPanel::clearMark(QListWidgetItem *item)
{
    FPanelItem *c  = dynamic_cast<FPanelItem*>(item);
    if (!c)
        return;

    if (_marked.contains(c->text()))
    {
        _marked.remove(c->text());
        recolorItem(c);
    }
} // FPanel::clearMark


////////////////////////////////////////////////////////////////////////
void FPanel::toggleMark(QListWidgetItem *item)
{
    FPanelItem *c  = dynamic_cast<FPanelItem*>(item);
    if (!c)
        return;

    if (_marked.contains(c->text()))
    {
        _marked.remove(c->text());
        recolorItem(c);
    }
    else
    {
        _marked.insert(c->text());
        recolorItem(c);
    }
} // FPanel::toggleMark


////////////////////////////////////////////////////////////////////////
void FPanel::setMode(const FPanel::Mode m)
{
    switch (m)
    {
    case FileSystem:
        _lab->setText("Local Files");
        break;
    case PRS505:
        _lab->setText("eBook");
        break;
    case SD:
        _lab->setText("SD");
        break;
    }
    _mode = m;
}

////////////////////////////////////////////////////////////////////////
void FPanel::moveUp()
{
    int        row = currentRow();
    if (row <= 0)
        return;
    FPanelItem *c  = dynamic_cast<FPanelItem*>(currentItem());
    if (!c)
        return;

    moveUp(row, c);
} // FPanel::moveUp


////////////////////////////////////////////////////////////////////////
void FPanel::moveUp(int row, FPanelItem *)
{
    FPanelItem *prev  = dynamic_cast<FPanelItem*>(item(row-1));
    if (!prev  ||  prev->text() == QString(".."))
        return;

    QListWidgetItem *i = takeItem(row);
    insertItem(row-1, i);
    setCurrentRow(row-1);
    orderSet(true);
} // FPanel::moveUp


////////////////////////////////////////////////////////////////////////
void FPanel::moveDown()
{
    int        row = currentRow();
    if (row <= 0  &&  row >= count()-1)
        return;
    FPanelItem *c  = dynamic_cast<FPanelItem*>(currentItem());
    if (!c  ||  c->text() == QString(".."))
        return;

    moveDown(row, c);
} // FPanel::moveDown


////////////////////////////////////////////////////////////////////////
void FPanel::moveDown(int row, FPanelItem *)
{
    QListWidgetItem *i = takeItem(row);
    insertItem(row+1, i);
    setCurrentRow(row+1);
    orderSet(true);
    return;
} // FPanel::moveDown


////////////////////////////////////////////////////////////////////////
void FPanel::saveOrder()
{
    // Save order
    QString fname = _dirname + "/" + order_fname;
    QFile   data(fname);
    if (data.open(QFile::WriteOnly | QFile::Truncate))
    {
        QTextStream out(&data);
        out.setCodec(QTextCodec::codecForName("UTF-8"));
        for (int i=0; i<count(); i++)
        {
            FPanelItem *c  = dynamic_cast<FPanelItem*>(item(i));
            if (c->text() != QString(".."))
                out << c->text() << endl;
        }
    }
    else
        QMessageBox::warning(this, tr("File open error"),
                             tr("Can't open file: <b>%1</b><br>"
                                "<center><b><font color=#ff0000>%2</font></b></center>")
                             .arg(fname).arg(data.errorString()));

    // Save name
    fname = _dirname + "/" + cname_fname;
    data.close();
    data.setFileName(fname);
    if (data.open(QFile::WriteOnly | QFile::Truncate))
    {
        QTextStream out(&data);
        out.setCodec(QTextCodec::codecForName("UTF-8"));
        out << _cname->text();
    }
    else
        QMessageBox::warning(this, tr("File open error"),
                             tr("Can't open file: <b>%1</b><br>"
                                "<center><b><font color=#ff0000>%2</font></b></center>")
                             .arg(fname).arg(data.errorString()));

    orderSet(false);
} // FPanel::saveOrder

////////////////////////////////////////////////////////////////////////
void FPanel::enumerate()
{
    char _dformat[16];
    QString d1(QString("%1").arg(count()));
    sprintf(_dformat, "%%0%dd-%%s", d1.length());

    bool ok = true;
    for (int i=0; i<count(); i++)
    {
        FPanelItem *c  = dynamic_cast<FPanelItem*>(item(i));
        if (c  &&  c->text() != QString(".."))
        {
            QString newName = c->text();
            newName.replace(QRegExp("^[0-9]+-"), "");
            newName.sprintf(_dformat, i, qPrintable(newName));
            if (newName != c->text()  &&  !_dir->rename(c->text(), newName))
            {
                QMessageBox::warning(this, tr("Rename error"),
                                     tr("Can't rename: <b>%1</b> to <b>%2</b>")
                                     .arg(c->text()).arg(newName));
                ok = false;
                break;
            }
        }
    }
    rereadDir();
    if (ok)
    {
        _dir->remove(order_fname);
        orderSet(false);
    }
} // FPanel::enumarate

////////////////////////////////////////////////////////////////////////
void FPanel::unEnumerate()
{
    bool ok = true;
    for (int i=0; i<count(); i++)
    {
        FPanelItem *c  = dynamic_cast<FPanelItem*>(item(i));
        if (c  &&  c->text() != QString(".."))
        {
            QString newName = c->text();
            newName.replace(QRegExp("^[0-9]+-"), "");
            if (newName != c->text()  &&  !_dir->rename(c->text(), newName))
            {
                QMessageBox::warning(this, tr("Rename error"),
                                     tr("Can't rename: <b>%1</b> to <b>%2</b>")
                                     .arg(c->text()).arg(newName));
                ok = false;
                break;
            }
        }
    }
    rereadDir();
    if (ok)
    {
        _dir->remove(order_fname);
        orderSet(false);
    }
} // FPanel::unEnumarate

////////////////////////////////////////////////////////////////////////
void FPanel::copyMarked(bool move, bool fb2conv)
{
    if (!_otherFPanel)
        return;
    QList<FPanelItem*> clist;

    if (_marked.size() > 0)
    {
        for (int i=0; i<count(); i++)
        {
            FPanelItem *c  = dynamic_cast<FPanelItem*>(item(i));
            if (_marked.contains(c->text()))
                clist += c;
        }
    }
    else
    {
        FPanelItem *c  = dynamic_cast<FPanelItem*>(currentItem());
        if (c && c->text() != QString(".") && c->text() != QString(".."))
            clist += c;
    }

    copyMarked(clist, move, fb2conv);
} //  FPanel::copyMarked


////////////////////////////////////////////////////////////////////////
void FPanel::copyMarked(const QList<FPanelItem*>& clist, bool move, bool fb2conv)
{
    if (clist.size() < 1)
        return;
    if (!_otherFPanel)
        return;

    reset_confirmations();

    // Confirmation
    Confirm conf(this);
    QString mtxt(move ? tr("Move") : tr("Copy"));
    if (fb2conv)
        mtxt += " and convert";
    QString action(move ? tr("moved") : tr("copied"));
    if (fb2conv)
        action += tr(" and converted");
    conf._ui.t1->setText(tr("%1 %2 files/dirs from:").arg(mtxt).arg(clist.size()));
    conf._ui.t2->setText(tr("&nbsp;&nbsp;&nbsp;&nbsp;<b><font color=#0000ff>%1</font></b><br>"
                            "To:<br>&nbsp;&nbsp;&nbsp;&nbsp;<b><font color=#0000ff>%2</font></b><br>")
                         .arg(_dirname).arg(_otherFPanel->pwd()));
    conf._ui.t3->setText(tr("Follow file(s)/dir(s) will be %1:")
                         .arg(action));
    int i=0;
    foreach(FPanelItem *c, clist)
    {
        conf._ui.flist->insertHtml(
            (QFileInfo(_dirname + "/" + c->text()).isDir() ? _dpref : _pref)
            + c->text() + _suff);
        if (Config::confMax()  &&  i++ >= Config::confMax())
        {
            conf._ui.flist->insertHtml("... more files/dirs skipped<br>");
            break;
        }
    }
    if (conf.exec1() != QDialog::Accepted)
        return;

    // Open log widget
    LogWidget *log = new LogWidget(this);
    log->show();
    action = move ? tr("Moving") : tr("Copying");
    if (fb2conv)
        action += tr(" and converting");
    if (Config::plainText())
        log->_ui.txt->insertPlainText(tr("%1 files to %2\n\n")
                                      .arg(action)
                                      .arg(_otherFPanel->pwd()));
    else
        log->_ui.txt->insertHtml(tr("%1 files to <b><font color=#0000ff>"
                                    "%2</font></b><br><br>")
                                 .arg(action)
                                 .arg(_otherFPanel->pwd()));
    TO_BOTTOM(log->_ui.txt);
    bool ok = true;

    // Separate files/direcotries: copy direcotries, then files
    QList<FPanelItem *> fitems;
    foreach(FPanelItem *c, clist)
    {
        if (c->text() == QString(".")  ||  c->text() == QString(".."))
            continue;

        QFileInfo fi(_dirname + "/" + c->text());
        if (fi.isDir())
        {
            bool rc = copyDir(fi.absoluteFilePath(), _otherFPanel->pwd(), log, move, fb2conv);
            ok = ok && rc;
            if (rc)
            {
                clearMark(c);
                if (move)
                    _dir->rmdir(fi.fileName());
            }
        }
        else
            fitems += c;
        if (log->canceled())
        {
            ok = false;
            break;
        }
    }
    if (!log->canceled()  &&  fitems.size() > 0)
    {
        int maxlen = 0;
        foreach(FPanelItem *c, fitems)
            maxlen = c->text().length() > maxlen ? c->text().length() : maxlen;

        foreach(FPanelItem *c, fitems)
        {
            bool rc = copyFile(_dirname + "/" + c->text(), _otherFPanel->pwd(), log, maxlen, move, fb2conv);
            ok = ok && rc;
            if (rc)
                clearMark(c);
            if (log->canceled())
            {
                ok = false;
                break;
            }
        }
    }

    // Reread destination and optionaly source dirs
    if (move)
        rereadDir();
    _otherFPanel->rereadDir();

    // Enable OK button
    if (ok  &&  Config::logDisapp())
        delete log;
    else
    {
        LOG_CANCEL(log)->setEnabled(false);
        LOG_OK(log)->setEnabled(true);
        LOG_OK(log)->setDefault(true);
    }
} // FPanel::copyMarked


////////////////////////////////////////////////////////////////////////
bool FPanel::copyFile(const QString& fname, const QString& dst, LogWidget *log,
                      int maxlen, bool move, bool fb2conv)
{
    bool      rc = true;
    QFileInfo fi(fname);

    // Overwrite ?
    bool fb2_convert = BookData::isFB2(fname)  &&  fb2conv;
    QString dest_name(fb2_convert ? (myBaseName(fi.fileName()) +
                                     (Config::F9LRF() ? ".lrf" : ".epub"))
                      : fi.fileName());
    QFileInfo dest_info(dst + "/" + dest_name);
    if (dest_info.exists())
    {
        if (dest_info.isDir())
            rc = ask_overwrite_dirf(dst + "/" + dest_name, log);
        else
            rc = ask_overwrite_file(dst + "/" + dest_name);
    }
    if (!rc)
    {
        if (Config::plainText())
            log->_ui.txt->insertPlainText(tr("File: %1 ... ").arg(fname));
        else
            log->_ui.txt->insertHtml(tr("File: <b><font color=#00c0c0>"
                                        "%1</font></b> ... ").arg(fname));
        for (int j=0; j<maxlen - fi.fileName().length(); j++)
            if (Config::plainText())
                log->_ui.txt->insertPlainText(" ");
            else
                log->_ui.txt->insertHtml("&nbsp;");
        if (Config::plainText())
            log->_ui.txt->insertPlainText(tr("SKIP\n"));
        else
            log->_ui.txt->insertHtml(tr("<b><font color=#00ff00>SKIP</font></b><br>"));
        TO_BOTTOM(log->_ui.txt);
        return false;
    }

    // Report start
    if (Config::plainText())
        log->_ui.txt->insertPlainText(tr("File: %1 ...").arg(fname));
    else
        log->_ui.txt->insertHtml(tr("File: <b><font color=#00c0c0>"
                                    "%1</font></b> ... ").arg(fname));
    for (int j=0; j<maxlen - fi.fileName().length(); j++)
        if (Config::plainText())
            log->_ui.txt->insertPlainText(" ");
        else
            log->_ui.txt->insertHtml("&nbsp;");
    TO_BOTTOM(log->_ui.txt);

    if (fb2_convert)
    {
        // Convert FB2 to EPUB
        if (Config::F9LRF())
            rc = FB2toLRF(fi.absoluteFilePath(),
                          dest_info.absoluteFilePath(), log->_ui.txt);
        else
            rc = FB2toEPUB(fi.absoluteFilePath(),
                           dest_info.absoluteFilePath(), log->_ui.txt);
    }
    else
    {
        // Copy
        QFile     oldf(fname);
        if (oldf.copy(dst + "/" + fi.fileName()))
        {
            if (Config::plainText())
                log->_ui.txt->insertPlainText(tr("OK"));
            else
                log->_ui.txt->insertHtml(tr("<b><font color=#00ff00>OK</font></b>"));
            if (move)
            {
                if (!oldf.remove())
                {
                    if (Config::plainText())
                    {
                        log->_ui.txt->insertPlainText(tr(",Not removed\n"));
                        log->_ui.txt->insertPlainText(QString("      %1\n")
                                                      .arg(oldf.errorString()));
                    }
                    else
                    {
                        log->_ui.txt->insertHtml(tr(",<b><font color=#ff0000>Not removed</font></b><br>"));
                        log->_ui.txt->insertHtml(QString("&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"
                                                         "<b><font color=#ff0000>%1</font></b><br>")
                                                 .arg(oldf.errorString()));
                    }
                    rc = false;
                }
            }
            if (Config::plainText())
                log->_ui.txt->insertPlainText(tr("\n"));
            else
                log->_ui.txt->insertHtml(tr("<br>"));
        }
        else
        {
            if (Config::plainText())
            {
                log->_ui.txt->insertPlainText(tr("FAIL\n"));
                log->_ui.txt->insertPlainText(QString("      %1\n")
                                              .arg(oldf.errorString()));
            }
            else
            {
                log->_ui.txt->insertHtml(tr("<b><font color=#ff0000>FAIL</font></b><br>"));
                log->_ui.txt->insertHtml(QString("&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"
                                                 "<b><font color=#ff0000>%1</font></b><br>")
                                         .arg(oldf.errorString()));
            }
            rc = false;
        }
    }

    TO_BOTTOM(log->_ui.txt);

    return rc;
} // FPanel::copyFile


////////////////////////////////////////////////////////////////////////
bool FPanel::copyDir(const QString& fname, const QString& dest,
                     LogWidget* log, bool move, bool fb2conv)
{
    QDir              dst(dest);
    QDir              src(fname);
    QString           srcf(QFileInfo(fname).fileName());
    QDir::Filters     f = QDir::AllEntries | QDir::Hidden | QDir::System;
    QDir::SortFlags   s = QDir::DirsFirst;
    QFileInfoList     info_list = src.entryInfoList(f, s);
    int               maxlen = 0;
    bool              rc = true;

    // Report
    if (Config::plainText())
        log->_ui.txt->insertPlainText(tr(">Dir: %1").arg(fname));
    else
        log->_ui.txt->insertHtml(tr("&gt;Dir: <b><font color=#808000>"
                                    "%1</font></b><br>").arg(fname));
    TO_BOTTOM(log->_ui.txt);

    // Make target dir
    QFileInfo dest_dir(dest + "/" + srcf);
    if (dest_dir.exists())
    {
        if (!dest_dir.isDir())
            rc &= ask_overwrite_file(dest + "/" + srcf);
    }
    else
        rc &= dst.mkdir(srcf);
    if (!rc)
        goto dir_report;

    // Files
    foreach(QFileInfo fi, info_list) {
        if (fi.isDir())
            continue;
        maxlen = fi.fileName().length() > maxlen ? fi.fileName().length() : maxlen;
    }

    foreach(QFileInfo fi, info_list) {
        if (fi.isDir())
            continue;
        rc &= copyFile(fi.absoluteFilePath(), dest + "/" + srcf, log, maxlen, move, fb2conv);
        if (log->canceled())
            return false;
    }
    if (log->canceled())
        return false;


    // Directories
    foreach(QFileInfo fi, info_list)
    {
        if (!fi.isDir())
            continue;
        QString fn = fi.fileName();
        if (fn == QString(".")  ||  fn == QString(".."))
            continue;
        rc &= copyDir(fname + "/" + fn, dest + "/" + srcf, log, move, fb2conv);
        if (move  &&  rc)
            rc &= src.rmdir(fn);
        if (log->canceled())
            return false;
    }

    // Report
 dir_report:
    if (Config::plainText())
        log->_ui.txt->insertPlainText(tr("<Dir: %1 - ").arg(fname));
    else
        log->_ui.txt->insertHtml(tr("&lt;Dir: <b><font color=#808000>"
                                    "%1</font></b> - ").arg(fname));
    if (rc)
        if (Config::plainText())
            log->_ui.txt->insertPlainText(tr("OK\n"));
        else
            log->_ui.txt->insertHtml(tr("<b><font color=#00ff00>OK</font></b><br>"));
    else
        if (Config::plainText())
            log->_ui.txt->insertPlainText(tr("FAIL\n"));
        else
            log->_ui.txt->insertHtml(tr("<b><font color=#ff0000>FAIL</font></b><br>"));
    TO_BOTTOM(log->_ui.txt);

    return rc;
} // FPanel::copyDir

////////////////////////////////////////////////////////////////////////
void FPanel::makedir()
{
    bool    ok;
    QString s = QInputDialog::getText(this, tr("Create directory"), tr("Enter new directory: "),
                                      QLineEdit::Normal, "", &ok);

    if (!ok)
        return;

    if (_dir->mkdir(s))
        rereadDir();
    else
        QMessageBox::warning(this, tr("Directory error"),
                             tr("Can't create directory: <b><font color=#0000ff>%1</font></b>")
                             .arg(s));
} // FPanel::makedir


////////////////////////////////////////////////////////////////////////
void FPanel::deleteMarked()
{
    QList<FPanelItem*> clist;

    if (_marked.size() > 0)
    {
        for (int i=0; i<count(); i++)
        {
            FPanelItem *c  = dynamic_cast<FPanelItem*>(item(i));
            if (_marked.contains(c->text()))
                clist += c;
        }
    }
    else
    {
        FPanelItem *c  = dynamic_cast<FPanelItem*>(currentItem());
        if (c && c->text() != QString(".") && c->text() != QString(".."))
            clist += c;
    }

    deleteMarked(clist);
} // FPanel::deleteMarked


////////////////////////////////////////////////////////////////////////
void FPanel::deleteMarked(const QList<FPanelItem*>& clist)
{
    reset_confirmations();

    // Confirmation
    Confirm conf(this);
    conf._ui.t1->setText(tr("Directory:"));
    conf._ui.t2->setText(tr("&nbsp;&nbsp;&nbsp;&nbsp;<b><font color=#0000ff>%1</font></b>")
                         .arg(_dirname));
    conf._ui.t3->setText(tr("Delete %1 file(s)/dir(s):").arg(clist.size()));
    int i=0;
    foreach(FPanelItem *c, clist)
    {
        conf._ui.flist->insertHtml(
            (QFileInfo(_dirname + "/" + c->text()).isDir() ? _dpref : _pref)
            + c->text() + _suff);
        if (Config::confMax()  &&  i++ >= Config::confMax())
        {
            conf._ui.flist->insertHtml("... more files/dirs skipped<br>");
            break;
        }
    }
    if (conf.exec1() != QDialog::Accepted)
        return;

    // Open log widget
    LogWidget *log = new LogWidget(this);
    log->show();
    if (Config::plainText())
        log->_ui.txt->insertPlainText(tr("Remove files from %1\n").arg(_dirname));
    else
        log->_ui.txt->insertHtml(tr("Remove files from <b><font color=#0000ff>"
                                         "%1</font></b><br><br>")
                                 .arg(_dirname));
    TO_BOTTOM(log->_ui.txt);
    bool ok = true;

    // Separate files/direcotries: delete direcotries, then files
    QList<FPanelItem *> fitems;
    foreach(FPanelItem *c, clist)
    {
        if (c->text() == QString(".")  ||  c->text() == QString(".."))
            continue;

        QFileInfo fi(_dirname + "/" + c->text());
        if (fi.isDir())
        {
            bool rc = deleteDir(fi.absoluteFilePath(), log, true);
            ok = ok && rc;
            if (rc)
            {
                clearMark(c);
                _dir->rmdir(fi.fileName());
            }
        }
        else
            fitems += c;
        if (log->canceled())
        {
            ok = false;
            break;
        }
    }
    if (!log->canceled()  &&  fitems.size() > 0)
    {
        int maxlen = 0;
        foreach(FPanelItem *c, fitems)
            maxlen = c->text().length() > maxlen ? c->text().length() : maxlen;

        foreach(FPanelItem *c, fitems)
        {
            bool rc = deleteFile(_dirname + "/" + c->text(), log, maxlen);
            ok = ok && rc;
            if (rc)
                clearMark(c);
            if (log->canceled())
            {
                ok = false;
                break;
            }
        }
    }

    rereadDir();

    if (ok  &&  Config::logDisapp())
        delete log;
    else
    {
        LOG_CANCEL(log)->setEnabled(false);
        LOG_OK(log)->setEnabled(true);
        LOG_OK(log)->setDefault(true);
    }
} // FPanel::deleteMarked


////////////////////////////////////////////////////////////////////////
bool FPanel::deleteFile(const QString& fname, LogWidget *log, int maxlen)
{
    bool      rc = true;
    QFileInfo fi(fname);

    if (Config::plainText())
        log->_ui.txt->insertPlainText(tr("File: %1 ... ").arg(fname));
    else
        log->_ui.txt->insertHtml(tr("<font color=#ff0000>File:</font> "
                                    "<b><font color=#00c0c0>"
                                    "%1</font></b> ... ").arg(fname));
    for (int j=0; j<maxlen - fi.fileName().length(); j++)
            if (Config::plainText())
                log->_ui.txt->insertPlainText(" ");
            else
                log->_ui.txt->insertHtml("&nbsp;");
    TO_BOTTOM(log->_ui.txt);

    QFile     oldf(fname);
    if (!oldf.remove())
    {
        if (Config::plainText())
        {
            log->_ui.txt->insertPlainText(tr("FAIL\n"));
            log->_ui.txt->insertPlainText(QString("      %1\n")
                                          .arg(oldf.errorString()));
        }
        else
        {
            log->_ui.txt->insertHtml(tr("<b><font color=#ff0000>FAIL</font></b><br>"));
            log->_ui.txt->insertHtml(QString("&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"
                                             "<b><font color=#ff0000>%1</font></b><br>")
                                     .arg(oldf.errorString()));
        }
        rc = false;
    }
    else
        if (Config::plainText())
            log->_ui.txt->insertPlainText(tr("OK\n"));
        else
            log->_ui.txt->insertHtml(tr("<b><font color=#00ff00>OK</font></b><br>"));
    TO_BOTTOM(log->_ui.txt);

    return rc;
} //  FPanel::deleteFile


////////////////////////////////////////////////////////////////////////
bool FPanel::deleteDir(const QString& fname, LogWidget *log, bool first_level)
{
    QDir              src(fname);
    QString           srcf(QFileInfo(fname).fileName());
    QDir::Filters     f = QDir::AllEntries | QDir::Hidden | QDir::System;
    QDir::SortFlags   s = QDir::DirsFirst;
    QFileInfoList     info_list = src.entryInfoList(f, s);
    int               maxlen = 0;
    bool              rc = true;

    // Report
    if (Config::plainText())
        log->_ui.txt->insertPlainText(tr(">Dir: %1\n").arg(fname));
    else
        log->_ui.txt->insertHtml(tr("<font color=#ff0000>&gt;Dir:</font> "
                                    "<b><font color=#808000>"
                                    "%1</font></b><br>").arg(fname));
    TO_BOTTOM(log->_ui.txt);

    if (first_level  &&  filesAmount(fname) > 0)
        rc = ask_delete_dir(fname);
    if (!rc)
    {
        if (Config::plainText())
        {
            log->_ui.txt->insertPlainText(tr("<Dir: %1 - ").arg(fname));
            log->_ui.txt->insertPlainText(tr("SKIP\n"));
        }
        else
        {
            log->_ui.txt->insertHtml(tr("<font color=#ff0000>&lt;Dir:</font> "
                                        "<b><font color=#808000>"
                                        "%1</font></b> - ").arg(fname));
            log->_ui.txt->insertHtml(tr("<b><font color=#00ff00>SKIP</font></b><br>"));
        }
        TO_BOTTOM(log->_ui.txt);
        return false;
    }

    // Files
    foreach(QFileInfo fi, info_list) {
        if (fi.isDir())
            continue;
        maxlen = fi.fileName().length() > maxlen ? fi.fileName().length() : maxlen;
    }

    foreach(QFileInfo fi, info_list) {
        if (fi.isDir())
            continue;
        rc &= deleteFile(fi.absoluteFilePath(), log, maxlen);
        if (log->canceled())
            return false;
    }
    if (log->canceled())
        return false;

    // Directories
    foreach(QFileInfo fi, info_list)
    {
        if (!fi.isDir())
            continue;
        QString fn = fi.fileName();
        if (fn == QString(".")  ||  fn == QString(".."))
            continue;
        rc &= deleteDir(fname + "/" + fn, log);
        if (rc)
            rc &= src.rmdir(fn);
        if (log->canceled())
            return false;
    }

    // Report
    if (Config::plainText())
        log->_ui.txt->insertPlainText(tr("<Dir: %1 - ").arg(fname));
    else
        log->_ui.txt->insertHtml(tr("<font color=#ff0000>&lt;Dir:</font> "
                                    "<b><font color=#808000>"
                                    "%1</font></b> - ").arg(fname));
    if (rc)
        if (Config::plainText())
            log->_ui.txt->insertPlainText(tr("OK\n"));
        else
            log->_ui.txt->insertHtml(tr("<b><font color=#00ff00>OK</font></b><br>"));
    else
        if (Config::plainText())
            log->_ui.txt->insertPlainText(tr("FAIL\n"));
        else
            log->_ui.txt->insertHtml(tr("<b><font color=#ff0000>FAIL</font></b><br>"));
    TO_BOTTOM(log->_ui.txt);
    return true;
} // FPanel::deleteDir


////////////////////////////////////////////////////////////////////////
bool FPanel::updateColl()
{
    if (!_media)
        return false;
    if (!_media->ok())
        return false;

    return _media->updateColl();
} // FPanel::updateColl


////////////////////////////////////////////////////////////////////////
void FPanel::viewFile()
{
    FPanelItem *c  = dynamic_cast<FPanelItem*>(currentItem());
    if (!c)
        return;

    QString  vn;
    QString  fname = _dirname + "/" + c->text();
    if (QFileInfo(fname).isDir())
        return;

    switch (BookData::getType(fname))
    {
    case BookData::LRF:
        if (Config::lrfViewer().isEmpty())
            return;
        vn = Config::lrfViewer();
        break;
    case BookData::FB2:
        if (Config::fb2Viewer().isEmpty())
            return;
        vn = Config::fb2Viewer();
        break;
    default:
        return;
    }

    new Viewer(vn, fname);
} // FPanel::viewFile


////////////////////////////////////////////////////////////////////////
void FPanel::orderSet(const bool changed)
{
    QBrush   br(changed ? *Config::ord_changed() : *Config::ord_unchanged());
    QPalette pal = _lab->palette();

    br.setStyle(Qt::SolidPattern);
    pal.setBrush(QPalette::Inactive, QPalette::Window, br);
    pal.setBrush(QPalette::Active, QPalette::Window, br);
    _lab->setPalette(pal);

    _order_changed = changed;
} // FPanel::orderSet


////////////////////////////////////////////////////////////////////////
void FPanel::reset_confirmations()
{
    _file = Init_stat;
    _dirf = Init_stat;
    _ddir = Init_stat;
} // FPanel::reset_confirmations


////////////////////////////////////////////////////////////////////////
// Delete non empty directory
bool FPanel::ask_delete_dir(const QString& fname)
{
    if (!Config::confDelDir()  ||  _ddir == Yes_to_all)
    {
        QFile(fname).remove();
        return true;
    }
    if (_ddir == No_to_all)
        return false;

    if (Config::confOverwr())
    {
        QMessageBox::StandardButton rc =
            QMessageBox::question(this, tr("Confirmation"),
                                  tr("Directory %1 is not empty. Delete anyway ?").arg(fname),
                                  QMessageBox::Yes | QMessageBox::YesToAll |
                                  QMessageBox::No  | QMessageBox::NoToAll);
        switch(rc)
        {
        case QMessageBox::YesToAll:
            _ddir = Yes_to_all;
        case QMessageBox::Yes:
            return true;
        case QMessageBox::NoToAll:
            _ddir = No_to_all;
        case QMessageBox::No:
            return false;
        default:
            break;
        }

    }
    return false;
} // FPanel::ask_delete_dir


////////////////////////////////////////////////////////////////////////
// Overwrite directory with file
bool FPanel::ask_overwrite_dirf(const QString& fname, LogWidget *log)
{
    bool rc = true;
    QString dirname(QFileInfo(fname).absolutePath());
    QString basename(QFileInfo(fname).fileName());

    if (!Config::confOverwr()  ||  _dirf == Yes_to_all)
    {
        if (Config::plainText())
            log->_ui.txt->insertPlainText("\n");
        else
            log->_ui.txt->insertHtml("<br>");
        rc = deleteDir(fname, log);

        if (rc)
            rc &= QDir(dirname).rmdir(basename);

        if (Config::plainText())
            log->_ui.txt->insertPlainText("\n");
        else
            log->_ui.txt->insertHtml("<br>");

        return rc;
    }
    if (_dirf == No_to_all)
        return false;

    if (Config::confOverwr())
    {
        QMessageBox::StandardButton rc1 =
            QMessageBox::question(this, tr("Confirmation"),
                                  tr("Directory %1 is exists. Overwrite with a file ?").arg(fname),
                                  QMessageBox::Yes | QMessageBox::YesToAll |
                                  QMessageBox::No  | QMessageBox::NoToAll);
        switch(rc1)
        {
        case QMessageBox::YesToAll:
            _dirf = Yes_to_all;
        case QMessageBox::Yes:
            if (Config::plainText())
                log->_ui.txt->insertPlainText("\n");
            else
                log->_ui.txt->insertHtml("<br>");
            rc = deleteDir(fname, log);

            if (rc)
                rc &= QDir(dirname).rmdir(basename);

            if (Config::plainText())
                log->_ui.txt->insertPlainText("\n");
            else
                log->_ui.txt->insertHtml("<br>");
            return rc;
        case QMessageBox::NoToAll:
            _dirf = No_to_all;
        case QMessageBox::No:
            return false;
        default:
            break;
        }

    }
    return false;
} // FPanel::ask_overwrite_dirf

////////////////////////////////////////////////////////////////////////
// Overwrite file with file or directory
bool FPanel::ask_overwrite_file(const QString& fname)
{
    if (!Config::confOverwr()  ||  _file == Yes_to_all)
    {
        QFile(fname).remove();
        return true;
    }
    if (_file == No_to_all)
        return false;

    if (Config::confOverwr())
    {
        QMessageBox::StandardButton rc =
            QMessageBox::question(this, tr("Confirmation"),
                                  tr("File %1 exists. Overwrite ?").arg(fname),
                                  QMessageBox::Yes | QMessageBox::YesToAll |
                                  QMessageBox::No  | QMessageBox::NoToAll);
        switch(rc)
        {
        case QMessageBox::YesToAll:
            _file = Yes_to_all;
        case QMessageBox::Yes:
            QFile(fname).remove();
            return true;
        case QMessageBox::NoToAll:
            _file = No_to_all;
        case QMessageBox::No:
            return false;
        default:
            break;
        }

    }
    return false;
} // FPanel::ask_overwrite_file


////////////////////////////////////////////////////////////////////////
int FPanel::filesAmount(const QString& fname)
{
    QDir::Filters     f = QDir::AllEntries | QDir::Hidden | QDir::System;
    QDir::SortFlags   s = QDir::DirsFirst;
    QFileInfoList     info_list(QDir(fname).entryInfoList(f, s));
    int rc = 0;

    foreach(QFileInfo fi, info_list) {
        QString fn = fi.fileName();
        if (fn != QString(".")  &&  fn != QString(".."))
            rc++;
    }

    return rc;
} // FPanel::filesAmount


////////////////////////////////////////////////////////////////////////
void FPanel::closeEditor(QWidget *editor, QAbstractItemDelegate::EndEditHint hint)
{
    QLineEdit *l = qobject_cast<QLineEdit *>(editor);

    if (l  &&  _renItem)
    {
        QString newName = l->text();
        if (newName != _oldName  &&  !_dir->exists(newName)
            && _dir->rename(_oldName, newName))
        {
            if (_marked.contains(_oldName))
            {
                _marked.remove(_oldName);
                _marked.insert(newName);
            }
            recolorItem(_renItem);
        }
        else
            _renItem->setText(_oldName);
    }

    _renItem = 0;
    QListWidget::closeEditor(editor, hint);
} // FPanel::closeEditor


#define C_NEQ(n, o) _c_neq(Config::sorder##n(), o)
#define C_CMP(n, o) Config::sortAsc() ? \
    _c_lt(Config::sorder##n(), Config::s##n##ReplFrom(), Config::s##n##ReplTo(), o) : \
    _c_gt(Config::sorder##n(), Config::s##n##ReplFrom(), Config::s##n##ReplTo(), o)
////////////////////////////////////////////////////////////////////////
class Entry
{
public:
    Entry(const QFileInfo& f) : fi(f), isBook(false) {}

    QFileInfo fi;
    bool      isBook;
    QString   author;
    QString   title;
    QDateTime date;

    bool      operator<(const Entry& other) const { return _compare(other); }
    bool      _compare(const Entry& other) const;

private:
    bool _c_neq(Config::Sorder S, const Entry& o) const;
    bool _c_lt(Config::Sorder S, const QRegExp& from, const QString& to, const Entry& o) const;
    bool _c_gt(Config::Sorder S, const QRegExp& from, const QString& to, const Entry& o) const;
};
bool Entry::_c_neq(Config::Sorder S, const Entry& o) const
{
    switch (S)
    {
    case Config::S_AUTHOR:
        return author != o.author;
    case Config::S_TITLE:
        return title != o.title;
    case Config::S_DATE:
        return date != o.date;
    default:
        return false;
    }
}
bool Entry::_c_lt(Config::Sorder S, const QRegExp& from, const QString& to,
                  const Entry& o) const
{
    QString c1;
    QString c2;

    switch (S)
    {
    case Config::S_AUTHOR:
        c1 = author;
        c2 = o.author;
        break;
    case Config::S_TITLE:
        c1 = title;
        c2 = o.title;
        break;
    case Config::S_DATE:
        return date < o.date;
    default:
        return false;
    }
    if (!from.isEmpty())
    {
        c1.replace(from, to);
        c2.replace(from, to);
    }
    return c1 < c2;
}
bool Entry::_c_gt(Config::Sorder S, const QRegExp& from, const QString& to,
                  const Entry& o) const
{
    QString c1;
    QString c2;

    switch (S)
    {
    case Config::S_AUTHOR:
        c1 = author;
        c2 = o.author;
        break;
    case Config::S_TITLE:
        c1 = title;
        c2 = o.title;
        break;
    case Config::S_DATE:
        return date > o.date;
    default:
        return false;
    }
    if (!from.isEmpty())
    {
        c1.replace(from, to);
        c2.replace(from, to);
    }
    return c1 > c2;
}
bool Entry::_compare(const Entry& other) const
{
    if (isBook)
    {
        if (other.isBook)
        {
            if (C_NEQ(1, other))
                return C_CMP(1, other);
            else if (C_NEQ(2, other))
                return C_CMP(2, other);
            else
                return C_CMP(3, other);
        }
        else
            return false;
    }
    else
    {
        if (other.isBook)
            return true;
        else
        {
            if (fi.isDir() &&  !other.fi.isDir())
                return true;
            else if (!fi.isDir() &&  other.fi.isDir())
                return false;
            else
                return fi.fileName() < other.fi.fileName();
        }
    }
    return false;
}

////////////////////////////////////////////////////////////////////////
void FPanel::sort()
{
    QDir::Filters     f = QDir::AllEntries | QDir::Hidden | QDir::System;
    QDir::SortFlags   s = QDir::DirsFirst;
    QFileInfoList     files = _dir->entryInfoList(f, s);

    if (QFile::exists(_dirname + "/" + order_fname)  &&
        (QMessageBox::question(this, tr("Confirmation"),
                               tr("Order already set. Overwrite ?"),
                               QMessageBox::Yes | QMessageBox::No)
         != QMessageBox::Yes))
            return;

    // Create entries list
    QList<Entry> elist;
    foreach (QFileInfo f, files)
    {
        BookData bd;
        Entry    e(f);

        if (f.fileName() == QString(".")  ||  f.fileName() == QString(".."))
            continue;
        if (Info::getBookData(this, f.absoluteFilePath(), &bd))
        {
            e.author = bd.author;
            e.title  = bd.title;
            e.isBook = true;
        }
        e.date   = f.lastModified();

        elist.append(e);
    }

    // Sort
    qSort(elist.begin(), elist.end());

    // Write back
    {
        QString fname = _dirname + "/" + order_fname;
        QFile   data(fname);
        if (data.open(QFile::WriteOnly | QFile::Truncate))
        {
            QTextStream out(&data);
            out.setCodec(QTextCodec::codecForName("UTF-8"));
            foreach (Entry e, elist)
                out << e.fi.fileName() << endl;
        }
        else
            QMessageBox::warning(this, tr("File open error"),
                                 tr("Can't open file: <b>%1</b><br>"
                                    "<center><b><font color=#ff0000>%2</font></b></center>")
                                 .arg(fname).arg(data.errorString()));
    }

    // Reread
    rereadDir();

} // FPanel::sort

////////////////////////////////////////////////////////////////////////
QString FPanel::getPath(const QString& fpath)
{
    if (!fpath.startsWith(_root))
        return fpath;

    QString rc = fpath.mid(_root.length());
    while (rc.startsWith("/"))
        rc = rc.mid(1);

    return rc;
} // Media::getPath


////////////////////////////////////////////////////////////////////////
void FPanel::bsort()
{
    QDir::Filters     f = QDir::AllEntries | QDir::Hidden | QDir::System;
    QDir::SortFlags   s = QDir::DirsFirst;
    QFileInfoList     files = _dir->entryInfoList(f, s);

    if (QFile::exists(_dirname + "/" + order_fname)  &&
        (QMessageBox::question(this, tr("Confirmation"),
                               tr("Order already set. Overwrite ?"),
                               QMessageBox::Yes | QMessageBox::No)
         != QMessageBox::Yes))
            return;

    //foreach (BookData b, books_order)
    //    qDebug("Bpath: \"%s\"", qPrintable(b.path));
    //foreach (QFileInfo f, files)
    //{
    //    qDebug("Fpath: \"%s\"", qPrintable(f.absoluteFilePath()));
    //    qDebug("Spath: \"%s\"", qPrintable(getPath(f.absoluteFilePath())));
    //}

    // Reorder
    QFileInfoList new_list;
    foreach (BookData b, books_order)
    {
        int i;
        for (i=0; i < files.size(); i++)
        {
            QFileInfo f = files[i];
            if (b.path == getPath(f.absoluteFilePath()))
            {
                new_list.append(f);
                break;
            }
        }
        if (i < files.size())
            files.removeAt(i);
    }
    new_list += files;

    // Write back
    {
        QString fname = _dirname + "/" + order_fname;
        QFile   data(fname);
        if (data.open(QFile::WriteOnly | QFile::Truncate))
        {
            QTextStream out(&data);
            out.setCodec(QTextCodec::codecForName("UTF-8"));
            foreach (QFileInfo f, new_list)
                out << f.fileName() << endl;
        }
        else
            QMessageBox::warning(this, tr("File open error"),
                                 tr("Can't open file: <b>%1</b><br>"
                                    "<center><b><font color=#ff0000>%2</font></b></center>")
                                 .arg(fname).arg(data.errorString()));
    }

    // Reread
    rereadDir();
} // FPanel::bsort


////////////////////////////////////////////////////////////////////////
void FPanel::asort()
{
    QString sortRoot = _root;
    switch (_mode)
    {
    case PRS505:
        sortRoot += "/" + Config::rootPRS();
        break;
    case SD:
        sortRoot += "/" + Config::rootSD();
        break;
    default:
        QMessageBox::information(this, tr("Disabled"), tr("Sort all disabled in FileSystem mode"));
        return;
    }

    if (_root.isEmpty())
    {
        QMessageBox::information(this, tr("Nothing to sort"), tr("No eBook/SD found - nothing to sort"));
        return;
    }

    if (QMessageBox::question(this, tr("Sort confirmation"),
                              tr("All subdirecotries under the <b><font color=#0000ff>%1</font></b> directory "
                                 "will be sorted now. All previous order will be lost.<br>"
                                 "<center><b><font color=#ff0000>Are you sure ?</font></b></center>")
                              .arg(H(sortRoot)),
                              QMessageBox::Yes | QMessageBox::No)
        != QMessageBox::Yes)
        return;

    // Log widget prepare
    _sortLog = new LogWidget(this);
    _sortLog->show();
    _sortLog->_ui.txt->insertPlainText(tr("Sorting directories:\n"));
    TO_BOTTOM(_sortLog->_ui.txt);

    // sort
    sortDir(sortRoot);

    // Enable OK button
    LOG_CANCEL(_sortLog)->setEnabled(false);
    LOG_OK(_sortLog)->setEnabled(true);
    LOG_OK(_sortLog)->setDefault(true);

} // FPanel::asort

void FPanel::sortDir(const QString& d)
{
    QDir::Filters     f = QDir::NoDotAndDotDot | QDir::AllEntries | QDir::Hidden | QDir::System;
    QDir::SortFlags   s = QDir::DirsFirst;
    QDir              dir(d);

    if (!dir.exists())
        return;

    _sortLog->_ui.txt->insertPlainText(tr("    %1\n").arg(d));
    TO_BOTTOM(_sortLog->_ui.txt);

    QFileInfoList     files = dir.entryInfoList(f, s);

    // Create entries list
    QList<Entry> elist;
    foreach (QFileInfo f, files)
    {
        BookData bd;
        Entry    e(f);

        if (Info::getBookData(this, f.absoluteFilePath(), &bd))
        {
            e.author = bd.author;
            e.title  = bd.title;
            e.isBook = true;
        }
        e.date   = f.lastModified();

        elist.append(e);
    }

    // Sort
    qSort(elist.begin(), elist.end());

    // Write back
    {
        QString fname = d + "/" + order_fname;
        QFile   data(fname);
        if (data.open(QFile::WriteOnly | QFile::Truncate))
        {
            QTextStream out(&data);
            out.setCodec(QTextCodec::codecForName("UTF-8"));
            foreach (Entry e, elist)
                out << e.fi.fileName() << endl;
        }
        else
            QMessageBox::warning(this, tr("File open error"),
                                 tr("Can't open file: <b>%1</b><br>"
                                    "<center><b><font color=#ff0000>%2</font></b></center>")
                                 .arg(fname).arg(data.errorString()));
    }
    if (_sortLog->canceled())
        return;

    // Go down
    foreach (QFileInfo f, files)
    {
        if (f.isDir())
            sortDir(d + "/" + f.fileName());
    }
} // FPanel::sortDir

void FPanel::F2()
{
    rereadDir();
}

void FPanel::F3()
{
    viewFile();
}

void FPanel::F4()
{
    FPanelItem *c  = dynamic_cast<FPanelItem*>(currentItem());

    if (c && c->text() != QString(".") && c->text() != QString(".."))
    {
        c->setFlags(c->flags() | Qt::ItemIsEditable);
        _renItem = c;
        _oldName = c->text();
        editItem(c);
        c->setFlags(c->flags() & ~Qt::ItemIsEditable);
    }
}

void FPanel::F5()
{
    copyMarked();
}

void FPanel::F6()
{
    copyMarked(true);
}

void FPanel::F7()
{
    makedir();
}

void FPanel::F8()
{
    deleteMarked();
}

void FPanel::F9()
{
    copyMarked(false, true);
}

void FPanel::F10()
{
    mngr505::confExit();
}

////////////////////////////////////////////////////////////////////////
// DEBUG
#if defined(LINUX) || defined (MACOSX)
static const unsigned int MAX_BACKTRACE = 64;
void FPanel::printBackTrace()
{
    void *trace[MAX_BACKTRACE];
    int  traceSize = backtrace(trace, MAX_BACKTRACE);
    char **msgs = backtrace_symbols(trace, traceSize);

    qDebug("\nTotal %d stack frames (/usr and /lib are filtered out):", traceSize);
    for (int i = 1; i < traceSize; ++i)
        if (strncmp(msgs[i], "/usr", 4)  &&  strncmp(msgs[i], "/lib", 4))
            qDebug("   %s", msgs[i]);
    qDebug("\n");
    free(msgs);
}
#elif defined(WINDOWS)
// I don't know how to do that under Windows
void FPanel::printBackTrace() {}
#else
// And especially I don't know how to do that under undefined OS
void FPanel::printBackTrace() {}
#endif
