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
#include "LibPanel.h"

#include "AuiDockArt.h"
#include "Commands.h"
#include "Config.h"
#include "CoverEdit.h"
#include "EditWithOptions.h"
#include "Images.h"
#include "LabelEditor.h"
#include "MainFrame.h"
#include "PlayListAppend.h"
#include "SelCoverFile.h"
#include "TagInfo.h"
#include "TrackEdit.h"
#include "Utils.h"

#include <wx/event.h>
#include <wx/uri.h>

#define     LISTCTRL_BORDER                 5

#define     guPANEL_TIMER_SELECTION         1
#define     guPANEL_TIMER_TEXTSEARCH        2

#define     guPANEL_TIMER_SELCHANGED        50
#define     guPANEL_TIMER_TEXTCHANGED       500

// -------------------------------------------------------------------------------- //
guLibPanel::guLibPanel( wxWindow * parent, guMediaViewer * mediaviewer ) :
    guAuiManagerPanel( parent ),
    m_SelChangedTimer( this, guPANEL_TIMER_SELECTION )
{
    guConfig *          Config = ( guConfig * ) guConfig::Get();

    m_MediaViewer = mediaviewer;
    m_Db = mediaviewer->GetDb();
    m_PlayerPanel = mediaviewer->GetPlayerPanel();
    m_ConfigPath = mediaviewer->ConfigPath() + wxT( "/library" );
    m_UpdateLock = false;


    m_VisiblePanels = Config->ReadNum( wxT( "VisiblePanels" ), guPANEL_LIBRARY_VISIBLE_DEFAULT, m_ConfigPath );

    CreateControls();

    //
    LoadLastLayout();
}

// -------------------------------------------------------------------------------- //
guLibPanel::~guLibPanel()
{
    // Save the Splitter positions into the main config
    guConfig * Config = ( guConfig * ) guConfig::Get();

    Config->WriteNum( wxT( "VisiblePanels" ), m_VisiblePanels, m_ConfigPath );
    Config->WriteStr( wxT( "Layout" ), m_AuiManager.SavePerspective(), m_ConfigPath );

	Disconnect( guPANEL_TIMER_SELECTION, wxEVT_TIMER, wxTimerEventHandler( guLibPanel::OnSelChangedTimer ), NULL, this );
//	Disconnect( guPANEL_TIMER_TEXTSEARCH, wxEVT_TIMER, wxTimerEventHandler( guLibPanel::OnTextChangedTimer ), NULL, this );
    //
    m_GenreListCtrl->Disconnect( wxEVT_COMMAND_LISTBOX_SELECTED,  wxListEventHandler( guLibPanel::OnGenreListSelected ), NULL, this );
    //m_GenreListCtrl->Disconnect( wxEVT_COMMAND_LIST_ITEM_DESELECTED,  wxListEventHandler( guLibPanel::OnGenreListSelected ), NULL, this );
    m_GenreListCtrl->Disconnect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxListEventHandler( guLibPanel::OnGenreListActivated ), NULL, this );

    m_LabelsListCtrl->Disconnect( wxEVT_COMMAND_LISTBOX_SELECTED,  wxListEventHandler( guLibPanel::OnLabelListSelected ), NULL, this );
    //m_LabelsListCtrl->Disconnect( wxEVT_COMMAND_LIST_ITEM_DESELECTED,  wxListEventHandler( guLibPanel::OnLabelListSelected ), NULL, this );
    m_LabelsListCtrl->Disconnect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxListEventHandler( guLibPanel::OnLabelListActivated ), NULL, this );

    m_ArtistListCtrl->Disconnect( wxEVT_COMMAND_LISTBOX_SELECTED,  wxListEventHandler( guLibPanel::OnArtistListSelected ), NULL, this );
    //m_ArtistListCtrl->Disconnect( wxEVT_COMMAND_LIST_ITEM_DESELECTED,  wxListEventHandler( guLibPanel::OnArtistListSelected ), NULL, this );
    m_ArtistListCtrl->Disconnect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxListEventHandler( guLibPanel::OnArtistListActivated ), NULL, this );

    m_AlbumListCtrl->Disconnect( wxEVT_COMMAND_LISTBOX_SELECTED,  wxListEventHandler( guLibPanel::OnAlbumListSelected ), NULL, this );
    m_AlbumListCtrl->Disconnect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxListEventHandler( guLibPanel::OnAlbumListActivated ), NULL, this );

    m_YearListCtrl->Disconnect( wxEVT_COMMAND_LISTBOX_SELECTED,  wxListEventHandler( guLibPanel::OnYearListSelected ), NULL, this );
    m_YearListCtrl->Disconnect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxListEventHandler( guLibPanel::OnYearListActivated ), NULL, this );

    m_RatingListCtrl->Disconnect( wxEVT_COMMAND_LISTBOX_SELECTED,  wxListEventHandler( guLibPanel::OnRatingListSelected ), NULL, this );
    m_RatingListCtrl->Disconnect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxListEventHandler( guLibPanel::OnRatingListActivated ), NULL, this );

    m_PlayCountListCtrl->Disconnect( wxEVT_COMMAND_LISTBOX_SELECTED,  wxListEventHandler( guLibPanel::OnPlayCountListSelected ), NULL, this );
    m_PlayCountListCtrl->Disconnect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxListEventHandler( guLibPanel::OnPlayCountListActivated ), NULL, this );

    m_ComposerListCtrl->Disconnect( wxEVT_COMMAND_LISTBOX_SELECTED,  wxListEventHandler( guLibPanel::OnComposerListSelected ), NULL, this );
    m_ComposerListCtrl->Disconnect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxListEventHandler( guLibPanel::OnComposerListActivated ), NULL, this );

    m_AlbumArtistListCtrl->Disconnect( wxEVT_COMMAND_LISTBOX_SELECTED,  wxListEventHandler( guLibPanel::OnAlbumArtistListSelected ), NULL, this );
    m_AlbumArtistListCtrl->Disconnect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxListEventHandler( guLibPanel::OnAlbumArtistListActivated ), NULL, this );

    m_SongListCtrl->Disconnect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxListEventHandler( guLibPanel::OnSongListActivated ), NULL, this );
    m_SongListCtrl->Disconnect( wxEVT_COMMAND_LIST_COL_CLICK, wxListEventHandler( guLibPanel::OnSongListColClicked ), NULL, this );

    Disconnect( ID_GENRE_PLAY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnGenrePlayClicked ), NULL, this );
    Disconnect( ID_GENRE_ENQUEUE_AFTER_ALL, ID_GENRE_ENQUEUE_AFTER_ARTIST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnGenreQueueClicked ), NULL, this );
    m_GenreListCtrl->Disconnect( ID_COPYTO_BASE, ID_COPYTO_BASE + guCOPYTO_MAXCOUNT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnGenreCopyToClicked ), NULL, this );
    Disconnect( ID_GENRE_SAVETOPLAYLIST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnGenreSavePlayListClicked ), NULL, this );

    Disconnect( ID_LABEL_PLAY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnLabelPlayClicked ), NULL, this );
    Disconnect( ID_LABEL_ENQUEUE_AFTER_ALL, ID_LABEL_ENQUEUE_AFTER_ARTIST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnLabelQueueClicked ), NULL, this );
    m_LabelsListCtrl->Disconnect( ID_COPYTO_BASE, ID_COPYTO_BASE + guCOPYTO_MAXCOUNT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnLabelCopyToClicked ), NULL, this );
    Disconnect( ID_LABEL_SAVETOPLAYLIST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnLabelSavePlayListClicked ), NULL, this );

    Disconnect( ID_ARTIST_PLAY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnArtistPlayClicked ), NULL, this );
    Disconnect( ID_ARTIST_ENQUEUE_AFTER_ALL, ID_ARTIST_ENQUEUE_AFTER_ARTIST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnArtistQueueClicked ), NULL, this );
    Disconnect( ID_ARTIST_EDITLABELS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnArtistEditLabelsClicked ), NULL, this );
    Disconnect( ID_ARTIST_EDITTRACKS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnArtistEditTracksClicked ), NULL, this );
    m_ArtistListCtrl->Disconnect( ID_COPYTO_BASE, ID_COPYTO_BASE + guCOPYTO_MAXCOUNT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnArtistCopyToClicked ), NULL, this );
    Disconnect( ID_ARTIST_SAVETOPLAYLIST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnArtistSavePlayListClicked ), NULL, this );

    Disconnect( ID_ALBUM_PLAY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnAlbumPlayClicked ), NULL, this );
    Disconnect( ID_ALBUM_ENQUEUE_AFTER_ALL, ID_ALBUM_ENQUEUE_AFTER_ARTIST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnAlbumQueueClicked ), NULL, this );
    Disconnect( ID_ALBUM_EDITLABELS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnAlbumEditLabelsClicked ), NULL, this );
    Disconnect( ID_ALBUM_EDITTRACKS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnAlbumEditTracksClicked ), NULL, this );
    Disconnect( ID_ALBUM_MANUALCOVER, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnAlbumDownloadCoverClicked ), NULL, this );
    Disconnect( ID_ALBUM_SELECT_COVER, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnAlbumSelectCoverClicked ), NULL, this );
    Disconnect( ID_ALBUM_COVER_DELETE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnAlbumDeleteCoverClicked ), NULL, this );
    Disconnect( ID_ALBUM_COVER_EMBED, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnAlbumEmbedCoverClicked ), NULL, this );
    m_AlbumListCtrl->Disconnect( ID_COPYTO_BASE, ID_COPYTO_BASE + guCOPYTO_MAXCOUNT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnAlbumCopyToClicked ), NULL, this );
    Disconnect( ID_ALBUM_SAVETOPLAYLIST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnAlbumSavePlayListClicked ), NULL, this );

    Disconnect( ID_YEAR_PLAY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnYearListPlayClicked ), NULL, this );
    Disconnect( ID_YEAR_ENQUEUE_AFTER_ALL, ID_YEAR_ENQUEUE_AFTER_ARTIST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnYearListQueueClicked ), NULL, this );
    Disconnect( ID_YEAR_EDITTRACKS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnYearListEditTracksClicked ), NULL, this );
    m_YearListCtrl->Disconnect( ID_COPYTO_BASE, ID_COPYTO_BASE + guCOPYTO_MAXCOUNT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnYearListCopyToClicked ), NULL, this );
    Disconnect( ID_YEAR_SAVETOPLAYLIST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnYearSavePlayListClicked ), NULL, this );

    Disconnect( ID_RATING_PLAY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnRatingListPlayClicked ), NULL, this );
    Disconnect( ID_RATING_ENQUEUE_AFTER_ALL, ID_RATING_ENQUEUE_AFTER_ARTIST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnRatingListQueueClicked ), NULL, this );
    Disconnect( ID_RATING_EDITTRACKS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnRatingListEditTracksClicked ), NULL, this );
    m_RatingListCtrl->Disconnect( ID_COPYTO_BASE, ID_COPYTO_BASE + guCOPYTO_MAXCOUNT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnRatingListCopyToClicked ), NULL, this );
    Disconnect( ID_RATING_SAVETOPLAYLIST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnArtistSavePlayListClicked ), NULL, this );

    Disconnect( ID_PLAYCOUNT_PLAY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnPlayCountListPlayClicked ), NULL, this );
    Disconnect( ID_PLAYCOUNT_ENQUEUE_AFTER_ALL, ID_PLAYCOUNT_ENQUEUE_AFTER_ARTIST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnPlayCountListQueueClicked ), NULL, this );
    Disconnect( ID_PLAYCOUNT_EDITTRACKS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnPlayCountListEditTracksClicked ), NULL, this );
    m_PlayCountListCtrl->Disconnect( ID_COPYTO_BASE, ID_COPYTO_BASE + guCOPYTO_MAXCOUNT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnPlayCountListCopyToClicked ), NULL, this );
    Disconnect( ID_PLAYCOUNT_SAVETOPLAYLIST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnPlayCountSavePlayListClicked ), NULL, this );

    Disconnect( ID_COMPOSER_PLAY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnComposerListPlayClicked ), NULL, this );
    Disconnect( ID_COMPOSER_ENQUEUE_AFTER_ALL, ID_COMPOSER_ENQUEUE_AFTER_ARTIST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnComposerListQueueClicked ), NULL, this );
    Disconnect( ID_COMPOSER_EDITTRACKS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnComposerListEditTracksClicked ), NULL, this );
    m_ComposerListCtrl->Disconnect( ID_COPYTO_BASE, ID_COPYTO_BASE + guCOPYTO_MAXCOUNT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnComposerListCopyToClicked ), NULL, this );
    Disconnect( ID_COMPOSER_SAVETOPLAYLIST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnComposerSavePlayListClicked ), NULL, this );

    Disconnect( ID_ALBUMARTIST_PLAY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnAlbumArtistListPlayClicked ), NULL, this );
    Disconnect( ID_ALBUMARTIST_ENQUEUE_AFTER_ALL, ID_ALBUMARTIST_ENQUEUE_AFTER_ARTIST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnAlbumArtistListQueueClicked ), NULL, this );
    Disconnect( ID_ALBUMARTIST_EDITTRACKS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnAlbumArtistListEditTracksClicked ), NULL, this );
    m_AlbumArtistListCtrl->Disconnect( ID_COPYTO_BASE, ID_COPYTO_BASE + guCOPYTO_MAXCOUNT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnAlbumArtistListCopyToClicked ), NULL, this );
    Disconnect( ID_ALBUMARTIST_SAVETOPLAYLIST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnAlbumArtistSavePlayListClicked ), NULL, this );

    Disconnect( ID_TRACKS_PLAY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnSongPlayClicked ), NULL, this );
//    Disconnect( ID_TRACKS_PLAYALL, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnSongPlayAllClicked ), NULL, this );
    Disconnect( ID_TRACKS_ENQUEUE_AFTER_ALL, ID_TRACKS_ENQUEUE_AFTER_ARTIST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnSongQueueClicked ), NULL, this );
    Disconnect( ID_TRACKS_EDITLABELS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnSongsEditLabelsClicked ), NULL, this );
    Disconnect( ID_TRACKS_EDITTRACKS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnSongsEditTracksClicked ), NULL, this );
    Disconnect( ID_TRACKS_SAVETOPLAYLIST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnSongSavePlayListClicked ), NULL, this );
    Disconnect( ID_TRACKS_SET_RATING_0, ID_TRACKS_SET_RATING_5, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnSongSetRating ), NULL, this );
    Disconnect( ID_TRACKS_SET_COLUMN, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnSongSetField ), NULL, this );
    Disconnect( ID_TRACKS_EDIT_COLUMN, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnSongEditField ), NULL, this );
    m_SongListCtrl->Disconnect( ID_COPYTO_BASE, ID_COPYTO_BASE + guCOPYTO_MAXCOUNT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnSongCopyToClicked ), NULL, this );

    Disconnect( ID_TRACKS_BROWSE_GENRE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnSongSelectGenre ), NULL, this );
    Disconnect( ID_TRACKS_BROWSE_ARTIST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnSongSelectArtist ), NULL, this );
    Disconnect( ID_TRACKS_BROWSE_ALBUMARTIST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnSongSelectAlbumArtist ), NULL, this );
    Disconnect( ID_TRACKS_BROWSE_COMPOSER, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnSongSelectComposer ), NULL, this );
    Disconnect( ID_TRACKS_BROWSE_ALBUM, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnSongSelectAlbum ), NULL, this );
    Disconnect( ID_TRACKS_DELETE_LIBRARY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnSongDeleteLibrary ), NULL, this );
    Disconnect( ID_TRACKS_DELETE_DRIVE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnSongDeleteDrive ), NULL, this );

    Disconnect( ID_LIBRARY_SEARCH, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnGoToSearch ), NULL, this );

//    Disconnect( ID_CONFIG_UPDATED, guConfigUpdatedEvent, wxCommandEventHandler( guLibPanel::OnConfigUpdated ), NULL, this );
}

// -------------------------------------------------------------------------------- //
void guLibPanel::CreateControls( void )
{
    wxPanel *           GenrePanel;
    wxPanel *           LabelsPanel;
    wxPanel *           AlbumPanel;
    wxPanel *           ArtistPanel;

	wxBoxSizer *        GenreSizer;
	wxBoxSizer *        LabelsSizer;
	wxBoxSizer *        ArtistSizer;
	wxBoxSizer *        AlbumSizer;
    wxPanel *           SongListPanel;
	wxBoxSizer *        SongListSizer;

    guConfig *          Config = ( guConfig * ) guConfig::Get();

    bool ShowCloseButton = Config->ReadBool( wxT( "ShowPaneCloseButton" ), true, wxT( "general" ) );

	GenreSizer = new wxBoxSizer( wxVERTICAL );
	GenrePanel = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );

	m_GenreListCtrl = new guGeListBox( GenrePanel, this, m_Db, _( "Genres" ) );
	GenreSizer->Add( m_GenreListCtrl, 1, wxEXPAND, LISTCTRL_BORDER );

	GenrePanel->SetSizer( GenreSizer );
	GenrePanel->Layout();
	GenreSizer->Fit( GenrePanel );

    m_AuiManager.AddPane( GenrePanel, wxAuiPaneInfo().Name( wxT( "Genres" ) ).Caption( _( "Genres" ) ).
            MinSize( 50, 50 ).Row( 1 ).
            Position( 1 ).Hide().
            CloseButton( ShowCloseButton ).
            Dockable( true ).Top() );



	LabelsPanel = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	LabelsSizer = new wxBoxSizer( wxVERTICAL );

	m_LabelsListCtrl = new guTaListBox( LabelsPanel, this, m_Db, _( "Labels" ) );
	LabelsSizer->Add( m_LabelsListCtrl, 1, wxEXPAND, LISTCTRL_BORDER );

	LabelsPanel->SetSizer( LabelsSizer );
	LabelsPanel->Layout();
	LabelsSizer->Fit( LabelsPanel );

    m_AuiManager.AddPane( LabelsPanel, wxAuiPaneInfo().Name( wxT( "Labels" ) ).Caption( _( "Labels" ) ).
            MinSize( 50, 50 ).Row( 1 ).
            Position( 0 ).Hide().
            CloseButton( ShowCloseButton ).
            Dockable( true ).Top() );



	ArtistPanel = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	ArtistSizer = new wxBoxSizer( wxVERTICAL );

	m_ArtistListCtrl = new guArListBox( ArtistPanel, this, m_Db, _( "Artists" ) );
	ArtistSizer->Add( m_ArtistListCtrl, 1, wxEXPAND, LISTCTRL_BORDER );

	ArtistPanel->SetSizer( ArtistSizer );
	ArtistPanel->Layout();
	ArtistSizer->Fit( ArtistPanel );

    m_AuiManager.AddPane( ArtistPanel, wxAuiPaneInfo().Name( wxT( "Artists" ) ).Caption( _( "Artists" ) ).
            MinSize( 50, 50 ).Row( 1 ).
            Position( 2 ).Hide().
            CloseButton( ShowCloseButton ).
            Dockable( true ).Top() );


	AlbumPanel = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	AlbumSizer = new wxBoxSizer( wxVERTICAL );

	m_AlbumListCtrl = new guAlListBox( AlbumPanel, this, m_Db, _( "Albums" ) );
	AlbumSizer->Add( m_AlbumListCtrl, 1, wxEXPAND, LISTCTRL_BORDER );

	AlbumPanel->SetSizer( AlbumSizer );
	AlbumPanel->Layout();
	AlbumSizer->Fit( AlbumPanel );

    m_AuiManager.AddPane( AlbumPanel, wxAuiPaneInfo().Name( wxT( "Albums" ) ).Caption( _( "Albums" ) ).
            MinSize( 50, 50 ).Row( 1 ).
            Position( 3 ).Hide().
            CloseButton( ShowCloseButton ).
            Dockable( true ).Top() );


	//
	// Years
	//
	wxPanel * YearPanel = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer * YearsSizer = new wxBoxSizer( wxVERTICAL );

	m_YearListCtrl = new guYeListBox( YearPanel, this, m_Db, _( "Years" ) );
	YearsSizer->Add( m_YearListCtrl, 1, wxEXPAND, LISTCTRL_BORDER );

	YearPanel->SetSizer( YearsSizer );
	YearPanel->Layout();
	YearsSizer->Fit( YearPanel );

    m_AuiManager.AddPane( YearPanel, wxAuiPaneInfo().Name( wxT( "Years" ) ).Caption( _( "Years" ) ).
            MinSize( 50, 50 ).Row( 2 ).
            Position( 0 ).Hide().
            CloseButton( ShowCloseButton ).
            Dockable( true ).Top() );

	//
	// Ratings
	//
	wxPanel * RatingPanel = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer * RatingSizer = new wxBoxSizer( wxVERTICAL );

	m_RatingListCtrl = new guRaListBox( RatingPanel, this, m_Db, _( "Ratings" ) );
	RatingSizer->Add( m_RatingListCtrl, 1, wxEXPAND, LISTCTRL_BORDER );

	RatingPanel->SetSizer( RatingSizer );
	RatingPanel->Layout();
	RatingSizer->Fit( RatingPanel );

    m_AuiManager.AddPane( RatingPanel, wxAuiPaneInfo().Name( wxT( "Ratings" ) ).Caption( _( "Ratings" ) ).
            MinSize( 50, 50 ).Row( 2 ).
            Position( 1 ).Hide().
            CloseButton( ShowCloseButton ).
            Dockable( true ).Top() );

	//
	// Plays
	//
	wxPanel * PlayCountPanel = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer * PlayCountSizer = new wxBoxSizer( wxVERTICAL );

	m_PlayCountListCtrl = new guPcListBox( PlayCountPanel, this, m_Db, _( "Plays" ) );
	PlayCountSizer->Add( m_PlayCountListCtrl, 1, wxEXPAND, LISTCTRL_BORDER );

	PlayCountPanel->SetSizer( PlayCountSizer );
	PlayCountPanel->Layout();
	PlayCountSizer->Fit( PlayCountPanel );

    m_AuiManager.AddPane( PlayCountPanel, wxAuiPaneInfo().Name( wxT( "Plays" ) ).Caption( _( "Plays" ) ).
            MinSize( 50, 50 ).Row( 2 ).
            Position( 2 ).Hide().
            CloseButton( ShowCloseButton ).
            Dockable( true ).Top() );

	//
	// Composers
	//
	wxPanel * ComposerPanel = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer * ComposerSizer = new wxBoxSizer( wxVERTICAL );

	m_ComposerListCtrl = new guCoListBox( ComposerPanel, this, m_Db, _( "Composers" ) );
	ComposerSizer->Add( m_ComposerListCtrl, 1, wxEXPAND, LISTCTRL_BORDER );

	ComposerPanel->SetSizer( ComposerSizer );
	ComposerPanel->Layout();
	ComposerSizer->Fit( ComposerPanel );

    m_AuiManager.AddPane( ComposerPanel, wxAuiPaneInfo().Name( wxT( "Composers" ) ).Caption( _( "Composers" ) ).
            MinSize( 50, 50 ).Row( 2 ).
            Position( 3 ).Hide().
            CloseButton( ShowCloseButton ).
            Dockable( true ).Top() );

	//
	// Album Artists
	//
	wxPanel * AlbumArtistPanel = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer * AlbumArtistSizer = new wxBoxSizer( wxVERTICAL );

	m_AlbumArtistListCtrl = new guAAListBox( AlbumArtistPanel, this, m_Db, _( "Album Artist" ) );
	AlbumArtistSizer->Add( m_AlbumArtistListCtrl, 1, wxEXPAND, LISTCTRL_BORDER );

	AlbumArtistPanel->SetSizer( AlbumArtistSizer );
	AlbumArtistPanel->Layout();
	AlbumArtistSizer->Fit( AlbumArtistPanel );

    m_AuiManager.AddPane( AlbumArtistPanel, wxAuiPaneInfo().Name( wxT( "AlbumArtists" ) ).Caption( _( "Album Artist" ) ).
            MinSize( 50, 50 ).Row( 2 ).
            Position( 4 ).Hide().
            CloseButton( ShowCloseButton ).
            Dockable( true ).Top() );

	//
	// Songs
	//
	SongListPanel = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	SongListSizer = new wxBoxSizer( wxVERTICAL );

	m_SongListCtrl = new guSoListBox( SongListPanel, m_MediaViewer, m_ConfigPath, guLISTVIEW_COLUMN_SELECT|guLISTVIEW_COLUMN_SORTING );
	//ReloadSongs();
	SongListSizer->Add( m_SongListCtrl, 1, wxEXPAND, 5 );

	SongListPanel->SetSizer( SongListSizer );
	SongListPanel->Layout();
	SongListSizer->Fit( SongListPanel );

    m_AuiManager.AddPane( SongListPanel, wxAuiPaneInfo().Name( wxT( "Tracks" ) ).Caption( _( "Tracks" ) ).
            MinSize( 50, 50 ).
            CenterPane() );


	Connect( guPANEL_TIMER_SELECTION, wxEVT_TIMER, wxTimerEventHandler( guLibPanel::OnSelChangedTimer ), NULL, this );
    //
    m_GenreListCtrl->Connect( wxEVT_COMMAND_LISTBOX_SELECTED,  wxListEventHandler( guLibPanel::OnGenreListSelected ), NULL, this );
    m_GenreListCtrl->Connect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxListEventHandler( guLibPanel::OnGenreListActivated ), NULL, this );

    m_LabelsListCtrl->Connect( wxEVT_COMMAND_LISTBOX_SELECTED,  wxListEventHandler( guLibPanel::OnLabelListSelected ), NULL, this );
    m_LabelsListCtrl->Connect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxListEventHandler( guLibPanel::OnLabelListActivated ), NULL, this );

    m_ArtistListCtrl->Connect( wxEVT_COMMAND_LISTBOX_SELECTED,  wxListEventHandler( guLibPanel::OnArtistListSelected ), NULL, this );
    m_ArtistListCtrl->Connect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxListEventHandler( guLibPanel::OnArtistListActivated ), NULL, this );

    m_AlbumListCtrl->Connect( wxEVT_COMMAND_LISTBOX_SELECTED,  wxListEventHandler( guLibPanel::OnAlbumListSelected ), NULL, this );
    m_AlbumListCtrl->Connect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxListEventHandler( guLibPanel::OnAlbumListActivated ), NULL, this );

    m_YearListCtrl->Connect( wxEVT_COMMAND_LISTBOX_SELECTED,  wxListEventHandler( guLibPanel::OnYearListSelected ), NULL, this );
    m_YearListCtrl->Connect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxListEventHandler( guLibPanel::OnYearListActivated ), NULL, this );

    m_RatingListCtrl->Connect( wxEVT_COMMAND_LISTBOX_SELECTED,  wxListEventHandler( guLibPanel::OnRatingListSelected ), NULL, this );
    m_RatingListCtrl->Connect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxListEventHandler( guLibPanel::OnRatingListActivated ), NULL, this );

    m_PlayCountListCtrl->Connect( wxEVT_COMMAND_LISTBOX_SELECTED,  wxListEventHandler( guLibPanel::OnPlayCountListSelected ), NULL, this );
    m_PlayCountListCtrl->Connect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxListEventHandler( guLibPanel::OnPlayCountListActivated ), NULL, this );

    m_ComposerListCtrl->Connect( wxEVT_COMMAND_LISTBOX_SELECTED,  wxListEventHandler( guLibPanel::OnComposerListSelected ), NULL, this );
    m_ComposerListCtrl->Connect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxListEventHandler( guLibPanel::OnComposerListActivated ), NULL, this );

    m_AlbumArtistListCtrl->Connect( wxEVT_COMMAND_LISTBOX_SELECTED,  wxListEventHandler( guLibPanel::OnAlbumArtistListSelected ), NULL, this );
    m_AlbumArtistListCtrl->Connect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxListEventHandler( guLibPanel::OnAlbumArtistListActivated ), NULL, this );

    m_SongListCtrl->Connect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxListEventHandler( guLibPanel::OnSongListActivated ), NULL, this );
    m_SongListCtrl->Connect( wxEVT_COMMAND_LIST_COL_CLICK, wxListEventHandler( guLibPanel::OnSongListColClicked ), NULL, this );

    Connect( ID_GENRE_PLAY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnGenrePlayClicked ), NULL, this );
    Connect( ID_GENRE_ENQUEUE_AFTER_ALL, ID_GENRE_ENQUEUE_AFTER_ARTIST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnGenreQueueClicked ), NULL, this );
    m_GenreListCtrl->Connect( ID_COPYTO_BASE, ID_COPYTO_BASE + guCOPYTO_MAXCOUNT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnGenreCopyToClicked ), NULL, this );
    Connect( ID_GENRE_SAVETOPLAYLIST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnGenreSavePlayListClicked ), NULL, this );

    Connect( ID_LABEL_PLAY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnLabelPlayClicked ), NULL, this );
    Connect( ID_LABEL_ENQUEUE_AFTER_ALL, ID_LABEL_ENQUEUE_AFTER_ARTIST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnLabelQueueClicked ), NULL, this );
    m_LabelsListCtrl->Connect( ID_COPYTO_BASE, ID_COPYTO_BASE + guCOPYTO_MAXCOUNT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnLabelCopyToClicked ), NULL, this );
    Connect( ID_LABEL_SAVETOPLAYLIST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnLabelSavePlayListClicked ), NULL, this );

    Connect( ID_ARTIST_PLAY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnArtistPlayClicked ), NULL, this );
    Connect( ID_ARTIST_ENQUEUE_AFTER_ALL, ID_ARTIST_ENQUEUE_AFTER_ARTIST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnArtistQueueClicked ), NULL, this );
    Connect( ID_ARTIST_EDITLABELS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnArtistEditLabelsClicked ), NULL, this );
    Connect( ID_ARTIST_EDITTRACKS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnArtistEditTracksClicked ), NULL, this );
    m_ArtistListCtrl->Connect( ID_COPYTO_BASE, ID_COPYTO_BASE + guCOPYTO_MAXCOUNT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnArtistCopyToClicked ), NULL, this );
    Connect( ID_ARTIST_SAVETOPLAYLIST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnArtistSavePlayListClicked ), NULL, this );

    Connect( ID_ALBUM_PLAY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnAlbumPlayClicked ), NULL, this );
    Connect( ID_ALBUM_ENQUEUE_AFTER_ALL, ID_ALBUM_ENQUEUE_AFTER_ARTIST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnAlbumQueueClicked ), NULL, this );
    Connect( ID_ALBUM_EDITLABELS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnAlbumEditLabelsClicked ), NULL, this );
    Connect( ID_ALBUM_EDITTRACKS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnAlbumEditTracksClicked ), NULL, this );
    Connect( ID_ALBUM_MANUALCOVER, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnAlbumDownloadCoverClicked ), NULL, this );
    Connect( ID_ALBUM_SELECT_COVER, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnAlbumSelectCoverClicked ), NULL, this );
    Connect( ID_ALBUM_COVER_DELETE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnAlbumDeleteCoverClicked ), NULL, this );
    Connect( ID_ALBUM_COVER_EMBED, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnAlbumEmbedCoverClicked ), NULL, this );
    m_AlbumListCtrl->Connect( ID_COPYTO_BASE, ID_COPYTO_BASE + guCOPYTO_MAXCOUNT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnAlbumCopyToClicked ), NULL, this );
    Connect( ID_ALBUM_SAVETOPLAYLIST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnAlbumSavePlayListClicked ), NULL, this );

    Connect( ID_YEAR_PLAY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnYearListPlayClicked ), NULL, this );
    Connect( ID_YEAR_ENQUEUE_AFTER_ALL, ID_YEAR_ENQUEUE_AFTER_ARTIST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnYearListQueueClicked ), NULL, this );
    Connect( ID_YEAR_EDITTRACKS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnYearListEditTracksClicked ), NULL, this );
    m_YearListCtrl->Connect( ID_COPYTO_BASE, ID_COPYTO_BASE + guCOPYTO_MAXCOUNT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnYearListCopyToClicked ), NULL, this );
    Connect( ID_YEAR_SAVETOPLAYLIST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnYearSavePlayListClicked ), NULL, this );

    Connect( ID_RATING_PLAY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnRatingListPlayClicked ), NULL, this );
    Connect( ID_RATING_ENQUEUE_AFTER_ALL, ID_RATING_ENQUEUE_AFTER_ARTIST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnRatingListQueueClicked ), NULL, this );
    Connect( ID_RATING_EDITTRACKS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnRatingListEditTracksClicked ), NULL, this );
    m_RatingListCtrl->Connect( ID_COPYTO_BASE, ID_COPYTO_BASE + guCOPYTO_MAXCOUNT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnRatingListCopyToClicked ), NULL, this );
    Connect( ID_RATING_SAVETOPLAYLIST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnArtistSavePlayListClicked ), NULL, this );

    Connect( ID_PLAYCOUNT_PLAY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnPlayCountListPlayClicked ), NULL, this );
    Connect( ID_PLAYCOUNT_ENQUEUE_AFTER_ALL, ID_PLAYCOUNT_ENQUEUE_AFTER_ARTIST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnPlayCountListQueueClicked ), NULL, this );
    Connect( ID_PLAYCOUNT_EDITTRACKS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnPlayCountListEditTracksClicked ), NULL, this );
    m_PlayCountListCtrl->Connect( ID_COPYTO_BASE, ID_COPYTO_BASE + guCOPYTO_MAXCOUNT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnPlayCountListCopyToClicked ), NULL, this );
    Connect( ID_PLAYCOUNT_SAVETOPLAYLIST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnPlayCountSavePlayListClicked ), NULL, this );

    Connect( ID_COMPOSER_PLAY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnComposerListPlayClicked ), NULL, this );
    Connect( ID_COMPOSER_ENQUEUE_AFTER_ALL, ID_COMPOSER_ENQUEUE_AFTER_ARTIST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnComposerListQueueClicked ), NULL, this );
    Connect( ID_COMPOSER_EDITTRACKS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnComposerListEditTracksClicked ), NULL, this );
    m_ComposerListCtrl->Connect( ID_COPYTO_BASE, ID_COPYTO_BASE + guCOPYTO_MAXCOUNT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnComposerListCopyToClicked ), NULL, this );
    Connect( ID_COMPOSER_SAVETOPLAYLIST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnComposerSavePlayListClicked ), NULL, this );

    Connect( ID_ALBUMARTIST_PLAY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnAlbumArtistListPlayClicked ), NULL, this );
    Connect( ID_ALBUMARTIST_ENQUEUE_AFTER_ALL, ID_ALBUMARTIST_ENQUEUE_AFTER_ARTIST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnAlbumArtistListQueueClicked ), NULL, this );
    Connect( ID_ALBUMARTIST_EDITTRACKS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnAlbumArtistListEditTracksClicked ), NULL, this );
    m_AlbumArtistListCtrl->Connect( ID_COPYTO_BASE, ID_COPYTO_BASE + guCOPYTO_MAXCOUNT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnAlbumArtistListCopyToClicked ), NULL, this );
    Connect( ID_ALBUMARTIST_SAVETOPLAYLIST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnAlbumArtistSavePlayListClicked ), NULL, this );

    Connect( ID_TRACKS_PLAY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnSongPlayClicked ), NULL, this );
    Connect( ID_TRACKS_ENQUEUE_AFTER_ALL, ID_TRACKS_ENQUEUE_AFTER_ARTIST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnSongQueueClicked ), NULL, this );
    Connect( ID_TRACKS_EDITLABELS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnSongsEditLabelsClicked ), NULL, this );
    Connect( ID_TRACKS_EDITTRACKS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnSongsEditTracksClicked ), NULL, this );
    Connect( ID_TRACKS_SAVETOPLAYLIST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnSongSavePlayListClicked ), NULL, this );
    Connect( ID_TRACKS_SET_RATING_0, ID_TRACKS_SET_RATING_5, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnSongSetRating ), NULL, this );
    Connect( ID_TRACKS_SET_COLUMN, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnSongSetField ), NULL, this );
    Connect( ID_TRACKS_EDIT_COLUMN, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnSongEditField ), NULL, this );
    m_SongListCtrl->Connect( ID_COPYTO_BASE, ID_COPYTO_BASE + guCOPYTO_MAXCOUNT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnSongCopyToClicked ), NULL, this );

    Connect( ID_TRACKS_BROWSE_GENRE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnSongSelectGenre ), NULL, this );
    Connect( ID_TRACKS_BROWSE_ARTIST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnSongSelectArtist ), NULL, this );
    Connect( ID_TRACKS_BROWSE_ALBUMARTIST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnSongSelectAlbumArtist ), NULL, this );
    Connect( ID_TRACKS_BROWSE_COMPOSER, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnSongSelectComposer ), NULL, this );
    Connect( ID_TRACKS_BROWSE_ALBUM, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnSongSelectAlbum ), NULL, this );
    Connect( ID_TRACKS_DELETE_LIBRARY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnSongDeleteLibrary ), NULL, this );
    Connect( ID_TRACKS_DELETE_DRIVE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnSongDeleteDrive ), NULL, this );

    Connect( ID_LIBRARY_SEARCH, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guLibPanel::OnGoToSearch ), NULL, this );

//    Connect( ID_CONFIG_UPDATED, guConfigUpdatedEvent, wxCommandEventHandler( guLibPanel::OnConfigUpdated ), NULL, this );
}

// -------------------------------------------------------------------------------- //
void guLibPanel::LoadLastLayout( void )
{
    guConfig * Config = ( guConfig * ) guConfig::Get();

    wxString LibraryLayout = Config->ReadStr( wxT( "Layout" ), wxEmptyString, m_ConfigPath );
    if( Config->GetIgnoreLayouts() || LibraryLayout.IsEmpty() )
    {
        m_VisiblePanels = guPANEL_LIBRARY_VISIBLE_DEFAULT;
        LibraryLayout  = wxT( "layout2|name=Genres;caption=" ) + wxString( _( "Genres" ) );
        LibraryLayout += wxT( ";state=2099196;dir=1;layer=0;row=1;pos=1;prop=113793;bestw=50;besth=50;minw=50;minh=50;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|" );
        LibraryLayout += wxT( "name=Labels;caption=" ) + wxString( _( "Labels" ) );
        LibraryLayout += wxT( ";state=2099198;dir=1;layer=0;row=1;pos=0;prop=66995;bestw=50;besth=50;minw=50;minh=50;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|" );
        LibraryLayout += wxT( "name=Artists;caption=" ) + wxString( _( "Artists" ) );
        LibraryLayout += wxT( ";state=2099196;dir=1;layer=0;row=1;pos=2;prop=80788;bestw=50;besth=50;minw=50;minh=50;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|" );
        LibraryLayout += wxT( "name=Albums;caption=" ) + wxString( _( "Albums" ) );
        LibraryLayout += wxT( ";state=2099196;dir=1;layer=0;row=1;pos=3;prop=138424;bestw=50;besth=50;minw=50;minh=50;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|" );
        LibraryLayout += wxT( "name=Years;caption=" ) + wxString( _( "Years" ) );
        LibraryLayout += wxT( ";state=2099198;dir=1;layer=0;row=2;pos=0;prop=100000;bestw=50;besth=50;minw=50;minh=50;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|" );
        LibraryLayout += wxT( "name=Ratings;caption=" ) + wxString( _( "Ratings" ) );
        LibraryLayout += wxT( ";state=2099198;dir=1;layer=0;row=2;pos=1;prop=100000;bestw=50;besth=50;minw=50;minh=50;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|" );
        LibraryLayout += wxT( "name=Plays;caption=" ) + wxString( _( "Plays" ) );
        LibraryLayout += wxT( ";state=2099198;dir=1;layer=0;row=2;pos=2;prop=100000;bestw=50;besth=50;minw=50;minh=50;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|" );
        LibraryLayout += wxT( "name=Composers;caption=" ) + wxString( _( "Composers" ) );
        LibraryLayout += wxT( ";state=2099198;dir=1;layer=0;row=2;pos=1;prop=100000;bestw=50;besth=50;minw=50;minh=50;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|" );
        LibraryLayout += wxT( "name=AlbumArtists;caption=" ) + wxString( _( "Album Artists" ) );
        LibraryLayout += wxT( ";state=2099198;dir=1;layer=0;row=2;pos=0;prop=100000;bestw=50;besth=50;minw=50;minh=50;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|" );
        LibraryLayout += wxT( "name=Tracks;caption=" ) + wxString( _( "Tracks" ) );
        LibraryLayout += wxT( ";state=768;dir=5;layer=0;row=0;pos=0;prop=100000;bestw=50;besth=50;minw=50;minh=50;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|" );
        LibraryLayout += wxT( "dock_size(1,0,1)=190|dock_size(5,0,0)=52|" );
    }

    //m_AuiManager.LoadPerspective( LibraryLayout, true );
    LoadPerspective( LibraryLayout, m_VisiblePanels );
}

// -------------------------------------------------------------------------------- //
void guLibPanel::InitPanelData( void )
{
    guLogMessage( wxT( "guLibPanel::IniPanelData( %i )" ), m_BaseCommand );

    m_PanelNames.Empty();
    m_PanelNames.Add( wxT( "Labels" ) );
    m_PanelNames.Add( wxT( "Genres" ) );
    m_PanelNames.Add( wxT( "Artists" ) );
    m_PanelNames.Add( wxT( "Composers" ) );
    m_PanelNames.Add( wxT( "AlbumArtists" ) );
    m_PanelNames.Add( wxT( "Albums" ) );
    m_PanelNames.Add( wxT( "Years" ) );
    m_PanelNames.Add( wxT( "Ratings" ) );
    m_PanelNames.Add( wxT( "Plays" ) );

    m_PanelIds.Empty();
    m_PanelIds.Add( guPANEL_LIBRARY_LABELS );
    m_PanelIds.Add( guPANEL_LIBRARY_GENRES );
    m_PanelIds.Add( guPANEL_LIBRARY_ARTISTS );
    m_PanelIds.Add( guPANEL_LIBRARY_COMPOSERS );
    m_PanelIds.Add( guPANEL_LIBRARY_ALBUMARTISTS );
    m_PanelIds.Add( guPANEL_LIBRARY_ALBUMS );
    m_PanelIds.Add( guPANEL_LIBRARY_YEARS );
    m_PanelIds.Add( guPANEL_LIBRARY_RATINGS );
    m_PanelIds.Add( guPANEL_LIBRARY_PLAYCOUNT );

    m_PanelCmdIds.Empty();
    m_PanelCmdIds.Add( m_BaseCommand + guLIBRARY_ELEMENT_LABELS );
    m_PanelCmdIds.Add( m_BaseCommand + guLIBRARY_ELEMENT_GENRES );
    m_PanelCmdIds.Add( m_BaseCommand + guLIBRARY_ELEMENT_ARTISTS );
    m_PanelCmdIds.Add( m_BaseCommand + guLIBRARY_ELEMENT_COMPOSERS );
    m_PanelCmdIds.Add( m_BaseCommand + guLIBRARY_ELEMENT_ALBUMARTISTS );
    m_PanelCmdIds.Add( m_BaseCommand + guLIBRARY_ELEMENT_ALBUMS );
    m_PanelCmdIds.Add( m_BaseCommand + guLIBRARY_ELEMENT_YEARS );
    m_PanelCmdIds.Add( m_BaseCommand + guLIBRARY_ELEMENT_RATINGS );
    m_PanelCmdIds.Add( m_BaseCommand + guLIBRARY_ELEMENT_PLAYCOUNT );
}

// -------------------------------------------------------------------------------- //
void guLibPanel::ReloadControls( void )
{
//    guLogMessage( wxT( "ReloadControls...%08X" ), m_VisiblePanels );
    //m_Db->LoadCache();
    m_UpdateLock = true;
    ReloadLabels( false );
    ReloadGenres( false );
    ReloadAlbumArtists( false );
    ReloadArtists( false );
    ReloadComposers( false );
    ReloadAlbums( false );
    ReloadYears( false );
    ReloadRatings( false );
    ReloadPlayCounts( false );
    ReloadSongs( false );
    m_UpdateLock = false;
}

// -------------------------------------------------------------------------------- //
void guLibPanel::ClearSearchText( void )
{
    if( m_MediaViewer )
    {
        m_MediaViewer->ClearSearchText();
    }
}

// -------------------------------------------------------------------------------- //
bool guLibPanel::DoTextSearch( const wxString &searchtext )
{
    guLogMessage( wxT( "guLibPanel::DoTextSearch( '%s' )" ), searchtext.c_str() );

    if( m_LastTextFilter == searchtext )
    {
        return true;
    }

    m_LastTextFilter = searchtext; //m_InputTextCtrl->GetLineText( 0 );
    if( !m_LastTextFilter.IsEmpty() )
    {
        if( m_LastTextFilter.Length() > 0 )
        {
            wxArrayString Words = guSplitWords( m_LastTextFilter );

            m_Db->SetTeFilters( Words, m_UpdateLock );
            if( !m_UpdateLock )
            {
                m_UpdateLock = true;
                ReloadLabels();
                ReloadGenres();
                ReloadAlbumArtists();
                ReloadArtists();
                ReloadComposers();
                ReloadAlbums();
                ReloadYears();
                ReloadRatings();
                ReloadPlayCounts();
                ReloadSongs();
                m_UpdateLock = false;
            }
//            m_InputTextCtrl->ShowCancelButton( true );
        }

        return true;
    }
    else
    {
        wxArrayString Words;
        //guLogMessage( wxT( "guLibPanel::SearchCancelled" ) );
        //m_InputTextCtrl->Clear();
        m_Db->SetTeFilters( Words, m_UpdateLock );
        m_UpdateLock = true;
    //    if( !m_UpdateLock )
    //    {
            ReloadLabels( false );
            ReloadGenres( false );
            ReloadAlbumArtists();
            ReloadArtists( false );
            ReloadComposers( false );
            ReloadAlbums( false );
            ReloadYears( false );
            ReloadRatings( false );
            ReloadPlayCounts( false );
            ReloadSongs( false );
            m_UpdateLock = false;
    //    }
//        m_InputTextCtrl->ShowCancelButton( false );
        return false;
    }
}

// -------------------------------------------------------------------------------- //
// GenreListBox Events
// -------------------------------------------------------------------------------- //

// -------------------------------------------------------------------------------- //
void guLibPanel::OnGenreListSelected( wxListEvent &event )
{
    if( m_UpdateLock )
        return;
    m_SelChangedObject = guPANEL_LIBRARY_GENRES;
    if( m_SelChangedTimer.IsRunning() )
        m_SelChangedTimer.Stop();
    m_SelChangedTimer.Start( guPANEL_TIMER_SELCHANGED, wxTIMER_ONE_SHOT );
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnGenreListActivated( wxListEvent &event )
{
    guTrackArray Songs;
    m_GenreListCtrl->GetSelectedSongs( &Songs );
    if( Songs.Count() )
    {
        guConfig * Config = ( guConfig * ) guConfig::Get();
        if( Config )
        {
            if( Config->ReadBool( wxT( "DefaultActionEnqueue" ), false, wxT( "general" ) ) )
            {
                m_PlayerPanel->AddToPlayList( Songs );
            }
            else
            {
                m_PlayerPanel->SetPlayList( Songs );
            }
        }
    }
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnGenrePlayClicked( wxCommandEvent &event )
{
    guTrackArray Songs;
    m_GenreListCtrl->GetSelectedSongs( &Songs );
    m_PlayerPanel->SetPlayList( Songs );
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnGenreQueueClicked( wxCommandEvent &event )
{
    guTrackArray Songs;
    m_GenreListCtrl->GetSelectedSongs( &Songs );
    if( Songs.Count() )
    {
        m_PlayerPanel->AddToPlayList( Songs, true, event.GetId() - ID_GENRE_ENQUEUE_AFTER_ALL );
    }
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnGenreCopyToClicked( wxCommandEvent &event )
{
    guTrackArray * Tracks = new guTrackArray();
    m_GenreListCtrl->GetSelectedSongs( Tracks );

    int Index = event.GetId() - ID_COPYTO_BASE;
    if( Index >= guCOPYTO_DEVICE_BASE )
    {
        Index -= guCOPYTO_DEVICE_BASE;
        event.SetId( ID_MAINFRAME_COPYTODEVICE_TRACKS );
    }
    else
    {
        event.SetId( ID_MAINFRAME_COPYTO );
    }
    event.SetInt( Index );
    event.SetClientData( ( void * ) Tracks );
    wxPostEvent( wxTheApp->GetTopWindow(), event );
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnGenreSavePlayListClicked( wxCommandEvent &event )
{
    int index;
    int count;
    wxArrayInt NewSongs;
    guTrackArray Tracks;
    m_GenreListCtrl->GetSelectedSongs( &Tracks );

    count = Tracks.Count();
    for( index = 0; index < count; index++ )
    {
        NewSongs.Add( Tracks[ index ].m_SongId );
    }

    SaveToPlayList( NewSongs );
}

// -------------------------------------------------------------------------------- //
// LabelsListBox Events
// -------------------------------------------------------------------------------- //

// -------------------------------------------------------------------------------- //
void guLibPanel::OnLabelListSelected( wxListEvent &event )
{
    if( m_UpdateLock )
        return;
    m_SelChangedObject = guPANEL_LIBRARY_LABELS;
    if( m_SelChangedTimer.IsRunning() )
        m_SelChangedTimer.Stop();
    m_SelChangedTimer.Start( guPANEL_TIMER_SELCHANGED, wxTIMER_ONE_SHOT );
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnLabelListActivated( wxListEvent &event )
{
    guTrackArray Songs;
    m_LabelsListCtrl->GetSelectedSongs( &Songs );
    if( Songs.Count() )
    {
        guConfig * Config = ( guConfig * ) guConfig::Get();
        if( Config )
        {
            if( Config->ReadBool( wxT( "DefaultActionEnqueue" ), false, wxT( "general" ) ) )
            {
                m_PlayerPanel->AddToPlayList( Songs );
            }
            else
            {
                m_PlayerPanel->SetPlayList( Songs );
            }
        }
    }
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnLabelPlayClicked( wxCommandEvent &event )
{
    guTrackArray Songs;
    m_LabelsListCtrl->GetSelectedSongs( &Songs );
    m_PlayerPanel->SetPlayList( Songs );
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnLabelQueueClicked( wxCommandEvent &event )
{
    guTrackArray Songs;
    m_LabelsListCtrl->GetSelectedSongs( &Songs );
    if( Songs.Count() )
    {
        m_PlayerPanel->AddToPlayList( Songs, true, event.GetId() - ID_LABEL_ENQUEUE_AFTER_ALL );
    }
}

//// -------------------------------------------------------------------------------- //
//void guLibPanel::OnLabelClearSelectClicked( wxCommandEvent &event )
//{
//    m_LabelsListCtrl->ClearSelection();
//}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnLabelCopyToClicked( wxCommandEvent &event )
{
    guTrackArray * Tracks = new guTrackArray();
    m_LabelsListCtrl->GetSelectedSongs( Tracks );

    int Index = event.GetId() - ID_COPYTO_BASE;
    if( Index >= guCOPYTO_DEVICE_BASE )
    {
        Index -= guCOPYTO_DEVICE_BASE;
        event.SetId( ID_MAINFRAME_COPYTODEVICE_TRACKS );
    }
    else
    {
        event.SetId( ID_MAINFRAME_COPYTO );
    }
    event.SetInt( Index );
    event.SetClientData( ( void * ) Tracks );
    wxPostEvent( wxTheApp->GetTopWindow(), event );
}

// -------------------------------------------------------------------------------- //
void guLibPanel::UpdateLabels( void )
{
    m_UpdateLock = true;
    ReloadLabels( false );
    ReloadGenres( false );
    ReloadAlbumArtists( false );
    ReloadArtists( false );
    ReloadComposers( false );
    ReloadAlbums( false );
    ReloadRatings( false );
    ReloadYears( false );
    ReloadPlayCounts( false );
    ReloadSongs( false );
    m_UpdateLock = false;
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnLabelSavePlayListClicked( wxCommandEvent &event )
{
    int index;
    int count;
    wxArrayInt NewSongs;
    guTrackArray Tracks;
    m_LabelsListCtrl->GetSelectedSongs( &Tracks );

    count = Tracks.Count();
    for( index = 0; index < count; index++ )
    {
        NewSongs.Add( Tracks[ index ].m_SongId );
    }

    SaveToPlayList( NewSongs );
}

// -------------------------------------------------------------------------------- //
// ArtistListBox Events
// -------------------------------------------------------------------------------- //

// -------------------------------------------------------------------------------- //
void guLibPanel::OnArtistListSelected( wxListEvent &event )
{
    if( m_UpdateLock )
        return;
    m_SelChangedObject = guPANEL_LIBRARY_ARTISTS;
    if( m_SelChangedTimer.IsRunning() )
        m_SelChangedTimer.Stop();
    m_SelChangedTimer.Start( guPANEL_TIMER_SELCHANGED, wxTIMER_ONE_SHOT );
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnArtistListActivated( wxListEvent &event )
{
    guTrackArray Songs;
    m_ArtistListCtrl->GetSelectedSongs( &Songs );
    if( Songs.Count() )
    {
        guConfig * Config = ( guConfig * ) guConfig::Get();
        if( Config )
        {
            if( Config->ReadBool( wxT( "DefaultActionEnqueue" ), false, wxT( "general" ) ) )
            {
                m_PlayerPanel->AddToPlayList( Songs );
            }
            else
            {
                m_PlayerPanel->SetPlayList( Songs );
            }
        }
    }
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnArtistPlayClicked( wxCommandEvent &event )
{
    guTrackArray Songs;
    m_ArtistListCtrl->GetSelectedSongs( &Songs );
    m_PlayerPanel->SetPlayList( Songs );
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnArtistQueueClicked( wxCommandEvent &event )
{
    guTrackArray Songs;
    m_ArtistListCtrl->GetSelectedSongs( &Songs );
    if( Songs.Count() )
    {
        m_PlayerPanel->AddToPlayList( Songs, true, event.GetId() - ID_ARTIST_ENQUEUE_AFTER_ALL );
    }
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnArtistEditLabelsClicked( wxCommandEvent &event )
{
    guListItems Artists;
    m_ArtistListCtrl->GetSelectedItems( &Artists );
    if( Artists.Count() )
    {
        guArrayListItems LabelSets = m_Db->GetArtistsLabels( m_ArtistListCtrl->GetSelectedItems() );

        guLabelEditor * LabelEditor = new guLabelEditor( this, m_Db, _( "Artist Labels Editor" ), false, &Artists, &LabelSets );
        if( LabelEditor )
        {
            if( LabelEditor->ShowModal() == wxID_OK )
            {
                // Update the labels in the artists files
                m_Db->UpdateArtistsLabels( LabelSets );
            }

            UpdateLabels();

            LabelEditor->Destroy();
        }
    }
}

// -------------------------------------------------------------------------------- //
void guLibPanel::DoEditTracks( guTrackArray &tracks )
{
    guImagePtrArray Images;
    wxArrayString Lyrics;
    wxArrayInt ChangedFlags;
    int Index;
    int Count = tracks.Count();
    for( Index = Count - 1; Index >= 0; Index-- )
    {
        if( tracks[ Index ].m_Offset )
        {
            tracks.RemoveAt( Index );
        }
    }
    if( tracks.Count() )
    {
        guTrackEditor * TrackEditor = new guTrackEditor( this, m_Db, &tracks, &Images, &Lyrics, &ChangedFlags );
        if( TrackEditor )
        {
            if( TrackEditor->ShowModal() == wxID_OK )
            {
                UpdateTracks( tracks, Images, Lyrics, ChangedFlags );

                // Update the track in database, playlist, etc
                m_MediaViewer->UpdatedTracks( guUPDATED_TRACKS_NONE, &tracks );
            }
            guImagePtrArrayClean( &Images );
            TrackEditor->Destroy();
        }
    }
}

// -------------------------------------------------------------------------------- //
void guLibPanel::UpdateTracks( const guTrackArray &tracks, const wxArrayInt &changedflags )
{
    guImagePtrArray Images;
    wxArrayString Lyrics;
    UpdateTracks( tracks, Images, Lyrics, changedflags );
}

// -------------------------------------------------------------------------------- //
void guLibPanel::UpdateTracks( const guTrackArray &tracks, const guImagePtrArray &images,
                    const wxArrayString &lyrics, const wxArrayInt &changedflags )
{
    m_MediaViewer->UpdateTracks( tracks, images, lyrics, changedflags );
}

// -------------------------------------------------------------------------------- //
void guLibPanel::UpdateTracksImages( const guTrackArray &tracks, const guImagePtrArray &images, const wxArrayInt &changedflags )
{
    guUpdateImages( tracks, images, changedflags );
}

// -------------------------------------------------------------------------------- //
void guLibPanel::UpdateTracksLyrics( const guTrackArray &tracks, const wxArrayString &lyrics, const wxArrayInt &changedflags )
{
    guUpdateLyrics( tracks, lyrics, changedflags );
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnArtistEditTracksClicked( wxCommandEvent &event )
{
    guTrackArray Tracks;
    m_ArtistListCtrl->GetSelectedSongs( &Tracks );
    if( !Tracks.Count() )
        return;

    DoEditTracks( Tracks );
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnArtistCopyToClicked( wxCommandEvent &event )
{
    guTrackArray * Tracks = new guTrackArray();
    m_ArtistListCtrl->GetSelectedSongs( Tracks );

    int Index = event.GetId() - ID_COPYTO_BASE;
    if( Index >= guCOPYTO_DEVICE_BASE )
    {
        Index -= guCOPYTO_DEVICE_BASE;
        event.SetId( ID_MAINFRAME_COPYTODEVICE_TRACKS );
    }
    else
    {
        event.SetId( ID_MAINFRAME_COPYTO );
    }
    event.SetInt( Index );
    event.SetClientData( ( void * ) Tracks );
    wxPostEvent( wxTheApp->GetTopWindow(), event );
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnArtistSavePlayListClicked( wxCommandEvent &event )
{
    int index;
    int count;
    wxArrayInt NewSongs;
    guTrackArray Tracks;
    m_ArtistListCtrl->GetSelectedSongs( &Tracks );

    count = Tracks.Count();
    for( index = 0; index < count; index++ )
    {
        NewSongs.Add( Tracks[ index ].m_SongId );
    }

    SaveToPlayList( NewSongs );
}

// -------------------------------------------------------------s------------------- //
// AlbumListBox Events
// -------------------------------------------------------------------------------- //

// -------------------------------------------------------------------------------- //
void guLibPanel::OnAlbumListSelected( wxListEvent &event )
{
    if( m_UpdateLock )
        return;
    m_SelChangedObject = guPANEL_LIBRARY_ALBUMS;
    if( m_SelChangedTimer.IsRunning() )
        m_SelChangedTimer.Stop();
    m_SelChangedTimer.Start( guPANEL_TIMER_SELCHANGED, wxTIMER_ONE_SHOT );
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnAlbumListActivated( wxListEvent &event )
{
    guTrackArray Songs;
    m_AlbumListCtrl->GetSelectedSongs( &Songs );
    if( Songs.Count() )
    {
        guConfig * Config = ( guConfig * ) guConfig::Get();
        if( Config )
        {
            if( Config->ReadBool( wxT( "DefaultActionEnqueue" ), false , wxT( "general" )) )
            {
                m_PlayerPanel->AddToPlayList( Songs );
            }
            else
            {
                m_PlayerPanel->SetPlayList( Songs );
            }
        }
    }
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnAlbumPlayClicked( wxCommandEvent &event )
{
    guTrackArray Songs;
    m_AlbumListCtrl->GetSelectedSongs( &Songs );
    m_PlayerPanel->SetPlayList( Songs );
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnAlbumQueueClicked( wxCommandEvent &event )
{
    guTrackArray Songs;
    m_AlbumListCtrl->GetSelectedSongs( &Songs );
    if( Songs.Count() )
    {
        m_PlayerPanel->AddToPlayList( Songs, true, event.GetId() - ID_ALBUM_ENQUEUE_AFTER_ALL );
    }
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnAlbumEditLabelsClicked( wxCommandEvent &event )
{
    guListItems Albums;
    m_AlbumListCtrl->GetSelectedItems( &Albums );
    if( Albums.Count() )
    {
        guArrayListItems LabelSets = m_Db->GetAlbumsLabels( m_AlbumListCtrl->GetSelectedItems() );

        guLabelEditor * LabelEditor = new guLabelEditor( this, m_Db, _( "Albums Labels Editor" ),
                            false, &Albums, &LabelSets );
        if( LabelEditor )
        {
            if( LabelEditor->ShowModal() == wxID_OK )
            {
                m_Db->UpdateAlbumsLabels( LabelSets );
            }
            LabelEditor->Destroy();

            UpdateLabels();
        }
    }
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnAlbumEditTracksClicked( wxCommandEvent &event )
{
    guTrackArray Tracks;
    //m_AlbumListCtrl->GetSelectedSongs( &Songs );
    m_Db->GetAlbumsSongs( m_AlbumListCtrl->GetSelectedItems(), &Tracks, true );
    if( !Tracks.Count() )
        return;

    DoEditTracks( Tracks );
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnAlbumDownloadCoverClicked( wxCommandEvent &event )
{
    wxArrayInt Albums = m_AlbumListCtrl->GetSelectedItems();
    if( Albums.Count() )
    {
        m_MediaViewer->DownloadAlbumCover( Albums[ 0 ] );
    }
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnAlbumSelectCoverClicked( wxCommandEvent &event )
{
    wxArrayInt Albums = m_AlbumListCtrl->GetSelectedItems();
    if( Albums.Count() )
    {
        m_MediaViewer->SelectAlbumCover( Albums[ 0 ] );
    }
}


// -------------------------------------------------------------------------------- //
void guLibPanel::DoDeleteAlbumCover( const int albumid )
{
    int CoverId = m_Db->GetAlbumCoverId( albumid );
    if( CoverId > 0 )
    {
        wxString CoverPath = m_Db->GetCoverPath( CoverId );
        if( !CoverPath.IsEmpty() )
        {
            if( !wxRemoveFile( CoverPath ) )
            {
                guLogError( wxT( "Could not remove the cover file '%s'" ), CoverPath.c_str() );
            }
        }
    }
    m_Db->SetAlbumCover( albumid, wxEmptyString );

    wxCommandEvent evt( wxEVT_COMMAND_MENU_SELECTED, ID_ALBUM_COVER_CHANGED );
    evt.SetInt( albumid );
    evt.SetClientData( this );
    wxPostEvent( wxTheApp->GetTopWindow(), evt );
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnAlbumDeleteCoverClicked( wxCommandEvent &event )
{
    wxArrayInt Albums = m_AlbumListCtrl->GetSelectedItems();
    if( Albums.Count() )
    {
        if( wxMessageBox( _( "Are you sure to delete the selected album cover?" ),
                          _( "Confirm" ),
                          wxICON_QUESTION|wxYES_NO|wxNO_DEFAULT, this ) == wxYES )
        {
            m_MediaViewer->DeleteAlbumCover( Albums );
        }
    }
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnAlbumEmbedCoverClicked( wxCommandEvent &event )
{
    int Index;
    int Count;
    wxArrayInt SelectedAlbums = m_AlbumListCtrl->GetSelectedItems();
    if( ( Count = SelectedAlbums.Count() ) )
    {
        for( Index = 0; Index < Count; Index++ )
        {
            m_MediaViewer->EmbedAlbumCover( SelectedAlbums[ Index ] );
        }
    }
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnAlbumCopyToClicked( wxCommandEvent &event )
{
    guLogMessage( wxT( "guLibPanel::OnAlbumCopyToClicked %i" ), event.GetId() );
    guTrackArray * Tracks = new guTrackArray();
    m_AlbumListCtrl->GetSelectedSongs( Tracks );

    int Index = event.GetId() - ID_COPYTO_BASE;
    if( Index >= guCOPYTO_DEVICE_BASE )
    {
        Index -= guCOPYTO_DEVICE_BASE;
        event.SetId( ID_MAINFRAME_COPYTODEVICE_TRACKS );
    }
    else
    {
        event.SetId( ID_MAINFRAME_COPYTO );
    }
    event.SetInt( Index );
    event.SetClientData( ( void * ) Tracks );
    wxPostEvent( wxTheApp->GetTopWindow(), event );
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnAlbumSavePlayListClicked( wxCommandEvent &event )
{
    int index;
    int count;
    wxArrayInt NewSongs;
    guTrackArray Tracks;
    m_AlbumListCtrl->GetSelectedSongs( &Tracks );

    count = Tracks.Count();
    for( index = 0; index < count; index++ )
    {
        NewSongs.Add( Tracks[ index ].m_SongId );
    }

    SaveToPlayList( NewSongs );
}

// -------------------------------------------------------------------------------- //
// SongListBox Events
// -------------------------------------------------------------------------------- //

// -------------------------------------------------------------------------------- //
void guLibPanel::OnSongListActivated( wxListEvent &event )
{
    guTrackArray Songs;
    m_SongListCtrl->GetSelectedSongs( &Songs );
    if( Songs.Count() )
    {
        guConfig * Config = ( guConfig * ) guConfig::Get();
        if( Config )
        {
            if( Config->ReadBool( wxT( "DefaultActionEnqueue" ), false, wxT( "general" ) ) )
            {
                m_PlayerPanel->AddToPlayList( Songs );
            }
            else
            {
                m_PlayerPanel->SetPlayList( Songs );
            }
        }
    }
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnSongPlayClicked( wxCommandEvent &event )
{
    guTrackArray Songs;
    m_SongListCtrl->GetSelectedSongs( &Songs );
    if( !Songs.Count() )
    {
        m_SongListCtrl->GetAllSongs( &Songs );
    }
    m_PlayerPanel->SetPlayList( Songs );
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnSongQueueClicked( wxCommandEvent &event )
{
    guTrackArray Songs;
    m_SongListCtrl->GetSelectedSongs( &Songs );
    if( Songs.Count() )
    {
        m_PlayerPanel->AddToPlayList( Songs, true, event.GetId() - ID_TRACKS_ENQUEUE_AFTER_ALL );
    }
}

// -------------------------------------------------------------------------------- //
void guLibPanel::PlayAllTracks( const bool enqueue )
{
    guTrackArray Tracks;
    m_SongListCtrl->GetAllSongs( &Tracks );
    if( enqueue )
        m_PlayerPanel->AddToPlayList( Tracks );
    else
        m_PlayerPanel->SetPlayList( Tracks );
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnSongsEditLabelsClicked( wxCommandEvent &event )
{
    guListItems Tracks;
    m_SongListCtrl->GetSelectedItems( &Tracks, false );
    if( Tracks.Count() )
    {
        guArrayListItems LabelSets = m_Db->GetSongsLabels( m_SongListCtrl->GetSelectedItems() );

        guLabelEditor * LabelEditor = new guLabelEditor( this, m_Db, _( "Tracks Labels Editor" ), false, &Tracks, &LabelSets );
        if( LabelEditor )
        {
            if( LabelEditor->ShowModal() == wxID_OK )
            {
                // Update the labels in the files
                m_Db->UpdateSongsLabels( LabelSets );
            }

            UpdateLabels();

            LabelEditor->Destroy();
        }
    }
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnSongsEditTracksClicked( wxCommandEvent &event )
{
    guTrackArray Tracks;
    m_SongListCtrl->GetSelectedSongs( &Tracks );
    if( !Tracks.Count() )
        return;

    DoEditTracks( Tracks );
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnSongCopyToClicked( wxCommandEvent &event )
{
    guTrackArray Tracks;
    m_SongListCtrl->GetSelectedSongs( &Tracks );

    if( !Tracks.Count() )
        m_SongListCtrl->GetAllSongs( &Tracks );

    if( Tracks.Count() )
    {
        int Index = event.GetId() - ID_COPYTO_BASE;
        if( Index >= guCOPYTO_DEVICE_BASE )
        {
            Index -= guCOPYTO_DEVICE_BASE;
            event.SetId( ID_MAINFRAME_COPYTODEVICE_TRACKS );
        }
        else
        {
            event.SetId( ID_MAINFRAME_COPYTO );
        }
        event.SetInt( Index );
        event.SetClientData( ( void * ) new guTrackArray( Tracks ) );
        wxPostEvent( wxTheApp->GetTopWindow(), event );
    }
}

// -------------------------------------------------------------------------------- //
void guLibPanel::SaveToPlayList( const wxArrayInt &tracks )
{
    if( tracks.Count() )
    {
        guListItems PlayLists;
        m_Db->GetPlayLists( &PlayLists, guPLAYLIST_TYPE_STATIC );

        guPlayListAppend * PlayListAppendDlg = new guPlayListAppend( wxTheApp->GetTopWindow(), m_Db, &tracks, &PlayLists );

        if( PlayListAppendDlg->ShowModal() == wxID_OK )
        {
            int PLId;
            int Selected = PlayListAppendDlg->GetSelectedPlayList();
            if( Selected == -1 )
            {
                wxString PLName = PlayListAppendDlg->GetPlaylistName();
                if( PLName.IsEmpty() )
                {
                    PLName = _( "UnNamed" );
                }
                PLId = m_Db->CreateStaticPlayList( PLName, tracks );
            }
            else
            {
                PLId = PlayLists[ Selected ].m_Id;
                wxArrayInt OldSongs;
                m_Db->GetPlayListSongIds( PLId, &OldSongs );
                if( PlayListAppendDlg->GetSelectedPosition() == 0 ) // BEGIN
                {
                    m_Db->UpdateStaticPlayList( PLId, tracks );
                    m_Db->AppendStaticPlayList( PLId, OldSongs );
                }
                else                                                // END
                {
                    m_Db->AppendStaticPlayList( PLId, tracks );
                }
            }
            m_Db->UpdateStaticPlayListFile( PLId );
            UpdatePlaylists();
        }
        PlayListAppendDlg->Destroy();
    }
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnSongSavePlayListClicked( wxCommandEvent &event )
{
    int index;
    int count;
    wxArrayInt NewSongs;
    guTrackArray Tracks;
    m_SongListCtrl->GetSelectedSongs( &Tracks );

    if( ( count = Tracks.Count() ) )
    {
        for( index = 0; index < count; index++ )
        {
            NewSongs.Add( Tracks[ index ].m_SongId );
        }
    }
    else
    {
        m_SongListCtrl->GetAllSongs( &Tracks );
        count = Tracks.Count();
        for( index = 0; index < count; index++ )
        {
            NewSongs.Add( Tracks[ index ].m_SongId );
        }
    }

    SaveToPlayList( NewSongs );
}

// -------------------------------------------------------------------------------- //
void guLibPanel::UpdatePlaylists( void )
{
    m_MediaViewer->UpdatePlaylists();
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnSongSetRating( wxCommandEvent &event )
{
    int Rating = event.GetId() - ID_TRACKS_SET_RATING_0;
    //guLogMessage( wxT( "OnSongSetRating( %i )" ), Rating );

    guTrackArray Tracks;
    m_SongListCtrl->GetSelectedSongs( &Tracks );

    m_MediaViewer->SetTracksRating( Tracks, Rating );
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnSongSetField( wxCommandEvent &event )
{
    int ColumnId = m_SongListCtrl->GetColumnId( m_SongListCtrl->GetLastColumnClicked() );
    //guLogMessage( wxT( "guLibPanel::OnSongSetField %i" ), ColumnId );

    guTrackArray Tracks;
    m_SongListCtrl->GetSelectedSongs( &Tracks );

    wxVariant NewData = m_SongListCtrl->GetLastDataClicked();

    //guLogMessage( wxT( "Setting Data to : %s" ), NewData.GetString().c_str() );

    // This should be done in a thread for huge selections of tracks...
    wxArrayInt ChangedFlags;
    int Index;
    int Count = Tracks.Count();
    for( Index = 0; Index < Count; Index++ )
    {
        ChangedFlags.Add( guTRACK_CHANGED_DATA_TAGS );
        guTrack * Track = &Tracks[ Index ];
        switch( ColumnId )
        {
            case guSONGS_COLUMN_NUMBER :
                Track->m_Number = NewData.GetLong();
                break;

            case guSONGS_COLUMN_TITLE :
                Track->m_SongName = NewData.GetString();
                break;

            case guSONGS_COLUMN_ARTIST :
                Track->m_ArtistName = NewData.GetString();
                break;

            case guSONGS_COLUMN_ALBUMARTIST :
                Track->m_AlbumArtist = NewData.GetString();
                break;

            case guSONGS_COLUMN_ALBUM :
                Track->m_AlbumName = NewData.GetString();
                break;

            case guSONGS_COLUMN_GENRE :
                Track->m_GenreName = NewData.GetString();
                break;

            case guSONGS_COLUMN_COMPOSER :
                Track->m_Composer = NewData.GetString();
                break;

            case guSONGS_COLUMN_DISK :
                Track->m_Disk = NewData.GetString();
                break;

            case guSONGS_COLUMN_YEAR :
                Track->m_Year = NewData.GetLong();
                break;

        }
    }

    UpdateTracks( Tracks, ChangedFlags );

    m_MediaViewer->UpdatedTracks( guUPDATED_TRACKS_NONE, &Tracks );
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnSongEditField( wxCommandEvent &event )
{
    int ColumnId = m_SongListCtrl->GetColumnId( m_SongListCtrl->GetLastColumnClicked() );
    //guLogMessage( wxT( "guLibPanel::OnSongSetField %i" ), ColumnId );

    guTrackArray Tracks;
    m_SongListCtrl->GetSelectedSongs( &Tracks );

    wxString Label = m_SongListCtrl->GetColumnNames()[ ColumnId ];
    wxVariant DefValue = m_SongListCtrl->GetLastDataClicked();

    wxArrayString Items;

    int Index;
    int Count = Tracks.Count();
    for( Index = 0; Index < Count; Index++ )
    {
        wxVariant Value;
        guTrack * Track = &Tracks[ Index ];

        switch( ColumnId )
        {
            case guSONGS_COLUMN_NUMBER :
                Value = ( long ) Track->m_Number;
                break;

            case guSONGS_COLUMN_TITLE :
                Value = Track->m_SongName;
                break;

            case guSONGS_COLUMN_ARTIST :
                Value = Track->m_ArtistName;
                break;

            case guSONGS_COLUMN_ALBUMARTIST :
                Value = Track->m_AlbumArtist;
                break;

            case guSONGS_COLUMN_ALBUM :
                Value = Track->m_AlbumName;
                break;

            case guSONGS_COLUMN_GENRE :
                Value = Track->m_GenreName;
                break;

            case guSONGS_COLUMN_COMPOSER :
                Value = Track->m_Composer;
                break;

            case guSONGS_COLUMN_DISK :
                Value = Track->m_Disk;
                break;

            case guSONGS_COLUMN_YEAR :
                Value = ( long ) Track->m_Year;
                break;
        }
        if( Items.Index( Value.GetString() ) == wxNOT_FOUND )
            Items.Add( Value.GetString() );
    }

    guEditWithOptions * FieldEditor = new guEditWithOptions( this, _( "Field Editor" ), Label, DefValue.GetString(), Items );

    if( FieldEditor )
    {
        if( FieldEditor->ShowModal() == wxID_OK )
        {
            DefValue = FieldEditor->GetData();

            //guLogMessage( wxT( "Setting Data to : %s" ), DefValue.GetString().c_str() );

            // This should be done in a thread for huge selections of tracks...
            wxArrayInt ChangedFlags;
            int Index;
            int Count = Tracks.Count();
            for( Index = 0; Index < Count; Index++ )
            {
                ChangedFlags.Add( guTRACK_CHANGED_DATA_TAGS );
                guTrack * Track = &Tracks[ Index ];
                switch( ColumnId )
                {
                    case guSONGS_COLUMN_NUMBER :
                        Track->m_Number = DefValue.GetLong();
                        break;

                    case guSONGS_COLUMN_TITLE :
                        Track->m_SongName = DefValue.GetString();
                        break;

                    case guSONGS_COLUMN_ARTIST :
                        Track->m_ArtistName = DefValue.GetString();
                        break;

                    case guSONGS_COLUMN_ALBUMARTIST :
                        Track->m_AlbumArtist = DefValue.GetString();
                        break;

                    case guSONGS_COLUMN_ALBUM :
                        Track->m_AlbumName = DefValue.GetString();
                        break;

                    case guSONGS_COLUMN_GENRE :
                        Track->m_GenreName = DefValue.GetString();
                        break;

                    case guSONGS_COLUMN_COMPOSER :
                        Track->m_Composer = DefValue.GetString();
                        break;

                    case guSONGS_COLUMN_DISK :
                        Track->m_Disk = DefValue.GetString();
                        break;

                    case guSONGS_COLUMN_YEAR :
                        Track->m_Year = DefValue.GetLong();
                        break;
                }
            }

            UpdateTracks( Tracks, ChangedFlags );

            m_MediaViewer->UpdatedTracks( guUPDATED_TRACKS_NONE, &Tracks );
        }
        FieldEditor->Destroy();
    }
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnSongListColClicked( wxListEvent &event )
{
    int ColId = m_SongListCtrl->GetColumnId( event.m_col );
    if( ColId == guSONGS_COLUMN_OFFSET )
        return;
    m_SongListCtrl->SetTracksOrder( ColId );

    // Create the Columns
    wxArrayString ColumnNames = m_SongListCtrl->GetColumnNames();
    int CurColId;
    int index;
    int count = ColumnNames.Count();
    for( index = 0; index < count; index++ )
    {
        CurColId = m_SongListCtrl->GetColumnId( index );
        m_SongListCtrl->SetColumnLabel( index,
            ColumnNames[ CurColId ]  + ( ( ColId == CurColId ) ? ( m_SongListCtrl->GetTracksOrderDesc() ? wxT( " ▼" ) : wxT( " ▲" ) ) : wxEmptyString ) );
    }

    ReloadSongs();
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnSongSelectGenre( wxCommandEvent &event )
{
    guTrackArray Tracks;
    m_SongListCtrl->GetSelectedSongs( &Tracks );
    wxArrayInt * Genres = new wxArrayInt();
    int index;
    int count = Tracks.Count();
    for( index = 0; index < count; index++ )
    {
        if( Genres->Index( Tracks[ index ].m_GenreId ) == wxNOT_FOUND )
        {
            Genres->Add( Tracks[ index ].m_GenreId );
        }
    }
    SelectGenres( Genres );
    delete Genres;
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnSongSelectArtist( wxCommandEvent &event )
{
    guTrackArray Tracks;
    m_SongListCtrl->GetSelectedSongs( &Tracks );
    wxArrayInt * Artists = new wxArrayInt();
    int index;
    int count = Tracks.Count();
    for( index = 0; index < count; index++ )
    {
        if( Artists->Index( Tracks[ index ].m_ArtistId ) == wxNOT_FOUND )
        {
            Artists->Add( Tracks[ index ].m_ArtistId );
        }
    }
    SelectArtists( Artists );
    delete Artists;
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnSongSelectAlbumArtist( wxCommandEvent &event )
{
    guTrackArray Tracks;
    m_SongListCtrl->GetSelectedSongs( &Tracks );
    wxArrayInt * AlbumArtists = new wxArrayInt();
    int index;
    int count = Tracks.Count();
    for( index = 0; index < count; index++ )
    {
        if( AlbumArtists->Index( Tracks[ index ].m_AlbumArtistId ) == wxNOT_FOUND )
        {
            AlbumArtists->Add( Tracks[ index ].m_AlbumArtistId );
        }
    }
    SelectAlbumArtists( AlbumArtists );
    delete AlbumArtists;
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnSongSelectComposer( wxCommandEvent &event )
{
    guTrackArray Tracks;
    m_SongListCtrl->GetSelectedSongs( &Tracks );
    wxArrayInt * Composers = new wxArrayInt();
    int index;
    int count = Tracks.Count();
    for( index = 0; index < count; index++ )
    {
        if( Composers->Index( Tracks[ index ].m_ComposerId ) == wxNOT_FOUND )
        {
            Composers->Add( Tracks[ index ].m_ComposerId );
        }
    }
    SelectComposers( Composers );
    delete Composers;
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnSongSelectAlbum( wxCommandEvent &event )
{
    guTrackArray Tracks;
    m_SongListCtrl->GetSelectedSongs( &Tracks );
    wxArrayInt * Albums = new wxArrayInt();

    int index;
    int count = Tracks.Count();
    for( index = 0; index < count; index++ )
    {
        if( Albums->Index( Tracks[ index ].m_AlbumId ) == wxNOT_FOUND )
        {
            Albums->Add( Tracks[ index ].m_AlbumId );
        }
    }
    SelectAlbums( Albums );
    delete Albums;
}

// -------------------------------------------------------------------------------- //
void guLibPanel::SetSelection( const int type, const int id )
{
    switch( type )
    {
        case guMEDIAVIEWER_SELECT_TRACK         : SelectTrack( id ); break;
        case guMEDIAVIEWER_SELECT_ARTIST        : SelectArtist( id ); break;
        case guMEDIAVIEWER_SELECT_ALBUM         : SelectAlbum( id ); break;
        case guMEDIAVIEWER_SELECT_ALBUMARTIST   : SelectAlbumArtist( id ); break;
        case guMEDIAVIEWER_SELECT_COMPOSER      : SelectComposer( id ); break;
        case guMEDIAVIEWER_SELECT_YEAR          : SelectYear( id ); break;
        case guMEDIAVIEWER_SELECT_GENRE         :
        {
            wxArrayInt Genres;
            Genres.Add( id );
            SelectGenres( &Genres );
            break;
        }
    }
}

// -------------------------------------------------------------------------------- //
void guLibPanel::SelectTrack( const int trackid )
{
    wxArrayString Words;
    m_UpdateLock = true;
    m_LastTextFilter = wxEmptyString;
    m_Db->SetTeFilters( Words, false  );
    ClearSearchText();
    ReloadLabels();
    ReloadGenres();
    ReloadAlbumArtists();
    ReloadArtists();
    ReloadComposers();
    ReloadAlbums();
    m_UpdateLock = false;
    ReloadSongs();
    m_SongListCtrl->SetSelection( m_Db->GetTrackIndex( trackid ) );
}

// -------------------------------------------------------------------------------- //
void guLibPanel::SelectAlbum( const int albumid )
{
    wxArrayString Words;
    m_UpdateLock = true;
    m_LastTextFilter = wxEmptyString;
    m_Db->SetTeFilters( Words, false );
    ClearSearchText();
    ReloadLabels();
    ReloadGenres();
    ReloadAlbumArtists();
    ReloadArtists();
    ReloadComposers();
    ReloadAlbums();
    m_UpdateLock = false;
    m_AlbumListCtrl->SetSelection( m_AlbumListCtrl->FindAlbum( albumid ) );
}

// -------------------------------------------------------------------------------- //
void guLibPanel::SelectArtist( const int artistid )
{
    wxArrayString Words;
    m_UpdateLock = true;
    m_LastTextFilter = wxEmptyString;
    m_Db->SetTeFilters( Words, false );
    ClearSearchText();
    ReloadLabels();
    ReloadGenres();
    ReloadAlbumArtists();
    ReloadArtists();
    ReloadComposers();
    m_UpdateLock = false;
    m_ArtistListCtrl->SetSelection( m_ArtistListCtrl->FindArtist( artistid ) );
}

// -------------------------------------------------------------------------------- //
void guLibPanel::SelectAlbumArtist( const int albumartistid )
{
    wxArrayString Words;
    m_UpdateLock = true;
    m_LastTextFilter = wxEmptyString;
    m_Db->SetTeFilters( Words, false );
    ClearSearchText();
    ReloadLabels();
    ReloadGenres();
    ReloadAlbumArtists();
    m_UpdateLock = false;
    m_AlbumArtistListCtrl->SetSelection( m_AlbumArtistListCtrl->FindItemId( albumartistid ) );
}

// -------------------------------------------------------------------------------- //
void guLibPanel::SelectComposer( const int composerid )
{
    wxArrayString Words;
    m_UpdateLock = true;
    m_LastTextFilter = wxEmptyString;
    m_Db->SetTeFilters( Words, false );
    ClearSearchText();
    ReloadLabels();
    ReloadGenres();
    ReloadAlbumArtists();
    ReloadComposers();
    m_UpdateLock = false;
    m_ComposerListCtrl->SetSelection( m_ComposerListCtrl->FindItemId( composerid ) );
}

// -------------------------------------------------------------------------------- //
void guLibPanel::SelectYear( const int year )
{
    wxArrayString Words;
    m_UpdateLock = true;
    m_LastTextFilter = wxEmptyString;
    m_Db->SetTeFilters( Words, false );
    ClearSearchText();
    ReloadLabels();
    ReloadGenres();
    ReloadAlbumArtists();
    ReloadArtists();
    ReloadComposers();
    ReloadYears();
    m_UpdateLock = false;
    m_YearListCtrl->SetSelection( m_YearListCtrl->FindYear( year ) );
}

// -------------------------------------------------------------------------------- //
void guLibPanel::SelectAlbumName( const wxString &album )
{
    wxArrayString Words;
    m_UpdateLock = true;
    m_LastTextFilter = wxEmptyString;
    m_Db->SetTeFilters( Words, false );
    ClearSearchText();
    ReloadLabels();
    ReloadGenres();
    ReloadAlbumArtists();
    ReloadArtists();
    ReloadComposers();
    ReloadAlbums();
    m_UpdateLock = false;
    m_AlbumListCtrl->SelectAlbumName( album );
}

// -------------------------------------------------------------------------------- //
void guLibPanel::SelectArtistName( const wxString &artist )
{
    wxArrayString Words;
    m_UpdateLock = true;
    m_LastTextFilter = wxEmptyString;
    m_Db->SetTeFilters( Words, false );
    ClearSearchText();
    ReloadLabels();
    ReloadGenres();
    ReloadAlbumArtists();
    ReloadArtists();
    ReloadComposers();
    m_UpdateLock = false;
    m_ArtistListCtrl->SelectArtistName( artist );
}

// -------------------------------------------------------------------------------- //
void guLibPanel::SelectGenres( wxArrayInt * genres )
{
    wxArrayString Words;
    m_UpdateLock = true;
    m_LastTextFilter = wxEmptyString;
    m_Db->SetTeFilters( Words, false );
    ClearSearchText();
    ReloadLabels();
    ReloadGenres();
    m_UpdateLock = false;
    m_GenreListCtrl->SetSelectedItems( * genres );
}

// -------------------------------------------------------------------------------- //
void guLibPanel::SelectArtists( wxArrayInt * artists )
{
    wxArrayString Words;
    m_UpdateLock = true;
    m_LastTextFilter = wxEmptyString;
    m_Db->SetTeFilters( Words, false );
    ClearSearchText();
    ReloadLabels();
    ReloadGenres();
    ReloadAlbumArtists();
    ReloadArtists();
    ReloadComposers();
    m_UpdateLock = false;
    m_ArtistListCtrl->SetSelectedItems( * artists );
}

// -------------------------------------------------------------------------------- //
void guLibPanel::SelectAlbumArtists( wxArrayInt * ids )
{
    wxArrayString Words;
    m_UpdateLock = true;
    m_LastTextFilter = wxEmptyString;
    m_Db->SetTeFilters( Words, false );
    ClearSearchText();
    ReloadLabels();
    ReloadGenres();
    ReloadComposers();
    ReloadAlbumArtists();
//    ReloadArtists();
    m_UpdateLock = false;
    m_AlbumArtistListCtrl->SetSelectedItems( * ids );
}

// -------------------------------------------------------------------------------- //
void guLibPanel::SelectComposers( wxArrayInt * ids )
{
    wxArrayString Words;
    m_UpdateLock = true;
    m_LastTextFilter = wxEmptyString;
    m_Db->SetTeFilters( Words, false );
    ClearSearchText();
    ReloadLabels();
    ReloadGenres();
    ReloadComposers();
//    ReloadAlbumArtists();
//    ReloadArtists();
    m_UpdateLock = false;
    m_ComposerListCtrl->SetSelectedItems( * ids );
}

// -------------------------------------------------------------------------------- //
void guLibPanel::SelectAlbums( wxArrayInt * albums )
{
    wxArrayString Words;
    m_UpdateLock = true;
    m_LastTextFilter = wxEmptyString;
    m_Db->SetTeFilters( Words, false );
    ClearSearchText();
    ReloadLabels();
    ReloadGenres();
    ReloadAlbumArtists();
    ReloadArtists();
    ReloadComposers();
    ReloadAlbums();
    m_UpdateLock = false;
    m_AlbumListCtrl->SetSelectedItems( * albums );
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnYearListSelected( wxListEvent &event )
{
    if( m_UpdateLock )
        return;
    m_SelChangedObject = guPANEL_LIBRARY_YEARS;
    if( m_SelChangedTimer.IsRunning() )
        m_SelChangedTimer.Stop();
    m_SelChangedTimer.Start( guPANEL_TIMER_SELCHANGED, wxTIMER_ONE_SHOT );
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnYearListActivated( wxListEvent &event )
{
    guTrackArray Songs;
    m_YearListCtrl->GetSelectedSongs( &Songs );
    if( Songs.Count() )
    {
        guConfig * Config = ( guConfig * ) guConfig::Get();
        if( Config )
        {
            if( Config->ReadBool( wxT( "DefaultActionEnqueue" ), false, wxT( "general" ) ) )
            {
                m_PlayerPanel->AddToPlayList( Songs );
            }
            else
            {
                m_PlayerPanel->SetPlayList( Songs );
            }
        }
    }
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnYearListPlayClicked( wxCommandEvent &event )
{
    guTrackArray Songs;
    m_YearListCtrl->GetSelectedSongs( &Songs );
    m_PlayerPanel->SetPlayList( Songs );
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnYearListQueueClicked( wxCommandEvent &event )
{
    guTrackArray Songs;
    m_YearListCtrl->GetSelectedSongs( &Songs );
    if( Songs.Count() )
    {
        m_PlayerPanel->AddToPlayList( Songs, true, event.GetId() - ID_YEAR_ENQUEUE_AFTER_ALL );
    }
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnYearListEditTracksClicked( wxCommandEvent &event )
{
    guTrackArray Tracks;
    m_YearListCtrl->GetSelectedSongs( &Tracks );
    if( !Tracks.Count() )
        return;

    DoEditTracks( Tracks );
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnYearListCopyToClicked( wxCommandEvent &event )
{
    guTrackArray * Tracks = new guTrackArray();
    m_YearListCtrl->GetSelectedSongs( Tracks );

    int Index = event.GetId() - ID_COPYTO_BASE;
    if( Index >= guCOPYTO_DEVICE_BASE )
    {
        Index -= guCOPYTO_DEVICE_BASE;
        event.SetId( ID_MAINFRAME_COPYTODEVICE_TRACKS );
    }
    else
    {
        event.SetId( ID_MAINFRAME_COPYTO );
    }
    event.SetInt( Index );
    event.SetClientData( ( void * ) Tracks );
    wxPostEvent( wxTheApp->GetTopWindow(), event );
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnYearSavePlayListClicked( wxCommandEvent &event )
{
    int index;
    int count;
    wxArrayInt NewSongs;
    guTrackArray Tracks;
    m_YearListCtrl->GetSelectedSongs( &Tracks );

    count = Tracks.Count();
    for( index = 0; index < count; index++ )
    {
        NewSongs.Add( Tracks[ index ].m_SongId );
    }

    SaveToPlayList( NewSongs );
}

// -------------------------------------------------------------------------------- //
// Rating List Box
// -------------------------------------------------------------------------------- //
void guLibPanel::OnRatingListSelected( wxListEvent &event )
{
    if( m_UpdateLock )
        return;
    m_SelChangedObject = guPANEL_LIBRARY_RATINGS;
    if( m_SelChangedTimer.IsRunning() )
        m_SelChangedTimer.Stop();
    m_SelChangedTimer.Start( guPANEL_TIMER_SELCHANGED, wxTIMER_ONE_SHOT );
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnRatingListActivated( wxListEvent &event )
{
    guTrackArray Songs;
    m_RatingListCtrl->GetSelectedSongs( &Songs );
    if( Songs.Count() )
    {
        guConfig * Config = ( guConfig * ) guConfig::Get();
        if( Config )
        {
            if( Config->ReadBool( wxT( "DefaultActionEnqueue" ), false, wxT( "general" ) ) )
            {
                m_PlayerPanel->AddToPlayList( Songs );
            }
            else
            {
                m_PlayerPanel->SetPlayList( Songs );
            }
        }
    }
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnRatingListPlayClicked( wxCommandEvent &event )
{
    guTrackArray Songs;
    m_RatingListCtrl->GetSelectedSongs( &Songs );
    m_PlayerPanel->SetPlayList( Songs );
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnRatingListQueueClicked( wxCommandEvent &event )
{
    guTrackArray Songs;
    m_RatingListCtrl->GetSelectedSongs( &Songs );
    if( Songs.Count() )
    {
        m_PlayerPanel->AddToPlayList( Songs, true, event.GetId() - ID_RATING_ENQUEUE_AFTER_ALL );
    }
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnRatingListEditTracksClicked( wxCommandEvent &event )
{
    guTrackArray Tracks;
    m_RatingListCtrl->GetSelectedSongs( &Tracks );
    if( !Tracks.Count() )
        return;

    DoEditTracks( Tracks );
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnRatingListCopyToClicked( wxCommandEvent &event )
{
    guTrackArray * Tracks = new guTrackArray();
    m_RatingListCtrl->GetSelectedSongs( Tracks );

    int Index = event.GetId() - ID_COPYTO_BASE;
    if( Index >= guCOPYTO_DEVICE_BASE )
    {
        Index -= guCOPYTO_DEVICE_BASE;
        event.SetId( ID_MAINFRAME_COPYTODEVICE_TRACKS );
    }
    else
    {
        event.SetId( ID_MAINFRAME_COPYTO );
    }
    event.SetInt( Index );
    event.SetClientData( ( void * ) Tracks );
    wxPostEvent( wxTheApp->GetTopWindow(), event );
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnRatingSavePlayListClicked( wxCommandEvent &event )
{
    int index;
    int count;
    wxArrayInt NewSongs;
    guTrackArray Tracks;
    m_RatingListCtrl->GetSelectedSongs( &Tracks );

    count = Tracks.Count();
    for( index = 0; index < count; index++ )
    {
        NewSongs.Add( Tracks[ index ].m_SongId );
    }

    SaveToPlayList( NewSongs );
}

// -------------------------------------------------------------------------------- //
// PlayCount List Box
// -------------------------------------------------------------------------------- //
void guLibPanel::OnPlayCountListSelected( wxListEvent &event )
{
    if( m_UpdateLock )
        return;
    m_SelChangedObject = guPANEL_LIBRARY_PLAYCOUNT;
    if( m_SelChangedTimer.IsRunning() )
        m_SelChangedTimer.Stop();
    m_SelChangedTimer.Start( guPANEL_TIMER_SELCHANGED, wxTIMER_ONE_SHOT );
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnPlayCountListActivated( wxListEvent &event )
{
    guTrackArray Songs;
    m_PlayCountListCtrl->GetSelectedSongs( &Songs );
    if( Songs.Count() )
    {
        guConfig * Config = ( guConfig * ) guConfig::Get();
        if( Config )
        {
            if( Config->ReadBool( wxT( "DefaultActionEnqueue" ), false, wxT( "general" ) ) )
            {
                m_PlayerPanel->AddToPlayList( Songs );
            }
            else
            {
                m_PlayerPanel->SetPlayList( Songs );
            }
        }
    }
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnPlayCountListPlayClicked( wxCommandEvent &event )
{
    guTrackArray Songs;
    m_PlayCountListCtrl->GetSelectedSongs( &Songs );
    m_PlayerPanel->SetPlayList( Songs );
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnPlayCountListQueueClicked( wxCommandEvent &event )
{
    guTrackArray Songs;
    m_PlayCountListCtrl->GetSelectedSongs( &Songs );
    if( Songs.Count() )
    {
        m_PlayerPanel->AddToPlayList( Songs, true, event.GetId() - ID_PLAYCOUNT_ENQUEUE_AFTER_ALL );
    }
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnPlayCountListEditTracksClicked( wxCommandEvent &event )
{
    guTrackArray Tracks;
    m_PlayCountListCtrl->GetSelectedSongs( &Tracks );
    if( !Tracks.Count() )
        return;

    DoEditTracks( Tracks );
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnPlayCountListCopyToClicked( wxCommandEvent &event )
{
    guTrackArray * Tracks = new guTrackArray();
    m_PlayCountListCtrl->GetSelectedSongs( Tracks );

    int Index = event.GetId() - ID_COPYTO_BASE;
    if( Index >= guCOPYTO_DEVICE_BASE )
    {
        Index -= guCOPYTO_DEVICE_BASE;
        event.SetId( ID_MAINFRAME_COPYTODEVICE_TRACKS );
    }
    else
    {
        event.SetId( ID_MAINFRAME_COPYTO );
    }
    event.SetInt( Index );
    event.SetClientData( ( void * ) Tracks );
    wxPostEvent( wxTheApp->GetTopWindow(), event );
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnPlayCountSavePlayListClicked( wxCommandEvent &event )
{
    int index;
    int count;
    wxArrayInt NewSongs;
    guTrackArray Tracks;
    m_PlayCountListCtrl->GetSelectedSongs( &Tracks );

    count = Tracks.Count();
    for( index = 0; index < count; index++ )
    {
        NewSongs.Add( Tracks[ index ].m_SongId );
    }

    SaveToPlayList( NewSongs );
}

// -------------------------------------------------------------------------------- //
// Composers List Box
// -------------------------------------------------------------------------------- //
void guLibPanel::OnComposerListSelected( wxListEvent &event )
{
    if( m_UpdateLock )
        return;
    m_SelChangedObject = guPANEL_LIBRARY_COMPOSERS;
    if( m_SelChangedTimer.IsRunning() )
        m_SelChangedTimer.Stop();
    m_SelChangedTimer.Start( guPANEL_TIMER_SELCHANGED, wxTIMER_ONE_SHOT );
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnComposerListActivated( wxListEvent &event )
{
    guTrackArray Songs;
    m_ComposerListCtrl->GetSelectedSongs( &Songs );
    if( Songs.Count() )
    {
        guConfig * Config = ( guConfig * ) guConfig::Get();
        if( Config )
        {
            if( Config->ReadBool( wxT( "DefaultActionEnqueue" ), false, wxT( "general" ) ) )
            {
                m_PlayerPanel->AddToPlayList( Songs );
            }
            else
            {
                m_PlayerPanel->SetPlayList( Songs );
            }
        }
    }
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnComposerListPlayClicked( wxCommandEvent &event )
{
    guTrackArray Songs;
    m_ComposerListCtrl->GetSelectedSongs( &Songs );

    m_PlayerPanel->SetPlayList( Songs );
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnComposerListQueueClicked( wxCommandEvent &event )
{
    guTrackArray Songs;
    m_ComposerListCtrl->GetSelectedSongs( &Songs );
    if( Songs.Count() )
    {
        m_PlayerPanel->AddToPlayList( Songs, true, event.GetId() - ID_COMPOSER_ENQUEUE_AFTER_ALL );
    }
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnComposerListEditTracksClicked( wxCommandEvent &event )
{
    guTrackArray Tracks;
    m_ComposerListCtrl->GetSelectedSongs( &Tracks );
    if( !Tracks.Count() )
        return;

    DoEditTracks( Tracks );
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnComposerListCopyToClicked( wxCommandEvent &event )
{
    guTrackArray * Tracks = new guTrackArray();
    m_ComposerListCtrl->GetSelectedSongs( Tracks );

    int Index = event.GetId() - ID_COPYTO_BASE;
    if( Index >= guCOPYTO_DEVICE_BASE )
    {
        Index -= guCOPYTO_DEVICE_BASE;
        event.SetId( ID_MAINFRAME_COPYTODEVICE_TRACKS );
    }
    else
    {
        event.SetId( ID_MAINFRAME_COPYTO );
    }
    event.SetInt( Index );
    event.SetClientData( ( void * ) Tracks );
    wxPostEvent( wxTheApp->GetTopWindow(), event );
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnComposerSavePlayListClicked( wxCommandEvent &event )
{
    int index;
    int count;
    wxArrayInt NewSongs;
    guTrackArray Tracks;
    m_ComposerListCtrl->GetSelectedSongs( &Tracks );

    count = Tracks.Count();
    for( index = 0; index < count; index++ )
    {
        NewSongs.Add( Tracks[ index ].m_SongId );
    }

    SaveToPlayList( NewSongs );
}

// -------------------------------------------------------------------------------- //
// Album Artists List Box
// -------------------------------------------------------------------------------- //
void guLibPanel::OnAlbumArtistListSelected( wxListEvent &event )
{
    if( m_UpdateLock )
        return;
    m_SelChangedObject = guPANEL_LIBRARY_ALBUMARTISTS;
    if( m_SelChangedTimer.IsRunning() )
        m_SelChangedTimer.Stop();
    m_SelChangedTimer.Start( guPANEL_TIMER_SELCHANGED, wxTIMER_ONE_SHOT );
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnAlbumArtistListActivated( wxListEvent &event )
{
    guTrackArray Songs;
    m_AlbumArtistListCtrl->GetSelectedSongs( &Songs );
    if( Songs.Count() )
    {
        guConfig * Config = ( guConfig * ) guConfig::Get();
        if( Config )
        {
            if( Config->ReadBool( wxT( "DefaultActionEnqueue" ), false, wxT( "general" ) ) )
            {
                m_PlayerPanel->AddToPlayList( Songs );
            }
            else
            {
                m_PlayerPanel->SetPlayList( Songs );
            }
        }
    }
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnAlbumArtistListPlayClicked( wxCommandEvent &event )
{
    guTrackArray Songs;
    m_AlbumArtistListCtrl->GetSelectedSongs( &Songs );

    m_PlayerPanel->SetPlayList( Songs );
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnAlbumArtistListQueueClicked( wxCommandEvent &event )
{
    guTrackArray Songs;
    m_AlbumArtistListCtrl->GetSelectedSongs( &Songs );
    if( Songs.Count() )
    {
        m_PlayerPanel->AddToPlayList( Songs, true, event.GetId() - ID_ALBUMARTIST_ENQUEUE_AFTER_ALL );
    }
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnAlbumArtistListEditTracksClicked( wxCommandEvent &event )
{
    guTrackArray Tracks;
    m_AlbumArtistListCtrl->GetSelectedSongs( &Tracks );
    if( !Tracks.Count() )
        return;

    DoEditTracks( Tracks );
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnAlbumArtistListCopyToClicked( wxCommandEvent &event )
{
    guTrackArray * Tracks = new guTrackArray();
    m_AlbumArtistListCtrl->GetSelectedSongs( Tracks );

    int Index = event.GetId() - ID_COPYTO_BASE;
    if( Index >= guCOPYTO_DEVICE_BASE )
    {
        Index -= guCOPYTO_DEVICE_BASE;
        event.SetId( ID_MAINFRAME_COPYTODEVICE_TRACKS );
    }
    else
    {
        event.SetId( ID_MAINFRAME_COPYTO );
    }
    event.SetInt( Index );
    event.SetClientData( ( void * ) Tracks );
    wxPostEvent( wxTheApp->GetTopWindow(), event );
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnAlbumArtistSavePlayListClicked( wxCommandEvent &event )
{
    int index;
    int count;
    wxArrayInt NewSongs;
    guTrackArray Tracks;
    m_AlbumArtistListCtrl->GetSelectedSongs( &Tracks );

    count = Tracks.Count();
    for( index = 0; index < count; index++ )
    {
        NewSongs.Add( Tracks[ index ].m_SongId );
    }

    SaveToPlayList( NewSongs );
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnSelChangedTimer( wxTimerEvent &event )
{
    DoSelectionChanged();
    m_SelChangedObject = 0;
}

// -------------------------------------------------------------------------------- //
void guLibPanel::DoSelectionChanged( void )
{
    switch( m_SelChangedObject )
    {
        case guPANEL_LIBRARY_TEXTSEARCH :
        {
            break;
        }

        case guPANEL_LIBRARY_LABELS :
        {
            m_Db->SetLaFilters( m_LabelsListCtrl->GetSelectedItems(), m_UpdateLock );
            if( !m_UpdateLock )
            {
                m_UpdateLock = true;
                ReloadGenres();
                ReloadComposers();
                ReloadAlbumArtists();
                ReloadArtists();
                ReloadAlbums();
                ReloadYears();
                ReloadRatings();
                ReloadPlayCounts();
                ReloadSongs();
                //
                //
                m_UpdateLock = false;
            }
            break;
        }

        case guPANEL_LIBRARY_GENRES :
        {
            //wxLongLong time = wxGetLocalTimeMillis();
            m_Db->SetGeFilters( m_GenreListCtrl->GetSelectedItems(), m_UpdateLock );

            if( !m_UpdateLock )
            {
                m_UpdateLock = true;
                ReloadComposers();
                ReloadAlbumArtists();
                ReloadArtists();
                ReloadYears();
                ReloadAlbums();
                ReloadRatings();
                ReloadPlayCounts();
                ReloadSongs();
                m_UpdateLock = false;
            }
            //time = wxGetLocalTimeMillis() - time;;
            //guLogWarning( wxT( "Genre Time : %u ms" ), time.GetLo() );
            break;
        }

        case guPANEL_LIBRARY_ARTISTS :
        {
            m_Db->SetArFilters( m_ArtistListCtrl->GetSelectedItems(), m_UpdateLock );
            if( !m_UpdateLock )
            {
                m_UpdateLock = true;
                ReloadAlbums();
                ReloadYears();
                ReloadRatings();
                ReloadPlayCounts();
                ReloadSongs();
                m_UpdateLock = false;
            }
            break;
        }

        case guPANEL_LIBRARY_YEARS :
        {
            m_Db->SetYeFilters( m_YearListCtrl->GetSelectedItems(), m_UpdateLock );
            if( !m_UpdateLock )
            {
                m_UpdateLock = true;
                ReloadAlbums();
                ReloadRatings();
                ReloadPlayCounts();
                ReloadSongs();
                m_UpdateLock = false;
            }
            break;
        }

        case guPANEL_LIBRARY_ALBUMS :
        {
            m_Db->SetAlFilters( m_AlbumListCtrl->GetSelectedItems(), m_UpdateLock );
            if( !m_UpdateLock )
            {
                m_UpdateLock = true;
                ReloadRatings();
                ReloadPlayCounts();
                ReloadSongs();
                m_UpdateLock = false;
            }
            break;
        }

        case guPANEL_LIBRARY_RATINGS :
        {
            m_Db->SetRaFilters( m_RatingListCtrl->GetSelectedItems() );
            if( !m_UpdateLock )
            {
                m_UpdateLock = true;
                ReloadSongs();
                m_UpdateLock = false;
            }
            break;
        }

        case guPANEL_LIBRARY_PLAYCOUNT :
        {
            m_Db->SetPcFilters( m_PlayCountListCtrl->GetSelectedItems() );
            if( !m_UpdateLock )
            {
                m_UpdateLock = true;
                ReloadSongs();
                m_UpdateLock = false;
            }
            break;
        }

        case guPANEL_LIBRARY_COMPOSERS :
        {
            m_Db->SetCoFilters( m_ComposerListCtrl->GetSelectedItems(), m_UpdateLock );
            if( !m_UpdateLock )
            {
                m_UpdateLock = true;
                ReloadAlbumArtists();
                ReloadArtists();
                ReloadYears();
                ReloadAlbums();
                ReloadRatings();
                ReloadPlayCounts();
                ReloadSongs();
                m_UpdateLock = false;
            }
            break;
        }

        case guPANEL_LIBRARY_ALBUMARTISTS :
        {
            m_Db->SetAAFilters( m_AlbumArtistListCtrl->GetSelectedItems(), m_UpdateLock );
            if( !m_UpdateLock )
            {
                m_UpdateLock = true;
                //ReloadComposers();
                ReloadArtists();
                ReloadYears();
                ReloadAlbums();
                ReloadRatings();
                ReloadPlayCounts();
                ReloadSongs();
                m_UpdateLock = false;
            }
            break;
        }
    }
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnSongDeleteLibrary( wxCommandEvent &event )
{
    if( m_SongListCtrl->GetSelectedCount() )
    {
        if( wxMessageBox( wxT( "Are you sure to remove the selected tracks from your library?" ),
            wxT( "Remove tracks from library" ), wxICON_QUESTION|wxYES|wxNO|wxNO_DEFAULT ) == wxYES )
        {
            guTrackArray Tracks;
            m_SongListCtrl->GetSelectedSongs( &Tracks );
            //
            m_Db->DeleteLibraryTracks( &Tracks, true );
            //
            m_SongListCtrl->ClearSelectedItems();

            ReloadControls();
        }
    }
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnSongDeleteDrive( wxCommandEvent &event )
{
    if( m_SongListCtrl->GetSelectedCount() )
    {
        if( wxMessageBox( _( "Are you sure to delete the selected tracks from your drive?\nThis will permanently erase the selected tracks." ),
            _( "Remove tracks from drive" ), wxICON_QUESTION|wxYES|wxNO|wxNO_DEFAULT ) == wxYES )
        {
            guTrackArray Tracks;
            m_SongListCtrl->GetSelectedSongs( &Tracks );
            //
            m_MediaViewer->DeleteTracks( &Tracks );
            //
            m_SongListCtrl->ClearSelectedItems();

            ReloadControls();
        }
    }
}

// -------------------------------------------------------------------------------- //
int guLibPanel::GetContextMenuFlags( void )
{
    return m_MediaViewer->GetContextMenuFlags();
}

// -------------------------------------------------------------------------------- //
void guLibPanel::CreateContextMenu( wxMenu * menu, const int windowid )
{
    m_MediaViewer->CreateContextMenu( menu, windowid );
}

// -------------------------------------------------------------------------------- //
void guLibPanel::CreateCopyToMenu( wxMenu * menu )
{
    m_MediaViewer->CreateCopyToMenu( menu );
}

// -------------------------------------------------------------------------------- //
void guLibPanel::OnGoToSearch( wxCommandEvent &event )
{
    if( m_MediaViewer )
    {
        m_MediaViewer->GoToSearch();
    }
}

// -------------------------------------------------------------------------------- //
void guLibPanel::NormalizeTracks( guTrackArray * tracks, const bool isdrag )
{
    if( m_MediaViewer )
        m_MediaViewer->NormalizeTracks( tracks, isdrag );
}

// -------------------------------------------------------------------------------- //
void guLibPanel::AlbumCoverChanged( void )
{
    ReloadAlbums( false );
}

// -------------------------------------------------------------------------------- //
void guLibPanel::UpdatedTracks( const guTrackArray * tracks )
{
//    if( m_SongListCtrl )
//        m_SongListCtrl->UpdatedTracks( tracks );

    ReloadControls();
}

// -------------------------------------------------------------------------------- //
void guLibPanel::UpdatedTrack( const guTrack * track )
{
//    if( m_SongListCtrl )
//        m_SongListCtrl->UpdatedTrack( track );

    ReloadControls();
}

// -------------------------------------------------------------------------------- //
