/*
Copyright (c) 2015 Holger Niessner

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#ifndef _DE431_H_
#define _DE431_H_

#include "jpleph.h"

#ifdef __cplusplus
  extern "C" {
#endif

void InitDE431(const char* filepath);
// most of the time centralBody_id likely is the Sun. However, for Moon, use centralBody_id=EPHEM_JPL_EARTH_ID=3
void GetDe431Coor(const double jde, const int planet_id, double * xyz, const int centralBody_id=CENTRAL_PLANET_ID);
// Not possible for a DE.
//void GetDe431OsculatingCoor(double jd0, double jd, int planet_id, double *xyz, const int centralBody_id=CENTRAL_PLANET_ID);

#ifdef __cplusplus
    }
#endif

#endif
