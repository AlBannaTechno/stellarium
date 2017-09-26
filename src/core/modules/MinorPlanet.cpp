/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau (some old code from the Planet class)
 * Copyright (C) 2010 Bogdan Marinov
 * Copyright (C) 2013-14 Georg Zotti (accuracy&speedup)
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
 
#include "MinorPlanet.hpp"

#include "StelApp.hpp"
#include "StelCore.hpp"

#include "StelTexture.hpp"
#include "StelTextureMgr.hpp"
#include "StelTranslator.hpp"
#include "StelUtils.hpp"
#include "StelFileMgr.hpp"
#include "Orbit.hpp"

#include <QRegExp>
#include <QDebug>

MinorPlanet::MinorPlanet(const QString& englishName,
			 double radius,
			 double oblateness,
			 Vec3f halocolor,
			 float albedo,
			 float roughness,
			 //float outgas_intensity,
			 //float outgas_falloff,
			 const QString& atexMapName,
			 const QString& aobjModelName,
			 posFuncType coordFunc,
			 void* auserDataPtr,
			 OsculatingFunctType *osculatingFunc,
			 bool acloseOrbit,
			 bool hidden,
			 const QString &pTypeStr)
	: Planet (englishName,
		  radius,
		  oblateness,
		  halocolor,
		  albedo,
		  roughness,
		  //0.f, // outgas_intensity,
		  //0.f, // outgas_falloff,
		  atexMapName,
		  "",
		  aobjModelName,
		  coordFunc,
		  auserDataPtr,
		  osculatingFunc,
		  acloseOrbit,
		  hidden,
		  false, //No atmosphere
		  true,  //Halo
		  pTypeStr),
	minorPlanetNumber(0),
	//absoluteMagnitude(0.0f),
	slopeParameter(-10.0f), // -10 == mark as uninitialized: used in getVMagnitude()
	//semiMajorAxis(0.),
	nameIsProvisionalDesignation(false),
	properName(englishName),
	b_v(99.f),
	specT(""),
	specB("")
{
	//TODO: Fix the name
	// - Detect numeric prefix and set number if any
	// - detect provisional designation
	// - create the HTML name
	//Try to detect number
	//TODO: Move this to the minor planet parse code in the plug-in?	
	/*
	QString name = englishName;
	QRegExp bracketedNumberPrefixPattern("^\\((\\d+)\\)\\s");
	QRegExp freeNumberPrefixPattern("^(\\d+)\\s[A-Za-z]{3,}");
	if (bracketedNumberPrefixPattern.indexIn(name) == 0)
	{
		QString numberString = bracketedNumberPrefixPattern.cap(1);
		bool ok = false;
		number = numberString.toInt(&ok);
		if (!ok)
			number = 0;

		//TODO: Handle a name consisting only of a number
		name.remove(0, numberString.length() + 3);
		htmlName = QString("(%1) ").arg(number);
	}
	else if (freeNumberPrefixPattern.indexIn(name) == 0)
	{
		QString numberString =freeNumberPrefixPattern.cap(1);
		bool ok = false;
		number = numberString.toInt(&ok);
		if (!ok)
			number = 0;

		//TODO: Handle a name consisting only of a number
		name.remove(0, numberString.length() + 3);
		htmlName = QString("(%1) ").arg(number);
	}*/

	//Try to detect a naming conflict
	if (englishName.endsWith('*'))
		properName = englishName.left(englishName.count() - 1);

	//Try to detect provisional designation	
	QString provisionalDesignation = renderProvisionalDesignationinHtml(englishName);
	if (!provisionalDesignation.isEmpty())
	{
		nameIsProvisionalDesignation = true;
		provisionalDesignationHtml = provisionalDesignation;
	}
}

MinorPlanet::~MinorPlanet()
{
	//Do nothing for the moment
}

/*
void MinorPlanet::setSemiMajorAxis(double value)
{
	semiMajorAxis = value;
	// GZ: in case we have very many asteroids, this helps improving speed usually without sacrificing accuracy:
	deltaJDE = 2.0*qMax(semiMajorAxis, 0.1)*StelCore::JD_SECOND;
}
*/

void MinorPlanet::setSpectralType(QString sT, QString sB)
{
	specT = sT;
	specB = sB;
}

void MinorPlanet::setColorIndexBV(float bv)
{
	b_v = bv;
}

void MinorPlanet::setMinorPlanetNumber(int number)
{
	if (minorPlanetNumber)
		return;

	minorPlanetNumber = number;
}

void MinorPlanet::setAbsoluteMagnitudeAndSlope(const float magnitude, const float slope)
{
	if (slope < -1.f || slope > 2.0)
	{
		// G "should" be between 0 and 1, but may be somewhat outside.
		qDebug() << "MinorPlanet::setAbsoluteMagnitudeAndSlope(): Invalid slope parameter value (must be between -1 and 2, mostly [0..1])";
		return;
	}

	//TODO: More checks?
	//TODO: Make it set-once like the number?

	absoluteMagnitude = magnitude;
	slopeParameter = slope;
}

void MinorPlanet::setProvisionalDesignation(QString designation)
{
	//TODO: This feature has to be implemented better, anyway.
	provisionalDesignationHtml = renderProvisionalDesignationinHtml(designation);
}

QString MinorPlanet::getEnglishName() const
{
	QString r = englishName;
	if (minorPlanetNumber)
		r = QString("(%1) %2").arg(minorPlanetNumber).arg(englishName);

	return r;
}

QString MinorPlanet::getNameI18n() const
{
	QString r = nameI18;
	if (minorPlanetNumber)
		r = QString("(%1) %2").arg(minorPlanetNumber).arg(nameI18);

	return r;
}

QString MinorPlanet::getInfoString(const StelCore *core, const InfoStringGroup &flags) const
{
	//Mostly copied from Planet::getInfoString():

	QString str;
	QTextStream oss(&str);
	double az_app, alt_app;
	StelUtils::rectToSphe(&az_app,&alt_app,getAltAzPosApparent(core));
	bool withDecimalDegree = StelApp::getInstance().getFlagShowDecimalDegrees();
	double distanceAu = getJ2000EquatorialPos(core).length();
	Q_UNUSED(az_app);

	if (flags&Name)
	{
		oss << "<h2>";
		if (nameIsProvisionalDesignation)
		{
			if (minorPlanetNumber)
				oss << QString("(%1) ").arg(minorPlanetNumber);
			oss << provisionalDesignationHtml;
		}
		else
			oss << getNameI18n();  // UI translation can differ from sky translation
		oss.setRealNumberNotation(QTextStream::FixedNotation);
		oss.setRealNumberPrecision(1);
		if (sphereScale != 1.f)
			oss << QString::fromUtf8(" (\xC3\x97") << sphereScale << ")";
		oss << "</h2>";
		if (!nameIsProvisionalDesignation && !provisionalDesignationHtml.isEmpty())
		{
			oss << QString(q_("Provisional designation: %1")).arg(provisionalDesignationHtml);
			oss << "<br>";
		}
	}

	if (flags&ObjectType && getPlanetType()!=isUNDEFINED)
	{
		oss << QString("%1: <b>%2</b>").arg(q_("Type"), q_(getPlanetTypeString())) << "<br />";
	}

	if (flags&Magnitude)
	{
		QString emag = "";
		if (core->getSkyDrawer()->getFlagHasAtmosphere() && (alt_app>-3.0*M_PI/180.0)) // Don't show extincted magnitude much below horizon where model is meaningless.
			emag = QString(" (%1: <b>%2</b>)").arg(q_("extincted to"), QString::number(getVMagnitudeWithExtinction(core), 'f', 2));

		oss << QString("%1: <b>%2</b>%3").arg(q_("Magnitude"), QString::number(getVMagnitude(core), 'f', 2), emag) << "<br />";
	}

	if (flags&AbsoluteMagnitude)
	{
		// Absolute magnitude H for Minor Planets is its magnitude when in solar radius 1AU, at distance 1AU, with phase angle 0.
		//TODO: Make sure absolute magnitude is a sane value
		//If the H-G system is not used, use the default radius/albedo mechanism
		// TODO: Apparently, the first solution makes "Absolute" magnitudes for 10pc distance. This is not the definition for Abs.Mag. for Minor Bodies!
		// Hint from Wikipedia:https://en.wikipedia.org/wiki/Absolute_magnitude#Solar_System_bodies_.28H.29: subtract 31.57 from the value...
		if (slopeParameter < -9.99f)
		{
			oss << QString("%1: %2").arg(q_("Absolute Magnitude")).arg(getVMagnitude(core) - 5. * (std::log10(distanceAu*AU/PARSEC)-1.), 0, 'f', 2) << "<br>";
		}
		else
		{
			oss << QString("%1: %2").arg(q_("Absolute Magnitude")).arg(absoluteMagnitude, 0, 'f', 2) << "<br>";
		}
	}

	if (flags&Extra && b_v<99.f)
		oss << QString("%1: <b>%2</b>").arg(q_("Color Index (B-V)"), QString::number(b_v, 'f', 2)) << "<br />";

	oss << getCommonInfoString(core, flags);

	if (flags&Distance)
	{
		double hdistanceAu = getHeliocentricEclipticPos().length();
		double hdistanceKm = AU * hdistanceAu;
		// TRANSLATORS: Unit of measure for distance - astronomical unit
		QString au = qc_("AU", "distance, astronomical unit");
		// TRANSLATORS: Unit of measure for distance - kilometers
		QString km = qc_("km", "distance");
		QString distAU, distKM;
		if (hdistanceAu < 0.1)
		{
			distAU = QString::number(hdistanceAu, 'f', 6);
			distKM = QString::number(hdistanceKm, 'f', 3);
		}
		else
		{
			distAU = QString::number(hdistanceAu, 'f', 3);
			distKM = QString::number(hdistanceKm / 1.0e6, 'f', 3);
			// TRANSLATORS: Unit of measure for distance - milliones kilometers
			km = qc_("Mio km", "distance");
		}
		oss << QString("%1: %2%3 (%4 %5)").arg(q_("Distance from Sun"), distAU, au, distKM, km) << "<br />";

		double distanceAu = getJ2000EquatorialPos(core).length();
		double distanceKm = AU * distanceAu;
		if (distanceAu < 0.1)
		{
			distAU = QString::number(distanceAu, 'f', 6);
			distKM = QString::number(distanceKm, 'f', 3);		}
		else
		{
			distAU = QString::number(distanceAu, 'f', 3);
			distKM = QString::number(distanceKm / 1.0e6, 'f', 3);
			// TRANSLATORS: Unit of measure for distance - milliones kilometers
			km = qc_("Mio km", "distance");
		}
		oss << QString("%1: %2%3 (%4 %5)").arg(q_("Distance"), distAU, au, distKM, km) << "<br />";
	}

	double angularSize = 2.*getAngularSize(core)*M_PI/180.;
	if (flags&Size && angularSize>=4.8e-7)
	{
		QString sizeStr = "";
		if (sphereScale!=1.f) // We must give correct diameters even if upscaling (e.g. Moon)
		{
			QString sizeOrig, sizeScaled;
			if (withDecimalDegree)
			{
				sizeOrig   = StelUtils::radToDecDegStr(angularSize / sphereScale,5,false,true);
				sizeScaled = StelUtils::radToDecDegStr(angularSize,5,false,true);
			}
			else
			{
				sizeOrig   = StelUtils::radToDmsStr(angularSize / sphereScale, true);
				sizeScaled = StelUtils::radToDmsStr(angularSize, true);
			}

			sizeStr = QString("%1, %2: %3").arg(sizeOrig, q_("scaled up to"), sizeScaled);
		}
		else
		{
			if (withDecimalDegree)
				sizeStr = StelUtils::radToDecDegStr(angularSize,5,false,true);
			else
				sizeStr = StelUtils::radToDmsStr(angularSize, true);
		}

		oss << QString("%1: %2").arg(q_("Apparent diameter"), sizeStr) << "<br />";
	}

	// If semi-major axis not zero then calculate and display orbital period for asteroid in days
	double siderealPeriod = getSiderealPeriod();
	if (flags&Extra)
	{
		if (!specT.isEmpty())
		{
			// TRANSLATORS: Tholen spectral taxonomic classification of asteroids
			oss << QString("%1: %2").arg(q_("Tholen spectral type"), specT) << "<br />";
		}

		if (!specB.isEmpty())
		{
			// TRANSLATORS: SMASSII spectral taxonomic classification of asteroids
			oss << QString("%1: %2").arg(q_("SMASSII spectral type"), specB) << "<br />";
		}

		if (siderealPeriod>0)
		{
			QString days = qc_("days", "duration");
			// Sidereal (orbital) period for solar system bodies in days and in Julian years (symbol: a)
			oss << QString("%1: %2 %3 (%4 a)").arg(q_("Sidereal period"), QString::number(siderealPeriod, 'f', 2), days, QString::number(siderealPeriod/365.25, 'f', 3)) << "<br />";
		}

		const Vec3d& observerHelioPos = core->getObserverHeliocentricEclipticPos();
		const double elongation = getElongation(observerHelioPos);

		QString pha, elo;
		if (withDecimalDegree)
		{
			pha = StelUtils::radToDecDegStr(getPhaseAngle(observerHelioPos),4,false,true);
			elo = StelUtils::radToDecDegStr(elongation,4,false,true);
		}
		else
		{
			pha = StelUtils::radToDmsStr(getPhaseAngle(observerHelioPos), true);
			elo = StelUtils::radToDmsStr(elongation, true);
		}

		oss << QString("%1: %2").arg(q_("Phase angle"), pha) << "<br />";
		oss << QString("%1: %2").arg(q_("Elongation"), elo) << "<br />";
	}

	postProcessInfoString(str, flags);

	return str;
}

double MinorPlanet::getSiderealPeriod() const
{
//	double period;
//	if (semiMajorAxis>0)
//		period = StelUtils::calculateSiderealPeriod(semimajorAxis);
//	else
//		period = 0;

//	return period;
	return StelUtils::calculateSiderealPeriod(((CometOrbit*)orbitPtr)->getSemimajorAxis());
}

float MinorPlanet::getVMagnitude(const StelCore* core) const
{
	//If the H-G system is not used, use the default radius/albedo mechanism
	if (slopeParameter < -9.99f) // G can be somewhat <0! Set to -10 to mark invalid.
	{
		return Planet::getVMagnitude(core);
	}

	//Calculate phase angle
	//(Code copied from Planet::getVMagnitude())
	//(this is actually vector subtraction + the cosine theorem :))
	const Vec3d& observerHelioPos = core->getObserverHeliocentricEclipticPos();
	const float observerRq = observerHelioPos.lengthSquared();
	const Vec3d& planetHelioPos = getHeliocentricEclipticPos();
	const float planetRq = planetHelioPos.lengthSquared();
	const float observerPlanetRq = (observerHelioPos - planetHelioPos).lengthSquared();
	const float cos_chi = (observerPlanetRq + planetRq - observerRq)/(2.0*std::sqrt(observerPlanetRq*planetRq));
	const float phaseAngle = std::acos(cos_chi);

	//Calculate reduced magnitude (magnitude without the influence of distance)
	//Source of the formulae: http://www.britastro.org/asteroids/dymock4.pdf
	// Same model as in Explanatory Supplement 2013, p.423
	const float tanPhaseAngleHalf=std::tan(phaseAngle*0.5f);
	const float phi1 = std::exp(-3.33f * std::pow(tanPhaseAngleHalf, 0.63f));
	const float phi2 = std::exp(-1.87f * std::pow(tanPhaseAngleHalf, 1.22f));
	float reducedMagnitude = absoluteMagnitude - 2.5f * std::log10( (1.0f - slopeParameter) * phi1 + slopeParameter * phi2 );

	//Calculate apparent magnitude
	float apparentMagnitude = reducedMagnitude + 5.0f * std::log10(std::sqrt(planetRq * observerPlanetRq));

	return apparentMagnitude;
}

void MinorPlanet::translateName(const StelTranslator &translator)
{
	nameI18 = translator.qtranslate(properName);
	if (englishName.endsWith('*'))
	{
		nameI18.append('*');
	}
}

QString MinorPlanet::renderProvisionalDesignationinHtml(QString plainTextName)
{
	QRegExp provisionalDesignationPattern("^(\\d{4}\\s[A-Z]{2})(\\d*)$");
	if (provisionalDesignationPattern.indexIn(plainTextName) == 0)
	{
		QString main = provisionalDesignationPattern.cap(1);
		QString suffix = provisionalDesignationPattern.cap(2);
		if (!suffix.isEmpty())
		{
			return (QString("%1<sub>%2</sub>").arg(main).arg(suffix));
		}
		else
		{
			return main;
		}
	}
	else
	{
		return QString();
	}
}

