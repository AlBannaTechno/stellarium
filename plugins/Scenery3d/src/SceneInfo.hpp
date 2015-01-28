/*
 * Stellarium Scenery3d Plug-in
 *
 * Copyright (C) 2011 Simon Parzer, Peter Neubauer, Georg Zotti
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

#ifndef _SCENEINFO_HPP_
#define _SCENEINFO_HPP_

#include "StelLocation.hpp"
#include "VecMath.hpp"

#include <QMap>
#include <QSharedPointer>

class QSettings;

//! Contains all the metadata necessary for a Scenery3d scene,
//! and can be loaded from special .ini files in a scene's folder.
struct SceneInfo
{
	SceneInfo() : isValid(false),id(),name(),description(),location()
	{}
	//! If this is a valid sceneInfo object loaded from file
	bool isValid;
	//! ID of the scene (relative directory)
	QString id;
	//! The full path to the scene's folder. Other paths (model files) are to be seen relative to this path.
	QString fullPath;
	//! Name of the scene
	QString name;
	//! Author of the scene
	QString author;
	//! A description, which can be displayed in the GUI - supporting HTML tags!
	QString description;
	//! Copyright string
	QString copyright;
	//! The name of the landscape to switch to. The landscape's position is applied on loading.
	QString landscapeName;
	//! The file name of the scenery .obj model
	QString modelScenery;
	//! The file name of the optional seperate ground model (used as a heightmap for walking)
	QString modelGround;
	//! Optional string depicting vertex order of models (XYZ, ZXY, ...)
	QString vertexOrder;

	//! Optional more accurate location information, which will override the landscape's position.
	QSharedPointer<StelLocation> location;
	//! Optional initial look-at vector (azimuth, elevation and FOV in degrees)
	Vec3f lookAt_fov; // (az_deg, alt_deg, fov_deg)

	//! The height at which the observer's eyes are placed. Default 1.65.
	double eyeLevel;
	//! The name of the grid space for displaying the world positon
	QString gridName;
	//! Offset of the center of the model in a given grid space
	Vec3d modelWorldOffset;
	//! The world grid space offset where the observer is placed upon loading
	Vec3d startWorldOffset;
	//! Relative start position in model space, calculated from the world offset and start offset with Z rotation, etc.
	Vec3d relativeStartPosition;
	//! If true, it indicates that the model file's bounding box is used for altitude calculation.
	bool altitudeFromModel;
	//! If true, it indicates that the model file's bounding box is used for starting position calculation.
	bool startPositionFromModel;
	//! If true, it indicates that the model file's bounding box is used for starting height calculation.
	bool groundNullHeightFromModel;

	//! If only a non-georeferenced OBJ can be provided, you can specify a matrix via .ini/[model]/obj_world_trafo.
	//! This will be applied to make sure that X=Grid-East, Y=Grid-North, Z=height.
	Mat4d obj2gridMatrix;
	//! The vertical axis rotation that must be applied to the scene, for meridian convergence.
	//! This is calculated from other fields in the file.
	Mat4d zRotateMatrix;
	//! The height value outside the ground model's heightmap, or used if no ground model exists.
	double groundNullHeight;

	//! Threshold for cutout transparency (no blending). Default is 0.5f
	float transparencyThreshold;
	//! Recalculate normals of the scene from face normals? Default false.
	bool sceneryGenerateNormals;
	//! Recalculate normals of the ground from face normals? Default false.
	bool groundGenerateNormals;

	//! Returns true if the location object is valid
	bool hasLocation() const { return !location.isNull(); }
	//! Returns true if the lookat_fov is valid
	bool hasLookAtFOV() const { return lookAt_fov[2] >= 0; }

	//! The folder for scenery is found here
	static const QString SCENES_PATH;
	//! Loads the scene metadata associated with this ID (directory) into the given object. Returns true on success.
	static bool loadByID(const QString& id, SceneInfo &info);
	//! Convenience method that finds the ID for the given name and calls loadByID.
	static bool loadByName(const QString& name, SceneInfo& info);
	//! Returns the ID for the given scene name.
	//! If multiple scenes exist with the same name, the first one found is returned. If no scene is found with this name, an empty string is returned.
	static QString getIDFromName(const QString& name);
	//! Returns all available scene IDs
	static QStringList getAllSceneIDs();
	//! Returns all available scene names
	static QStringList getAllSceneNames();

	//! The meta type ID associated to the SceneInfo type
	static int metaTypeId;
private:
	//! Builds a mapping of available scene names to the folders they are contained in, similar to the LandscapeMgr's method
	static QMap<QString,QString> getNameToIDMap();
	static int initMetaType();
};

class StoredView;
typedef QList<StoredView> StoredViewList;

//! A structure which stores a specific view position, view direction and FOV, together with a textual description.
struct StoredView
{
	StoredView() : position(0,0,0), view_fov(0,0,-1000)
	{}

	//! A descriptive label
	QString label;
	//! A description of the view
	QString description;
	//! Stored grid position
	Vec3f position;
	//! Alt/Az angles in degrees + field of view
	Vec3f view_fov;
	//! True if this is a position stored next to the scene definition (viewpoints.ini). If false, this is a user-defined view (from userdir\stellarium\scenery3d\userviews.ini).
	bool isGlobal;

	//! Returns a list of all global views of a scene.
	//! If the scene is invalid, an empty list is returned.
	static StoredViewList getGlobalViewsForScene(const SceneInfo& scene);
	//! Returns a list of all user-generated views of a scene.
	//! If the scene is invalid, an empty list is returned.
	static StoredViewList getUserViewsForScene(const SceneInfo& scene);
private:
	static void readArray(QSettings& ini,StoredViewList& list, int size, bool isGlobal);
};

Q_DECLARE_METATYPE(SceneInfo)

#endif
