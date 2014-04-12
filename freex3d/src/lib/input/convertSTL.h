/****************************************************************************
    This file is part of the FreeWRL/FreeX3D Distribution.

    Copyright 2014 CRC Canada. (http://www.crc.gc.ca)

    FreeWRL/FreeX3D is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    FreeWRL/FreeX3D is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FreeWRL/FreeX3D.  If not, see <http://www.gnu.org/licenses/>.
****************************************************************************/


#ifndef __FREEWRL_CONVERTSTL_H__
#define __FREEWRL_CONVERTSTL_H__

int stlDTFT(const unsigned char* buffer, int len);
void analyzeSTLdata(struct Vector* vertices);
float fwl_getLastSTLScaling(void);
float getLastSTLScale(void);
void fwl_stl_set_rendering_type(int nv);
    


#endif //INCLUDE_STL_FILES
