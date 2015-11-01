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

#include "Aberration.hpp"
#include "StelUtils.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"

static const int TERMS=36;

// Data structures to hold arguments and coefficients of Ron-Vondrak theory
struct arg
{
	double L2;
	double L3;
	double L4;
	double L5;
	double L6;
	double L7;
	double L8;
	double LL;
	double D;
	double MM;
	double F;
};

struct XYZ
{
	double sin1;
	double sin2;
	double cos1;
	double cos2;
};

// Table 23.A; argument column (p. 154-155)
static const struct arg arguments[TERMS] =
{
      //L2  L3  L4  L5  L6  L7  L8  L'   D  M'   F
	{0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0},
	{0,  2,  0,  0,  0,  0,  0,  0,  0,  0,  0},
	{0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0},
	{0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0},
	{0,  3,  0,  0,  0,  0,  0,  0,  0,  0,  0},
	{0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0},
	{0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1},
	{0,  0,  0,  0,  0,  0,  0,  1,  0,  1,  0},
	{0,  0,  0,  2,  0,  0,  0,  0,  0,  0,  0},
	{0,  2,  0, -1,  0,  0,  0,  0,  0,  0,  0},
	{0,  3, -8,  3,  0,  0,  0,  0,  0,  0,  0},
	{0,  5, -8,  3,  0,  0,  0,  0,  0,  0,  0},
	{2, -1,  0,  0,  0,  0,  0,  0,  0,  0,  0},
	{1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
	{0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0},
	{0,  1,  0, -2,  0,  0,  0,  0,  0,  0,  0},
	{0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0},
	{0,  1,  0,  1,  0,  0,  0,  0,  0,  0,  0},
	{2, -2,  0,  0,  0,  0,  0,  0,  0,  0,  0},
	{0,  1,  0, -1,  0,  0,  0,  0,  0,  0,  0},
	{0,  4,  0,  0,  0,  0,  0,  0,  0,  0,  0},
	{0,  3,  0, -2,  0,  0,  0,  0,  0,  0,  0},
	{1, -2,  0,  0,  0,  0,  0,  0,  0,  0,  0},
	{2, -3,  0,  0,  0,  0,  0,  0,  0,  0,  0},
	{0,  0,  0,  0,  2,  0,  0,  0,  0,  0,  0},
	{2,  4,  0,  0,  0,  0,  0,  0,  0,  0,  0},
	{0,  3, -2,  0,  0,  0,  0,  0,  0,  0,  0},
	{0,  0,  0,  0,  0,  0,  0,  1,  2, -1,  0},
	{8, 12,  0,  0,  0,  0,  0,  0,  0,  0,  0},
	{8, 14,  0,  0,  0,  0,  0,  0,  0,  0,  0},
	{0,  0,  2,  0,  0,  0,  0,  0,  0,  0,  0},
	{3,  4,  0,  0,  0,  0,  0,  0,  0,  0,  0},
	{0,  2,  0, -2,  0,  0,  0,  0,  0,  0,  0},
	{3, -3,  0,  0,  0,  0,  0,  0,  0,  0,  0},
	{0,  2, -2,  0,  0,  0,  0,  0,  0,  0,  0},
	{0,  0,  0,  0,  0,  0,  0,  1, -2,  0,  0}
};

// Table 23.A; X' columns (p. 154-155)
static const struct XYZ x_coefficients[TERMS] =
{
      //      X' sin   ,     X'  cos
	{-1719914,   -2,     -25,     0},
	{    6434,  141,   28007,  -107},
	{     715,    0,       0,     0},
	{     715,    0,       0,     0},
	{     486,   -5,    -236,    -4},
	{     159,    0,       0,     0},
	{       0,    0,       0,     0},
	{      39,    0,       0,     0},
	{      33,    0,     -10,     0},
	{      31,    0,       1,     0},
	{       8,    0,     -28,     0},
	{       8,    0,     -28,     0},
	{      21,    0,       0,     0},
	{     -19,    0,       0,     0},
	{      17,    0,       0,     0},
	{      16,    0,       0,     0},
	{      16,    0,       0,     0},
	{      11,    0,      -1,     0},
	{       0,    0,     -11,     0},
	{     -11,    0,      -2,     0},
	{      -7,    0,      -8,     0},
	{     -10,    0,       0,     0},
	{      -9,    0,       0,     0},
	{      -9,    0,       0,     0},
	{       0,    0,      -9,     0},
	{       0,    0,      -9,     0},
	{       8,    0,       0,     0},
	{       8,    0,       0,     0},
	{      -4,    0,      -7,     0},
	{      -4,    0,      -7,     0},
	{      -6,    0,      -5,     0},
	{      -1,    0,      -1,     0},
	{       4,    0,      -6,     0},
	{       0,    0,      -7,     0},
	{       5,    0,      -5,     0},
	{       5,    0,       0,     0}
};

// Table 23.A; Y' columns (p. 154-155)
static const struct XYZ y_coefficients[TERMS] =
{
	{      25,  -13, 1578089,   156},
	{   25697,  -95,   -5904,  -130},
	{       6,    0,    -657,     0},
	{       0,    0,    -656,     0},
	{    -216,   -4,    -446,     5},
	{       2,    0,    -147,     0},
	{       0,    0,      26,     0},
	{       0,    0,     -36,     0},
	{      -9,    0,     -30,     0},
	{       1,    0,     -28,     0},
	{      25,    0,       8,     0},
	{     -25,    0,      -8,     0},
	{       0,    0,     -19,     0},
	{       0,    0,      17,     0},
	{       0,    0,     -16,     0},
	{       0,    0,      15,     0},
	{       1,    0,     -15,     0},
	{      -1,    0,     -10,     0},
	{     -10,    0,       0,     0},
	{      -2,    0,       9,     0},
	{      -8,    0,       6,     0},
	{       0,    0,       9,     0},
	{       0,    0,      -9,     0},
	{       0,    0,      -8,     0},
	{      -8,    0,       0,     0},
	{       8,    0,       0,     0},
	{       0,    0,      -8,     0},
	{       0,    0,      -7,     0},
	{      -6,    0,      -4,     0},
	{       6,    0,      -4,     0},
	{      -4,    0,       5,     0},
	{      -2,    0,      -7,     0},
	{      -5,    0,      -4,     0},
	{      -6,    0,       0,     0},
	{      -4,    0,      -5,     0},
	{       0,    0,      -5,     0}
};

// Table 23.A; Z' columns (p. 154-155)
static const struct XYZ z_coefficients[TERMS] =
{
	{      10,   32,  684185,  -358},
	{   11141,  -48,   -2559,   -55},
	{     -15,    0,    -282,     0},
	{       0,    0,    -285,     0},
	{     -94,    0,    -193,     0},
	{      -6,    0,     -61,     0},
	{       0,    0,      59,     0},
	{       0,    0,      16,     0},
	{      -5,    0,     -13,     0},
	{       0,    0,     -12,     0},
	{      11,    0,       3,     0},
	{     -11,    0,      -3,     0},
	{       0,    0,      -8,     0},
	{       0,    0,       8,     0},
	{       0,    0,      -7,     0},
	{       1,    0,       7,     0},
	{      -3,    0,      -6,     0},
	{      -1,    0,       5,     0},
	{      -4,    0,       0,     0},
	{      -1,    0,       4,     0},
	{      -3,    0,       3,     0},
	{       0,    0,       4,     0},
	{       0,    0,      -4,     0},
	{       0,    0,      -4,     0},
	{      -3,    0,       0,     0},
	{       3,    0,       0,     0},
	{       0,    0,      -3,     0},
	{       0,    0,      -3,     0},
	{      -3,    0,       2,     0},
	{       3,    0,      -2,     0},
	{      -2,    0,       2,     0},
	{       1,    0,      -4,     0},
	{      -2,    0,      -2,     0},
	{      -3,    0,       0,     0},
	{      -2,    0,      -2,     0},
	{       0,    0,      -2,     0}
};

Aberration::Aberration()
	: preTransfoMat(Mat4d::identity()), invertPreTransfoMat(Mat4d::identity()),
	  preTransfoMatf(Mat4f::identity()), invertPreTransfoMatf(Mat4f::identity()),
	  postTransfoMat(Mat4d::identity()), invertPostTransfoMat(Mat4d::identity()),
	  postTransfoMatf(Mat4f::identity()), invertPostTransfoMatf(Mat4f::identity())
{

}

void Aberration::setPreTransfoMat(const Mat4d& m)
{
	preTransfoMat=m;
	invertPreTransfoMat=m.inverse();
	preTransfoMatf.set(m[0], m[1], m[2], m[3], m[4], m[5], m[6], m[7], m[8], m[9], m[10], m[11], m[12], m[13], m[14], m[15]);
	invertPreTransfoMatf.set(invertPreTransfoMat[0], invertPreTransfoMat[1], invertPreTransfoMat[2], invertPreTransfoMat[3],
				 invertPreTransfoMat[4], invertPreTransfoMat[5], invertPreTransfoMat[6], invertPreTransfoMat[7],
				 invertPreTransfoMat[8], invertPreTransfoMat[9], invertPreTransfoMat[10], invertPreTransfoMat[11],
				 invertPreTransfoMat[12], invertPreTransfoMat[13], invertPreTransfoMat[14], invertPreTransfoMat[15]);
}

void Aberration::setPostTransfoMat(const Mat4d& m)
{
	postTransfoMat=m;
	invertPostTransfoMat=m.inverse();
	postTransfoMatf.set(m[0], m[1], m[2], m[3], m[4], m[5], m[6], m[7], m[8], m[9], m[10], m[11], m[12], m[13], m[14], m[15]);
	invertPostTransfoMatf.set(invertPostTransfoMat[0], invertPostTransfoMat[1], invertPostTransfoMat[2], invertPostTransfoMat[3],
				  invertPostTransfoMat[4], invertPostTransfoMat[5], invertPostTransfoMat[6], invertPostTransfoMat[7],
				  invertPostTransfoMat[8], invertPostTransfoMat[9], invertPostTransfoMat[10], invertPostTransfoMat[11],
				  invertPostTransfoMat[12], invertPostTransfoMat[13], invertPostTransfoMat[14], invertPostTransfoMat[15]);
}

void Aberration::innerAberrationForward(Vec3d& equPos) const
{
	const double JDE = StelApp::getInstance().getCore()->getJDE();
	double A, RA, Dec, correctedRA, correctedDec;
	int i;
	// speed of light in 10-8 au per day
	double c = 17314463350.0;

	// calculate T
	double T = (JDE - 2451545.0) / 36525.0;

	// calculate planetary perturbutions
	double L2 = 3.1761467 + 1021.3285546 * T;
	double L3 = 1.7534703 + 628.3075849 * T;
	double L4 = 6.2034809 + 334.0612431 * T;
	double L5 = 0.5995464 + 52.9690965 * T;
	double L6 = 0.8740168 + 21.329909095 * T;
	double L7 = 5.4812939 + 7.4781599 * T;
	double L8 = 5.3118863 + 3.8133036 * T;
	double LL = 3.8103444 + 8399.6847337 * T;
	double D = 5.1984667 + 7771.3771486 * T;
	double MM = 2.3555559 + 8328.6914289 * T;
	double F = 1.6279052 + 8433.4661601 * T;

	double X = 0;
	double Y = 0;
	double Z = 0;

	// sum the terms
	for (i=0; i<TERMS; i++)
	{
		A = arguments[i].L2 * L2 + arguments[i].L3 * L3 + arguments[i].L4 * L4 + arguments[i].L5 * L5 +
		    arguments[i].L6 * L6 + arguments[i].L7 * L7 + arguments[i].L8 * L8 + arguments[i].LL * LL +
		    arguments[i].D * D   + arguments[i].MM * MM + arguments[i].F * F;

		X += (x_coefficients[i].sin1 + x_coefficients[i].sin2 * T) * sin (A) + (x_coefficients[i].cos1 + x_coefficients[i].cos2 * T) * cos (A);
		Y += (y_coefficients[i].sin1 + y_coefficients[i].sin2 * T) * sin (A) + (y_coefficients[i].cos1 + y_coefficients[i].cos2 * T) * cos (A);
		Z += (z_coefficients[i].sin1 + z_coefficients[i].sin2 * T) * sin (A) + (z_coefficients[i].cos1 + z_coefficients[i].cos2 * T) * cos (A);
	}

	StelUtils::rectToSphe(&RA, &Dec, equPos);

	correctedRA  =  RA + (Y * cos(RA) - X * sin(RA)) / (c * cos(Dec));
	correctedDec = Dec + ((X * cos(RA) + Y * sin(RA)) * sin(Dec) - Z * cos(Dec))/-c;

	StelUtils::spheToRect(correctedRA, correctedDec, equPos);
}

void Aberration::innerAberrationBackward(Vec3d& equPos) const
{
	const double JDE = StelApp::getInstance().getCore()->getJDE();
	double A, RA, Dec, correctedRA, correctedDec;
	int i;
	// speed of light in 10-8 au per day
	double c = 17314463350.0;

	// calculate T
	double T = (JDE - 2451545.0) / 36525.0;

	// calculate planetary perturbutions
	double L2 = 3.1761467 + 1021.3285546 * T;
	double L3 = 1.7534703 + 628.3075849 * T;
	double L4 = 6.2034809 + 334.0612431 * T;
	double L5 = 0.5995464 + 52.9690965 * T;
	double L6 = 0.8740168 + 21.329909095 * T;
	double L7 = 5.4812939 + 7.4781599 * T;
	double L8 = 5.3118863 + 3.8133036 * T;
	double LL = 3.8103444 + 8399.6847337 * T;
	double D = 5.1984667 + 7771.3771486 * T;
	double MM = 2.3555559 + 8328.6914289 * T;
	double F = 1.6279052 + 8433.4661601 * T;

	double X = 0;
	double Y = 0;
	double Z = 0;

	// sum the terms
	for (i=0; i<TERMS; i++)
	{
		A = arguments[i].L2 * L2 + arguments[i].L3 * L3 + arguments[i].L4 * L4 + arguments[i].L5 * L5 +
		    arguments[i].L6 * L6 + arguments[i].L7 * L7 + arguments[i].L8 * L8 + arguments[i].LL * LL +
		    arguments[i].D * D   + arguments[i].MM * MM + arguments[i].F * F;

		X += (x_coefficients[i].sin1 + x_coefficients[i].sin2 * T) * sin (A) + (x_coefficients[i].cos1 + x_coefficients[i].cos2 * T) * cos (A);
		Y += (y_coefficients[i].sin1 + y_coefficients[i].sin2 * T) * sin (A) + (y_coefficients[i].cos1 + y_coefficients[i].cos2 * T) * cos (A);
		Z += (z_coefficients[i].sin1 + z_coefficients[i].sin2 * T) * sin (A) + (z_coefficients[i].cos1 + z_coefficients[i].cos2 * T) * cos (A);
	}

	StelUtils::rectToSphe(&RA, &Dec, equPos);

	// FIXME: A dirty hack for testing
	correctedRA  =  RA - (Y * cos(RA) - X * sin(RA)) / (c * cos(Dec));
	correctedDec = Dec - ((X * cos(RA) + Y * sin(RA)) * sin(Dec) - Z * cos(Dec))/-c;

	StelUtils::spheToRect(correctedRA, correctedDec, equPos);
}

void Aberration::forward(Vec3d& equPos) const
{
	equPos.transfo4d(preTransfoMat);
	innerAberrationForward(equPos);
	equPos.transfo4d(postTransfoMat);
}

void Aberration::backward(Vec3d& equPos) const
{
	equPos.transfo4d(invertPostTransfoMat);
	innerAberrationBackward(equPos);
	equPos.transfo4d(invertPreTransfoMat);
}

void Aberration::forward(Vec3f& equPos) const
{
	Vec3d vf(equPos[0], equPos[1], equPos[2]);
	vf.transfo4d(preTransfoMat);
	innerAberrationForward(vf);
	vf.transfo4d(postTransfoMat);
	equPos.set(vf[0], vf[1], vf[2]);
}

void Aberration::backward(Vec3f& equPos) const
{
	equPos.transfo4d(invertPostTransfoMatf);
	Vec3d vf(equPos[0], equPos[1], equPos[2]);
	innerAberrationBackward(vf);
	equPos.set(vf[0], vf[1], vf[2]);
	equPos.transfo4d(invertPreTransfoMatf);
}
