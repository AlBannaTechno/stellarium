/*
 * Stellarium
 * Copyright (C) 2007 Guillaume Chereau
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
 
 
#ifndef _SIMPLEGRID_HPP_
#define _SIMPLEGRID_HPP_

#include <list>

#include "Grid.hpp"


class SimpleGrid : public Grid
{
public:
    SimpleGrid(Navigator* nav = NULL) : Grid(nav) {}
    
    ~SimpleGrid() {}
    void insert(StelObject* obj)
    {
        all.push_back(obj);
        Grid::insert(obj);
    }
    void filterIntersect(const Disk& s);


private:
    // all the objects
    typedef std::list<StelObject*> AllObjects;
    AllObjects all;
};

#endif // _SIMPLEGRID_HPP_
 
