/*
 * Stellarium
 * Copyright (C) 2014 Marcos Cardinot
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

#ifndef _STELADDON_HPP_
#define _STELADDON_HPP_

#include <QDir>
#include <QObject>
#include <QStringBuilder>
#include <QtDebug>

#include "AddOn.hpp"
#include "StelApp.hpp"
#include "StelFileMgr.hpp"
#include "StelModuleMgr.hpp"
#include "qzipreader.h"

// pure virtual (abstract) class to provide an interface for addons
class StelAddOn : public QObject
{
	Q_OBJECT
public:
	// check add-ons which are already installed
	virtual QStringList checkInstalledAddOns() const = 0;

	// install add-on from a available file
	virtual AddOn::Status installFromFile(const QString& idInstall,
					      const QString& downloadedFilepath,
					      const QStringList& selectedFiles) const = 0;

	// uninstall add-on
	virtual AddOn::Status uninstallAddOn(const QString& idInstall,
					     const QStringList& selectedFiles) const = 0;
};

#endif // _STELADDON_HPP_
