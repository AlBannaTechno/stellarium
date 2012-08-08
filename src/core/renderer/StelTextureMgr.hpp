/*
 * Stellarium
 * Copyright (C) 2006 Fabien Chereau
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

#ifndef _STELTEXTUREMGR_HPP_
#define _STELTEXTUREMGR_HPP_

#include <QObject>

#include "StelTexture.hpp"

class QNetworkReply;
class QThread;


//! @class StelTextureMgr
//! Manage texture loading.
//!
//! Note that this class is OBSOLETE!
//! StelRenderer and StelTextureNew should be used instead of StelTextureMgr and StelTexture.
//!
//! Provides a method for loading images in a separate thread.
class StelTextureMgr : QObject
{
public:
	explicit StelTextureMgr(class StelRenderer* renderer);
	virtual ~StelTextureMgr();

	//! Initialize some variable from the openGL contex.
	//! Must be called after the creation of the GLContext.
	void init();

	//! Load an image from a file and create a new texture from it
	//! @param filename the texture file name, can be absolute path if starts with '/' otherwise
	//!    the file will be looked in stellarium standard textures directories.
	//! @param params the texture creation parameters.
	//! @return NULL if filename is empty or on failure.
	StelTextureSP createTexture(const QString& filename, const TextureParams& params=TextureParams());

	//! Load an image from a file and create a new texture from it in a new thread.
	//! @param url the texture file name or URL, can be absolute path if starts with '/' otherwise
	//!    the file will be looked in stellarium standard textures directories.
	//! @param params the texture creation parameters.
	//! @param lazyLoading define whether the texture should be actually loaded only when needed, i.e. when bind() is called the first time.
	//! @return NULL if filename is empty or on failure.
	StelTextureSP createTextureThread(const QString& url, const TextureParams& params=TextureParams(), bool lazyLoading=true);

private:
	//! A thread used by the TextureLoader object to avoid pausing the main thread too long.
	QThread* loaderThread;

	//! StelRenderer used to handle platform-specific texture functionality.
	class StelRenderer* renderer;
};

#endif // _STELTEXTUREMGR_HPP_
