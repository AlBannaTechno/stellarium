/*
Copyright (C) 2015 Alexander Wolf

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Library General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
*/

#ifndef _ABERRATION_HPP_
#define _ABERRATION_HPP_

#include "VecMath.hpp"
#include "StelProjector.hpp"

class Aberration: public StelProjector::ModelViewTranform
{
public:
	Aberration();

	//! Apply aberration.
	//! @param equPos
	void forward(Vec3d& equPos) const;

	//! Remove aberration from position ("reduce").
	//! @param equPos
	void backward(Vec3d& equPos) const;

	//! Apply aberration.
	//! @param equPos
	void forward(Vec3f& equPos) const;

	//! Remove aberration from position ("reduce").
	//! @param equPos
	void backward(Vec3f& equPos) const;

	void combine(const Mat4d& m)
	{
		setPreTransfoMat(preTransfoMat*m);
	}

	Mat4d getApproximateLinearTransfo() const {return postTransfoMat*preTransfoMat;}

	StelProjector::ModelViewTranformP clone() const {Aberration* aber = new Aberration(); *aber=*this; return StelProjector::ModelViewTranformP(aber);}

	//! Set the transformation matrices used to transform input vector to Equ frame.
	void setPreTransfoMat(const Mat4d& m);
	void setPostTransfoMat(const Mat4d& m);

private:
	void innerAberrationForward(Vec3d& equPos) const;
	void innerAberrationBackward(Vec3d& equPos) const;

	//! Used to pretransform coordinates into Equ frame.
	Mat4d preTransfoMat;
	Mat4d invertPreTransfoMat;
	Mat4f preTransfoMatf;
	Mat4f invertPreTransfoMatf;

	//! Used to postransform refracted coordinates from Equ to view.
	Mat4d postTransfoMat;
	Mat4d invertPostTransfoMat;
	Mat4f postTransfoMatf;
	Mat4f invertPostTransfoMatf;
};

#endif  // _ABERRATION_HPP_
