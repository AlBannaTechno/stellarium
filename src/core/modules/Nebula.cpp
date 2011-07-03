/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau
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

#include <QTextStream>
#include <QFile>
#include <QString>

#include "Nebula.hpp"
#include "NebulaMgr.hpp"
#include "StelTexture.hpp"
#include "StelNavigator.hpp"
#include "StelUtils.hpp"
#include "StelApp.hpp"
#include "StelTextureMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelCore.hpp"
#include "StelPainter.hpp"

#include <QDebug>
#include <QBuffer>

StelTextureSP Nebula::texCircle;
float Nebula::circleScale = 1.f;
float Nebula::hintsBrightness = 1;
Vec3f Nebula::labelColor = Vec3f(0.4,0.3,0.5);
Vec3f Nebula::circleColor = Vec3f(0.8,0.8,0.1);


/* ___________________________________________________
 *
 * New Nebula class with NGC catalogue from W Steinitz
 * ___________________________________________________
 */

Nebula::Nebula() :
		M_nb(0),
		NGC_nb(0),
		IC_nb(0)
{
	nameI18 = "";
	angularSize = -1;
}

Nebula::~Nebula()
{
}

QString Nebula::getTypeString(void) const
{
	QString wsType;

	switch(nType)
	{
		case NebGx:
			wsType = q_("Galaxy");
			break;
		case NebGNe:
			wsType = q_("Galactic Nebula");
			break;
		case NebPNe:
			wsType = q_("Planetary nebula");
			break;
		case NebOpenC:
			wsType = q_("Open cluster");
			break;
		case NebGlobC:
			wsType = q_("Globular cluster");
			break;
		case NebEmis:
			wsType = q_("Emission nebula");
			break;
		case NebStar:
			wsType = q_("Star"); // TODO: May look strange to stellarium users?
			break;
		case NebUnknown:
			wsType = q_("Unknown");
			break;
		default:
			wsType = q_("Undocumented type");
			break;
	}
	return wsType;
}

QString Nebula::getInfoString(const StelCore *core, const InfoStringGroup& flags) const
{
	QString str;
	QTextStream oss(&str);

	if ((flags&Name) || (flags&CatalogNumber))
		oss << "<h2>";

	if (nameI18!="" && flags&Name)
	{
		oss << getNameI18n();
	}

	if (flags&CatalogNumber)
	{
		if (nameI18!="" && flags&Name)
			oss << " (";

		QStringList catIds;
		if ((M_nb > 0) && (M_nb < 111))
			catIds << QString("M %1").arg(M_nb);
		if (NGC_nb > 0)
			catIds << QString("NGC %1").arg(NGC_nb);
		if (IC_nb > 0)
			catIds << QString("IC %1").arg(IC_nb);
		oss << catIds.join(" - ");

		if (nameI18!="" && flags&Name)
			oss << ")";
	}

	if ((flags&Name) || (flags&CatalogNumber))
		oss << "</h2>";

	if (flags&Extra1)
		oss << q_("Type: <b>%1</b>").arg(getTypeString()) << "<br>";

	if (mag < 50 && flags&Magnitude)
		oss << q_("Magnitude: <b>%1</b>").arg(mag, 0, 'f', 2) << "<br>";

	oss << getPositionInfoString(core, flags);

	if (angularSize>0 && flags&Size)
		oss << q_("Size: %1").arg(StelUtils::radToDmsStr(angularSize*M_PI/180.)) << "<br>";

	postProcessInfoString(str, flags);

	return str;
}

float Nebula::getSelectPriority(const StelNavigator *nav) const
{
	if( ((NebulaMgr*)StelApp::getInstance().getModuleMgr().getModule("NebulaMgr"))->getFlagHints() )
	{
		// make very easy to select IF LABELED
		return -10;
	}
	else
	{
		if (getVMagnitude(nav)>20) return 20;
		return getVMagnitude(nav);
	}
}

Vec3f Nebula::getInfoColor(void) const
{
	return StelApp::getInstance().getVisionModeNight() ? Vec3f(0.6, 0.0, 0.4) : ((NebulaMgr*)StelApp::getInstance().getModuleMgr().getModule("NebulaMgr"))->getLabelsColor();
}

double Nebula::getCloseViewFov(const StelNavigator*) const
{
	return angularSize>0 ? angularSize * 4 : 1;
}

void Nebula::drawHints(StelPainter& sPainter, float maxMagHints)
{
	//if (mag>maxMagHints)
	//	return;
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	float lum = 1.f;//qMin(1,4.f/getOnScreenSize(core))*0.8;
	sPainter.setColor(circleColor[0]*lum*hintsBrightness, circleColor[1]*lum*hintsBrightness, circleColor[2]*lum*hintsBrightness, 1);
	Nebula::texCircle->bind();
	sPainter.drawSprite2dMode(XY[0], XY[1], 4);
	//float pixelPerDegree = M_PI/180.*sPainter.getProjector()->getPixelPerRadAtCenter();
	//sPainter.drawEllipse(XY[0], XY[1], sizeX*pixelPerDegree, sizeY*pixelPerDegree);
	//sPainter.drawCircle(XY[0], XY[1], sizeX*pixelPerDegree);
	//qDebug("x=%f y=%f color[0]=%f", XY[0], XY[1], circleColor[0]*lum*hintsBrightness);
}

void Nebula::drawLabel(StelPainter& sPainter, float maxMagLabel)
{
	if (mag>maxMagLabel)
		return;
	
	sPainter.setColor(labelColor[0], labelColor[1], labelColor[2], hintsBrightness);
	float size = getAngularSize(NULL)*M_PI/180.*sPainter.getProjector()->getPixelPerRadAtCenter();
	float shift = 4.f + size/1.8f;
	QString str;
	if (nameI18!="")
		str = getNameI18n();
	else
	{
		if (M_nb > 0)
			str = QString("M %1").arg(M_nb);
		else if (NGC_nb > 0)
			str = QString("NGC %1").arg(NGC_nb);
		else if (IC_nb > 0)
			str = QString("IC %1").arg(IC_nb);
	}

	sPainter.drawText(XY[0]+shift, XY[1]+shift, str, 0, 0, 0, false);
}


bool Nebula::readNGC(QString& record)
{
	int nb;

	// TODO: No longer purely numeric, e.g. NGC 5144A and NGC 5144B
	// TODO: Number of components also
	nb = record.mid(2,6).toInt();

	// Is it NGC or IC object?
	if (record.mid(0,1) == QString("I"))
	{
		IC_nb = nb;
	}
	else
	{
		NGC_nb = nb;
	}

	readIdentifiers(record);
	parseRecord(record, nb);
	
	return true;
}

void Nebula::readIdentifiers(const QString& record)
{
	int NGCType = record.mid(15, 2).toInt();
	
	// this is a huge performance drag if called every frame, so cache here
	switch (NGCType)
	{
		case 1: nType = NebGx; break;		// galaxy
		case 2: nType = NebGNe; break;		// galactic nebula (NEW)
		case 3: nType = NebPNe; break;		// planetary nebula
		case 4: nType = NebOpenC; break;	// open cluster
		case 5: nType = NebGlobC; break;	// globular cluster
		case 6: nType = NebEmis; break;		// part of Galaxy, e.g. H-II region (NEW)
		case 7: nType = NebCopy; break;		// !repeated object! (NEW)
		case 8: nType = NebInNGC; break;	// !object in NGC catalogue! (NEW)
		case 9: nType = NebStar; break;		// star (NEW)
		default:  nType = NebUnknown;
	}	
}

//! \brief Degrees (or Hours), Mins, Secs to decimal
struct DMS
{
	float Degrees, Minutes, Secs;
	
	DMS(const float _degs, const float _mins, const float _secs)
		: Degrees(_degs), Minutes(_mins), Secs(_secs)
	{}
	
	float toDecimal()
	{
		return Degrees + Minutes/60.0 + Secs/3600.0;
	}	
};

//! \brief Read the following positional info:
//		o constellation
//		o RA, declination
//		o B,V mag, surface bightness
//		o Semimajor/semiminor axis
//		o PA (degrees)
//		o Type
void Nebula::parseRecord(const QString& record, int idx)
{
	// In NGC catalogue?
	bNGCObject = (record.mid(0, 1) == QString("N")) ? true : false;
	
	// Dreyer object?
	if (record.mid(12, 1) == QString("*"))
		bDreyerObject = true;
	else
		bDreyerObject = false;
	
	// Constellation
	constellationAbbr = record.mid(20, 3);
	
	// right ascension
	const float raHrs = record.mid(27, 2).toFloat();
	const float raMins = record.mid(31, 2).toFloat();
	const float raSecs = record.mid(34, 4).toFloat();
	float ra = DMS(raHrs, raMins, raSecs).toDecimal();
	
	// declination
	const float decDegs = record.mid(42, 2).toFloat();
	const float decMins = record.mid(46, 2).toFloat();
	const float decSecs = record.mid(50, 2).toFloat();
	const QString decSgn = record.mid(39, 1);
	float dec = DMS(decDegs, decMins, decSecs).toDecimal();
	if ( decSgn == QString("-")) dec *= -1.;
	
	// debug
	if (idx==5139 || idx==2168)
	{
		qDebug("data: %s", record.toLatin1().data() );
		debugNGC(ra);
	}

	ra *= M_PI/12.;     // Convert from hours to rad
	dec *= M_PI/180.;    // Convert from deg to rad

	// Calc the Cartesian coord with RA and DE
	StelUtils::spheToRect(ra, dec, XYZ);	

	// Read the blue (photographic) and visual magnitude
	magB = record.mid(56, 4).toFloat();
	mag = magV = record.mid(64, 4).toFloat();

	// B-V colour
	BmV = record.mid(70, 3).toFloat();
	
	// surface brightness
	SBrightness = record.mid(74,4).toFloat();
	
	// Calc the angular size in radian
	// TODO: this should be independant of tex_angular_size
	// TODO: now have major and minor axes and PA available for galaxies
	// TODO: also have surface brightness (rendering option?)
	
	sizeX = record.mid(81, 5).toFloat();
	sizeY = record.mid(89, 5).toFloat();
	PAdeg = record.mid(95, 3).toFloat();
	float size;		// size (arcmin)
	if (sizeY > 0)
		size = 0.5 * (sizeX + sizeY);
	else
		size = sizeX;

	angularSize = size/60;

	// Hubble classification
	hubbleType = record.mid(98, 9);
	
	// TODO: PGC/GCGC/ other designations
	PGC_nb = record.mid(113, 6).toInt();
	altDesig1 = record.mid(119, 27);
	
	// Messier number
	if (altDesig1.contains("M "))
	{
		M_nb = altDesig1.mid(2, 3).toInt();
	}

	// TODO: where is this used?
	pointRegion = SphericalRegionP(new SphericalPoint(getJ2000EquatorialPos(NULL)));
}

void Nebula::debugNGC(float ra)
{
	qDebug("RA: %f\n", ra);
	qDebug("For omegaCen, should be: 13hr 26m 47 == %f\n",
		13 +26.0/60 +47.0/3600);
}

void Nebula::readNGC(QDataStream& in)
{
	bool isIc;
	int nb;
	float ra, dec;
	unsigned int type;
	in >> isIc >> nb >> ra >> dec >> mag >> angularSize >> type;
	if (isIc)
	{
		IC_nb = nb;
	}
	else
	{
		NGC_nb = nb;
	}
	StelUtils::spheToRect(ra,dec,XYZ);
	Q_ASSERT(fabs(XYZ.lengthSquared()-1.)<0.000000001);
	nType = (Nebula::NebulaType)type;
	pointRegion = SphericalRegionP(new SphericalPoint(getJ2000EquatorialPos(NULL)));
}

#if 0
QFile filess("filess.dat");
QDataStream out;
out.setVersion(QDataStream::Qt_4_5);
bool Nebula::readNGC(char *recordstr)
{
	int rahr;
	float ramin;
	int dedeg;
	float demin;
	int nb;

	sscanf(&recordstr[1],"%d",&nb);

	if (recordstr[0] == 'I')
	{
		IC_nb = nb;
	}
	else
	{
		NGC_nb = nb;
	}

	sscanf(&recordstr[12],"%d %f",&rahr, &ramin);
	sscanf(&recordstr[22],"%d %f",&dedeg, &demin);
	float RaRad = (double)rahr+ramin/60;
	float DecRad = (float)dedeg+demin/60;
	if (recordstr[21] == '-') DecRad *= -1.;

	RaRad*=M_PI/12.;     // Convert from hours to rad
	DecRad*=M_PI/180.;    // Convert from deg to rad

	// Calc the Cartesian coord with RA and DE
	StelUtils::spheToRect(RaRad,DecRad,XYZ);

	sscanf(&recordstr[47],"%f",&mag);
	if (mag < 1) mag = 99;

	// Calc the angular size in radian : TODO this should be independant of tex_angular_size
	float size;
	sscanf(&recordstr[40],"%f",&size);

	angularSize = size/60;
	if (angularSize<0)
		angularSize=0;

	// this is a huge performance drag if called every frame, so cache here
	if (!strncmp(&recordstr[8],"Gx",2)) { nType = NebGx;}
	else if (!strncmp(&recordstr[8],"OC",2)) { nType = NebOpenC;}
	else if (!strncmp(&recordstr[8],"Gb",2)) { nType = NebGlobC;}
	else if (!strncmp(&recordstr[8],"Nb",2)) { nType = NebN;}
	else if (!strncmp(&recordstr[8],"Pl",2)) { nType = NebPNe;}
	else if (!strncmp(&recordstr[8],"  ",2)) { return false;}
	else if (!strncmp(&recordstr[8]," -",2)) { return false;}
	else if (!strncmp(&recordstr[8]," *",2)) { return false;}
	else if (!strncmp(&recordstr[8],"D*",2)) { return false;}
	else if (!strncmp(&recordstr[7],"***",3)) { return false;}
	else if (!strncmp(&recordstr[7],"C+N",3)) { nType = NebCn;}
	else if (!strncmp(&recordstr[8]," ?",2)) { nType = NebUnknown;}
	else { nType = NebUnknown;}

	if (!filess.isOpen())
	{
		filess.open(QIODevice::WriteOnly);
		out.setDevice(&filess);
	}
	out << ((bool)(recordstr[0] == 'I')) << nb << RaRad << DecRad << mag << angularSize << ((unsigned int)nType);
	if (nb==5369 && recordstr[0] == 'I')
		filess.close();

	return true;
}
#endif


