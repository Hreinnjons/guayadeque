// -------------------------------------------------------------------------------- //
//	Copyright (C) 2008-2010 J.Rios
//	anonbeat@gmail.com
//
//    This Program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2, or (at your option)
//    any later version.
//
//    This Program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; see the file LICENSE.  If not, write to
//    the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
//    http://www.gnu.org/copyleft/gpl.html
//
// -------------------------------------------------------------------------------- //
#include "FileBrowser.h"

#include "AuiDockArt.h"
#include "Config.h"
#include "Images.h"
#include "LibUpdate.h"
#include "TagInfo.h"
#include "Utils.h"

#include <wx/aui/aui.h>
#include <wx/arrimpl.cpp>
#include <wx/artprov.h>

WX_DEFINE_OBJARRAY(guFileItemArray);

// -------------------------------------------------------------------------------- //
// guGenericDirCtrl
// -------------------------------------------------------------------------------- //
BEGIN_EVENT_TABLE(guGenericDirCtrl, wxGenericDirCtrl)
    EVT_TREE_BEGIN_LABEL_EDIT( wxID_ANY, guGenericDirCtrl::OnBeginRenameDir )
    EVT_TREE_END_LABEL_EDIT( wxID_ANY, guGenericDirCtrl::OnEndRenameDir )
END_EVENT_TABLE()

// -------------------------------------------------------------------------------- //
void guGenericDirCtrl::SetupSections()
{
    AddSection( wxT( "/" ), wxT( "/" ), 1 );
}

// -------------------------------------------------------------------------------- //
void guGenericDirCtrl::OnBeginRenameDir( wxTreeEvent &event )
{
    m_RenameName = GetPath();
    OnBeginEditItem( event );
}

// -------------------------------------------------------------------------------- //
void guGenericDirCtrl::OnEndRenameDir( wxTreeEvent &event )
{
    OnEndEditItem( event );
    m_FileBrowserDirCtrl->RenamedDir( m_RenameName, GetPath() );
}

// -------------------------------------------------------------------------------- //
void guGenericDirCtrl::FolderRename( void )
{
    wxTreeCtrl * TreeCtrl = GetTreeCtrl();
    TreeCtrl->EditLabel( TreeCtrl->GetSelection() );
}

// -------------------------------------------------------------------------------- //
// guFileBrowserDirCtrl
// -------------------------------------------------------------------------------- //
guFileBrowserDirCtrl::guFileBrowserDirCtrl( wxWindow * parent, guDbLibrary * db, const wxString &dirpath ) :
    wxPanel( parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL )
{
    m_Db = db;
    m_AddingFolder = false;

	wxBoxSizer * MainSizer;
	MainSizer = new wxBoxSizer( wxVERTICAL );

	m_DirCtrl = new guGenericDirCtrl( this, wxID_ANY, dirpath, wxDefaultPosition, wxDefaultSize, wxDIRCTRL_DIR_ONLY|wxSUNKEN_BORDER|wxDIRCTRL_EDIT_LABELS, wxEmptyString, 0 );

	m_DirCtrl->ShowHidden( false );
	MainSizer->Add( m_DirCtrl, 1, wxEXPAND, 5 );

	this->SetSizer( MainSizer );
	this->Layout();

    m_DirCtrl->Connect( wxEVT_COMMAND_TREE_ITEM_MENU, wxTreeEventHandler( guFileBrowserDirCtrl::OnContextMenu ), NULL, this );
}

// -------------------------------------------------------------------------------- //
guFileBrowserDirCtrl::~guFileBrowserDirCtrl()
{
    m_DirCtrl->Disconnect( wxEVT_COMMAND_TREE_ITEM_MENU, wxTreeEventHandler( guFileBrowserDirCtrl::OnContextMenu ), NULL, this );
}

// -------------------------------------------------------------------------------- //
void guFileBrowserDirCtrl::OnContextMenu( wxTreeEvent &event )
{
    wxMenu Menu;
    wxMenuItem * MenuItem;

    wxPoint Point = event.GetPoint();

    MenuItem = new wxMenuItem( &Menu, ID_FILESYSTEM_FOLDER_PLAY, _( "Play" ), _( "Play the selected folder" ) );
    MenuItem->SetBitmap( guImage( guIMAGE_INDEX_player_tiny_light_play ) );
    Menu.Append( MenuItem );

    MenuItem = new wxMenuItem( &Menu, ID_FILESYSTEM_FOLDER_ENQUEUE, _( "Enqueue" ), _( "Add the selected folder to playlist" ) );
    MenuItem->SetBitmap( guImage( guIMAGE_INDEX_tiny_add ) );
    Menu.Append( MenuItem );

    Menu.AppendSeparator();

    MenuItem = new wxMenuItem( &Menu, ID_FILESYSTEM_FOLDER_NEW, _( "New Folder" ), _( "Create a new folder" ) );
    Menu.Append( MenuItem );

    MenuItem = new wxMenuItem( &Menu, ID_FILESYSTEM_FOLDER_RENAME, _( "Rename" ), _( "Rename the selected folder" ) );
    MenuItem->SetBitmap( guImage( guIMAGE_INDEX_tiny_edit ) );
    Menu.Append( MenuItem );

    MenuItem = new wxMenuItem( &Menu, ID_FILESYSTEM_FOLDER_DELETE, _( "Remove" ), _( "Remove the selected folder" ) );
    Menu.Append( MenuItem );

    PopupMenu( &Menu, Point );
    event.Skip();
}

void guFileBrowserDirCtrl::RenamedDir( const wxString &oldname, const wxString &newname )
{
    guLogMessage( wxT( "'%s' -> '%s'" ), oldname.c_str(), newname.c_str() );

    if( m_AddingFolder )
    {
        wxTreeCtrl * TreeCtrl = m_DirCtrl->GetTreeCtrl();
        if( newname.IsEmpty() )
        {
            TreeCtrl->Delete( TreeCtrl->GetSelection() );
        }
        //m_DirCtrl->ReCreateTree();
        m_AddingFolder = false;
    }
    else
    {
        if( oldname != newname )
        {
            m_Db->UpdatePaths( oldname, newname );
        }
    }
}

// -------------------------------------------------------------------------------- //
void guFileBrowserDirCtrl::FolderNew( void )
{
    wxTreeCtrl * TreeCtrl = m_DirCtrl->GetTreeCtrl();
    wxTreeItemId FolderParent = TreeCtrl->GetSelection();
    if( FolderParent.IsOk() )
    {
        m_AddingFolder = true;

        wxString NewDirName = m_DirCtrl->GetPath() + wxT( "/" ) + _( "New Folder" );
        int Index = 1;
        while( wxDirExists( NewDirName ) )
        {
            NewDirName = m_DirCtrl->GetPath() + wxT( "/" ) + _( "New Folder" );
            NewDirName += wxString::Format( wxT( "%i" ), Index++ );
        }

        if( wxMkdir( NewDirName, 0770 ) )
        {
            TreeCtrl->Collapse( FolderParent );
            //TreeCtrl->Expand( m_AddFolderParent );
            m_DirCtrl->ExpandPath( NewDirName );
            wxTextCtrl * TextCtrl = TreeCtrl->EditLabel( TreeCtrl->GetSelection() );
            TextCtrl->SetSelection( -1, -1 );
        }
        else
        {
            guLogError( wxT( "Could not create the new directory" ) );
        }
    }
}

// -------------------------------------------------------------------------------- //
bool RemoveDirItems( const wxString &path )
{
    wxString FileName;
    wxString CurPath = path;
    if( !CurPath.EndsWith( wxT( "/" ) ) )
        CurPath += wxT( "/" );
    //guLogMessage( wxT( "Deleting folder %s" ), CurPath.c_str() );
    wxDir Dir( CurPath );
    if( Dir.IsOpened() )
    {
        if( Dir.GetFirst( &FileName, wxEmptyString, wxDIR_FILES | wxDIR_HIDDEN | wxDIR_DIRS | wxDIR_DOTDOT ) )
        {
            do {
                if( FileName != wxT( "." ) && FileName != wxT( ".." ) )
                {
                    if( wxDirExists( CurPath + FileName ) )
                    {
                        if( !RemoveDirItems( CurPath + FileName ) )
                            return false;
                        if( !wxRmdir( CurPath + FileName ) )
                            return false;
                    }
                    else
                    {
                        if( !wxRemoveFile( CurPath + FileName ) )
                            return false;
                    }
                }
            } while( Dir.GetNext( &FileName ) );
        }
        return true;
    }
    return false;
}

// -------------------------------------------------------------------------------- //
void guFileBrowserDirCtrl::FolderDelete( void )
{
    wxTreeCtrl * TreeCtrl = m_DirCtrl->GetTreeCtrl();
    wxTreeItemId FolderId = TreeCtrl->GetSelection();
    wxDirItemData * FolderData = ( wxDirItemData *  ) TreeCtrl->GetItemData( FolderId );
    if( wxMessageBox( _( "Are you sure to delete the selected path ?" ),
                     _( "Confirm" ),
                     wxICON_QUESTION | wxYES_NO, this ) == wxYES )
    {
        if( RemoveDirItems( FolderData->m_path ) && wxRmdir( FolderData->m_path ) )
        {
            TreeCtrl->Delete( FolderId );
        }
        else
        {
            wxMessageBox( _( "Error deleting the folder " ) + FolderData->m_path,
                _( "Error" ), wxICON_ERROR | wxOK, this );
        }
    }
}


// -------------------------------------------------------------------------------- //
// guFilesListBox
// -------------------------------------------------------------------------------- //
wxString guFILES_COLUMN_NAMES[] = {
    _( "Name" ),
    _( "Size" ),
    _( "Modified" )
};

// -------------------------------------------------------------------------------- //
guFilesListBox::guFilesListBox( wxWindow * parent, guDbLibrary * db ) :
    guListView( parent, wxLB_MULTIPLE | guLISTVIEW_COLUMN_SELECT | guLISTVIEW_COLUMN_SORTING | guLISTVIEW_ALLOWDRAG )
{
    m_Db = db;

    guConfig * Config = ( guConfig * ) guConfig::Get();

    m_Order = Config->ReadNum( wxT( "Order" ), 0, wxT( "FileBrowser" ) );
    m_OrderDesc = Config->ReadNum( wxT( "OrderDesc" ), false, wxT( "FileBrowser" ) );

    int ColId;
    wxString ColName;
    int index;
    int count = sizeof( guFILES_COLUMN_NAMES ) / sizeof( wxString );
    for( index = 0; index < count; index++ )
    {
        ColId = Config->ReadNum( wxString::Format( wxT( "FileBrowserCol%u" ), index ), index, wxT( "FileBrowserColumns" ) );

        ColName = guFILES_COLUMN_NAMES[ ColId ];

        ColName += ( ( ColId == m_Order ) ? ( m_OrderDesc ? wxT( " ▼" ) : wxT( " ▲" ) ) : wxEmptyString );

        guListViewColumn * Column = new guListViewColumn(
            ColName,
            ColId,
            Config->ReadNum( wxString::Format( wxT( "FileBrowserColWidth%u" ), index ), 80, wxT( "FileBrowserColumns" ) ),
            Config->ReadBool( wxString::Format( wxT( "FileBrowserColShow%u" ), index ), true, wxT( "FileBrowserColumns" ) )
            );
        InsertColumn( Column );
    }

    ReloadItems();
}


// -------------------------------------------------------------------------------- //
guFilesListBox::~guFilesListBox()
{
    guConfig * Config = ( guConfig * ) guConfig::Get();
    int index;
    int count = sizeof( guFILES_COLUMN_NAMES ) / sizeof( wxString );
    for( index = 0; index < count; index++ )
    {
        Config->WriteNum( wxString::Format( wxT( "FileBrowserCol%u" ), index ),
                          ( * m_Columns )[ index ].m_Id, wxT( "FileBrowserColumns" ) );
        Config->WriteNum( wxString::Format( wxT( "FileBrowserColWidth%u" ), index ),
                          ( * m_Columns )[ index ].m_Width, wxT( "FileBrowserColumns" ) );
        Config->WriteBool( wxString::Format( wxT( "FileBrowserColShow%u" ), index ),
                           ( * m_Columns )[ index ].m_Enabled, wxT( "FileBrowserColumns" ) );
    }

    Config->WriteNum( wxT( "Order" ), m_Order, wxT( "FileBrowser" ) );
    Config->WriteBool( wxT( "OrderDesc" ), m_OrderDesc, wxT( "FileBrowser" ) );
}

// -------------------------------------------------------------------------------- //
wxString guFilesListBox::OnGetItemText( const int row, const int col ) const
{
//    guLogMessage( wxT( "GetItem: %i  %i" ), row, col );
//    if( row < 0 || col < 0 )
//        return wxEmptyString;

    guFileItem * FileItem;
    FileItem = &m_Files[ row ];
    switch( ( * m_Columns )[ col ].m_Id )
    {
//        case guFILEBROWSER_COLUMN_TYPE :
//            return wxEmptyString;

        case guFILEBROWSER_COLUMN_NAME :
          return FileItem->m_Name;

        case guFILEBROWSER_COLUMN_SIZE :
          return SizeToString( FileItem->m_Size );

        case guFILEBROWSER_COLUMN_TIME :
        {
          wxDateTime FileTime;
          FileTime.Set( ( time_t ) FileItem->m_Time );
          return FileTime.FormatDate();
        }
    }
    return wxEmptyString;
}

// -------------------------------------------------------------------------------- //
void guFilesListBox::DrawItem( wxDC &dc, const wxRect &rect, const int row, const int col ) const
{
    if( ( * m_Columns )[ col ].m_Id == guFILEBROWSER_COLUMN_TYPE )
    {
        guFileItem * FileItem = &m_Files[ row ];
        dc.SetBackgroundMode( wxTRANSPARENT );
        int ImageIndex = guFILEITEM_IMAGE_INDEX_OTHER;
        if( FileItem->m_Type == guFILEITEM_TYPE_FOLDER )
        {
            ImageIndex = guFILEITEM_IMAGE_INDEX_FOLDER;
        }
        else if( FileItem->m_Type == guFILEITEM_TYPE_AUDIO )
        {
            ImageIndex = guFILEITEM_IMAGE_INDEX_AUDIO;
        }
        else if( FileItem->m_Type == guFILEITEM_TYPE_IMAGE )
        {
            ImageIndex = guFILEITEM_IMAGE_INDEX_IMAGE;
        }
        m_TreeImageList->Draw( ImageIndex, dc, rect.x + 1, rect.y + 1, wxIMAGELIST_DRAW_TRANSPARENT );
    }
    else
    {
        guListView::DrawItem( dc, rect, row, col );
    }
}


// -------------------------------------------------------------------------------- //
void inline GetFileDetails( const wxString &filename, guFileItem * fileitem )
{
    wxStructStat St;
    wxStat( filename, &St );
    fileitem->m_Type = ( ( St.st_mode & S_IFMT ) == S_IFDIR ) ? 0 : 3;
    fileitem->m_Size = St.st_size;
    fileitem->m_Time = St.st_ctime;
}

// -------------------------------------------------------------------------------- //
static int wxCMPFUNC_CONV CompareFileTypeA( guFileItem ** item1, guFileItem ** item2 )
{
    if( ( * item1 )->m_Name == wxT( ".." ) )
        return -1;
    else if( ( * item2 )->m_Name == wxT( ".." ) )
        return 1;

    if( ( * item1 )->m_Type == ( * item2 )->m_Type )
        return 0;
    else if( ( * item1 )->m_Type > ( * item2 )->m_Type )
        return -1;
    else
        return 1;
}

// -------------------------------------------------------------------------------- //
static int wxCMPFUNC_CONV CompareFileTypeD( guFileItem ** item1, guFileItem ** item2 )
{
    if( ( * item1 )->m_Name == wxT( ".." ) )
        return -1;
    else if( ( * item2 )->m_Name == wxT( ".." ) )
        return 1;

    if( ( * item1 )->m_Type == ( * item2 )->m_Type )
        return 0;
    else if( ( * item1 )->m_Type > ( * item2 )->m_Type )
        return 1;
    else
        return -1;
}

// -------------------------------------------------------------------------------- //
static int wxCMPFUNC_CONV CompareFileNameA( guFileItem ** item1, guFileItem ** item2 )
{
    int type = CompareFileTypeD( item1, item2 );
    if( !type )
        return ( * item1 )->m_Name.Cmp( ( * item2 )->m_Name );
    return type;
}

// -------------------------------------------------------------------------------- //
static int wxCMPFUNC_CONV CompareFileNameD( guFileItem ** item1, guFileItem ** item2 )
{
    int type = CompareFileTypeD( item1, item2 );
    if( !type )
        return ( * item2 )->m_Name.Cmp( ( * item1 )->m_Name );
    return type;
}

// -------------------------------------------------------------------------------- //
static int wxCMPFUNC_CONV CompareFileSizeA( guFileItem ** item1, guFileItem ** item2 )
{
    int type = CompareFileTypeD( item1, item2 );
    if( !type )
    {
        if( ( * item1 )->m_Size == ( * item2 )->m_Size )
            return 0;
        else if( ( * item1 )->m_Size > ( * item2 )->m_Size )
            return 1;
        else
            return -1;
    }
    return type;
}

// -------------------------------------------------------------------------------- //
static int wxCMPFUNC_CONV CompareFileSizeD( guFileItem ** item1, guFileItem ** item2 )
{
    int type = CompareFileTypeD( item1, item2 );
    if( !type )
    {
        if( ( * item1 )->m_Size == ( * item2 )->m_Size )
            return 0;
        else if( ( * item2 )->m_Size > ( * item1 )->m_Size )
            return 1;
        else
            return -1;
    }
    return type;
}

// -------------------------------------------------------------------------------- //
static int wxCMPFUNC_CONV CompareFileTimeA( guFileItem ** item1, guFileItem ** item2 )
{
    int type = CompareFileTypeD( item1, item2 );
    if( !type )
    {
        if( ( * item1 )->m_Time == ( * item2 )->m_Time )
            return 0;
        else if( ( * item1 )->m_Time > ( * item2 )->m_Time )
            return 1;
        else
            return -1;
    }
    return type;
}

// -------------------------------------------------------------------------------- //
static int wxCMPFUNC_CONV CompareFileTimeD( guFileItem ** item1, guFileItem ** item2 )
{
    int type = CompareFileTypeD( item1, item2 );
    if( !type )
    {
        if( ( * item1 )->m_Time == ( * item2 )->m_Time )
            return 0;
        else if( ( * item2 )->m_Time > ( * item1 )->m_Time )
            return 1;
        else
            return -1;
    }
    return type;
}

// -------------------------------------------------------------------------------- //
void guFilesListBox::GetItemsList( void )
{
    if( !m_CurDir.IsEmpty() && wxDirExists( m_CurDir ) )
    {
        wxDir Dir( m_CurDir );
        if( Dir.IsOpened() )
        {
            wxString FileName;
            if( Dir.GetFirst( &FileName, wxEmptyString, wxDIR_FILES | wxDIR_DIRS | wxDIR_DOTDOT ) )
            {
                do {
                    if( FileName != wxT( "." ) )
                    {
                        guFileItem * FileItem = new guFileItem();
                        FileItem->m_Name = FileName;
                        GetFileDetails( m_CurDir + FileName, FileItem );
                        if( guIsValidAudioFile( FileName.Lower() ) )
                            FileItem->m_Type = guFILEITEM_TYPE_AUDIO;
                        else if( guIsValidImageFile( FileName.Lower() ) )
                            FileItem->m_Type = guFILEITEM_TYPE_IMAGE;
                        m_Files.Add( FileItem );
                    }
                } while( Dir.GetNext( &FileName ) );
            }
        }
    }

    switch( m_Order )
    {
        case guFILEBROWSER_COLUMN_TYPE :
        {
            m_Files.Sort( m_OrderDesc ? CompareFileTypeD : CompareFileTypeA );
            break;
        }

        case guFILEBROWSER_COLUMN_NAME :
        {
            m_Files.Sort( m_OrderDesc ? CompareFileNameD : CompareFileNameA );
            break;
        }

        case guFILEBROWSER_COLUMN_SIZE :
        {
            m_Files.Sort( m_OrderDesc ? CompareFileSizeD : CompareFileSizeA );
            break;
        }

        case guFILEBROWSER_COLUMN_TIME :
        {
            m_Files.Sort( m_OrderDesc ? CompareFileTimeD : CompareFileTimeA );
            break;
        }
    }

//    wxCommandEvent event( wxEVT_COMMAND_MENU_SELECTED, ID_MAINFRAME_UPDATE_SELINFO );
//    AddPendingEvent( event );
}

// -------------------------------------------------------------------------------- //
void guFilesListBox::ReloadItems( bool reset )
{
    wxASSERT( m_Db );

    //
    wxArrayInt Selection;
    int FirstVisible = 0;

    if( reset )
    {
        SetSelection( -1 );
    }
    else
    {
        Selection = GetSelectedItems( false );
        FirstVisible = GetFirstVisibleLine();
    }

    m_Files.Empty();

    GetItemsList();

    SetItemCount( m_Files.Count() );

    if( !reset )
    {
      SetSelectedItems( Selection );
      ScrollToLine( FirstVisible );
    }
    RefreshAll();
}

// -------------------------------------------------------------------------------- //
void guFilesListBox::CreateContextMenu( wxMenu * Menu ) const
{
//    wxArrayInt Selection = GetSelectedItems();
//    if( Selection.Count() )
//    {
//        wxMenuItem * MenuItem;
//        MenuItem = new wxMenuItem( Menu, ID_PODCASTS_ITEM_PLAY, _( "Play" ), _( "Play current selected songs" ) );
//        MenuItem->SetBitmap( guImage( guIMAGE_INDEX_player_tiny_light_play ) );
//        Menu->Append( MenuItem );
//
//        MenuItem = new wxMenuItem( Menu, ID_PODCASTS_ITEM_ENQUEUE, _( "Enqueue" ), _( "Add current selected songs to playlist" ) );
//        MenuItem->SetBitmap( guImage( guIMAGE_INDEX_add ) );
//        Menu->Append( MenuItem );
//
//        Menu->AppendSeparator();
//
//        MenuItem = new wxMenuItem( Menu, ID_PODCASTS_ITEM_DEL, _( "Delete" ), _( "Delete the current selected podcasts" ) );
//        MenuItem->SetBitmap( guImage( guIMAGE_INDEX_del ) );
//        Menu->Append( MenuItem );
//
//        Menu->AppendSeparator();
//
//        MenuItem = new wxMenuItem( Menu, ID_PODCASTS_ITEM_DOWNLOAD, _( "Download" ), _( "Download the current selected podcasts" ) );
//        MenuItem->SetBitmap( guImage( guIMAGE_INDEX_tiny_doc_save ) );
//        Menu->Append( MenuItem );
//
//        Menu->AppendSeparator();
//        MenuItem = new wxMenuItem( Menu, ID_PODCASTS_ITEM_COPYTO, _( "Copy to..." ), _( "Copy the current selected podcasts to a directory or device" ) );
//        MenuItem->SetBitmap( guImage( guIMAGE_INDEX_edit_copy ) );
//        Menu->Append( MenuItem );
//    }
//    else
//    {
//        wxMenuItem * MenuItem;
//        MenuItem = new wxMenuItem( Menu, wxID_ANY, _( "No selected items..." ), _( "Copy the current selected podcasts to a directory or device" ) );
//        MenuItem->SetBitmap( guImage( guIMAGE_INDEX_tiny_status_error ) );
//        Menu->Append( MenuItem );
//    }
}

// -------------------------------------------------------------------------------- //
int inline guFilesListBox::GetItemId( const int row ) const
{
    return row;
}

// -------------------------------------------------------------------------------- //
wxString inline guFilesListBox::GetItemName( const int row ) const
{
    return m_Files[ row ].m_Name;
}

// -------------------------------------------------------------------------------- //
void guFilesListBox::SetOrder( int columnid )
{
    if( m_Order != columnid )
    {
        m_Order = columnid;
        m_OrderDesc = ( columnid != 0 );
    }
    else
        m_OrderDesc = !m_OrderDesc;

    int CurColId;
    int index;
    int count = sizeof( guFILES_COLUMN_NAMES ) / sizeof( wxString );
    for( index = 0; index < count; index++ )
    {
        CurColId = GetColumnId( index );
        SetColumnLabel( index,
            guFILES_COLUMN_NAMES[ CurColId ]  + ( ( CurColId == m_Order ) ?
                ( m_OrderDesc ? wxT( " ▼" ) : wxT( " ▲" ) ) : wxEmptyString ) );
    }

    ReloadItems();
}

// -------------------------------------------------------------------------------- //
int guFilesListBox::GetSelectedSongs( guTrackArray * tracks ) const
{
//    wxArrayInt Selection = GetSelectedItems();
//    int Index;
//    int Count = Selection.Count();
//    for( Index = 0; Index < Count; Index++ )
//    {
//        guPodcastItem PodcastItem;
//        if( ( m_Db->GetPodcastItemId( Selection[ Index ], &PodcastItem ) != wxNOT_FOUND ) &&
//            ( PodcastItem.m_Status == guPODCAST_STATUS_READY ) &&
//            ( wxFileExists( PodcastItem.m_FileName ) ) )
//        {
//            guTrack * Track = new guTrack();
//            if( Track )
//            {
//                Track->m_Type = guTRACK_TYPE_PODCAST;
//                Track->m_SongId = PodcastItem.m_Id;
//                Track->m_FileName = PodcastItem.m_FileName;
//                Track->m_SongName = PodcastItem.m_Title;
//                Track->m_ArtistName = PodcastItem.m_Author;
//                Track->m_AlbumId = PodcastItem.m_ChId;
//                Track->m_AlbumName = PodcastItem.m_Channel;
//                Track->m_Length = PodcastItem.m_Length;
//                Track->m_PlayCount = PodcastItem.m_PlayCount;
//                Track->m_GenreName = wxT( "Podcasts" );
//                Track->m_Number = Index;
//                Track->m_Rating = -1;
//                Track->m_CoverId = 0;
//                Track->m_Year = 0; // Get year from item date
//                tracks->Add( Track );
//            }
//        }
//    }
//    return tracks->Count();
    return 0;
}

// -------------------------------------------------------------------------------- //
int guFilesListBox::GetDragFiles( wxFileDataObject * files )
{
    wxArrayInt Selection = GetSelectedItems();
    int index;
    int count = Selection.Count();
    for( index = 0; index < count; index++ )
    {
       files->AddFile( m_CurDir + m_Files[ Selection[ index ] ].m_Name );
    }
    return count;
}

// -------------------------------------------------------------------------------- //
void guFilesListBox::SetPath( const wxString &path )
{
    m_CurDir = path;
    if( !m_CurDir.EndsWith( wxT( "/" ) ) )
        m_CurDir += wxT( "/" );
    ReloadItems();
}

// -------------------------------------------------------------------------------- //
wxString guFilesListBox::GetPath( const int item )
{
    wxString RetVal;
    //guLogMessage( wxT( "GetPath( %i )" ), item );
    if( item >= 0 )
    {
        if( m_Files[ item ].m_Name == wxT( ".." ) )
        {
            RetVal = m_CurDir.BeforeLast( wxT( '/' ) ).BeforeLast( wxT( '/' ) );
            guLogMessage( wxT( "1) Path : %s" ), RetVal.c_str() );
            return RetVal;
        }

        wxFileName FileName( m_Files[ item ].m_Name );
        FileName.MakeAbsolute( m_CurDir );
        //guLogMessage( wxT( "Path : %s" ), FileName.GetFullPath().c_str() );
        return FileName.GetFullPath();
    }
    return wxEmptyString;
}

// -------------------------------------------------------------------------------- //
int guFilesListBox::GetType( const int item )
{
    if( item >= 0 )
    {
        return m_Files[ item ].m_Type;
    }
    return wxNOT_FOUND;
}



// -------------------------------------------------------------------------------- //
// guFileBrowserFileCtrl
// -------------------------------------------------------------------------------- //
guFileBrowserFileCtrl::guFileBrowserFileCtrl( wxWindow * parent, guDbLibrary * db, guFileBrowserDirCtrl * dirctrl ) :
    wxPanel( parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL )
{
    m_Db = db;
    m_DirCtrl = dirctrl;
    wxImageList * ImageList = dirctrl->GetImageList();
    ImageList->Add( wxArtProvider::GetBitmap( wxT( "audio-x-generic" ), wxART_OTHER, wxSize( 16, 16 ) ) );
    ImageList->Add( wxArtProvider::GetBitmap( wxT( "image-x-generic" ), wxART_OTHER, wxSize( 16, 16 ) ) );

	wxBoxSizer * MainSizer;
	MainSizer = new wxBoxSizer( wxVERTICAL );

	m_FilesListBox = new guFilesListBox( this, db );
	m_FilesListBox->SetTreeImageList( ImageList );
	MainSizer->Add( m_FilesListBox, 1, wxEXPAND, 5 );

	this->SetSizer( MainSizer );
	this->Layout();
}

// -------------------------------------------------------------------------------- //
guFileBrowserFileCtrl::~guFileBrowserFileCtrl()
{
}



// -------------------------------------------------------------------------------- //
// guFileBrowser
// -------------------------------------------------------------------------------- //
BEGIN_EVENT_TABLE( guFileBrowser, wxPanel )
    EVT_TREE_BEGIN_DRAG( wxID_ANY, guFileBrowser::OnDirBeginDrag)
END_EVENT_TABLE()

// -------------------------------------------------------------------------------- //
guFileBrowser::guFileBrowser( wxWindow * parent, guDbLibrary * db, guPlayerPanel * playerpanel ) :
    wxPanel( parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL )
{
    m_Db = db;
    m_PlayerPanel = playerpanel;

    guConfig *  Config = ( guConfig * ) guConfig::Get();

    m_AuiManager.SetManagedWindow( this );
    m_AuiManager.SetArtProvider( new guAuiDockArt() );
    m_AuiManager.SetFlags( wxAUI_MGR_ALLOW_FLOATING |
                           wxAUI_MGR_TRANSPARENT_DRAG |
                           wxAUI_MGR_TRANSPARENT_HINT );
    wxAuiDockArt * AuiDockArt = m_AuiManager.GetArtProvider();
    AuiDockArt->SetColour( wxAUI_DOCKART_INACTIVE_CAPTION_TEXT_COLOUR,
            wxSystemSettings::GetColour( wxSYS_COLOUR_INACTIVECAPTIONTEXT ) );
    AuiDockArt->SetColour( wxAUI_DOCKART_ACTIVE_CAPTION_TEXT_COLOUR,
            wxSystemSettings::GetColour( wxSYS_COLOUR_CAPTIONTEXT ) );

    AuiDockArt->SetColour( wxAUI_DOCKART_ACTIVE_CAPTION_COLOUR,
            wxSystemSettings::GetColour( wxSYS_COLOUR_ACTIVEBORDER ) );

    AuiDockArt->SetColour( wxAUI_DOCKART_ACTIVE_CAPTION_GRADIENT_COLOUR,
            wxSystemSettings::GetColour( wxSYS_COLOUR_3DSHADOW ) );

    AuiDockArt->SetColour( wxAUI_DOCKART_INACTIVE_CAPTION_COLOUR,
            wxSystemSettings::GetColour( wxSYS_COLOUR_INACTIVEBORDER ) );

    AuiDockArt->SetColour( wxAUI_DOCKART_INACTIVE_CAPTION_GRADIENT_COLOUR,
            wxSystemSettings::GetColour( wxSYS_COLOUR_3DSHADOW ) );

    AuiDockArt->SetColour( wxAUI_DOCKART_GRADIENT_TYPE,
            wxAUI_GRADIENT_VERTICAL );

    m_VisiblePanels = Config->ReadNum( wxT( "FBVisiblePanels" ), guPANEL_FILEBROWSER_VISIBLE_DEFAULT, wxT( "Positions" ) );


    wxArrayString LibPaths = Config->ReadAStr( wxT( "LibPath" ), wxEmptyString, wxT( "LibPaths" ) );
    if( !LibPaths.Count() )
        LibPaths.Add( wxGetHomeDir() );

    m_DirCtrl = new guFileBrowserDirCtrl( this, m_Db,
        Config->ReadStr( wxT( "Path" ), LibPaths[ 0 ], wxT( "FileBrowser" ) ) );

    m_AuiManager.AddPane( m_DirCtrl,
            wxAuiPaneInfo().Name( wxT( "FileBrowserDirCtrl" ) ).Caption( _( "Directories" ) ).
            MinSize( 60, 28 ).Row( 0 ).Layer( 0 ).Position( 0 ).
            CloseButton( false ).
            Dockable( true ).Left() );

    m_FilesCtrl = new guFileBrowserFileCtrl( this, db, m_DirCtrl );
    m_FilesCtrl->SetPath( m_DirCtrl->GetPath() );

    m_AuiManager.AddPane( m_FilesCtrl,
            wxAuiPaneInfo().Name( wxT( "FileBrowserFilesCtrl" ) ).
            Dockable( true ).CenterPane() );

    wxString FileBrowserLayout = Config->ReadStr( wxT( "FileBrowser" ), wxEmptyString, wxT( "Positions" ) );
    if( Config->GetIgnoreLayouts() || FileBrowserLayout.IsEmpty() )
    {
        m_AuiManager.Update();
        m_VisiblePanels = guPANEL_FILEBROWSER_VISIBLE_DEFAULT;
    }
    else
    {
        m_AuiManager.LoadPerspective( FileBrowserLayout, true );
    }

	m_DirCtrl->Connect( wxEVT_COMMAND_TREE_SEL_CHANGED, wxTreeEventHandler( guFileBrowser::OnDirItemChanged ), NULL, this );
    m_FilesCtrl->Connect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED,  wxListEventHandler( guFileBrowser::OnFileItemActivated ), NULL, this );
	m_FilesCtrl->Connect( wxEVT_COMMAND_LIST_COL_CLICK, wxListEventHandler( guFileBrowser::OnFilesColClick ), NULL, this );

    Connect( ID_FILESYSTEM_FOLDER_PLAY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guFileBrowser::OnFolderPlay ), NULL, this );
    Connect( ID_FILESYSTEM_FOLDER_ENQUEUE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guFileBrowser::OnFolderEnqueue ), NULL, this );
    Connect( ID_FILESYSTEM_FOLDER_NEW, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guFileBrowser::OnFolderNew ), NULL, this );
    Connect( ID_FILESYSTEM_FOLDER_RENAME, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guFileBrowser::OnFolderRename ), NULL, this );
    Connect( ID_FILESYSTEM_FOLDER_DELETE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guFileBrowser::OnFolderDelete ), NULL, this );

}

// -------------------------------------------------------------------------------- //
guFileBrowser::~guFileBrowser()
{
    guConfig *  Config = ( guConfig * ) guConfig::Get();

    Config->WriteNum( wxT( "FBVisiblePanels" ), m_VisiblePanels, wxT( "Positions" ) );
    Config->WriteStr( wxT( "FileBrowser" ), m_AuiManager.SavePerspective(), wxT( "Positions" ) );
    Config->WriteStr( wxT( "Path" ), m_DirCtrl->GetPath(), wxT( "FileBrowser" ) );

	m_DirCtrl->Disconnect( wxEVT_COMMAND_TREE_SEL_CHANGED, wxTreeEventHandler( guFileBrowser::OnDirItemChanged ), NULL, this );
    m_FilesCtrl->Disconnect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED,  wxListEventHandler( guFileBrowser::OnFileItemActivated ), NULL, this );
	m_FilesCtrl->Disconnect( wxEVT_COMMAND_LIST_COL_CLICK, wxListEventHandler( guFileBrowser::OnFilesColClick ), NULL, this );

    Disconnect( ID_FILESYSTEM_FOLDER_PLAY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guFileBrowser::OnFolderPlay ), NULL, this );
    Disconnect( ID_FILESYSTEM_FOLDER_ENQUEUE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guFileBrowser::OnFolderEnqueue ), NULL, this );
    Disconnect( ID_FILESYSTEM_FOLDER_NEW, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guFileBrowser::OnFolderNew ), NULL, this );
    Disconnect( ID_FILESYSTEM_FOLDER_RENAME, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guFileBrowser::OnFolderRename ), NULL, this );
    Disconnect( ID_FILESYSTEM_FOLDER_DELETE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guFileBrowser::OnFolderDelete ), NULL, this );

    m_AuiManager.UnInit();
}

// -------------------------------------------------------------------------------- //
void guFileBrowser::OnDirItemChanged( wxTreeEvent &event )
{
    guLogMessage( wxT( "The current selected directory is '%s'" ), m_DirCtrl->GetPath().c_str() );

    m_FilesCtrl->SetPath( m_DirCtrl->GetPath() );
}

// -------------------------------------------------------------------------------- //
void guFileBrowser::OnFileItemActivated( wxListEvent &Event )
{
    wxArrayInt Selection = m_FilesCtrl->GetSelectedItems();
    if( Selection.Count() )
    {
        if( m_FilesCtrl->GetType( Selection[ 0 ] ) == guFILEITEM_TYPE_FOLDER )
        {
            m_DirCtrl->SetPath( m_FilesCtrl->GetPath( Selection[ 0 ] ) );
        }
        else
        {
            wxArrayString Files;
            Files.Add( m_FilesCtrl->GetPath( Selection[ 0 ] ) );

            guConfig * Config = ( guConfig * ) guConfig::Get();
            if( Config )
            {
                if( Config->ReadBool( wxT( "DefaultActionEnqueue" ), false , wxT( "General" )) )
                {
                    m_PlayerPanel->AddToPlayList( Files[ 0 ] );
                }
                else
                {
                    m_PlayerPanel->SetPlayList( Files );
                }
            }

        }
    }
}

// -------------------------------------------------------------------------------- //
void guFileBrowser::OnFilesColClick( wxListEvent &event )
{
    int col = event.GetColumn();
    if( col < 0 )
        return;
    m_FilesCtrl->SetOrder( col );
}

// -------------------------------------------------------------------------------- //
void guFileBrowser::OnDirBeginDrag( wxTreeEvent &event )
{
    wxFileDataObject Files;

    Files.AddFile( m_DirCtrl->GetPath() );
    wxDropSource source( Files, this );

    wxDragResult Result = source.DoDragDrop();
    if( Result )
    {
    }
}

// -------------------------------------------------------------------------------- //
void guFileBrowser::OnFolderPlay( wxCommandEvent &event )
{
    wxArrayString Files;
    Files.Add( m_DirCtrl->GetPath() );
    m_PlayerPanel->SetPlayList( Files );
}

// -------------------------------------------------------------------------------- //
void guFileBrowser::OnFolderEnqueue( wxCommandEvent &event )
{
    m_PlayerPanel->AddToPlayList( m_DirCtrl->GetPath() );
}

// -------------------------------------------------------------------------------- //
void guFileBrowser::OnFolderNew( wxCommandEvent &event )
{
    m_DirCtrl->FolderNew();
}

// -------------------------------------------------------------------------------- //
void guFileBrowser::OnFolderRename( wxCommandEvent &event )
{
    m_DirCtrl->FolderRename();
}

// -------------------------------------------------------------------------------- //
void guFileBrowser::OnFolderDelete( wxCommandEvent &event )
{
    m_DirCtrl->FolderDelete();
}

// -------------------------------------------------------------------------------- //