#pragma once

#include <QDateTime>
#include <QHash>
#include <QString>
#include <QStringList>

struct IrcMessage {
    QString                prefix;
    QString                nick;
    QString                user;
    QString                host;
    QString                command;
    QStringList            params;
    QString                trailing;
    QHash<QString,QString> tags;
    QDateTime              serverTime; // parsed from tags["time"] if present

    bool isValid() const { return !command.isEmpty(); }
};

class IrcParser
{
public:
    static IrcMessage parse(const QString &raw);
    static QString    decodeLine(const QByteArray &raw);
};
