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
#ifndef guAUIMANAGEDPANEL_H
#define guAUIMANAGEDPANEL_H

#include <wx/wx.h>
#include <wx/aui/aui.h>

// -------------------------------------------------------------------------------- //
class guAuiManagedPanel : public wxPanel
{
  protected :
    wxAuiManager *  m_Manager;

    void            OnChangedSize( wxSizeEvent &event );

  public :
    guAuiManagedPanel( wxWindow * parent, wxAuiManager * manager );
    ~guAuiManagedPanel();

    virtual void    CreateShowHideMenu( wxMenu * menu ) {};

};

#endif
// -------------------------------------------------------------------------------- //
