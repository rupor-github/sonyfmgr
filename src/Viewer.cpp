/*
 * $Id$
 *
 * Viewer implementation
 */

#include <QMessageBox>

#include "Viewer.h"

////////////////////////////////////////////////////////////////////////
Viewer::Viewer(QString& proc, QString& param) : _proc(0)
{
    QStringList params;

    _out.clear();
    _proc = new QProcess(0);
    _proc->setProcessChannelMode(QProcess::MergedChannels);

    connect(_proc, SIGNAL(readyReadStandardError()),
            this,  SLOT(on_read()));
    connect(_proc, SIGNAL(readyReadStandardOutput()),
            this,  SLOT(on_read()));
    connect(_proc, SIGNAL(finished(int, QProcess::ExitStatus)),
            this,  SLOT(on_end(int, QProcess::ExitStatus)));

    params += param;
    _proc->start(proc, params);
} // Viewer::Viewer


////////////////////////////////////////////////////////////////////////
Viewer::~Viewer()
{
    delete _proc;
} // Viewer::~Viewer


////////////////////////////////////////////////////////////////////////
void Viewer::on_end(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitCode  ||  exitStatus)
    {
        QString estat = exitStatus ? "CRASHED" : "OK";
        QMessageBox::information(0, tr("Viewer status"),
                                 _out.isEmpty() ?
                                 tr("Exit code: %1, exit status: %2")
                                 .arg(exitCode).arg(estat) :
                                 tr("Exit code: %1, exit status: %2\n\n"
                                            "Process output:\n"
                                            "--------------\n%3\n"
                                            "--------------")
                                 .arg(exitCode).arg(estat).arg(_out));
    }
    deleteLater();
} // Viewer::on_end


////////////////////////////////////////////////////////////////////////
void Viewer::on_read()
{
    _out += _proc->readAll();
} // Viewer::on_end
