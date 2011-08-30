/*
 * $Id$
 *
 * FB2toLRF via external FB2LRF utility: implementation
 */

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QProcess>
#include <QScrollBar>

#include "Config.h"
#include "FB2toLRF.h"

#define tr QObject::tr
#define MSG(P, H) do {           \
    if (Config::plainText())     \
        txt->insertPlainText(P); \
    else                         \
        txt->insertHtml(H);      \
} while (0)
#define ERR(P, H) do {           \
    if (Config::plainText())     \
        txt->insertPlainText(P); \
    else                         \
        txt->insertHtml(H);      \
    return false;                \
} while (0)
#define TO_BOTTOM(w) do {                   \
    qApp->processEvents();                  \
    QScrollBar *s = w->verticalScrollBar(); \
    if (s)                                  \
        s->setValue(s->maximum());          \
} while(0)

////////////////////////////////////////////////////////////////////////
/*
 *static QString replPath(const QString& path)
 *{
 *#if defined(WINDOWS)
 *    return path;
 *#else
 *    QString rc = Config::wineRoot() + path;
 *    rc.replace("/", "\\");
 *    return rc;
 *#endif
 *} // replPath
 *
 */
static QString replPath(const QString& path)
{
#if defined(WINDOWS)
return QDir::toNativeSeparators( path );
#else
return QDir::toNativeSeparators( Config::wineRoot() + path );
#endif
} // replPath

////////////////////////////////////////////////////////////////////////
bool FB2toLRF(const QString& fb2Name, const QString& LRFName,
               QTextEdit *txt)
{
    QString cmd = Config::fb2lrfCmd();
    QString outName(LRFName);

    // Check command
    if (!cmd.contains("%INP") || !cmd.contains("%OUT"))
    {
        MSG(tr("\n    Invalid converter command line: both %INP and %OUT should be present\n"),
            tr("&nbsp;&nbsp;&nbsp;&nbsp;<br><b><font color=#ff0000>Invalid converter command line:</font> "
               "both %INP and %OUT should be present</b><br>"));
        ERR(tr("FAIL\n"),
            tr("<b><font color=#ff0000>FAIL</font></b><br>"));
    }
    if (Config::fb2UseTmp())
        outName = Config::fb2lrfTmp() + "/" + QFileInfo(LRFName).fileName();

    // Create command and parameter list
    QStringList params = cmd.split(" ", QString::SkipEmptyParts);
    for (int i=0; i<params.size(); i++)
    {
        params[i].replace("%PGM",    QDir::toNativeSeparators(Config::fb2LRF()));
        params[i].replace("%STYLES", replPath(Config::fb2Styles()));
        params[i].replace("%TEMP",   replPath(Config::fb2lrfTmp()));
        params[i].replace("%INP",    replPath(QDir::cleanPath(fb2Name)));
        params[i].replace("%OUT",    replPath(QDir::cleanPath(outName)));
    }
    cmd = params[0];
    if (!params.isEmpty())
        params.removeFirst();
    QString cmdLine = cmd;
    for (int i=0; i<params.size(); i++)
        cmdLine += " " + params[i];
    //qDebug("FB2LRF cmd: %s", qPrintable(cmd));
    //for (int i=0; i<params.size(); i++)
    //    qDebug("FB2LRF par #%d: %s", i, qPrintable(params[i]));

    // Call FB2LRF tool
    QProcess proc(0);

    // Environment
    QStringList sys_env = QProcess::systemEnvironment();
    QStringList new_env = Config::fb2lrfEnv().split('\n', QString::SkipEmptyParts);
    QMap<QString,QString> new_map, sys_map;
    for (int i=0; i<sys_env.size(); i++)
    {
        int n = sys_env[i].indexOf('=');
        if (n != -1)
            sys_map.insert(sys_env[i].left(n), sys_env[i].mid(n+1));
    }
    for (int i=0; i<new_env.size(); i++)
    {
        int n = new_env[i].indexOf('=');
        if (n != -1)
            new_map.insert(new_env[i].left(n), new_env[i].mid(n+1));
    }

    for (QMap<QString,QString>::const_iterator nit = new_map.constBegin();
         nit != new_map.constEnd(); ++nit)
    {
        QMap<QString,QString>::iterator sit = sys_map.find(nit.key());
        if (sit == sys_map.end())
            sys_map.insert(nit.key(), nit.value());
        else if (Config::fb2lrfOver())
            sit.value() = nit.value();
    }

    sys_env.clear();
    for (QMap<QString,QString>::const_iterator sit = sys_map.constBegin();
         sit != sys_map.constEnd(); ++sit)
        sys_env += QString("%1=%2").arg(sit.key()).arg(sit.value());
    //for (int i=0; i<sys_env.size(); i++) qDebug("\e[0m%s", qPrintable(sys_env[i])); // +++++DEBUG
    proc.setEnvironment(sys_env);

    proc.setProcessChannelMode(QProcess::MergedChannels);
    proc.start(cmd, params, QIODevice::ReadOnly);
    if (Config::fb2lrfSCmd())
    {
        MSG(tr("\n    %1\n").arg(cmdLine),
            tr("<br>&nbsp;&nbsp;&nbsp;&nbsp;<b><font color=#0000ff>%1</font></b><br>").arg(cmdLine));
        TO_BOTTOM(txt);
    }

    if (!proc.waitForStarted(-1))
    {
        if (Config::plainText())
        {
            txt->insertPlainText(tr("\n    Can't start process:"));
            txt->insertPlainText(tr("\n    Command: %1").arg(cmd));
            for (int i=0; i<params.size(); i++)
                txt->insertPlainText(tr("\n    Param #%1: <b>%2</b>").arg(i).arg(params[i]));
            txt->insertPlainText("\n");
        }
        else
        {
            txt->insertHtml(tr("<br>&nbsp;&nbsp;&nbsp;&nbsp;<b><font color=#ff0000>Can't start process</b></font>"));
            txt->insertHtml(tr("<br>&nbsp;&nbsp;&nbsp;&nbsp;Command: <b>%1</b>").arg(cmd));
            for (int i=0; i<params.size(); i++)
                txt->insertHtml(tr("<br>&nbsp;&nbsp;&nbsp;&nbsp;Param #%1: <b>%2</b>").arg(i).arg(params[i]));
            txt->insertHtml("<br>");
        }
        TO_BOTTOM(txt);
        ERR(tr("FAIL\n"),
            tr("<b><font color=#ff0000>FAIL</font></b><br>"));
    }

    QString allOutput;
    if (Config::fb2lrfSErr())
    {
        // Show output only on error
        while (proc.waitForReadyRead(-1))
            allOutput.append(proc.readAll());
    }
    else
    {
        // Show output always
        while (proc.waitForReadyRead(-1))
        {
            QString dat(proc.readAll());
            QTextCursor tc = txt->textCursor();
            tc.movePosition(QTextCursor::End);
            txt->setTextCursor(tc);
            if (dat.contains('\r'))
            {
                // Possible "text progress bar" reply
                QStringList sl = dat.split('\r');
                for (QStringList::const_iterator i = sl.constBegin();
                     i != sl.constEnd(); ++i)
                {
                    tc = txt->textCursor();
                    tc.movePosition(QTextCursor::StartOfLine);
                    tc.select(QTextCursor::LineUnderCursor);
                    tc.removeSelectedText();
                    txt->setTextCursor(tc);
                    txt->insertPlainText(*i);
                    qApp->processEvents();
                }
            }
            else
            {
                // Regular reply
                txt->insertPlainText(dat);
                TO_BOTTOM(txt);
            }
        }
        txt->insertPlainText("\n");
    }

    proc.waitForFinished(-1);
    if (proc.exitStatus() == QProcess::NormalExit)
    {
        if (proc.exitCode())
        {
            MSG("\n", "<br>");
            txt->insertPlainText(allOutput);
            TO_BOTTOM(txt);
            ERR(tr("FAIL, exit code=%1\n").arg(proc.exitCode()),
                tr("<b><font color=#ff0000>FAIL, exit code=%1</font></b><br>").arg(proc.exitCode()));
        }
        else
            MSG(tr("OK"), tr("<b><font color=#00ff00>OK</font></b>"));
    }
    else
    {
        MSG("\n", "<br>");
        txt->insertPlainText(allOutput);
        TO_BOTTOM(txt);
        ERR(tr("FAIL - Crashed\n"),
            tr("<b><font color=#ff0000>FAIL - Crashed</font></b><br>"));
    }

    if ((proc.exitStatus() != QProcess::NormalExit)  ||  (proc.exitCode() != 0))
        return false;

    if (Config::fb2UseTmp())
    {
        QFile     oldf(outName);
        MSG(tr(", move %1 to %2\n").arg(outName).arg(LRFName),
            tr(", move <b><font color=#0000ff>%1</font></b> to "
               "<b><font color=#0000ff>%2</font></b><br>").arg(outName).arg(LRFName));
        TO_BOTTOM(txt);
        if (oldf.copy(LRFName))
        {
            if (oldf.remove())
                MSG(tr("OK\n"), tr("<b><font color=#00ff00>OK</font></b><br>"));
            else
                ERR(tr("Remove failed: %1").arg(oldf.errorString()),
                    tr("Remove failed: <font color=#ff0000>%1</font></b>").arg(oldf.errorString()));
        }
        else
            ERR(tr("Copy failed: %1").arg(oldf.errorString()),
                tr("Copy failed: <font color=#ff0000>%1</font></b>").arg(oldf.errorString()));
    }
    else
        MSG(tr("\n"), tr("<br>"));
    TO_BOTTOM(txt);

    return true;
} // FB2toLRF
