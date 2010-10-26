/*
 * $Id$
 *
 * Config definition
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <QColor>
#include <QString>
#include <QRegExp>

#include "ui_Config.h"

class Config: public QDialog {
    Q_OBJECT

public:
    Config(QWidget *par=0);
    ~Config();

    enum Sorder { S_UNKNOWN, S_AUTHOR, S_TITLE, S_DATE };
    Ui_Dialog   _ui;

    static const QColor valid;
    static const QColor invalid;

    static void    save();
    static void    init();
    static void    revert();
    static void    reread();

    // _backup_crt property definition
    public:
        static bool backupCrt() { return _backup_crt; }
    private:
        static bool _backup_crt;

    // _backup_ext property definition
    public:
        static QString backupExt() { return _backup_ext; }
    private:
        static QString _backup_ext;
    private slots:
        void backup_e(const QString& t) { _backup_ext = t; }


private slots:
    void        backup_c(bool checked);

private:
    void        setFileExists(const QString& fname, QString& v, QLabel *l);
    void        setDirExists(const QString& fname, QString& v, QLabel *l);
    void        setFname(QLineEdit *l, QString& v, const QString& t);
    void        setDname(QLineEdit *l, QString& v, const QString& t);
    bool        repl_valid(bool v);
    bool        repl_rules_general(const QString& r, QRegExp& from_re, QString& to,
                                   QString& rules);
    static bool parseReplacement(const QString& r, QString& from, QString& to);

    static bool    _init;
};

#endif
