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

#ifndef _NEBULA_H_
#define _NEBULA_H_

#include "StelObject.hpp"
#include "Projector.hpp"
#include "Navigator.hpp"
#include "Translator.hpp"
#include "STextureTypes.hpp"

class SFont;
class ToneReproducer;

/*
Gx Galaxy
OC Open star cluster
Gb Globular star cluster, usually in the Milky Way Galaxy
Nb Bright emission or reflection nebula
Pl Planetary nebula
C+N Cluster associated with nebulosity
Ast Asterism or group of a few stars
Kt Knot or nebulous region in an external galaxy
*** Triple star
D* Double star
* Single star
? Uncertain type or may not exist
blank Unidentified at the place given, or type unknown
- Object called nonexistent in the RNGC (Sulentic and Tifft 1973)
PD Photographic plate defect
*/


class Nebula : public StelObject
{
friend class NebulaMgr;
public:
	enum nebula_type { NEB_GX, NEB_OC, NEB_GC, NEB_N, NEB_PN, NEB_DN, NEB_IG, NEB_CN, NEB_UNKNOWN };

    Nebula();
    ~Nebula();

	wstring getInfoString(const Navigator * nav) const;
	wstring getShortInfoString(const Navigator * nav = NULL) const;
	std::string getType(void) const {return "Nebula";}
	Vec3d getEarthEquatorialPos(const Navigator *nav) const {return nav->j2000_to_earth_equ(XYZ);}
	// observer centered J2000 coordinates
	Vec3d getObsJ2000Pos(const Navigator *nav) const {return XYZ;}
	double getCloseViewFov(const Navigator * nav = NULL) const;
	float getMagnitude(const Navigator * nav = NULL) const {return mag;}
	float getSelectPriority(const Navigator *nav) const;

	virtual Vec3f getInfoColor(void) const;

	void setLabelColor(const Vec3f& v) {label_color = v;}
	void setCircleColor(const Vec3f& v) {circle_color = v;}

	//! reads a texture into memory
	//! read a nebula texture for a given nebula set and record of the
	//! nebula_textures.fab file for that set.
	//! @param setName The string name of the set of nebulas being processed
	//! @param record The string containing the current record from nebula_textures.fab
	bool readTexture(const string& setName, const string& record);
	bool readNGC(char *record);

	wstring getNameI18n(void) const {return nameI18;}
	string getEnglishName(void) const {return englishName;}
    
	//! @brief Get the printable nebula Type
	//! @return the nebula type code.
	wstring getTypeString(void) const;

	//! Translate nebula name using the passed translator
	void translateName(Translator& trans) {nameI18 = trans.translate(englishName);}
	
	virtual float getOnScreenSize(const Projector *prj, const Navigator *nav = NULL) const
	{
		return angular_size * (prj->getViewportHeight()/prj->getFov());
	}

	//! Get the convex polygon matching the nebula image in J2000 frame
	StelGeom::ConvexPolygon getConvexPolygon() {return StelGeom::ConvexPolygon(tex_quad_vertex[0], tex_quad_vertex[1], tex_quad_vertex[2], tex_quad_vertex[3]);}
	
private:
	void draw_chart(const Projector* prj, const Navigator * nav);
	void draw_tex(const Projector* prj, const Navigator * nav, ToneReproducer* eye);
	void draw_no_tex(const Projector* prj, const Navigator * nav, ToneReproducer* eye);
    void draw_name(const Projector* prj);
    void draw_circle(const Projector* prj, const Navigator * nav);
    bool hasTex(void) {return neb_tex;}
    
	unsigned int M_nb;			// Messier Catalog number
	unsigned int NGC_nb;			// New General Catalog number
	unsigned int IC_nb;			// Index Catalog number
	/*unsigned int UGC_nb;			// Uppsala General  Catalog number*/
	string englishName;				// English name
	wstring nameI18;				// Nebula name
	string credit;					// Nebula image credit
	float mag;						// Apparent magnitude
	float angular_size;				// Angular size in degree
	Vec3f XYZ;						// Cartesian equatorial position
	Vec3d XY;						// Store temporary 2D position
	nebula_type nType;

	ManagedSTextureSP neb_tex;			// Texture
	Vec3f tex_quad_vertex[4];		// The 4 vertex used to draw the nebula texture
	float luminance;			// Object luminance to use (value computed to compensate
						// the texture average luminosity)
	
	static STextureSP tex_circle;	// The symbolic circle texture
	static SFont* nebula_font;		// Font used for names printing
	static float hints_brightness;

	static Vec3f label_color, circle_color;
	static float circleScale;		// Define the scaling of the hints circle
	static bool flagBright;			// Define if nebulae must be drawn in bright mode
	static bool flagShowTexture;	// Define if textures must be drawn
	
	static const float RADIUS_NEB;
};

#endif // _NEBULA_H_
