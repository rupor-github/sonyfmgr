/*
 * $Id$
 *
 * FPanel item definitions
 */

#ifndef FPANELITEM_H
#define FPANELITEM_H

#include <QFileInfo>
#include <QIcon>
#include <QListWidgetItem>
#include <QString>

class FPanelItem: public QListWidgetItem, public QFileInfo {

public:
    FPanelItem(const QIcon &icon, const QString &text, QListWidget *par,
               const QFileInfo finfo);
    ~FPanelItem() { }

    bool operator<(const FPanelItem& other) const;
};

#endif
