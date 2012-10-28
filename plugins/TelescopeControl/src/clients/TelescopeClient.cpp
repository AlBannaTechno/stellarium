/*
 * Stellarium Telescope Control plug-in
 * 
 * Copyright (C) 2006 Johannes Gajdosik
 * Copyright (C) 2009-2011 Bogdan Marinov
 * 
 * This module was originally written by Johannes Gajdosik in 2006
 * as a core module of Stellarium. In 2009 it was significantly extended with
 * GUI features and later split as an external plug-in module by Bogdan Marinov.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#include "TelescopeClient.hpp"
#include "TelescopeClientDummy.hpp"
#include "TelescopeClientTcp.hpp"
#include "TelescopeClientDirectLx200.hpp"
#include "TelescopeClientDirectNexStar.hpp"
#ifdef Q_OS_WIN32
#include "TelescopeClientAscom.hpp"
#endif
#include "StelApp.hpp"
#include "StelUtils.hpp"
#include "StelTranslator.hpp"
#include "StelCore.hpp"

#include <cmath>

#include <QDebug>
#include <QRegExp>
#include <QString>
#include <QTextStream>

#ifdef Q_OS_WIN32
	#include <windows.h> // GetSystemTimeAsFileTime()
#else
	#include <sys/time.h>
#endif

TelescopeClient::TelescopeClient(const QString &name) : name(name)
{
	nameI18n = name;
}

Vec3f TelescopeClient::getInfoColor() const
{
	bool nightMode = StelApp::getInstance().getVisionModeNight();
	return nightMode ? Vec3f(0.8, 0.2, 0.2) : Vec3f(1, 1, 1);
}

QString TelescopeClient::getInfoString(const StelCore* core, const InfoStringGroup& flags) const
{
	QString str;
	QTextStream oss(&str);
	if (flags&Name)
	{
		oss << "<h2>" << nameI18n << "</h2>";
	}

	oss << getPositionInfoString(core, flags);

	postProcessInfoString(str, flags);

	return str;
}

qint64 getNow(void)
{
// At the moment this can't be done in a platform-independent way with Qt
// (QDateTime and QTime don't support microsecond precision)
#ifdef Q_OS_WIN32
	FILETIME file_time;
	GetSystemTimeAsFileTime(&file_time);
	return (*((__int64*)(&file_time))/10) - 86400000000LL*134774;
#else
	struct timeval tv;
	gettimeofday(&tv,0);
	return tv.tv_sec * 1000000LL + tv.tv_usec;
#endif
}
