/*
 * $Id$
 *
 * DeviceSelect definition
 */

#ifndef DEVICESELECT_H
#define DEVICESELECT_H

#include <QDialog>
#include <QString>

class QAbstractButton;
class QIcon;
class QDialogButtonBox;
class QGridLayout;
class DeviceSelect: public QDialog {
    Q_OBJECT

public:
    DeviceSelect(QWidget *par=0);
    ~DeviceSelect() {}

    void addDevice(const QIcon &icon, const QString &text);
    QString selected() { return _selected; }

private slots:
    void clicked(QAbstractButton *b);

private:
    QString          _selected;
    QGridLayout      *_gLayout;
    QDialogButtonBox *_bBox;
    QDialogButtonBox *_cBox;
};

#endif
