#pragma once
#include <QHostAddress>
#include <QNetworkRequest>
#include <QUrl>

// Returns true for addresses that should never be contacted by link previews or DCC.
// Covers loopback, link-local, RFC-1918 private, CG-NAT, IANA special-purpose ranges,
// and IPv6 equivalents — used as an SSRF guard and DCC peer-validation helper.
inline bool isPrivateAddress(const QHostAddress &a)
{
    if (a.isNull())      return true;
    if (a.isLoopback())  return true;
    if (a.isMulticast()) return true;

    // IPv4 private / special ranges
    if (a.isInSubnet(QHostAddress("0.0.0.0"),       8))  return true;  // "this" network
    if (a.isInSubnet(QHostAddress("10.0.0.0"),      8))  return true;  // RFC 1918
    if (a.isInSubnet(QHostAddress("100.64.0.0"),   10))  return true;  // CG-NAT (RFC 6598)
    if (a.isInSubnet(QHostAddress("127.0.0.0"),     8))  return true;  // loopback
    if (a.isInSubnet(QHostAddress("169.254.0.0"),  16))  return true;  // link-local
    if (a.isInSubnet(QHostAddress("172.16.0.0"),   12))  return true;  // RFC 1918
    if (a.isInSubnet(QHostAddress("192.0.0.0"),    24))  return true;  // IETF protocol
    if (a.isInSubnet(QHostAddress("192.0.2.0"),    24))  return true;  // TEST-NET-1
    if (a.isInSubnet(QHostAddress("192.168.0.0"),  16))  return true;  // RFC 1918
    if (a.isInSubnet(QHostAddress("198.18.0.0"),   15))  return true;  // benchmarking
    if (a.isInSubnet(QHostAddress("198.51.100.0"), 24))  return true;  // TEST-NET-2
    if (a.isInSubnet(QHostAddress("203.0.113.0"),  24))  return true;  // TEST-NET-3

    // IPv6 private / special ranges
    if (a.isInSubnet(QHostAddress("::1"),          128)) return true;  // loopback
    if (a.isInSubnet(QHostAddress("::ffff:0:0"),    96)) return true;  // IPv4-mapped
    if (a.isInSubnet(QHostAddress("64:ff9b::"),     96)) return true;  // IPv4/IPv6 translation
    if (a.isInSubnet(QHostAddress("fc00::"),         7)) return true;  // unique-local (ULA)
    if (a.isInSubnet(QHostAddress("fe80::"),        10)) return true;  // link-local
    if (a.isInSubnet(QHostAddress("2001:db8::"),    32)) return true;  // documentation

    return false;
}

// Returns true if the URL should be blocked without a DNS lookup.
// Handles scheme enforcement, literal private IPs, and well-known private hostnames.
inline bool isBlockedBySchemeOrLiteral(const QUrl &url)
{
    const QString scheme = url.scheme().toLower();
    if (scheme != "http" && scheme != "https") return true;

    const QString host = url.host().toLower();
    if (host.isEmpty()) return true;
    if (host == "localhost" || host.endsWith(".local")) return true;

    QHostAddress addr(host);
    if (!addr.isNull())
        return isPrivateAddress(addr);

    return false;
}

// Builds a request pinned to a DNS-validated address: connect by IP while
// keeping the original hostname for the Host header and TLS validation/SNI.
// QNAM would otherwise re-resolve the hostname itself, letting a rebinding DNS
// server swap in a private address between our check and the actual connect.
inline QNetworkRequest pinnedRequest(const QUrl &url, const QHostAddress &addr)
{
    QUrl pinned = url;
    pinned.setHost(addr.toString());
    QNetworkRequest req(pinned);
    QByteArray hostHeader = url.host(QUrl::FullyEncoded).toUtf8();
    if (url.port() != -1)
        hostHeader += ':' + QByteArray::number(url.port());
    req.setRawHeader("Host", hostHeader);
    req.setPeerVerifyName(url.host());
    // A raw Host header doesn't survive HTTP/2 (:authority comes from the URL)
    req.setAttribute(QNetworkRequest::Http2AllowedAttribute, false);
    return req;
}
