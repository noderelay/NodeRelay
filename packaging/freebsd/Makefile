PORTNAME=	uplink
DISTVERSION=	2026.7.0
CATEGORIES=	irc net

MAINTAINER=	joseph.d.harris78@gmail.com
COMMENT=	Fast, secure IRCv3 IRC client
WWW=		https://github.com/noderelay/UplinkIRC

LICENSE=	GPLv3

# tomlplusplus: header-only TOML parser — install devel/tomlplusplus so
# CMake's find_package(tomlplusplus) succeeds without FetchContent.
#
# qt6-keychain: check security/qtkeychain for Qt6 support.
# If no Qt6-capable port exists, CMakeLists.txt falls back to FetchContent
# (frankosterfeld/qtkeychain @ 0.14.3) — remove the network restriction
# below if relying on that fallback during port build (not recommended).
BUILD_DEPENDS=	${LOCALBASE}/include/toml++/toml.hpp:devel/tomlplusplus \
		qt6keychain>0:security/qt6-keychain

USES=		cmake desktop-file-utils qt:6
USE_QT=		base network svg widgets buildtools:build cmake:build

USE_GITHUB=	yes
GH_ACCOUNT=	noderelay
GH_PROJECT=	UplinkIRC
GH_TAGNAME=	v${DISTVERSION}

# Themes are installed by CMake into %%DATADIR%%/themes via the
# install(DIRECTORY themes/ ...) rule in CMakeLists.txt.

.include <bsd.port.mk>
