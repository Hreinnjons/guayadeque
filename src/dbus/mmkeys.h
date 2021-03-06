// -------------------------------------------------------------------------------- //
//	Copyright (C) 2008-2016 J.Rios anonbeat@gmail.com
//
//    This Program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 3, or (at your option)
//    any later version.
//
//    This Program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; see the file LICENSE.  If not, write to
//    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//    http://www.gnu.org/copyleft/gpl.html
//
// -------------------------------------------------------------------------------- //
#ifndef MMKEYS_H
#define MMKEYS_H

#include "gudbus.h"
#include "PlayerPanel.h"

// -------------------------------------------------------------------------------- //
class guMMKeys : public guDBusClient
{
  protected :
    guPlayerPanel *                 m_PlayerPanel;

  public :
    guMMKeys( guDBusServer * server, guPlayerPanel * playerpanel );
    ~guMMKeys();

    virtual DBusHandlerResult       HandleMessages( guDBusMessage * msg, guDBusMessage * reply = NULL );

    void                            GrabMediaPlayerKeys( const unsigned int time );
    void                            ReleaseMediaPlayerKeys( void );

};

#endif
