/*
 * $Id$
 *
 * mngr505 unmount: Linux specific code
 */
#include <errno.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#if defined(LINUX)
#include <mntent.h>
#include <linux/loop.h>
#endif
#include <QList>
#include <QVector>
#include <QMessageBox>

#include "mngr505.h"

#define numb_el(x)  (sizeof(x)/sizeof(x[0]))

static QString errMsg;

#if defined(LINUX)
////////////////////////////////////////////////////////////////////////
class Mntent {
public:
    Mntent(mntent *e) : fsname((const char *)e->mnt_fsname),
                        dir((const char *)e->mnt_dir),
                        type((const char *)e->mnt_type),
                        opts((const char *)e->mnt_opts),
                        freq(e->mnt_freq),
                        passno(e->mnt_passno)
        { }
    QString fsname;
    QString dir;
    QString type;
    QString opts;
    int     freq;
    int     passno;
};

////////////////////////////////////////////////////////////////////////
bool del_loop(const char *device)
{
    int fd, rc;

    fd = open(device, O_RDONLY);
    if (fd < 0)
        return false;
    rc = ioctl(fd, LOOP_CLR_FD, 0);
    close(fd);

    return rc ? false : true;
} // del_loop

////////////////////////////////////////////////////////////////////////
static bool umount1(const char *trg)
{
    char                *mtab_name=0;
    QList<Mntent*>      mount_list;
    QString             devname;

    // Save mtab
    static const char   *mtab[] = { "/etc/mtab", "/proc/mounts" };
    FILE                *mount_table=0;
    struct mntent       *mount_entry=0;

    for (unsigned i=0; i<numb_el(mtab); i++)
        if ( (mount_table = setmntent(mtab[i], "r")) != 0)
        {
            mtab_name = (char *)mtab[i];
            break;
        }
    if (!mount_table)
    {
        errMsg = "No mount table found.";
        return false;
    }

    while ((mount_entry = getmntent(mount_table)) != 0)
        mount_list.append(new Mntent(mount_entry));
    endmntent(mount_table);


    // Remove target system from a list
    for (int i=0; i<mount_list.size(); i++)
        if (mount_list[i]->dir == QString(trg)) {
            Mntent *e = mount_list[i];
            devname = e->fsname;
            mount_list.removeAt(i);
            delete e;
            break;
        }

    // Delete loop device (if necessary) and write back mount table
    if (!devname.isEmpty())
    {
        del_loop(trg);

        if ( (mount_table = setmntent(mtab_name, "w")) == 0)
        {
            while (!mount_list.isEmpty())
                delete mount_list.takeFirst();
            errMsg.sprintf("open %s: %s", mtab_name, strerror(errno));
            return false;
        }
        foreach(Mntent *entry, mount_list) {
            mntent  mount_entry;

            mount_entry.mnt_fsname = strdup(qPrintable(entry->fsname));
            mount_entry.mnt_dir    = strdup(qPrintable(entry->dir));
            mount_entry.mnt_type   = strdup(qPrintable(entry->type));
            mount_entry.mnt_opts   = strdup(qPrintable(entry->opts));
            mount_entry.mnt_freq   = entry->freq;
            mount_entry.mnt_passno = entry->passno;

            addmntent(mount_table, &mount_entry);

            free(mount_entry.mnt_fsname);
            free(mount_entry.mnt_dir);
            free(mount_entry.mnt_type);
            free(mount_entry.mnt_opts);
        }
        endmntent(mount_table);
    }

    // Free mount lists
    while (!mount_list.isEmpty())
        delete mount_list.takeFirst();

    if (umount(trg) < 0)
    {
        errMsg.sprintf("umount: %s", strerror(errno));
        return false;
    }

    return true;
} // umount1

#elif defined(MACOSX)

////////////////////////////////////////////////////////////////////////
static bool umount1(const char *trg)
{
    QString cmd;

    cmd.sprintf("/usr/bin/hdiutil detach %s",trg);
    int ret = system(cmd.toAscii());

    if (ret>>8 != 0)
    {
        errMsg.sprintf("detach: %s", strerror(errno));
        return false;
    }

    return true;
} // umount1
#endif


////////////////////////////////////////////////////////////////////////
void mngr505::umount()
{
    _ui.lFPanel->cd("/", true);
    if (umount1(qPrintable(_ui.lFPanel->root())))
    {
        QMessageBox::information(0, tr("umount complete"),
                                 tr("eBook on %1 successfully unmounted.")
                                 .arg(_ui.lFPanel->root()));
        _ui.lFPanel->notFound("/");
    }
    else
        QMessageBox::information(0, tr("umount failure"),
                                 tr("Can't umnount eBook on %1:\n%2")
                                 .arg(_ui.lFPanel->root()).arg(errMsg));
} // mngr505::umount
