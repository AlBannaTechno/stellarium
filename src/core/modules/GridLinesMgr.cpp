/*
 * Stellarium
 * Copyright (C) 2007 Fabien Chereau
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

#include <set>
#include <QSettings>
#include <QDebug>
#include <QFontMetrics>
#include <QtOpenGL>

#include "GridLinesMgr.hpp"
#include "StelApp.hpp"

#include "StelTranslator.hpp"
#include "StelProjector.hpp"
#include "StelFader.hpp"
#include "Planet.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelCore.hpp"
#include "StelPainter.hpp"
#include "StelSkyDrawer.hpp"

//! @class SkyGrid
//! Class which manages a grid to display in the sky.
//! TODO needs support for DMS/DMS labelling, not only HMS/DMS
class SkyGrid
{
public:
	// Create and precompute positions of a SkyGrid
	SkyGrid(StelCore::FrameType frame);
	virtual ~SkyGrid();
	void draw(const StelCore* prj) const;
	void setFontSize(double newFontSize);
	void setColor(const Vec3f& c) {color = c;}
	const Vec3f& getColor() {return color;}
	void update(double deltaTime) {fader.update((int)(deltaTime*1000));}
	void setFadeDuration(float duration) {fader.setDuration((int)(duration*1000.f));}
	void setFlagshow(bool b){fader = b;}
	bool getFlagshow(void) const {return fader;}
private:
	Vec3f color;
	StelCore::FrameType frameType;
	QFont font;
	LinearFader fader;
};


//! @class SkyLine
//! Class which manages a line to display around the sky like the ecliptic line.
class SkyLine
{
public:
	enum SKY_LINE_TYPE
	{
		EQUATOR,
		ECLIPTIC,
		MERIDIAN,
		HORIZON,
		GALACTICPLANE
	};
	// Create and precompute positions of a SkyGrid
	SkyLine(SKY_LINE_TYPE _line_type = EQUATOR);
	virtual ~SkyLine();
	void draw(StelCore* core) const;
	void setColor(const Vec3f& c) {color = c;}
	const Vec3f& getColor() {return color;}
	void update(double deltaTime) {fader.update((int)(deltaTime*1000));}
	void setFadeDuration(float duration) {fader.setDuration((int)(duration*1000.f));}
	void setFlagshow(bool b){fader = b;}
	bool getFlagshow(void) const {return fader;}
	void setFontSize(double newSize);
	//! Re-translates the label.
	void updateLabel();
private:
	SKY_LINE_TYPE line_type;
	Vec3f color;
	StelCore::FrameType frameType;
	LinearFader fader;
	QFont font;
	QString label;
};

// rms added color as parameter
SkyGrid::SkyGrid(StelCore::FrameType frame) : color(0.2,0.2,0.2), frameType(frame)
{
	font.setPixelSize(12);
}

SkyGrid::~SkyGrid()
{
}

void SkyGrid::setFontSize(double newFontSize)
{
	font.setPixelSize(newFontSize);
}

// Conversion into mas = milli arcsecond
static const double RADIAN_MAS = 180./M_PI*1000.*60.*60.;
static const double DEGREE_MAS = 1000.*60.*60.;

// Step sizes in arcsec
static const double STEP_SIZES_DMS[] = {0.05, 0.2, 1., 5., 10., 60., 300., 600., 1200., 3600., 3600.*5., 3600.*10.};
static const double STEP_SIZES_HMS[] = {0.05, 0.2, 1.5, 7.5, 15., 15.*5., 15.*10., 15.*60., 15.*60.*5., 15.*60*10., 15.*60*60};

//! Return the angular grid step in degree which best fits the given scale
static double getClosestResolutionDMS(double pixelPerRad)
{
	double minResolution = 80.;
	double minSizeArcsec = minResolution/pixelPerRad*180./M_PI*3600;
	for (unsigned int i=0;i<12;++i)
		if (STEP_SIZES_DMS[i]>minSizeArcsec)
		{
			return STEP_SIZES_DMS[i]/3600.;
		}
	return 10.;
}

//! Return the angular grid step in degree which best fits the given scale
static double getClosestResolutionHMS(double pixelPerRad)
{
	double minResolution = 80.;
	double minSizeArcsec = minResolution/pixelPerRad*180./M_PI*3600;
	for (unsigned int i=0;i<11;++i)
		if (STEP_SIZES_HMS[i]>minSizeArcsec)
		{
			return STEP_SIZES_HMS[i]/3600.;
		}
	return 15.;
}

struct ViewportEdgeIntersectCallbackData
{
	ViewportEdgeIntersectCallbackData(StelPainter* p) : sPainter(p) {;}
	StelPainter* sPainter;
	Vec4f textColor;
	QString text;		// Label to display at the intersection of the lines and screen side
	double raAngle;		// Used for meridians
	StelCore::FrameType frameType;
};

// Callback which draws the label of the grid
void viewportEdgeIntersectCallback(const Vec3d& screenPos, const Vec3d& direction, void* userData)
{
	ViewportEdgeIntersectCallbackData* d = static_cast<ViewportEdgeIntersectCallbackData*>(userData);
	Vec3d direc(direction);
	direc.normalize();
	const Vec4f tmpColor = d->sPainter->getColor();
	d->sPainter->setColor(d->textColor[0], d->textColor[1], d->textColor[2], d->textColor[3]);

	QString text;
	if (d->text.isEmpty())
	{
		// We are in the case of meridians, we need to determine which of the 2 labels (3h or 15h to use)
		Vec3d tmpV;
		d->sPainter->getProjector()->unProject(screenPos, tmpV);
		double lon, lat;
		StelUtils::rectToSphe(&lon, &lat, tmpV);
		switch (d->frameType)
		{
			case StelCore::FrameAltAz:
			{
				double raAngle = M_PI-d->raAngle;
				lon = M_PI-lon;
				if (raAngle<0)
					raAngle=+2.*M_PI;
				if (lon<0)
					lon=+2.*M_PI;

				if (std::fabs(2.*M_PI-lon)<0.01)
				{
					// We are at meridian 0
					lon = 0.;
				}
				if (std::fabs(lon-raAngle) < 0.01)
					text = StelUtils::radToDmsStrAdapt(raAngle);
				else
				{
					const double delta = raAngle<M_PI ? M_PI : -M_PI;
					if (raAngle==2*M_PI && delta==-M_PI)
					{
						text = StelUtils::radToDmsStrAdapt(0);
					}
					else
					{
						text = StelUtils::radToDmsStrAdapt(raAngle+delta);
					}
				}
				break;
			}
			case StelCore::FrameGalactic:
			{
				double raAngle = M_PI-d->raAngle;
				lon = M_PI-lon;
				if (raAngle<0)
					raAngle=+2.*M_PI;
				if (lon<0)
					lon=+2.*M_PI;

				if (std::fabs(2.*M_PI-lon)<0.01)
				{
					// We are at meridian 0
					lon = 0.;
				}
				if (std::fabs(lon-raAngle) < 0.01)
					text = StelUtils::radToDmsStrAdapt(-raAngle+M_PI);
				else
				{
					const double delta = raAngle<M_PI ? M_PI : -M_PI;
					text = StelUtils::radToDmsStrAdapt(-raAngle-delta+M_PI);
				}
				break;
			}
			default:
			{
				if (std::fabs(2.*M_PI-lon)<0.01)
				{
					// We are at meridian 0
					lon = 0.;
				}
				if (std::fabs(lon-d->raAngle) < 0.01)
					text = StelUtils::radToHmsStrAdapt(d->raAngle);
				else
				{
					const double delta = d->raAngle<M_PI ? M_PI : -M_PI;
					text = StelUtils::radToHmsStrAdapt(d->raAngle+delta);
				}
			}
		}
	}
	else
		text = d->text;

	double angleDeg = std::atan2(-direc[1], -direc[0])*180./M_PI;
	float xshift=6.f;
	if (angleDeg>90. || angleDeg<-90.)
	{
		angleDeg+=180.;
		xshift=-d->sPainter->getFontMetrics().width(text)-6.f;
	}

	d->sPainter->drawText(screenPos[0], screenPos[1], text, angleDeg, xshift, 3);
	d->sPainter->setColor(tmpColor[0], tmpColor[1], tmpColor[2], tmpColor[3]);
	glDisable(GL_TEXTURE_2D);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

//! Draw the sky grid in the current frame
void SkyGrid::draw(const StelCore* core) const
{
	const StelProjectorP prj = core->getProjection(frameType, frameType!=StelCore::FrameAltAz ? StelCore::RefractionAuto : StelCore::RefractionOff);
	if (!fader.getInterstate())
		return;

	// Look for all meridians and parallels intersecting with the disk bounding the viewport
	// Check whether the pole are in the viewport
	bool northPoleInViewport = false;
	bool southPoleInViewport = false;
	Vec3f win;
	if (prj->project(Vec3f(0,0,1), win) && prj->checkInViewport(win))
		northPoleInViewport = true;
	if (prj->project(Vec3f(0,0,-1), win) && prj->checkInViewport(win))
		southPoleInViewport = true;
	// Get the longitude and latitude resolution at the center of the viewport
	Vec3d centerV;
	prj->unProject(prj->getViewportPosX()+prj->getViewportWidth()/2, prj->getViewportPosY()+prj->getViewportHeight()/2+1, centerV);
	double lon2, lat2;
	StelUtils::rectToSphe(&lon2, &lat2, centerV);

	const double gridStepParallelRad = M_PI/180.*getClosestResolutionDMS(prj->getPixelPerRadAtCenter());
	double gridStepMeridianRad;
	if (northPoleInViewport || southPoleInViewport)
		gridStepMeridianRad = (frameType==StelCore::FrameAltAz || frameType==StelCore::FrameGalactic) ? M_PI/180.* 10. : M_PI/180.* 15.;
	else
	{
		const double closetResLon = (frameType==StelCore::FrameAltAz || frameType==StelCore::FrameGalactic) ? getClosestResolutionDMS(prj->getPixelPerRadAtCenter()*std::cos(lat2)) : getClosestResolutionHMS(prj->getPixelPerRadAtCenter()*std::cos(lat2));
		gridStepMeridianRad = M_PI/180.* ((northPoleInViewport || southPoleInViewport) ? 15. : closetResLon);
	}

	// Get the bounding halfspace
	const SphericalCap& viewPortSphericalCap = prj->getBoundingCap();

	// Compute the first grid starting point. This point is close to the center of the screen
	// and lays at the intersection of a meridien and a parallel
	lon2 = gridStepMeridianRad*((int)(lon2/gridStepMeridianRad+0.5));
	lat2 = gridStepParallelRad*((int)(lat2/gridStepParallelRad+0.5));
	Vec3d firstPoint;
	StelUtils::spheToRect(lon2, lat2, firstPoint);
	firstPoint.normalize();

	// Q_ASSERT(viewPortSphericalCap.contains(firstPoint));

	// Initialize a painter and set openGL state
	StelPainter sPainter(prj);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Normal transparency mode
	Vec4f textColor(color[0], color[1], color[2], 0);
	if (StelApp::getInstance().getVisionModeNight())
	{
		// instead of a filter which just zeros G&B, set the red
		// value to the mean brightness of RGB.
		float red = (color[0] + color[1] + color[2]) / 3.0;
		textColor[0] = red;
		textColor[1] = 0.; textColor[2] = 0.;
		sPainter.setColor(red, 0, 0, fader.getInterstate());
	}
	else
	{
		sPainter.setColor(color[0],color[1],color[2], fader.getInterstate());
	}

	textColor*=2;
	textColor[3]=fader.getInterstate();

	sPainter.setFont(font);
	ViewportEdgeIntersectCallbackData userData(&sPainter);
	userData.textColor = textColor;
	userData.frameType = frameType;

	/////////////////////////////////////////////////
	// Draw all the meridians (great circles)
	SphericalCap meridianSphericalCap(Vec3d(1,0,0), 0);
	Mat4d rotLon = Mat4d::zrotation(gridStepMeridianRad);
	Vec3d fpt = firstPoint;
	Vec3d p1, p2;
	int maxNbIter = (int)(M_PI/gridStepMeridianRad);
	int i;
	for (i=0; i<maxNbIter; ++i)
	{
		StelUtils::rectToSphe(&lon2, &lat2, fpt);
		userData.raAngle = lon2;

		meridianSphericalCap.n = fpt^Vec3d(0,0,1);
		meridianSphericalCap.n.normalize();
		if (!SphericalCap::intersectionPoints(viewPortSphericalCap, meridianSphericalCap, p1, p2))
		{
			if (viewPortSphericalCap.d<meridianSphericalCap.d && viewPortSphericalCap.contains(meridianSphericalCap.n))
			{
				// The meridian is fully included in the viewport, draw it in 3 sub-arcs to avoid length > 180.
				const Mat4d& rotLon120 = Mat4d::rotation(meridianSphericalCap.n, 120.*M_PI/180.);
				Vec3d rotFpt=fpt;
				rotFpt.transfo4d(rotLon120);
				Vec3d rotFpt2=rotFpt;
				rotFpt2.transfo4d(rotLon120);
				sPainter.drawGreatCircleArc(fpt, rotFpt, NULL, viewportEdgeIntersectCallback, &userData);
				sPainter.drawGreatCircleArc(rotFpt, rotFpt2, NULL, viewportEdgeIntersectCallback, &userData);
				sPainter.drawGreatCircleArc(rotFpt2, fpt, NULL, viewportEdgeIntersectCallback, &userData);
				fpt.transfo4d(rotLon);
				continue;
			}
			else
				break;
		}

		Vec3d middlePoint = p1+p2;
		middlePoint.normalize();
		if (!viewPortSphericalCap.contains(middlePoint))
			middlePoint*=-1.;

		// Draw the arc in 2 sub-arcs to avoid lengths > 180 deg
		sPainter.drawGreatCircleArc(p1, middlePoint, NULL, viewportEdgeIntersectCallback, &userData);
		sPainter.drawGreatCircleArc(p2, middlePoint, NULL, viewportEdgeIntersectCallback, &userData);

		fpt.transfo4d(rotLon);
	}

	if (i!=maxNbIter)
	{
		rotLon = Mat4d::zrotation(-gridStepMeridianRad);
		fpt = firstPoint;
		fpt.transfo4d(rotLon);
		for (int j=0; j<maxNbIter-i; ++j)
		{
			StelUtils::rectToSphe(&lon2, &lat2, fpt);
			userData.raAngle = lon2;

			meridianSphericalCap.n = fpt^Vec3d(0,0,1);
			meridianSphericalCap.n.normalize();
			if (!SphericalCap::intersectionPoints(viewPortSphericalCap, meridianSphericalCap, p1, p2))
				break;

			Vec3d middlePoint = p1+p2;
			middlePoint.normalize();
			if (!viewPortSphericalCap.contains(middlePoint))
				middlePoint*=-1;

			sPainter.drawGreatCircleArc(p1, middlePoint, NULL, viewportEdgeIntersectCallback, &userData);
			sPainter.drawGreatCircleArc(p2, middlePoint, NULL, viewportEdgeIntersectCallback, &userData);

			fpt.transfo4d(rotLon);
		}
	}

	/////////////////////////////////////////////////
	// Draw all the parallels (small circles)
	SphericalCap parallelSphericalCap(Vec3d(0,0,1), 0);
	rotLon = Mat4d::rotation(firstPoint^Vec3d(0,0,1), gridStepParallelRad);
	fpt = firstPoint;
	maxNbIter = (int)(M_PI/gridStepParallelRad)-1;
	for (i=0; i<maxNbIter; ++i)
	{
		StelUtils::rectToSphe(&lon2, &lat2, fpt);
		userData.text = StelUtils::radToDmsStrAdapt(lat2);

		parallelSphericalCap.d = fpt[2];
		if (parallelSphericalCap.d>0.9999999)
			break;

		const Vec3d rotCenter(0,0,parallelSphericalCap.d);
		if (!SphericalCap::intersectionPoints(viewPortSphericalCap, parallelSphericalCap, p1, p2))
		{
			if ((viewPortSphericalCap.d<parallelSphericalCap.d && viewPortSphericalCap.contains(parallelSphericalCap.n))
				|| (viewPortSphericalCap.d<-parallelSphericalCap.d && viewPortSphericalCap.contains(-parallelSphericalCap.n)))
			{
				// The parallel is fully included in the viewport, draw it in 3 sub-arcs to avoid lengths >= 180 deg
				static const Mat4d rotLon120 = Mat4d::zrotation(120.*M_PI/180.);
				Vec3d rotFpt=fpt;
				rotFpt.transfo4d(rotLon120);
				Vec3d rotFpt2=rotFpt;
				rotFpt2.transfo4d(rotLon120);
				sPainter.drawSmallCircleArc(fpt, rotFpt, rotCenter, viewportEdgeIntersectCallback, &userData);
				sPainter.drawSmallCircleArc(rotFpt, rotFpt2, rotCenter, viewportEdgeIntersectCallback, &userData);
				sPainter.drawSmallCircleArc(rotFpt2, fpt, rotCenter, viewportEdgeIntersectCallback, &userData);
				fpt.transfo4d(rotLon);
				continue;
			}
			else
				break;
		}

		// Draw the arc in 2 sub-arcs to avoid lengths > 180 deg
		Vec3d middlePoint = p1-rotCenter+p2-rotCenter;
		middlePoint.normalize();
		middlePoint*=(p1-rotCenter).length();
		middlePoint+=rotCenter;
		if (!viewPortSphericalCap.contains(middlePoint))
		{
			middlePoint-=rotCenter;
			middlePoint*=-1.;
			middlePoint+=rotCenter;
		}

		sPainter.drawSmallCircleArc(p1, middlePoint, rotCenter, viewportEdgeIntersectCallback, &userData);
		sPainter.drawSmallCircleArc(p2, middlePoint, rotCenter, viewportEdgeIntersectCallback, &userData);

		fpt.transfo4d(rotLon);
	}

	if (i!=maxNbIter)
	{
		rotLon = Mat4d::rotation(firstPoint^Vec3d(0,0,1), -gridStepParallelRad);
		fpt = firstPoint;
		fpt.transfo4d(rotLon);
		for (int j=0; j<maxNbIter-i; ++j)
		{
			StelUtils::rectToSphe(&lon2, &lat2, fpt);
			userData.text = StelUtils::radToDmsStrAdapt(lat2);

			parallelSphericalCap.d = fpt[2];
			const Vec3d rotCenter(0,0,parallelSphericalCap.d);
			if (!SphericalCap::intersectionPoints(viewPortSphericalCap, parallelSphericalCap, p1, p2))
			{
				if ((viewPortSphericalCap.d<parallelSphericalCap.d && viewPortSphericalCap.contains(parallelSphericalCap.n))
					 || (viewPortSphericalCap.d<-parallelSphericalCap.d && viewPortSphericalCap.contains(-parallelSphericalCap.n)))
				{
					// The parallel is fully included in the viewport, draw it in 3 sub-arcs to avoid lengths >= 180 deg
					static const Mat4d rotLon120 = Mat4d::zrotation(120.*M_PI/180.);
					Vec3d rotFpt=fpt;
					rotFpt.transfo4d(rotLon120);
					Vec3d rotFpt2=rotFpt;
					rotFpt2.transfo4d(rotLon120);
					sPainter.drawSmallCircleArc(fpt, rotFpt, rotCenter, viewportEdgeIntersectCallback, &userData);
					sPainter.drawSmallCircleArc(rotFpt, rotFpt2, rotCenter, viewportEdgeIntersectCallback, &userData);
					sPainter.drawSmallCircleArc(rotFpt2, fpt, rotCenter, viewportEdgeIntersectCallback, &userData);
					fpt.transfo4d(rotLon);
					continue;
				}
				else
					break;
			}

			// Draw the arc in 2 sub-arcs to avoid lengths > 180 deg
			Vec3d middlePoint = p1-rotCenter+p2-rotCenter;
			middlePoint.normalize();
			middlePoint*=(p1-rotCenter).length();
			middlePoint+=rotCenter;
			if (!viewPortSphericalCap.contains(middlePoint))
			{
				middlePoint-=rotCenter;
				middlePoint*=-1.;
				middlePoint+=rotCenter;
			}

			sPainter.drawSmallCircleArc(p1, middlePoint, rotCenter, viewportEdgeIntersectCallback, &userData);
			sPainter.drawSmallCircleArc(p2, middlePoint, rotCenter, viewportEdgeIntersectCallback, &userData);

			fpt.transfo4d(rotLon);
		}
	}
}


SkyLine::SkyLine(SKY_LINE_TYPE _line_type) : color(0.f, 0.f, 1.f)
{
	font.setPixelSize(14);
	line_type = _line_type;

	updateLabel();
}

SkyLine::~SkyLine()
{
}

void SkyLine::setFontSize(double newFontSize)
{
	font.setPixelSize(newFontSize);
}

void SkyLine::updateLabel()
{
	switch (line_type)
	{
		case MERIDIAN:
			frameType = StelCore::FrameAltAz;
			label = q_("Meridian");
			break;
		case ECLIPTIC:
			frameType = StelCore::FrameObservercentricEcliptic;
			label = q_("Ecliptic");
			break;
		case EQUATOR:
			frameType = StelCore::FrameEquinoxEqu;
			label = q_("Equator");
			break;
		case HORIZON:
			frameType = StelCore::FrameAltAz;
			label = q_("Horizon");
			break;
		case GALACTICPLANE:
			frameType = StelCore::FrameGalactic;
			label = q_("Galactic Plane");
			break;
	}
}

void SkyLine::draw(StelCore *core) const
{
	if (!fader.getInterstate())
		return;

	StelProjectorP prj = core->getProjection(frameType, frameType!=StelCore::FrameAltAz ? StelCore::RefractionAuto : StelCore::RefractionOff);

	// Get the bounding halfspace
	const SphericalCap& viewPortSphericalCap = prj->getBoundingCap();

	// Initialize a painter and set openGL state
	StelPainter sPainter(prj);
	sPainter.setColor(color[0], color[1], color[2], fader.getInterstate());
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Normal transparency mode

	Vec4f textColor(color[0], color[1], color[2], 0);	
	textColor*=2;
	textColor[3]=fader.getInterstate();

	ViewportEdgeIntersectCallbackData userData(&sPainter);	
	sPainter.setFont(font);
	userData.textColor = textColor;	
	userData.text = label;
	/////////////////////////////////////////////////
	// Draw the line
	SphericalCap meridianSphericalCap(Vec3d(0,0,1), 0);	
	Vec3d fpt(1,0,0);
	if (line_type==MERIDIAN)
	{
		meridianSphericalCap.n.set(0,1,0);
	}

	Vec3d p1, p2;
	if (!SphericalCap::intersectionPoints(viewPortSphericalCap, meridianSphericalCap, p1, p2))
	{
		if ((viewPortSphericalCap.d<meridianSphericalCap.d && viewPortSphericalCap.contains(meridianSphericalCap.n))
			|| (viewPortSphericalCap.d<-meridianSphericalCap.d && viewPortSphericalCap.contains(-meridianSphericalCap.n)))
		{
			// The meridian is fully included in the viewport, draw it in 3 sub-arcs to avoid length > 180.
			const Mat4d& rotLon120 = Mat4d::rotation(meridianSphericalCap.n, 120.*M_PI/180.);
			Vec3d rotFpt=fpt;
			rotFpt.transfo4d(rotLon120);
			Vec3d rotFpt2=rotFpt;
			rotFpt2.transfo4d(rotLon120);
			sPainter.drawGreatCircleArc(fpt, rotFpt, NULL, viewportEdgeIntersectCallback, &userData);
			sPainter.drawGreatCircleArc(rotFpt, rotFpt2, NULL, viewportEdgeIntersectCallback, &userData);
			sPainter.drawGreatCircleArc(rotFpt2, fpt, NULL, viewportEdgeIntersectCallback, &userData);
			return;
		}
		else
			return;
	}


	Vec3d middlePoint = p1+p2;
	middlePoint.normalize();
	if (!viewPortSphericalCap.contains(middlePoint))
		middlePoint*=-1.;

	// Draw the arc in 2 sub-arcs to avoid lengths > 180 deg
	sPainter.drawGreatCircleArc(p1, middlePoint, NULL, viewportEdgeIntersectCallback, &userData);
	sPainter.drawGreatCircleArc(p2, middlePoint, NULL, viewportEdgeIntersectCallback, &userData);

// 	// Johannes: use a big radius as a dirty workaround for the bug that the
// 	// ecliptic line is not drawn around the observer, but around the sun:
// 	const Vec3d vv(1000000,0,0);

}

GridLinesMgr::GridLinesMgr()
{
	setObjectName("GridLinesMgr");
	equGrid = new SkyGrid(StelCore::FrameEquinoxEqu);
	equJ2000Grid = new SkyGrid(StelCore::FrameJ2000);
	galacticGrid = new SkyGrid(StelCore::FrameGalactic);
	aziGrid = new SkyGrid(StelCore::FrameAltAz);
	equatorLine = new SkyLine(SkyLine::EQUATOR);
	eclipticLine = new SkyLine(SkyLine::ECLIPTIC);
	meridianLine = new SkyLine(SkyLine::MERIDIAN);
	horizonLine = new SkyLine(SkyLine::HORIZON);
	galacticPlaneLine = new SkyLine(SkyLine::GALACTICPLANE);
}

GridLinesMgr::~GridLinesMgr()
{
	delete equGrid;
	delete equJ2000Grid;
	delete galacticGrid;
	delete aziGrid;
	delete equatorLine;
	delete eclipticLine;
	delete meridianLine;
	delete horizonLine;
	delete galacticPlaneLine;
}

/*************************************************************************
 Reimplementation of the getCallOrder method
*************************************************************************/
double GridLinesMgr::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("NebulaMgr")->getCallOrder(actionName)+10;
	return 0;
}

void GridLinesMgr::init()
{
	QSettings* conf = StelApp::getInstance().getSettings();
	Q_ASSERT(conf);

	setAzimuthalGridDisplayed(conf->value("viewing/flag_azimuthal_grid").toBool());
	setEquatorGridDisplayed(conf->value("viewing/flag_equatorial_grid").toBool());
	setEquatorJ2000GridDisplayed(conf->value("viewing/flag_equatorial_J2000_grid").toBool());
	setGalacticGridDisplayed(conf->value("viewing/flag_galactic_grid").toBool());
	setEquatorLineDisplayed(conf->value("viewing/flag_equator_line").toBool());
	setEclipticLineDisplayed(conf->value("viewing/flag_ecliptic_line").toBool());
	setMeridianLineDisplayed(conf->value("viewing/flag_meridian_line").toBool());
	setHorizonLineDisplayed(conf->value("viewing/flag_horizon_line").toBool());
	setGalacticPlaneLineDisplayed(conf->value("viewing/flag_galactic_plane_line").toBool());
	
	StelApp& app = StelApp::getInstance();
	connect(&app, SIGNAL(colorSchemeChanged(const QString&)), this, SLOT(setStelStyle(const QString&)));
	connect(&app, SIGNAL(languageChanged()), this, SLOT(updateLineLabels()));
}

void GridLinesMgr::update(double deltaTime)
{
	// Update faders
	equGrid->update(deltaTime);
	equJ2000Grid->update(deltaTime);
	galacticGrid->update(deltaTime);
	aziGrid->update(deltaTime);
	equatorLine->update(deltaTime);
	eclipticLine->update(deltaTime);
	meridianLine->update(deltaTime);
	horizonLine->update(deltaTime);
	galacticPlaneLine->update(deltaTime);
}

void GridLinesMgr::draw(StelCore* core)
{
	equGrid->draw(core);
	galacticGrid->draw(core);
	equJ2000Grid->draw(core);
	aziGrid->draw(core);
	equatorLine->draw(core);
	eclipticLine->draw(core);
	meridianLine->draw(core);
	horizonLine->draw(core);
	galacticPlaneLine->draw(core);
}

void GridLinesMgr::setStelStyle(const QString& section)
{
	QSettings* conf = StelApp::getInstance().getSettings();

	// Load colors from config file
	QString defaultColor = conf->value(section+"/default_color").toString();
	setEquatorGridColor(StelUtils::strToVec3f(conf->value(section+"/equatorial_color", defaultColor).toString()));
	setEquatorJ2000GridColor(StelUtils::strToVec3f(conf->value(section+"/equatorial_J2000_color", defaultColor).toString()));
	setGalacticGridColor(StelUtils::strToVec3f(conf->value(section+"/galactic_color", defaultColor).toString()));
	setAzimuthalGridColor(StelUtils::strToVec3f(conf->value(section+"/azimuthal_color", defaultColor).toString()));
	setEquatorLineColor(StelUtils::strToVec3f(conf->value(section+"/equator_color", defaultColor).toString()));
	setEclipticLineColor(StelUtils::strToVec3f(conf->value(section+"/ecliptic_color", defaultColor).toString()));
	setMeridianLineColor(StelUtils::strToVec3f(conf->value(section+"/meridian_color", defaultColor).toString()));
	setHorizonLineColor(StelUtils::strToVec3f(conf->value(section+"/horizon_color", defaultColor).toString()));
	setGalacticPlaneLineColor(StelUtils::strToVec3f(conf->value(section+"/galactic_plane_color", defaultColor).toString()));
}

void GridLinesMgr::updateLineLabels()
{
	equatorLine->updateLabel();
	eclipticLine->updateLabel();
	meridianLine->updateLabel();
	horizonLine->updateLabel();
}

//! Set flag for displaying Azimuthal Grid
void GridLinesMgr::setAzimuthalGridDisplayed(const bool displayed)
{
	if(displayed != aziGrid->getFlagshow()) {
		aziGrid->setFlagshow(displayed);
		emit azimuthalGridDisplayedChanged(displayed);
	}
}
//! Get flag for displaying Azimuthal Grid
bool GridLinesMgr::isAzimuthalGridDisplayed(void) const
{
	return aziGrid->getFlagshow();
}
Vec3f GridLinesMgr::getAzimuthalGridColor(void) const
{
	return aziGrid->getColor();
}
void GridLinesMgr::setAzimuthalGridColor(const Vec3f& newColor)
{
	if(newColor != aziGrid->getColor()) {
		aziGrid->setColor(newColor);
		emit azimuthalGridColorChanged(newColor);
	}
}

//! Set flag for displaying Equatorial Grid
void GridLinesMgr::setEquatorGridDisplayed(const bool displayed)
{
	if(displayed != equGrid->getFlagshow()) {
		equGrid->setFlagshow(displayed);
		emit equatorGridDisplayedChanged(displayed);
	}
}
//! Get flag for displaying Equatorial Grid
bool GridLinesMgr::isEquatorGridDisplayed(void) const
{
	return equGrid->getFlagshow();
}
Vec3f GridLinesMgr::getEquatorGridColor(void) const
{
	return equGrid->getColor();
}
void GridLinesMgr::setEquatorGridColor(const Vec3f& newColor)
{
	if(newColor != equGrid->getColor()) {
		equGrid->setColor(newColor);
		emit equatorGridColorChanged(newColor);
	}
}

//! Set flag for displaying Equatorial J2000 Grid
void GridLinesMgr::setEquatorJ2000GridDisplayed(const bool displayed)
{
	if(displayed != equJ2000Grid->getFlagshow()) {
		equJ2000Grid->setFlagshow(displayed);
		emit equatorJ2000GridDisplayedChanged(displayed);
	}
}
//! Get flag for displaying Equatorial J2000 Grid
bool GridLinesMgr::isEquatorJ2000GridDisplayed(void) const
{
	return equJ2000Grid->getFlagshow();
}
Vec3f GridLinesMgr::getEquatorJ2000GridColor(void) const
{
	return equJ2000Grid->getColor();
}
void GridLinesMgr::setEquatorJ2000GridColor(const Vec3f& newColor)
{
	if(newColor != equJ2000Grid->getColor()) {
		equJ2000Grid->setColor(newColor);
		emit equatorJ2000GridColorChanged(newColor);
	}
}

//! Set flag for displaying Equatorial J2000 Grid
void GridLinesMgr::setGalacticGridDisplayed(const bool displayed)
{
	if(displayed != galacticGrid->getFlagshow()) {
		galacticGrid->setFlagshow(displayed);
		emit galacticGridDisplayedChanged(displayed);
	}
}
//! Get flag for displaying Equatorial J2000 Grid
bool GridLinesMgr::isGalacticGridDisplayed(void) const
{
	return galacticGrid->getFlagshow();
}
Vec3f GridLinesMgr::getGalacticGridColor(void) const
{
	return galacticGrid->getColor();
}
void GridLinesMgr::setGalacticGridColor(const Vec3f& newColor)
{
	if(newColor != galacticGrid->getColor()) {
		galacticGrid->setColor(newColor);
		emit galacticGridColorChanged(newColor);
	}
}

//! Set flag for displaying Equatorial Line
void GridLinesMgr::setEquatorLineDisplayed(const bool displayed)
{
	if(displayed != equatorLine->getFlagshow()) {
		equatorLine->setFlagshow(displayed);
		emit equatorLineDisplayedChanged(displayed);
	}
}
//! Get flag for displaying Equatorial Line
bool GridLinesMgr::isEquatorLineDisplayed(void) const
{
	return equatorLine->getFlagshow();
}
Vec3f GridLinesMgr::getEquatorLineColor(void) const
{
	return equatorLine->getColor();
}
void GridLinesMgr::setEquatorLineColor(const Vec3f& newColor)
{
	if(newColor != equatorLine->getColor()) {
		equatorLine->setColor(newColor);
		emit equatorLineColorChanged(newColor);
	}
}

//! Set flag for displaying Ecliptic Line
void GridLinesMgr::setEclipticLineDisplayed(const bool displayed)
{
	if(displayed != eclipticLine->getFlagshow()) {
		eclipticLine->setFlagshow(displayed);
		emit eclipticLineDisplayedChanged(displayed);
	}
}
//! Get flag for displaying Ecliptic Line
bool GridLinesMgr::isEclipticLineDisplayed(void) const
{
	return eclipticLine->getFlagshow();
}
Vec3f GridLinesMgr::getEclipticLineColor(void) const
{
	return eclipticLine->getColor();
}
void GridLinesMgr::setEclipticLineColor(const Vec3f& newColor)
{
	if(newColor != eclipticLine->getColor()) {
		eclipticLine->setColor(newColor);
		emit eclipticLineColorChanged(newColor);
	}
}


//! Set flag for displaying Meridian Line
void GridLinesMgr::setMeridianLineDisplayed(const bool displayed)
{
	if(displayed != meridianLine->getFlagshow()) {
		meridianLine->setFlagshow(displayed);
		emit meridianLineDisplayedChanged(displayed);
	}
}
//! Get flag for displaying Meridian Line
bool GridLinesMgr::isMeridianLineDisplayed(void) const
{
	return meridianLine->getFlagshow();
}
Vec3f GridLinesMgr::getMeridianLineColor(void) const
{
	return meridianLine->getColor();
}
void GridLinesMgr::setMeridianLineColor(const Vec3f& newColor)
{
	if(newColor != meridianLine->getColor()) {
		meridianLine->setColor(newColor);
		emit meridianLineColorChanged(newColor);
	}
}

//! Set flag for displaying Horizon Line
void GridLinesMgr::setHorizonLineDisplayed(const bool displayed)
{
	if(displayed != horizonLine->getFlagshow()) {
		horizonLine->setFlagshow(displayed);
		emit horizonLineDisplayedChanged(displayed);
	}
}
//! Get flag for displaying Horizon Line
bool GridLinesMgr::isHorizonLineDisplayed(void) const
{
	return horizonLine->getFlagshow();
}
Vec3f GridLinesMgr::getHorizonLineColor(void) const
{
	return horizonLine->getColor();
}
void GridLinesMgr::setHorizonLineColor(const Vec3f& newColor)
{
	if(newColor != horizonLine->getColor()) {
		horizonLine->setColor(newColor);
		emit horizonLineColorChanged(newColor);
	}
}

//! Set flag for displaying GalacticPlane Line
void GridLinesMgr::setGalacticPlaneLineDisplayed(const bool displayed)
{
	if(displayed != galacticPlaneLine->getFlagshow()) {
		galacticPlaneLine->setFlagshow(displayed);
		emit galacticPlaneLineDisplayedChanged(displayed);
	}
}
//! Get flag for displaying GalacticPlane Line
bool GridLinesMgr::isGalacticPlaneLineDisplayed(void) const
{
	return galacticPlaneLine->getFlagshow();
}
Vec3f GridLinesMgr::getGalacticPlaneLineColor(void) const
{
	return galacticPlaneLine->getColor();
}
void GridLinesMgr::setGalacticPlaneLineColor(const Vec3f& newColor)
{
	if(newColor != galacticPlaneLine->getColor()) {
		galacticPlaneLine->setColor(newColor);
		emit galacticPlaneLineColorChanged(newColor);
	}
}
