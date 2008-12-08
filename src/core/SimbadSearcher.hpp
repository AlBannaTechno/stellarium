/*
 * Copyright (C) 2008 Fabien Chereau
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

#ifndef SIMBADSEARCHER_HPP_
#define SIMBADSEARCHER_HPP_

#include "VecMath.hpp"
#include <QObject>
#include <QMap>

class QNetworkReply;

//! @class SimbadLookupReply
//! Contains all the information about a current simbad lookup query.
//! Instances of this class are generated by the SimbadSearcher class.
class SimbadLookupReply : public QObject
{
	Q_OBJECT;
	Q_ENUMS(SimbadLookupStatus);
	
public:
	friend class SimbadSearcher;
	
	//! Possible status for a simbad query.
	enum SimbadLookupStatus
	{
		SimbadLookupQuerying,
		SimbadLookupErrorOccured,
		SimbadLookupFinished
	};

	~SimbadLookupReply();
	
	//! Get the result list of matching objectName/position.
	QMap<QString, Vec3d> getResults() const {return resultPositions;}
	
	//! Get the current status.
	SimbadLookupStatus getCurrentStatus() const {return currentStatus;}
	
	//! Get a I18n string describing the current status. It can be used e.g for reporting in widgets.
	QString getCurrentStatusString() const;
	
	//! Get the error descrition string. Return empty string if no error occured.
	QString getErrorString() const {return errorString;}
	
signals:
	//! Triggered when the lookup status change.
	void statusChanged();
	
private slots:
	void httpQueryFinished();
	
private:
	//! Private constructor can be called by SimbadSearcher only.
	SimbadLookupReply(QNetworkReply* r);
	
	//! The reply used internally.
	QNetworkReply* reply;
	
	//! The list of resulting objectNames/Position in ICRS.
	QMap<QString, Vec3d> resultPositions;
	
	//! Current lookup status.
	SimbadLookupStatus currentStatus;
	
	//! The error description. Empty if no errors occured.
	QString errorString;
};


//! @class SimbadSearcher
//! Provides lookup features into the online Simbad service from CDS.
//! See http://simbad.u-strasbg.fr for more info.
class SimbadSearcher : public QObject
{
	Q_OBJECT;

public:
	SimbadSearcher(QObject* parent);

	//! Lookup in Simbad for object which have a name starting with @em objectName.
	//! @param objectName the possibly truncated object name.
	//! @param maxNbResult the maximum number of returned result.
	//! @return a new SimbadLookupReply which is owned by the caller.
	SimbadLookupReply* lookup(const QString& objectName, int maxNbResult=1);
		
private:	
	//! The network manager used query simbad
	class QNetworkAccessManager* networkMgr;
};

#endif /*SIMBADSEARCHER_HPP_*/
