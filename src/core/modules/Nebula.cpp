/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau
 * Copyright (C) 2011 Alexander Wolf
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

#include "Nebula.hpp"
#include "NebulaMgr.hpp"

#include "renderer/StelRenderer.hpp"
#include "renderer/StelTextureNew.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelModuleMgr.hpp"
#include "StelUtils.hpp"

#include <QBuffer>
#include <QDebug>
#include <QFile>
#include <QString>
#include <QTextStream>

float Nebula::circleScale = 1.f;
float Nebula::hintsBrightness = 1;
Vec3f Nebula::labelColor = Vec3f(0.4,0.3,0.5);
Vec3f Nebula::circleColor = Vec3f(0.8,0.8,0.1);

/* ___________________________________________________
 *
 * New Nebula class with NGC catalogue from W Steinitz
 * ___________________________________________________
 */
Nebula::NebulaHintTextures::~NebulaHintTextures()
{
	if(!initialized){return;}

	delete texCircle; 
	delete texGalaxy;
	delete texOpenCluster;    
	delete texGlobularCluster;
	delete texPlanetaryNebula;   
	delete texDiffuseNebula;
	delete texOpenClusterWithNebulosity;
	
	initialized = false;
}

void Nebula::NebulaHintTextures::lazyInit(StelRenderer* renderer)
{
	if(initialized){return;}

	texCircle          = renderer->createTexture("textures/neb.png");      // Load circle texture
	texGalaxy          = renderer->createTexture("textures/neb_gal.png");  // Load ellipse texture
	texOpenCluster     = renderer->createTexture("textures/neb_ocl.png");  // Load open cluster marker texture
	texGlobularCluster = renderer->createTexture("textures/neb_gcl.png");  // Load globular cluster marker texture
	texPlanetaryNebula = renderer->createTexture("textures/neb_pnb.png");  // Load planetary nebula marker texture
	texDiffuseNebula   = renderer->createTexture("textures/neb_dif.png");  // Load diffuse nebula marker texture
	texOpenClusterWithNebulosity = renderer->createTexture("textures/neb_ocln.png");  // Load Ocl/Nebula marker texture

	initialized = true;
}


Nebula::Nebula() :
		M_nb(0),
		NGC_nb(0),
		IC_nb(0),
		C_nb(0)
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
			wsType += q_(" (") + q_(hubbleType) + q_(")");
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
		if ((C_nb > 0) && (C_nb < 110))
			catIds << QString("C %1").arg(C_nb);
		oss << catIds.join(" - ");

		if (nameI18!="" && flags&Name)
			oss << ")";
	}

	if ((flags&Name) || (flags&CatalogNumber))
		oss << "</h2>";

	if (flags&Extra1)
		oss << q_("Type: <b>%1</b>").arg(getTypeString()) << "<br>";

	if (mag < 50 && flags&Magnitude)
	{
	    if (core->getSkyDrawer()->getFlagHasAtmosphere())
		oss << q_("Magnitude: <b>%1</b> (extincted to: <b>%2</b>)").arg(QString::number(getVMagnitude(core, false), 'f', 2),
										QString::number(getVMagnitude(core, true), 'f', 2)) << "<br>";
	    else
		oss << q_("Magnitude: <b>%1</b>").arg(getVMagnitude(core, false), 0, 'f', 2) << "<br>";
	}
	oss << getPositionInfoString(core, flags);

	if (sizeX>0 && flags&Size)
	{
		if (sizeY>0)
			oss << q_("Size: %1 x %2").arg(StelUtils::radToDmsStr(sizeX/60. * M_PI/180.)).arg(StelUtils::radToDmsStr(sizeY/60. * M_PI/180.)) << "<br>";
		else
			oss << q_("Size: %1").arg(StelUtils::radToDmsStr(sizeX/60. * M_PI/180.)) << "<br>";
	}
	postProcessInfoString(str, flags);

	return str;
}

float Nebula::getVMagnitude(const StelCore* core, bool withExtinction) const
{
    float extinctionMag=0.0; // track magnitude loss
    if (withExtinction && core->getSkyDrawer()->getFlagHasAtmosphere())
    {
	Vec3d altAz=getAltAzPosApparent(core);
	altAz.normalize();
	core->getSkyDrawer()->getExtinction().forward(&altAz[2], &extinctionMag);
    }

    return mag+extinctionMag;
}


float Nebula::getSelectPriority(const StelCore* core) const
{
	if( ((NebulaMgr*)StelApp::getInstance().getModuleMgr().getModule("NebulaMgr"))->getFlagHints() )
	{
		// make very easy to select IF LABELED
		return -10.f;
	}
	else
	{
		if (getVMagnitude(core, false)>20.f) return 20.f;
		return getVMagnitude(core, false);
	}
}

Vec3f Nebula::getInfoColor(void) const
{
	Vec3f col = ((NebulaMgr*)StelApp::getInstance().getModuleMgr().getModule("NebulaMgr"))->getLabelsColor();
	return StelApp::getInstance().getVisionModeNight() ? StelUtils::getNightColor(col) : col;
}

double Nebula::getCloseViewFov(const StelCore*) const
{
	return angularSize>0 ? angularSize * 4 : 1;
}

void Nebula::drawHints(StelRenderer* renderer, StelProjectorP projector, float maxMagHints, NebulaHintTextures& hintTextures)
{
	float lim = mag;
	if (lim > 50) lim = 15.f;

	// temporary workaround of this bug: https://bugs.launchpad.net/stellarium/+bug/1115035 --AW
	if (getEnglishName().contains("Pleiades"))
		lim = 5.f;

	if (lim>maxMagHints)
		return;
	renderer->setBlendMode(BlendMode_Add);
	float lum = 1.f;//qMin(1,4.f/getOnScreenSize(core))*0.8;

    float pixelPerDegree = M_PI/180. * projector->getPixelPerRadAtCenter();

	Vec3f col(circleColor[0]*lum*hintsBrightness, circleColor[1]*lum*hintsBrightness, circleColor[2]*lum*hintsBrightness);
	if (StelApp::getInstance().getVisionModeNight())
		col = StelUtils::getNightColor(col);

	renderer->setGlobalColor(col[0], col[1], col[2], 1);
/*
	if (nType == NebGx)		 hintTextures.texGalaxy->bind();
	else if (nType == NebOc) hintTextures.texOpenCluster->bind();
	else if (nType == NebGc) hintTextures.texGlobularCluster->bind();
	else if (nType == NebN)  hintTextures.texDiffuseNebula->bind();
	else if (nType == NebPn) hintTextures.texPlanetaryNebula->bind();
	else if (nType == NebCn) hintTextures.texOpenClusterWithNebulosity->bind();
	else hintTextures.texCircle->bind();
*/
	switch (nType) {
		case NebGx:
			/* draw red ellipse */
			//qDebug("x=%f y=%f sizeX=%f sizeY=%f", XY[0], XY[1], sizeX*pixelPerDegree/60., sizeY*pixelPerDegree/60.);
            //sPainter.setShadeModel(StelPainter::ShadeModelFlat);
			if(sizeY < 1e-3)
                renderer->drawEllipse(XY[0], XY[1], sizeX*pixelPerDegree/60, sizeX*pixelPerDegree/60, PAdeg);
			else
                renderer->drawEllipse(XY[0], XY[1], sizeX*pixelPerDegree/60., sizeY*pixelPerDegree/60., PAdeg);
            //hintTextures.texGalaxy->bind();
            break;
			
		case NebPNe:
            hintTextures.texPlanetaryNebula->bind();
            break;
			
		case NebOpenC:
            hintTextures.texOpenCluster->bind();
            break;
			
		case NebGlobC:
            hintTextures.texGlobularCluster->bind();
            break;
			
        case NebGNe:
			hintTextures.texDiffuseNebula->bind();
			break;

        case NebEmis:
			hintTextures.texOpenClusterWithNebulosity->bind();
			break;

		default:
			hintTextures.texCircle->bind();
	}

	renderer->drawTexturedRect(XY[0] - 6, XY[1] - 6, 12, 12);
}


void Nebula::drawLabel(StelRenderer* renderer, StelProjectorP projector, float maxMagLabel)
{
	float lim = mag;
	if (lim > 50) lim = 15.f;

	// temporary workaround of this bug: https://bugs.launchpad.net/stellarium/+bug/1115035 --AW
	if (getEnglishName().contains("Pleiades"))
		lim = 5.f;

	if (lim>maxMagLabel)
		return;

	Vec3f col(labelColor[0], labelColor[1], labelColor[2]);
	if (StelApp::getInstance().getVisionModeNight())
		col = StelUtils::getNightColor(col);
	renderer->setGlobalColor(col[0], col[1], col[2], hintsBrightness);

	QString str;
	if (!nameI18.isEmpty())
		str = getNameI18n();
	else
	{
		if (M_nb > 0)
			str = QString("M %1").arg(M_nb);
		else if (C_nb > 0)
			str = QString("C %1").arg(C_nb);
		else if (NGC_nb > 0)
			str = QString("NGC %1").arg(NGC_nb);
		else if (IC_nb > 0)
			str = QString("IC %1").arg(IC_nb);		
	}                   

	float size = getAngularSize(NULL) * M_PI / 180.0 * projector->getPixelPerRadAtCenter();
	float shift = 4.f + size / 1.8f;

	renderer->drawText(TextParams(XY[0] + shift, XY[1] + shift, str).useGravity().projector(projector));
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
#if defined(GEN_BIN_CATALOG)
	_ra = ra; _dec = dec;
#endif
	// Calc the Cartesian coord with RA and DE
	StelUtils::spheToRect(ra, dec, XYZ);	

	// Read the blue (photographic) and visual magnitude
	magB = record.mid(56, 4).toFloat();
	mag = magV = record.mid(64, 4).toFloat();

	// B-V colour
	BminusV = record.mid(70, 3).toFloat();
	
	// surface brightness
	SBrightness = record.mid(74, 4).toFloat();
	
	// TODO: also have surface brightness (rendering option?)
	
	sizeX = record.mid(79, 7).toFloat();
	sizeY = record.mid(87, 7).toFloat();
	PAdeg = record.mid(95, 3).toFloat();
	float size;		// size (arcmin)
	if (sizeY > 0)
		size = 0.5 * (sizeX + sizeY);
	else
		size = sizeX;

	angularSize = size/60;

	// Hubble classification
	hubbleType = record.mid(98, 14);
	
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
	// GZ: Trace the undefined entries...
	//if (type >= 5) {
	//	qDebug()<< (isIc?"IC" : "NGC") << nb << " type " << type ;
	//}
	if (type == 5) {
		qDebug()<< (isIc?"IC" : "NGC") << nb << " type " << type ;
	}
	pointRegion = SphericalRegionP(new SphericalPoint(getJ2000EquatorialPos(NULL)));
}

void Nebula::readExtendedNGC(QDataStream& in)
{
	int nb;
	qint32 type;
	float ra, dec;

	in 	>> bNGCObject
		>> nb
		>> bDreyerObject
		>> type
		>> constellationAbbr
		>> ra
		>> dec
		>> magB
		>> magV
		>> BminusV
		>> SBrightness
		>> sizeX
		>> sizeY
		>> PAdeg
		>> hubbleType
		>> PGC_nb
		>> M_nb;

	if (bNGCObject)
	{
		NGC_nb = nb;
	}
	else
	{
		IC_nb = nb;
	}
	
	float size;		// size (arcmin)
	if (sizeY > 0)
		size = 0.5 * (sizeX + sizeY);
	else
		size = sizeX;
	angularSize = size/60;

	mag = magV; // default is visual magnitude
	
	StelUtils::spheToRect(ra,dec,XYZ);
	Q_ASSERT(fabs(XYZ.lengthSquared()-1.)<0.000000001);
	nType = (Nebula::NebulaType)type;
	pointRegion = SphericalRegionP(new SphericalPoint(getJ2000EquatorialPos(NULL)));

}
#if defined(GEN_BIN_CATALOG)
void Nebula::writeExtendedNGC(QDataStream& out)
{
	out << bNGCObject
		<< ((NGC_nb > 0) ? NGC_nb : IC_nb)
		<< bDreyerObject
		<< ((qint32) nType)
		<< constellationAbbr
		<< _ra
		<< _dec
		<< magB
		<< magV
		<< BminusV
		<< SBrightness
		<< sizeX
		<< sizeY
		<< PAdeg
		<< hubbleType
		<< PGC_nb
		<< M_nb;
}
#endif

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


