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
#ifndef _ABERRATION_H_
#define _ABERRATION_H_

#ifdef __cplusplus
extern "C" {
#endif

//! Compute and return aberration.
//! @param JDE Julian day (TT)
//! @param RA, radians
//! @param Dec, radians
//! @return correctedRA, radians
//! @return correctedDec, radians
//! Ref: J. Meeus "Astronomical Algorithms" (2nd ed., with corrections as of August 10, 2009) p.153-157.
void getAberration(const double JDE, double RA, double Dec, double *correctedRA, double *correctedDec);

#ifdef __cplusplus
}
#endif

#endif
