/*
 * $Id$
 *
 * FPanel item implementation
 */

#include "FPanelItem.h"

////////////////////////////////////////////////////////////////////////
FPanelItem::FPanelItem(const QIcon &icon, const QString &text, QListWidget *par,
                       const QFileInfo finfo)
    : QListWidgetItem(icon, text, par), QFileInfo(finfo)
{
} // FPanelItem::FPanelItem


////////////////////////////////////////////////////////////////////////
bool FPanelItem::operator<(const FPanelItem& other) const
{
    return text() < other.text();
} // FPanelItem::operator<
