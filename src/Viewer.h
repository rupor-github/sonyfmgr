/*
 * $Id$
 *
 * Viewer definitions
 */

#ifndef VIEWER_H
#define VIEWER_H

#include <QObject>
#include <QProcess>
#include <QString>

class QProcess;
class Viewer : public QObject
{
    Q_OBJECT

public:
    Viewer(QString& proc, QString& param);
    ~Viewer();

private slots:
    void on_read();
    void on_end(int exitCode, QProcess::ExitStatus exitStatus);

private:
    QProcess *_proc;
    QString  _out;
};

#endif
