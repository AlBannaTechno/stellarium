/*
 * Stellarium Telescope Control plug-in
 * 
 * Copyright (C) 2006 Johannes Gajdosik
 * Copyright (C) 2009-2010 Bogdan Marinov
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "TelescopeClient.hpp"
#include "TelescopeClientDummy.hpp"
#include "TelescopeClientTCP.hpp"
#include "TelescopeClientDirectLx200.hpp"
#include "TelescopeClientDirectNexStar.hpp"
#ifdef Q_OS_WIN32
#include "TelescopeClientAscom.hpp"
#endif
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


TelescopeClient *TelescopeClient::create(const QString &url)
{
	// example url: My_first_telescope:TCP:J2000:localhost:10000:500000
	// split to:
	// name    = My_first_telescope
	// type    = TCP
	// equinox = J2000
	// params  = localhost:10000:500000
	//
	// The params part is optional.  We will use QRegExp to validate
	// the url and extact the components.

	// note, in a reg exp, [^:] matches any chararacter except ':'
	QRegExp urlSchema("^([^:]*):([^:]*):([^:]*)(?::(.*))?$");
	QString name, type, equinox, params;
	if (urlSchema.exactMatch(url))
	{
		// trimmed removes whitespace on either end of a QString
		name = urlSchema.capturedTexts().at(1).trimmed();
		type = urlSchema.capturedTexts().at(2).trimmed();
		equinox = urlSchema.capturedTexts().at(3).trimmed();
		params = urlSchema.capturedTexts().at(4).trimmed();
	}
	else
	{
		qWarning() << "WARNING - telescope definition" << url << "not recognised";
		return NULL;
	}

	Equinox eq = EquinoxJ2000;
	if (equinox == "JNow")
		eq = EquinoxJNow;

	qDebug() << "Creating telescope" << url << "; name/type/equinox/params:" << name << type << ((eq == EquinoxJNow) ? "JNow" : "J2000") << params;

	TelescopeClient * newTelescope = 0;
	
	if (type == "Dummy")
	{
		newTelescope = new TelescopeClientDummy(name, params);
	}
	else if (type == "TCP")
	{
		newTelescope = new TelescopeTCP(name, params, eq);
	}
	else if (type == "Lx200") //BM: One of the rare occasions of painless extension
	{
		newTelescope = new TelescopeClientDirectLx200(name, params, eq);
	}
	else if (type == "NexStar")
	{
		newTelescope = new TelescopeClientDirectNexStar(name, params, eq);
	}
#ifdef Q_OS_WIN32
	else if (type == "Ascom")
	{
		newTelescope = new TelescopeClientAscom(name, params, eq);
	}
#endif
	else
	{
		qWarning() << "WARNING - unknown telescope type" << type << "- not creating a telescope object for url" << url;
	}
	
	if (newTelescope && !newTelescope->isInitialized())
	{
		qDebug() << "TelescopeClient::create(): Unable to create a telescope client.";
		delete newTelescope;
		newTelescope = 0;
	}
	return newTelescope;
}

TelescopeClient* TelescopeClient::create(const QVariantMap &telescopeProperties)
{
	//
}


TelescopeClient::TelescopeClient(const QString &name) : name(name)
{
	nameI18n = name;
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

//! returns the current system time in microseconds since the Epoch
//! Prior to revision 6308, it was necessary to put put this method in an
//! #ifdef block, as duplicate function definition caused errors during static
//! linking.
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
