/*
 * $Id$
 *
 * DeviceSelect implementation
 */
#include <QGridLayout>
#include <QIcon>
#include <QLabel>
#include <QPushButton>
#include <QDialogButtonBox>

#include "DeviceSelect.h"

////////////////////////////////////////////////////////////////////////
DeviceSelect::DeviceSelect(QWidget *par) : QDialog(par)
{
    setWindowTitle("Select device");
    _gLayout = new QGridLayout(this);
    QLabel *l = new QLabel("There are a few devices which are<br>"
                           "suitable for collection management.<br>"
                           "<b><center>Please select one of them.</center></b><br>", this);
    _gLayout->addWidget(l, 0, 0);
    _cBox = new QDialogButtonBox(this);
    _cBox->setOrientation(Qt::Vertical);
    connect(_cBox, SIGNAL(clicked(QAbstractButton *)), this, SLOT(clicked(QAbstractButton *)));
    _gLayout->addWidget(_cBox, 1, 0, Qt::AlignHCenter);
    _bBox = new QDialogButtonBox(this);
    _bBox->setOrientation(Qt::Horizontal);
    _bBox->setStandardButtons(QDialogButtonBox::Cancel);
    connect(_bBox, SIGNAL(rejected()), this, SLOT(reject()));
    _gLayout->addWidget(_bBox, 2, 0);
} // DeviceSelect::DeviceSelect

////////////////////////////////////////////////////////////////////////
void DeviceSelect::addDevice(const QIcon &icon, const QString &text)
{
    _cBox->addButton(new QPushButton(icon, text, this), QDialogButtonBox::ActionRole);
} // DeviceSelect::addDevice

////////////////////////////////////////////////////////////////////////
void DeviceSelect::clicked(QAbstractButton *b)
{
    _selected = b->text();
    accept();
} // DeviceSelect::clicked
