/************************************************************************************
   Copyright (C) 2020 MariaDB Corporation AB

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with this library; if not see <http://www.gnu.org/licenses>
   or write to the Free Software Foundation, Inc.,
   51 Franklin St., Fifth Floor, Boston, MA 02110, USA
*************************************************************************************/


#ifndef _VERSION_H_
#define _VERSION_H_

#define MACPP_VERSION_MAJOR 1
#define MACPP_VERSION_MINOR 0
#define MACPP_VERSION_PATCH 4

#define MACPP_VERSION_STR "1.0.4"

#define MACPP_VERSION "1.00.0004"

#define MACPP_ERR_PREFIX "[1.0.4]"

#define MACPP_DEFAULT_PLUGINS_SUBDIR "plugin"

namespace sql
{
namespace mariadb
{

struct Version final {
  static const char* version;
  static const unsigned int majorVersion= MACPP_VERSION_MAJOR;
  static const unsigned int minorVersion= MACPP_VERSION_MINOR;
  static const unsigned int patchVersion= MACPP_VERSION_PATCH;
};
}
}

#endif /* _Version_h_ */
