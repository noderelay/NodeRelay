#include "config/keychainhelper.h"

namespace KeychainHelper {

QString read(const QString &) { return {}; }
bool    write(const QString &, const QString &) { return false; }
void    remove(const QString &) {}
void    readAsync(const QString &, std::function<void(const QString &)> cb) { cb({}); }

}
