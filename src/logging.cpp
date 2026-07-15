#include "logging.h"

// Debug output is off by default; enable at runtime, e.g.:
//   QT_LOGGING_RULES="uplink.irc.debug=true" ./Uplink
//   QT_LOGGING_RULES="uplink.*.debug=true"   ./Uplink
Q_LOGGING_CATEGORY(lcIrc,     "uplink.irc",     QtInfoMsg)
Q_LOGGING_CATEGORY(lcDcc,     "uplink.dcc",     QtInfoMsg)
Q_LOGGING_CATEGORY(lcPreview, "uplink.preview", QtInfoMsg)
