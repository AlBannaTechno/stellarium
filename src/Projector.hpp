/*
 * Stellarium
 * Copyright (C) 2003 Fabien Chereau
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

#ifndef _PROJECTOR_H_
#define _PROJECTOR_H_

#include <vector>
#include <map>

#include "GLee.h"
#include "vecmath.h"
#include "SFont.hpp"
#include "Mapping.hpp"
#include "SphereGeometry.hpp"

class InitParser;

//! @brief Class which handle projection in stellarium.

//! It overrides a number of openGL functions to enable non-linear projection, 
//! such as fisheye or stereographic projections.
//! This class also provide drawing primitives that are optimized according to the projection mode.
class Projector
{
public:
	//! Supported reference frame types
	enum FRAME_TYPE
	{
		FRAME_LOCAL,
		FRAME_HELIO,
		FRAME_EARTH_EQU,
		FRAME_J2000
	};
	
    ///////////////////////////////////////////////////////////////////////////
    // Main constructor
	Projector(const Vector4<GLint>& viewport, double _fov = 60.);
	~Projector();
	
	void init(const InitParser& conf);
	
	// Set the standard modelview matrices used for projection
	void set_modelview_matrices(const Mat4d& _mat_earth_equ_to_eye,
				    const Mat4d& _mat_helio_to_eye,
				    const Mat4d& _mat_local_to_eye,
				    const Mat4d& _mat_j2000_to_eye);	
	// Flags
	void setFlagGravityLabels(bool gravity) { gravityLabels = gravity; }
	bool getFlagGravityLabels() const { return gravityLabels; }

	
	//! Register a new projection mapping
	void registerProjectionMapping(Mapping *c);
	
	///////////////////////////////////////////////////////////////////////////
	// Methods for controlling viewport and mask 
	enum PROJECTOR_MASK_TYPE
	{
		DISK,
		NONE
	};
	
	static const char *maskTypeToString(PROJECTOR_MASK_TYPE type);
    static PROJECTOR_MASK_TYPE stringToMaskType(const string &s);
    
	//! Get the type of the mask if any
	PROJECTOR_MASK_TYPE getMaskType(void) const {return maskType;}
	void setMaskType(PROJECTOR_MASK_TYPE m) {maskType = m; }
	
	//! define viewport size, center(relative to lower left corner)
    //! and diameter of FOV disk
	void setViewport(int x, int y, int w, int h,
	                 double cx, double cy, double fov_diam);

	//! Get the lower left corner of the viewport and the width, height
	const Vector4<GLint>& getViewport(void) const {return viewport_xywh;}

	//! Get the center of the viewport relative to the lower left corner
	Vec2d getViewportCenter(void) const
	  {return Vec2d(viewport_center[0]-viewport_xywh[0],
	                viewport_center[1]-viewport_xywh[1]);}

	//! Get the diameter of the FOV disk
	double getViewportFovDiameter(void) const {return viewport_fov_diameter;}
	
	//! Get the horizontal viewport offset in pixels
	int getViewportPosX(void) const {return viewport_xywh[0];}
	
	//! Get the vertical viewport offset in pixels
	int getViewportPosY(void) const {return viewport_xywh[1];}
	
	//! Get the viewport size in pixels
	int getViewportWidth(void) const {return viewport_xywh[2];}
	int getViewportHeight(void) const {return viewport_xywh[3];}
	
	//! somehow the user has managed to resize the window
	void windowHasBeenResized(int width,int height);

	//! Return a polygon matching precisely the real viewport defined by the area on the screen 
	//! where projection is valid. Normally, nothing should be drawn outside this area.
	//! This viewport is usually the rectangle defined by the screen, but in case of non-linear
	//! projection, it can also be a more complex shape.
	std::vector<Vec2d> getViewportVertices2d() const;
	
	//! Return a convex polygon on the sphere which includes the viewport in the current frame
	//! @param margin an extra margin in pixel which extends the polygon size
	StelGeom::ConvexPolygon getViewportConvexPolygon(double margin=0.) const;

	//! unproject the entire viewport depending on mapping, maskType,
	//! viewport_fov_diameter, viewport_center, viewport_xywh
	StelGeom::ConvexS unprojectViewport(void) const;

	//! Set whether a disk mask must be drawn over the viewport
	void setViewportMaskDisk(void) {setMaskType(Projector::DISK);}
	//! Get whether a disk mask must be drawn over the viewport
	bool getViewportMaskDisk(void) const {return getMaskType()==Projector::DISK;}

	//! Set whether no mask must be drawn over the viewport
	void setViewportMaskNone(void) {setMaskType(Projector::NONE);}
	
	//! Set the current openGL viewport to projector's viewport
	void applyViewport(void) const {glViewport(viewport_xywh[0], viewport_xywh[1], viewport_xywh[2], viewport_xywh[3]);}	
	
	void set_clipping_planes(double znear, double zfar);
	void get_clipping_planes(double* zn, double* zf) const {*zn = zNear; *zf = zFar;}
	
	///////////////////////////////////////////////////////////////////////////
	// Methods for controlling the PROJECTION matrix
	bool getFlipHorz(void) const {return (flip_horz < 0.0);}
	bool getFlipVert(void) const {return (flip_vert < 0.0);}
	void setFlipHorz(bool flip) {
		flip_horz = flip ? -1.0 : 1.0;
		glFrontFace(needGlFrontFaceCW()?GL_CW:GL_CCW); 
	}
	void setFlipVert(bool flip) {
		flip_vert = flip ? -1.0 : 1.0;
		glFrontFace(needGlFrontFaceCW()?GL_CW:GL_CCW); 
	}
	bool needGlFrontFaceCW(void) const
		{return (flip_horz*flip_vert < 0.0);}

	//! Get whether the GL_POINT_SPRITE extension is available now
	bool getflagGlPointSprite() const {return flagGlPointSprite;}

	//! Set the Field of View in degree
	void setFov(double f);
	//! Get the Field of View in degree
	double getFov(void) const {return fov;}
	
	//! Get the average number of pixel per radian
	double getPixelPerRad(void) const {return MY_MIN(getViewportWidth(),getViewportHeight())/(fov*M_PI/180.);}

	//! Set the maximum Field of View in degree
	void setMaxFov(double max);
	//! Get the maximum Field of View in degree
	double getMaxFov(void) const {return max_fov;}
	//! Return the initial default FOV in degree
	double getInitFov() const {return initFov;}
	

	///////////////////////////////////////////////////////////////////////////
	// Full projection methods
	// Return true if the 2D pos is inside the viewport
	bool check_in_viewport(const Vec3d& pos) const
		{return (pos[1]>=viewport_xywh[1] && pos[0]>=viewport_xywh[0] &&
		pos[1]<=(viewport_xywh[1] + viewport_xywh[3]) && pos[0]<=(viewport_xywh[0] + viewport_xywh[2]));}

	//! Project the vector v from the current frame into the viewport
	//! @param v the vector in the current frame
	//! @param win the projected vector in the viewport 2D frame
	//! @return true if the projected coordinate is valid
	bool project(const Vec3d& v, Vec3d& win) const
	{
		// really important speedup:
		win[0] = modelViewMatrix.r[0]*v[0] + modelViewMatrix.r[4]*v[1]
				+ modelViewMatrix.r[8]*v[2] + modelViewMatrix.r[12];
		win[1] = modelViewMatrix.r[1]*v[0] + modelViewMatrix.r[5]*v[1]
				+ modelViewMatrix.r[9]*v[2] + modelViewMatrix.r[13];
		win[2] = modelViewMatrix.r[2]*v[0] + modelViewMatrix.r[6]*v[1]
				+ modelViewMatrix.r[10]*v[2] + modelViewMatrix.r[14];
		const bool rval = mapping->forward(win);
		// very important: even when the projected point comes from an
		// invisible region of the sky (rval=false), we must finish
		// reprojecting, so that OpenGl can successfully eliminate
		// polygons by culling.
		win[0] = viewport_center[0] + flip_horz * view_scaling_factor * win[0];
		win[1] = viewport_center[1] + flip_vert * view_scaling_factor * win[1];
		win[2] = (win[2] - zNear) / (zNear - zFar);
		return rval;
	}

	//! Project the vector v from the current frame into the viewport
	//! @param v the vector in the current frame
	//! @param win the projected vector in the viewport 2D frame
	//! @return true if the projected point is inside the viewport
	bool projectCheck(const Vec3d& v, Vec3d& win) const {return (project(v, win) && check_in_viewport(win));}

	//! Project the vector v from the viewport frame into the current frame 
	//! @param win the vector in the viewport 2D frame
	//! @param v the projected vector in the current frame
	//! @return true if the projected coordinate is valid
	bool unProject(const Vec3d& win, Vec3d& v) const {return unProject(win[0], win[1], v);}
	bool unProject(double x, double y, Vec3d& v) const;

	const Mapping &getMapping(void) const {return *mapping;}

	//! Project the vectors v1 and v2 from the current frame into the viewport.
	//! @param v1 the first vector in the current frame
	//! @param v2 the second vector in the current frame
	//! @param win1 the first projected vector in the viewport 2D frame
	//! @param win2 the second projected vector in the viewport 2D frame
	//! @return true if at least one of the projected vector is within the viewport
	bool projectLineCheck(const Vec3d& v1, Vec3d& win1, const Vec3d& v2, Vec3d& win2) const
		{return project(v1, win1) && project(v2, win2) && (check_in_viewport(win1) || check_in_viewport(win2));}

	//! Set the frame in which we want to draw from now on
	//! The frame will be the current one until this method or setCustomFrame is called again
	//! @param frameType the type
	void setCurrentFrame(FRAME_TYPE frameType) const;

	//! Set a custom model view matrix, it is valid until the next call to setCurrentFrame or setCustomFrame
	//! @param the openGL MODELVIEW matrix to use
	void setCustomFrame(const Mat4d&) const;

	//! Set the current projection mapping to use
	//! The mapping must have been registered before beeing used
	//! @param projectionName a string which can be e.g. "perspective", "stereographic", "fisheye", "cylinder"
	void setCurrentProjection(const std::string& projectionName);
	
	//! Get the current projection mapping name
	std::string getCurrentProjection() {return currentProjectionType;}


	///////////////////////////////////////////////////////////////////////////
	// Standard methods for drawing primitives in general (non-linear) mode
	///////////////////////////////////////////////////////////////////////////

	// Fill with black around the viewport
	void draw_viewport_shape(void);
	
	//! Generalisation of glVertex3v for non-linear projections. This method does not
	//! manage the lighting operations properly.
	void drawVertex3v(const Vec3d& v) const;
	void drawVertex3(double x, double y, double z) const {drawVertex3v(Vec3d(x, y, z));}

	//! Draw the string at the given position and angle with the given font
	//! @param x horizontal position of the lower left corner of the first character of the text in pixel
	//! @param y horizontal position of the lower left corner of the first character of the text in pixel
	//! @param str the text to print
	//! @param angleDeg rotation angle in degree. Rotation is around x,y
	//! @param xshift shift in pixel in the rotated x direction
	//! @param yshift shift in pixel in the rotated y direction
	//! @param noGravity don't take into account the fact that the text should be written with gravity
	void drawText(const SFont* font, float x, float y, const string& str, float angleDeg=0.f, float xshift=0.f, float yshift=0.f, bool noGravity=true) const
		{drawText(font, x, y, StelUtils::stringToWstring(str), angleDeg, xshift, yshift);}
	void drawText(const SFont* font, float x, float y, const wstring& str, float angleDeg=0.f, float xshift=0.f, float yshift=0.f, bool noGravity=true) const;

	//! Draw a parallel arc in the current frame, starting from point start
	//!  going in the positive longitude direction and with the given length in radian.
	//! @param start the starting position of the parallel in the current frame
	//! @param length the angular length in radian (or distance on the unit sphere)
	//! @param labelAxis if true display a label indicating the latitude at begining and at the end of the arc
	//! @param textColor color to use for rendering text. If NULL use the current openGL painting color.
	//! @param nbSeg if not==-1,indicate how many line segments should be used for drawing the arc, if==-1
	//!   this value is automatically adjusted to prevent seeing the curve as a polygon
	void drawParallel(const Vec3d& start, double length, bool labelAxis=false, const SFont* font=NULL, const Vec4f* textColor=NULL, int nbSeg=-1) const;
	
	//! Draw a meridian arc in the current frame, starting from point start
	//!  going in the positive latitude direction if longitude is in [0;180], in the negative direction
	//!  otherwise, and with the given length in radian. The length can be up to 2 pi.
	//! @param start the starting position of the meridian in the current frame
	//! @param length the angular length in radian (or distance on the unit sphere)
	//! @param labelAxis if true display a label indicating the longitude at begining and at the end of the arc
	//! @param textColor color to use for rendering text. If NULL use the current openGL painting color.
	//! @param nbSeg if not==-1,indicate how many line segments should be used for drawing the arc, if==-1
	//!  this value is automatically adjusted to prevent seeing the curve as a polygon
	void drawMeridian(const Vec3d& start, double length, bool labelAxis=false, const SFont* font=NULL, const Vec4f* textColor=NULL, int nbSeg=-1) const;
	
	//! Draw a square using the current texture at the given projected 2d position
	//! @param x x position in the viewport in pixel
	//! @param y y position in the viewport in pixel
	//! @param size the size of a square side in pixel
	void drawSprite2dMode(double x, double y, double size) const;
	
	//! Draw a rotated square using the current texture at the given projected 2d position
	//! @param x x position in the viewport in pixel
	//! @param y y position in the viewport in pixel
	//! @param size the size of a square side in pixel
	//! @param rotation rotation angle in degree
	void drawSprite2dMode(double x, double y, double size, double rotation) const;
	
	//! Draw a rotated rectangle using the current texture at the given projected 2d position
	//! @param x x position in the viewport in pixel
	//! @param y y position in the viewport in pixel
	//! @param sizex the size of the rectangle x side in pixel
	//! @param sizey the size of the rectangle y side in pixel
	//! @param rotation rotation angle in degree
	void drawRectSprite2dMode(double x, double y, double sizex, double sizey, double rotation) const;
	
	//! Draw a GL_POINT at the given position
	//! @param x x position in the viewport in pixel
	//! @param y y position in the viewport in pixel
	void drawPoint2d(double x, double y) const;
	
	// Reimplementation of gluSphere : glu is overrided for non standard projection
	void sSphere(GLdouble radius, GLdouble one_minus_oblateness, GLint slices, GLint stacks, int orient_inside = 0) const;

	// Reimplementation of gluCylinder : glu is overrided for non standard projection
	void sCylinder(GLdouble radius, GLdouble height, GLint slices, GLint stacks, int orient_inside = 0) const;

	// Draw a disk with a special texturing mode having texture center at center
	// The disk is made up of concentric circles with increasing refinement.
	// The number of slices of the outmost circle is (inner_fan_slices<<level).
	void sFanDisk(double radius,int inner_fan_slices,int level) const;

	// Draw a disk with a special texturing mode having texture center at center
	void sDisk(GLdouble radius, GLint slices, GLint stacks, int orient_inside = 0) const;	
		
	// Draw a ring with a radial texturing
	void sRing(GLdouble r_min, GLdouble r_max, GLint slices, GLint stacks, int orient_inside) const;

	// Draw a fisheye texture in a sphere
	void sSphere_map(GLdouble radius, GLint slices, GLint stacks, double texture_fov = 2.*M_PI, int orient_inside = 0) const;
	
	///////////////////////////////////////////////////////////////////////////
	// Methods for linear mode
	///////////////////////////////////////////////////////////////////////////
	
	// Reimplementation of gluCylinder
	void sCylinderLinear(GLdouble radius, GLdouble height, GLint slices, GLint stacks, int orient_inside = 0) const;
	
	// Reimplementation of gluSphere
	void sSphereLinear(GLdouble radius, GLdouble one_minus_oblateness, GLint slices, GLint stacks, int orient_inside = 0) const;


private:

	void drawTextGravity180(const SFont* font, float x, float y, const wstring& str, 
			      bool speed_optimize = 1, float xshift = 0, float yshift = 0) const;
		
	//! Init the real openGL Matrices to a 2d orthographic projection
	void initGlMatrixOrtho2d(void) const;
	
	//! The current projector mask
	PROJECTOR_MASK_TYPE maskType;

	double initFov;				// initial default FOV in degree
	double fov;					// Field of view in degree
	double min_fov;				// Minimum fov in degree
	double max_fov;				// Maximum fov in degree
	double zNear, zFar;			// Near and far clipping planes

	Vector4<GLint> viewport_xywh;	// Viewport parameters
	Vec2d viewport_center;		// Viewport center in screen pixel
	double viewport_fov_diameter;	// diameter of a circle with 180 degrees diameter in screen pixel

	Mat4d projectionMatrix;		// Projection matrix

	double view_scaling_factor;	// ??
	double flip_horz,flip_vert;	// Whether to flip in horizontal or vertical directions

	Mat4d mat_earth_equ_to_eye;		// Modelview Matrix for earth equatorial projection
	Mat4d mat_j2000_to_eye;         // for precessed equ coords
	Mat4d mat_helio_to_eye;			// Modelview Matrix for earth equatorial projection
	Mat4d mat_local_to_eye;			// Modelview Matrix for earth equatorial projection
	
	bool gravityLabels;				// should label text align with the horizon?
	
	bool flagGlPointSprite;			// Define whether glPointSprite is activated
	
	mutable Mat4d modelViewMatrix;			// openGL MODELVIEW Matrix
	
	const Mapping *mapping;
	std::map<std::string,const Mapping*> projectionMapping;
	
	std::string currentProjectionType;	// Type of the projection currently used
};

#endif // _PROJECTOR_H_
