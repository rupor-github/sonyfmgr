/*
 * $Id$
 *
 * EPUBed main window
 */
#include <QBuffer>
#include <QCloseEvent>
#include <QFile>
#include <QFileInfo>
#include <QFileDialog>
#include <QMenu>
#include <QMessageBox>
#include <QSettings>
#include <QStringList>
#include <QTextStream>
#include <QTextCodec>

#include "Config.h"
#include "EPUBed.h"
#include "unzip.h"
#include "zip.h"

const char *EPUBed::_version = "0.01";
const char *EPUBed::_company = "HaraSoft";
const char *EPUBed::_appName = "EPUBed";
const char *EPUBed::mtype    = "mimetype";

const QColor EPUBed::bg_color(255, 255, 255);
const QColor EPUBed::changed_color(128, 0, 0);
const QColor EPUBed::orig_color(0, 0, 0);

#define tr QObject::tr
void ERR(const QString& s) { QMessageBox::critical(0, tr("ERROR"), s); }

////////////////////////////////////////////////////////////////////////
EPUBed::EPUBed(): QMainWindow(0), _currData(_comps.end())
{
    Config::init();

    setWindowTitle("EPUBed");
    //qApp->setWindowIcon(QIcon(":/icons/Graphics/EPUBed.png"));

    _ui.setupUi(this);

    _ui.comp_list->setSelectionMode(QAbstractItemView::NoSelection);
    _ui.comp_revert->setEnabled(false);

    // File menu
    QMenu    *menu;

    menu = menuBar()->addMenu(tr("&File"));
    menu->addAction(tr("&Open"), this, SLOT(open()));

    _saveAct = new QAction(tr("&Save"), this);
    _saveAct->setEnabled(false);
    connect(_saveAct, SIGNAL(triggered()), this, SLOT(save()));
    menu->addAction(_saveAct);

    menu->addSeparator();
    menu->addAction(tr("&Quit"), this, SLOT(quit()));

    menu = menuBar()->addMenu(tr("&Configuration"));
    menu->addAction(tr("&Configure..."), this, SLOT(config()));
    menu->addAction(tr("&Revert to default"), this, SLOT(config_revert()));
    menu->addSeparator();
    menu->addAction(tr("&Import configuration"), this, SLOT(config_import()));
    menu->addAction(tr("&Export configuration"), this, SLOT(config_export()));

    menu = menuBar()->addMenu(tr("&Actions"));
    menu->addAction(tr("&Sort"), this, SLOT(sort()));
    menu->addAction(tr("&Restore original order"), this, SLOT(restore()));

    menu = menuBar()->addMenu(tr("&Help"));
    menu->addAction(tr("&About"), this, SLOT(about()));
    menu->addAction(tr("About &Qt"), qApp, SLOT(aboutQt()));

    connect(_ui.comp_list,
            SIGNAL(currentItemChanged(QListWidgetItem *, QListWidgetItem *)),
            this,
            SLOT(newItemSelected(QListWidgetItem *, QListWidgetItem *)));
    connect(_ui.comp_editor, SIGNAL(textChanged()), this, SLOT(editorChanged()));
    connect(_ui.comp_revert, SIGNAL(clicked()),     this, SLOT(revert()));
} // EPUBed::EPUBed()


////////////////////////////////////////////////////////////////////////
EPUBed::~EPUBed()
{
} // EPUBed::~EPUBed


////////////////////////////////////////////////////////////////////////
void EPUBed::about()
{
    QMessageBox::about(0, tr("EPUBed"),
    tr("<h3><center>EPUB editor</center></h3>"
       "<p>EPUBed is a simple application for EPUB edit/view</p>"
       "<center>Version: %1</center>").arg(_version));
} // EPUBed::about


////////////////////////////////////////////////////////////////////////
void EPUBed::config()
{
    Config *c = new Config(this);

    if (c->exec() == QDialog::Accepted)
        c->save();
} // EPUBed::config


////////////////////////////////////////////////////////////////////////
void EPUBed::config_revert()
{
    Config::revert();
    Config::init();
    Config::save();
} // EPUBed::config_revert


////////////////////////////////////////////////////////////////////////
void EPUBed::config_import()
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
} // EPUBed::config_import


////////////////////////////////////////////////////////////////////////
void EPUBed::config_export()
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
} // EPUBed::config_export

////////////////////////////////////////////////////////////////////////
void EPUBed::closeEvent(QCloseEvent *event)
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

    st.setValue("work_dir", _dir);

    st.endGroup();
    event->accept();
} // EPUBed::closeEvent


////////////////////////////////////////////////////////////////////////
void EPUBed::show()
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
        QVariantList vl = v.toList();
        QList<int>   sl;
        foreach (QVariant var, vl)
            sl += var.toInt();

        _ui.pSplitter->setSizes(sl);
    }

    _dir = st.value("work_dir").toString();
    QMainWindow::show();

    st.endGroup();
} // EPUBed::show

////////////////////////////////////////////////////////////////////////
void EPUBed::open()
{
    // Open file name
    _fname = QFileDialog::getOpenFileName(this, tr("Open File"),
                                          _dir,
                                          tr("EPUB (*.epub);; All files (*)"));
    if (_fname.isNull())
        return;
    _dir = QFileInfo(_fname).absolutePath();

    _ui.comp_list->blockSignals(true);
    _ui.comp_list->clear();
    _ui.comp_list->blockSignals(false);
    _ui.comp_editor->clear();
    _ui.comp_revert->setEnabled(false);
    _comps.clear();
    _orig_order.clear();

    // Input file
    UnZip uz;
    QFile *i_file = new QFile(_fname);
    if (!i_file->open(QIODevice::ReadOnly))
        return ERR(tr("Can't open <b><font color=#0000ff>%1"
                      "</font></b>: <b><font color=#ff0000>"
                      "%2</font></b>").arg(_fname).arg(i_file->errorString()));

    UnZip::ErrorCode ec = uz.openArchive(i_file);
    if (ec != UnZip::Ok)
        return ERR(tr("Can't open <b><font color=#0000ff>%1"
                      "</font></b>: <b><font color=#ff0000>"
                      "%2</font></b>").arg(_fname).arg(uz.formatError(ec)));

    // Extract everything
    QStringList      flist = uz.fileList();
    if (flist.size() < 2)
        return ERR(tr("<b><font color=#0000ff>%1</font></b> -- Wrong archive "
                      "structure: should be at least two components.")
                   .arg(_fname));
    if (flist[0] != QString(mtype))
        return ERR(tr("<b><font color=#0000ff>%1</font></b> -- Wrong archive "
                      "structure: <b>mimetype</b> should be the first component.")
                   .arg(_fname));

    foreach (QString f, flist)
    {
        QBuffer     outbuff;
        if (!outbuff.open(QIODevice::ReadWrite))
            return ERR(tr("Can't open internal output buffer"));

        ec = uz.extractFile(f, &outbuff, UnZip::SkipPaths);
        if (ec != UnZip::Ok)
            return ERR(tr("Can't extract <b>%1</b> from <b><font color=#0000ff>%2"
                          "</font><b>: <b><font color=#ff0000>"
                          "%2</font></b>").arg(f).arg(_fname).arg(uz.formatError(ec)));

        _comps.insert(f, Data(outbuff.data()));
        _orig_order.append(f);
    }
    uz.closeArchive();

    // Insert to comp list
    foreach(QString c, _orig_order)
    {
        QListWidgetItem *i = new QListWidgetItem(c, _ui.comp_list);
        i->setData(Qt::TextColorRole, orig_color);
    }
} // EPUBed::open


////////////////////////////////////////////////////////////////////////
void EPUBed::save()
{
    /*
     * Create new EPUB file
     * --------------------
     */
    QString tmpName = _fname + ".tmp";
    Zip     zip;

    Zip::ErrorCode ec = zip.createArchive(tmpName);
    if (ec != Zip::Ok)
        return ERR(tr("Can't open <b><font color=#0000ff>%1"
                      "</font></b>: <b><font color=#ff0000>%2</font></b>")
                   .arg(tmpName).arg(zip.formatError(ec)));

    QListWidgetItem *curr = _ui.comp_list->currentItem();
    if (curr)
    {
        QMap<QString,Data>::iterator it = _comps.find(curr->text());
        if (it != _comps.end()  &&  it.value().modified)
            it.value().new_txt = QTextCodec::codecForName("UTF-8")->fromUnicode(
                _ui.comp_editor->toPlainText());
    }

    for (int i=0; i<_ui.comp_list->count(); i++)
    {
        QListWidgetItem *item = _ui.comp_list->item(i);
        QMap<QString,Data>::iterator it = _comps.find(item->text());
        if (it != _comps.end())
        {
            QBuffer buff(it.value().modified ? &it.value().new_txt :
                         &it.value().org_txt);

            ec = zip.addFile(buff, it.key(),  i== 0 ? Zip::Store : Zip::AutoFull);
            if (ec != Zip::Ok)
                return ERR(tr("Can't add %1: <b><font color=#ff0000>%2</font></b>")
                           .arg(it.key()).arg(zip.formatError(ec)));
            if (it.value().modified)
            {
                it.value().modified = false;
                it.value().org_txt = it.value().new_txt;
            }
        }
        if (item == _ui.comp_list->currentItem())
        {
            item->setData(Qt::TextColorRole, bg_color);
            item->setData(Qt::BackgroundColorRole, orig_color);
            _ui.comp_revert->setEnabled(false);
        }
        else
        {
            item->setData(Qt::TextColorRole, orig_color);
            item->setData(Qt::BackgroundColorRole, bg_color);
        }
    }

    ec = zip.closeArchive();
    if (ec != Zip::Ok)
        return ERR(tr("Can't close archive %1: <b><font color=#ff0000>%2</font></b>")
                   .arg(tmpName).arg(zip.formatError(ec)));

    /*
     * Backup orginal (or delete if backup not selected)
     * ------------------------------------------------
     */
    if (Config::backupCrt())
    {
        QString backName = _fname + Config::backupExt();
        if (QFile::exists(backName)  &&  !QFile::remove(backName))
            return ERR(tr("Can't remove <b><font color=#0000ff>%1</font></b>")
                       .arg(backName));
        if (!QFile::rename(_fname, backName))
            return ERR(tr("Can't move <b><font color=#0000ff>%1</font></b>"
                           "to <b><font color=#0000ff>%2</font></b>")
                       .arg(_fname).arg(backName));
    }
    else
    {
        if (!QFile::remove(_fname))
            return ERR(tr("Can't remove <b><font color=#0000ff>%1</font></b>")
                       .arg(_fname));
    }

    /*
     * Move result file to correct place
     * ---------------------------------
     */
    if (!QFile::rename(tmpName, _fname))
        return ERR(tr("Can't move <b><font color=#0000ff>%1</font></b>"
                      "to <b><font color=#0000ff>%2</font></b>")
                   .arg(tmpName).arg(_fname));
} // EPUBed::save


////////////////////////////////////////////////////////////////////////
void EPUBed::sort()
{
    _ui.comp_list->sortItems();
    QList<QListWidgetItem *> items = _ui.comp_list->findItems(mtype, Qt::MatchExactly);
    Q_ASSERT(items.size() == 1);
    int row = _ui.comp_list->row(items[0]);
    QListWidgetItem *i = _ui.comp_list->takeItem(row);
    _ui.comp_list->insertItem(0, i);
} // EPUBed::sort


////////////////////////////////////////////////////////////////////////
void EPUBed::restore()
{
    _ui.comp_list->blockSignals(true);
    _ui.comp_list->clear();
    foreach(QString c, _orig_order)
    {
        new QListWidgetItem(c, _ui.comp_list);
    }
    _ui.comp_list->blockSignals(false);
} // EPUBed::restore


////////////////////////////////////////////////////////////////////////
void EPUBed::newItemSelected(QListWidgetItem *newItem, QListWidgetItem *oldItem)
{
    //qDebug("new: \"%s\"; old: \"%s\"",
    //       newItem ? qPrintable(newItem->text()) : "NULL",
    //       oldItem ? qPrintable(oldItem->text()) : "NULL");
    if (newItem == oldItem)
        return;

    if (oldItem  &&  _currData != _comps.end())
    {
        _currData.value().new_txt = QTextCodec::codecForName("UTF-8")->fromUnicode(
            _ui.comp_editor->toPlainText());
        if (_currData.value().modified)
        {
            oldItem->setData(Qt::TextColorRole, changed_color);
            oldItem->setData(Qt::BackgroundColorRole, bg_color);
        }
        else
        {
            oldItem->setData(Qt::TextColorRole, orig_color);
            oldItem->setData(Qt::BackgroundColorRole, bg_color);
        }
    }

    if (newItem)
    {
        QMap<QString,Data>::iterator it = _comps.find(newItem->text());
        if (it != _comps.end())
        {
            _ui.comp_editor->blockSignals(true);
            _ui.comp_editor->clear();
            _ui.comp_editor->setPlainText(QTextCodec::codecForName("UTF-8")->toUnicode(
                                              it.value().modified ?
                                              it.value().new_txt :
                                              it.value().org_txt));
            _ui.comp_editor->blockSignals(false);
            _ui.comp_revert->setEnabled(it.value().modified);
            _currData = it;

            if (_currData.value().modified)
            {
                newItem->setData(Qt::TextColorRole, bg_color);
                newItem->setData(Qt::BackgroundColorRole, changed_color);
            }
            else
            {
                newItem->setData(Qt::TextColorRole, bg_color);
                newItem->setData(Qt::BackgroundColorRole, orig_color);
            }
        }
    }
} // EPUBed::newItemSelected


////////////////////////////////////////////////////////////////////////
void EPUBed::revert()
{
    if (_currData == _comps.end())
        return;
    _ui.comp_editor->blockSignals(true);
    _ui.comp_editor->setPlainText(QTextCodec::codecForName("UTF-8")->toUnicode(
                                      _currData.value().org_txt));
    _ui.comp_revert->setEnabled(false);
    _currData.value().modified = false;
    _ui.comp_editor->blockSignals(false);
    checkSaveAct();

    if (_ui.comp_list->currentItem())
    {
       _ui.comp_list->currentItem()->setData(Qt::TextColorRole, bg_color);
       _ui.comp_list->currentItem()->setData(Qt::BackgroundColorRole, orig_color);
    }
} // EPUBed::revert


////////////////////////////////////////////////////////////////////////
void EPUBed::editorChanged()
{
    if (_currData == _comps.end())
        return;
    _currData.value().modified = true;
    _saveAct->setEnabled(true);
    _ui.comp_revert->setEnabled(true);

    if (_ui.comp_list->currentItem())
    {
        _ui.comp_list->currentItem()->setData(Qt::TextColorRole, bg_color);
        _ui.comp_list->currentItem()->setData(Qt::BackgroundColorRole, changed_color);
    }
} // EPUBed::editorChanged


////////////////////////////////////////////////////////////////////////
void EPUBed::checkSaveAct()
{
    _saveAct->setEnabled(false);
    for (QMap<QString,Data>::const_iterator it = _comps.constBegin();
         it != _comps.constEnd(); ++it)
    {
        if (it.value().modified)
        {
            _saveAct->setEnabled(true);
            break;
        }
    }
} // EPUBed::checkSaveAct


////////////////////////////////////////////////////////////////////////
void EPUBed::quit()
{
    checkSaveAct();

    if (!_saveAct->isEnabled())
        qApp->closeAllWindows();
    else if (QMessageBox::question(0, tr("Exit confirmation"),
                                   tr("Some unsaved components exist. Do you really want to quit ?"),
                                   QMessageBox::Yes | QMessageBox::No,
                                   QMessageBox::Yes)
             == QMessageBox::Yes)
        qApp->closeAllWindows();
} // EPUBed::quit
