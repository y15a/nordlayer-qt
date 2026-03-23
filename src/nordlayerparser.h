#ifndef NORDLAYER_PARSER_H
#define NORDLAYER_PARSER_H

#include "types.h"
#include <QString>
#include <QList>

class NordLayerParser {
public:
    static QString stripAnsi(const QString &raw);
    static StatusInfo parseStatus(const QString &output);
    static QList<Gateway> parseGateways(const QString &output);
    static SettingsInfo parseSettings(const QString &output);
};

#endif // NORDLAYER_PARSER_H
