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
#include "MediaCtrl.h"

#include "Config.h"
#include "FileRenamer.h" // NormalizeField
#include "PlayerPanel.h"
#include "Settings.h"
#include "TagInfo.h"
#include "Utils.h"

#include <wx/uri.h>

DEFINE_EVENT_TYPE( guEVT_MEDIA_LOADED )
//DEFINE_EVENT_TYPE( guEVT_MEDIA_SET_NEXT_MEDIA )
DEFINE_EVENT_TYPE( guEVT_MEDIA_FINISHED )
DEFINE_EVENT_TYPE( guEVT_MEDIA_CHANGED_STATE )
DEFINE_EVENT_TYPE( guEVT_MEDIA_BUFFERING )
DEFINE_EVENT_TYPE( guEVT_MEDIA_LEVELINFO )
DEFINE_EVENT_TYPE( guEVT_MEDIA_TAGINFO )
DEFINE_EVENT_TYPE( guEVT_MEDIA_CHANGED_BITRATE )
DEFINE_EVENT_TYPE( guEVT_MEDIA_CHANGED_POSITION )
DEFINE_EVENT_TYPE( guEVT_MEDIA_CHANGED_LENGTH )
DEFINE_EVENT_TYPE( guEVT_MEDIA_ERROR )
DEFINE_EVENT_TYPE( guEVT_MEDIA_FADEOUT_FINISHED )
DEFINE_EVENT_TYPE( guEVT_MEDIA_FADEIN_STARTED )

#define guFADERPLAYBIN_FAST_FADER_TIME          (1000)
#define guFADERPLAYBIN_SHORT_FADER_TIME         (200)

#define GST_AUDIO_TEST_SRC_WAVE_SILENCE         4

#define GST_TO_WXSTRING( str )  ( wxString( str, wxConvUTF8 ) )

//#define guLogDebug(...)  guLogMessage(__VA_ARGS__)
#define guLogDebug(...)
//#define guSHOW_DUMPFADERPLAYBINS     1

#ifdef guSHOW_DUMPFADERPLAYBINS
// -------------------------------------------------------------------------------- //
static void DumpFaderPlayBins( const guFaderPlayBinArray &playbins, guFaderPlayBin * current )
{
    guLogMessage( wxT( "CurrentPlayBin: %li" ), current ? current->GetId() : 0l );
    if( !playbins.Count() )
    {
        guLogMessage( wxT( "The faderplaybins list is empty" ) );
        return;
    }

    guLogDebug( wxT( " * * * * * * * * * * current stream list * * * * * * * * * *" ) );
    int Index;
    int Count = playbins.Count();
    for( Index = 0; Index < Count; Index++ )
    {
        guFaderPlayBin * FaderPlayBin = playbins[ Index ];

        wxString StateName;

        switch( FaderPlayBin->GetState() )
        {
            case guFADERPLAYBIN_STATE_WAITING :	 	        StateName = wxT( "waiting" );		    break;
            case guFADERPLAYBIN_STATE_PLAYING :	 	        StateName = wxT( "playing" );		    break;
            case guFADERPLAYBIN_STATE_PAUSED :	 	        StateName = wxT( "paused" );		    break;
            case guFADERPLAYBIN_STATE_STOPPED :	 	        StateName = wxT( "stopped" );		    break;

            case guFADERPLAYBIN_STATE_FADEIN :   	        StateName = wxT( "fading in" ); 	    break;
            case guFADERPLAYBIN_STATE_WAITING_EOS : 	    StateName = wxT( "waiting for EOS" );   break;
            case guFADERPLAYBIN_STATE_FADEOUT : 	        StateName = wxT( "fading out" ); 	    break;
            case guFADERPLAYBIN_STATE_FADEOUT_PAUSE :       StateName = wxT( "fading->paused" );    break;
            case guFADERPLAYBIN_STATE_FADEOUT_STOP :        StateName = wxT( "fading->stopped" );   break;

            case guFADERPLAYBIN_STATE_PENDING_REMOVE:	    StateName = wxT( "pending remove" );    break;
            case guFADERPLAYBIN_STATE_ERROR :	            StateName = wxT( "error" );             break;
            default :                                       StateName = wxT( "other" );             break;
        }

        //if( !FaderPlayBin->Uri().IsEmpty() )
        {
            guLogDebug( wxT( "[%li] '%s'" ), FaderPlayBin->GetId(), StateName.c_str() );
        }
    }
    guLogDebug( wxT( " * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * " ) );
}
#endif

// -------------------------------------------------------------------------------- //
//static guFaderPlayBin * FindFaderPlayBin( const guFaderPlayBinArray &playbins, GstElement * element )
//{
//    int Index;
//    int Count = playbins.Count();
//    for( Index = 0; Index < Count; Index++ )
//    {
//        GstElement * e = element;
//        while( e )
//        {
//            if( e == GST_ELEMENT( playbins[ Index ]->m_Playbin ) )
//            {
//                return playbins[ Index ];
//            }
//			e = GST_ELEMENT_PARENT( e );
//        }
//    }
//    return NULL;
//}
//
// -------------------------------------------------------------------------------- //
//static guFaderPlayBin * FindFaderPlayBin( const guFaderPlayBinArray &playbins, int statemask )
//{
//    int Index;
//    int Count = playbins.Count();
//    for( Index = 0; Index < Count; Index++ )
//    {
//        if( playbins[ Index ]->m_State & statemask )
//            return playbins[ Index ];
//    }
//    return NULL;
//}


// -------------------------------------------------------------------------------- //
extern "C" {

// -------------------------------------------------------------------------------- //
static gboolean gst_bus_async_callback( GstBus * bus, GstMessage * message, guFaderPlayBin * ctrl )
{
    switch( GST_MESSAGE_TYPE( message ) )
    {
        case GST_MESSAGE_ERROR :
        {
            GError * err;
            //gchar * debug;
            gst_message_parse_error( message, &err, NULL );

            ctrl->SetState( guFADERPLAYBIN_STATE_ERROR );

            guMediaCtrl * MediaCtrl = ctrl->GetPlayer();
            if( MediaCtrl && ctrl->IsOk() )
            {
                MediaCtrl->SetLastError( err->code );
                ctrl->SetErrorCode( err->code );
                ctrl->SetState( guFADERPLAYBIN_STATE_ERROR );

                wxString * ErrorStr = new wxString( err->message, wxConvUTF8 );

                guLogError( wxT( "Gstreamer error '%s'" ), ErrorStr->c_str() );

                guMediaEvent event( guEVT_MEDIA_ERROR );
                event.SetClientData( ( void * ) ErrorStr );
                MediaCtrl->SendEvent( event );
            }

            g_error_free( err );
            //g_free( debug );
            break;
        }

//        case GST_MESSAGE_STATE_CHANGED:
//        {
////            GstState oldstate, newstate, pendingstate;
////            gst_message_parse_state_changed( message, &oldstate, &newstate, &pendingstate );
//
////            //guLogMessage( wxT( "State changed %u -> %u (%u)" ), oldstate, newstate, pendingstate );
////            if( pendingstate == GST_STATE_VOID_PENDING )
////            {
////                wxMediaEvent event( wxEVT_MEDIA_STATECHANGED );
////                ctrl->AddPendingEvent( event );
////            }
//            break;
//        }

        case GST_MESSAGE_BUFFERING :
        {
            gint        Percent;
            gst_message_parse_buffering( message, &Percent );

            guLogDebug( wxT( "Buffering (%li): %i%%" ), ctrl->GetId(), Percent );

            if( Percent != 100 )
            {
                if( !ctrl->IsBuffering() )
                    ctrl->Pause();
            }
            else
            {
                ctrl->Play();
            }
            ctrl->SetBuffering( Percent != 100 );

            guMediaEvent event( guEVT_MEDIA_BUFFERING );
            event.SetInt( Percent );
            ctrl->SendEvent( event );
            break;
        }

        case GST_MESSAGE_EOS :
        {
            guMediaEvent event( guEVT_MEDIA_FINISHED );
            event.SetExtraLong( ctrl->GetId() );
            ctrl->SendEvent( event );

            ctrl->SetState( guFADERPLAYBIN_STATE_PENDING_REMOVE );

            ctrl->GetPlayer()->ScheduleCleanUp();

            guLogDebug( wxT( "***** EOS received..." ) );
          break;
        }

        case GST_MESSAGE_TAG :
        {
            // The stream discovered new tags.
            GstTagList * tags;
            //gchar * title = NULL;
            unsigned int bitrate = 0;
            // Extract from the message the GstTagList.
            //This generates a copy, so we must remember to free it.
            gst_message_parse_tag( message, &tags );

            guRadioTagInfo * RadioTagInfo = new guRadioTagInfo();

            gst_tag_list_get_string( tags, GST_TAG_ORGANIZATION, &RadioTagInfo->m_Organization );
            gst_tag_list_get_string( tags, GST_TAG_LOCATION, &RadioTagInfo->m_Location );
            gst_tag_list_get_string( tags, GST_TAG_TITLE, &RadioTagInfo->m_Title );
            gst_tag_list_get_string( tags, GST_TAG_GENRE, &RadioTagInfo->m_Genre );

            //guLogMessage( wxT( "New Tag Found:\n'%s'\n'%s'\n'%s'\n'%s'" ),
            //    wxString( RadioTagInfo->m_Organization, wxConvUTF8 ).c_str(),
            //    wxString( RadioTagInfo->m_Location, wxConvUTF8 ).c_str(),
            //    wxString( RadioTagInfo->m_Title, wxConvUTF8 ).c_str(),
            //    wxString( RadioTagInfo->m_Genre, wxConvUTF8 ).c_str() );

            if( RadioTagInfo->m_Organization || RadioTagInfo->m_Location ||
                RadioTagInfo->m_Title || RadioTagInfo->m_Genre )
            {
                guMediaEvent event( guEVT_MEDIA_TAGINFO );
                event.SetClientData( RadioTagInfo );
                ctrl->SendEvent( event );
            }
            else
            {
                delete RadioTagInfo;
            }

            gst_tag_list_get_uint( tags, GST_TAG_BITRATE, &bitrate );
            if( bitrate )
            {
                guMediaEvent event( guEVT_MEDIA_CHANGED_BITRATE );
                event.SetInt( bitrate );
                event.SetExtraLong( ctrl->GetId() );
                ctrl->SendEvent( event );
            }

            // Free the tag list
            gst_tag_list_free( tags );
            break;
        }

        case GST_MESSAGE_ELEMENT :
        {
            if( ctrl == ctrl->GetPlayer()->CurrentPlayBin() )
            {
                const GstStructure * s = gst_message_get_structure( message );
                const gchar * name = gst_structure_get_name( s );

                ////guLogDebug( wxT( "MESSAGE_ELEMENT %s" ), wxString( element ).c_str() );
                if( !strcmp( name, "level" ) )
                {
                    guLevelInfo * LevelInfo = new guLevelInfo();
                    guMediaEvent event( guEVT_MEDIA_LEVELINFO );
                    gint channels;
                    const GValue * list;
                    const GValue * value;
                    GValueArray * avalue;

//                    if( !gst_structure_get_clock_time( s, "endtime", &LevelInfo->m_EndTime ) )
//                        guLogWarning( wxT( "Could not parse endtime" ) );
//
//                    LevelInfo->m_EndTime /= GST_MSECOND;

//                    GstFormat format = GST_FORMAT_TIME;
//                    if( gst_element_query_position( ctrl->OutputSink(), &format, ( gint64 * ) &LevelInfo->m_OutTime ) )
//                    {
//                        LevelInfo->m_OutTime /= GST_MSECOND;
//                    }

                    ////guLogDebug( wxT( "endtime: %" GST_TIME_FORMAT ", channels: %d" ), GST_TIME_ARGS( endtime ), channels );

                    // we can get the number of channels as the length of any of the value lists
    //                list = gst_structure_get_value( s, "rms" );
    //                channels = LevelInfo->m_Channels = gst_value_list_get_size( list );
    //                value = gst_value_list_get_value( list, 0 );
    //                LevelInfo->m_RMS_L = g_value_get_double( value );
    //                if( channels > 1 )
    //                {
    //                    value = gst_value_list_get_value( list, 1 );
    //                    LevelInfo->m_RMS_R = g_value_get_double( value );
    //                }

                    list = gst_structure_get_value( s, "peak" );
                    avalue = ( GValueArray * ) g_value_get_boxed( list );

                    channels = avalue->n_values;
                    //value = g_value_array_get_nth( avalue, 0 );
                    value = avalue->values;
                    LevelInfo->m_Peak_L = g_value_get_double( value );
                    if( channels > 1 )
                    {
                        //value = g_value_array_get_nth( avalue, 1 );
                        value = avalue->values + 1;
                        LevelInfo->m_Peak_R = g_value_get_double( value );
                    }


                    list = gst_structure_get_value( s, "decay" );
                    avalue = ( GValueArray * ) g_value_get_boxed( list );

                    //value = g_value_array_get_nth( avalue, 0 );
                    value = avalue->values;
                    LevelInfo->m_Decay_L = g_value_get_double( value );
                    if( channels > 1 )
                    {
                        //value = g_value_array_get_nth( avalue, 1 );
                        value = avalue->values + 1;
                        LevelInfo->m_Decay_R = g_value_get_double( value );
                    }


    //                //guLogDebug( wxT( "    RMS: %f dB, peak: %f dB, decay: %f dB" ),
    //                    event.m_LevelInfo.m_RMS_L,
    //                    event.m_LevelInfo.m_Peak_L,
    //                    event.m_LevelInfo.m_Decay_L );

    //                // converting from dB to normal gives us a value between 0.0 and 1.0 */
    //                rms = pow( 10, rms_dB / 20 );
    //                  //guLogDebug( wxT( "    normalized rms value: %f" ), rms );
                    event.SetClientObject( ( wxClientData * ) LevelInfo );
                    event.SetExtraLong( ctrl->GetId() );
                    ctrl->SendEvent( event );
                }
            }
            break;
        }

        //
        case GST_MESSAGE_APPLICATION :
        {
            const GstStructure * Struct;
            const char * Name;

            Struct = gst_message_get_structure( message );
            Name = gst_structure_get_name( Struct );
            guLogDebug( wxT( "Got Application Message %s" ), GST_TO_WXSTRING( Name ).c_str() );

            if( !strcmp( Name, guFADERPLAYBIN_MESSAGE_FADEIN_START ) )
            {
                if( !ctrl->EmittedStartFadeIn() )
                {
                    ctrl->FadeInStart();
                }
            }
            else if( !strcmp( Name, guFADERPLAYBIN_MESSAGE_FADEOUT_DONE ) )
            {
                if( !ctrl->EmittedStartFadeIn() )
                {
                    ctrl->FadeInStart();
                }
                ctrl->FadeOutDone();
            }

            break;
        }

        default:
            break;
    }

    return TRUE;
}

// -------------------------------------------------------------------------------- //
static void gst_about_to_finish( GstElement * playbin, guFaderPlayBin * ctrl )
{
    if( !ctrl->NextUri().IsEmpty() )
    {
        ctrl->AboutToFinish();
    }
    else if( !ctrl->EmittedStartFadeIn() )
    {
        ctrl->FadeInStart();
    }
}

// -------------------------------------------------------------------------------- //
static void gst_audio_changed( GstElement * playbin, guFaderPlayBin * ctrl )
{
    ctrl->AudioChanged();
}

// -------------------------------------------------------------------------------- //
// idle handler used to clean up finished streams
static gboolean cleanup_mediactrl( guMediaCtrl * player )
{
    player->DoCleanUp();
	return false;
}

// -------------------------------------------------------------------------------- //
static bool tick_timeout( guMediaCtrl * mediactrl )
{
    mediactrl->UpdatePosition();
	return true;
}

// -------------------------------------------------------------------------------- //
static bool seek_timeout( guFaderPlayBin * faderplaybin )
{
    faderplaybin->DoStartSeek();
	return false;
}

// -------------------------------------------------------------------------------- //
static bool pause_timeout( GstElement * playbin )
{
    gst_element_set_state( playbin, GST_STATE_PAUSED );
    return false;
}

// -------------------------------------------------------------------------------- //
static GstPadProbeReturn record_locked( GstPad * pad, GstPadProbeInfo * info, guFaderPlayBin * ctrl )
{
    guLogDebug( wxT( "Record_Unlocked" ) );

    ctrl->SetRecordFileName();

    gst_pad_remove_probe( pad, GST_PAD_PROBE_INFO_ID( info ) );

    return GST_PAD_PROBE_OK;
}

// -------------------------------------------------------------------------------- //
static GstPadProbeReturn add_record_element( GstPad * pad, GstPadProbeInfo * info, guFaderPlayBin * ctrl )
{
    guLogDebug( wxT( "add_record_element" ) );

    ctrl->AddRecordElement( pad );

    gst_pad_remove_probe( pad, GST_PAD_PROBE_INFO_ID( info ) );

    return GST_PAD_PROBE_OK;
}

// -------------------------------------------------------------------------------- //
static GstPadProbeReturn remove_record_element( GstPad * pad, GstPadProbeInfo * info, guFaderPlayBin * ctrl )
{
    guLogDebug( wxT( "remove_record_element" ) );

    ctrl->RemoveRecordElement( pad );

    gst_pad_remove_probe( pad, GST_PAD_PROBE_INFO_ID( info ) );

    return GST_PAD_PROBE_OK;
}

// -------------------------------------------------------------------------------- //
bool IsValidElement( GstElement * element )
{
    if( !GST_IS_ELEMENT( element ) )
    {
        if( G_IS_OBJECT( element ) )
            g_object_unref( element );
        return false;
    }
    return true;
}

}






// -------------------------------------------------------------------------------- //
guMediaCtrl::guMediaCtrl( guPlayerPanel * playerpanel )
{
    m_PlayerPanel = playerpanel;
    m_CurrentPlayBin = NULL;
    m_CleanUpId = 0;
    m_IsRecording = false;
    m_LastPosition = 0;

	// now that the sink is running, start polling for playing position.
	// might want to replace this with a complicated set of pad probes
	// to avoid polling, but duration queries on the sink are better
	// as they account for internal buffering etc.  maybe there's a way
	// to account for that in a pad probe callback on the sink's sink pad?
    gint ms_period = 1000 / 4;
    m_TickTimeoutId = g_timeout_add( ms_period, GSourceFunc( tick_timeout ), this );

    if( Init() )
    {
        UpdatedConfig();
    }
}

// -------------------------------------------------------------------------------- //
guMediaCtrl::~guMediaCtrl()
{
    if( m_TickTimeoutId )
    {
        g_source_remove( m_TickTimeoutId );
        m_TickTimeoutId = 0;
    }

    if( m_CleanUpId )
    {
        g_source_remove( m_CleanUpId );
        m_CleanUpId = 0;
    }

    CleanUp();

    gst_deinit();
}

// -------------------------------------------------------------------------------- //
void guMediaCtrl::CleanUp( void )
{
    Lock();
    int Index;
    int Count = m_FaderPlayBins.Count();
    for( Index = 0; Index < Count; Index++ )
    {
        delete m_FaderPlayBins[ Index ];
    }

    Unlock();
}

// -------------------------------------------------------------------------------- //
void guMediaCtrl::SendEvent( guMediaEvent &event )
{
    wxPostEvent( m_PlayerPanel, event );
}

// -------------------------------------------------------------------------------- //
void guMediaCtrl::UpdatePosition( void )
{
	gint64 Pos = -1;
	gint64 Duration = -1;

	Lock();
	if( m_CurrentPlayBin )
	{
	    m_CurrentPlayBin->Lock();
		Pos = -1;
		gst_element_query_position( m_CurrentPlayBin->m_OutputSink, GST_FORMAT_TIME, &Pos );


		Duration = -1;
        gst_element_query_duration( m_CurrentPlayBin->m_OutputSink, GST_FORMAT_TIME, &Duration );

        Pos = Pos / GST_MSECOND;
        if( Pos != m_LastPosition )
        {
            if( !m_CurrentPlayBin->AboutToFinishPending() || Pos < m_LastPosition )
            {
                //guLogMessage( wxT( "Sent UpdatePositon event for %i" ), m_CurrentPlayBin->GetId() );
                guMediaEvent event( guEVT_MEDIA_CHANGED_POSITION );
                event.SetInt( Pos );
                event.SetExtraLong( Duration / GST_MSECOND );
                event.SetClientData( ( void * ) m_CurrentPlayBin->GetId() );
                SendEvent( event );
                if( m_CurrentPlayBin->AboutToFinishPending() )
                    m_CurrentPlayBin->ResetAboutToFinishPending();
            }
            m_LastPosition = Pos;
        }
        //guLogDebug( wxT( "Current fade volume: %0.2f" ), m_CurrentPlayBin->GetFaderVolume() );
        m_CurrentPlayBin->Unlock();
	}
	Unlock();
}

// -------------------------------------------------------------------------------- //
bool guMediaCtrl::RemovePlayBin( guFaderPlayBin * playbin )
{
    guLogDebug( wxT( "guMediaCtrl::RemovePlayBin (%li)" ), playbin->m_Id );
    bool RetVal = false;

    Lock();
    if( m_CurrentPlayBin == playbin )
        m_CurrentPlayBin = NULL;

    if( m_FaderPlayBins.Index( playbin ) != wxNOT_FOUND )
    {
        m_FaderPlayBins.Remove( playbin );
        RetVal = true;
    }
    Unlock();
    return RetVal;
}

// -------------------------------------------------------------------------------- //
wxFileOffset guMediaCtrl::Position( void )
{
    wxFileOffset Pos = 0;
    Lock();
    if( m_CurrentPlayBin )
    {
        Pos = m_CurrentPlayBin->Position();
    }
    Unlock();
    return Pos;
}

// -------------------------------------------------------------------------------- //
wxFileOffset guMediaCtrl::Length( void )
{
    Lock();
    wxFileOffset Len = 0;
    if( m_CurrentPlayBin )
    {
        Len = m_CurrentPlayBin->Length();
    }
    Unlock();
    return Len;
}

// -------------------------------------------------------------------------------- //
bool guMediaCtrl::SetVolume( double volume )
{
    //guLogDebug( wxT( "Volume: %0.5f" ), volume );
    m_Volume = volume;
    Lock();
    int Index;
    int Count = m_FaderPlayBins.Count();
    for( Index = 0; Index < Count; Index++ )
    {
        m_FaderPlayBins[ Index ]->SetVolume( volume );
    }
    Unlock();
    return true;
}

// -------------------------------------------------------------------------------- //
void guMediaCtrl::SetEqualizerBand( const int band, const int value )
{
    m_EqBands[ band ] = value;
    Lock();
    int Index;
    int Count = m_FaderPlayBins.Count();
    for( Index = 0; Index < Count; Index++ )
    {
        m_FaderPlayBins[ Index ]->SetEqualizerBand( band, value );
    }
    Unlock();
}

// -------------------------------------------------------------------------------- //
bool guMediaCtrl::SetEqualizer( const wxArrayInt &eqbands )
{
    m_EqBands = eqbands;
    Lock();
    int Index;
    int Count = m_FaderPlayBins.Count();
    for( Index = 0; Index < Count; Index++ )
    {
        m_FaderPlayBins[ Index ]->SetEqualizer( eqbands );
    }
    Unlock();
    return true;
}

// -------------------------------------------------------------------------------- //
void guMediaCtrl::ResetEqualizer( void )
{
    int Index;
    int Count = m_EqBands.Count();
    for( Index = 0; Index < Count; Index++ )
    {
        m_EqBands[ Index ] = 0;
    }

    Lock();
    Count = m_FaderPlayBins.Count();
    for( Index = 0; Index < Count; Index++ )
    {
        m_FaderPlayBins[ Index ]->SetEqualizer( m_EqBands );
    }
    Unlock();
}

// -------------------------------------------------------------------------------- //
guMediaState guMediaCtrl::GetState( void )
{
    guMediaState State = guMEDIASTATE_STOPPED;
    Lock();
    if( m_CurrentPlayBin )
    {
        guLogDebug( wxT( "guMediaCtrl::GetState %i" ), m_CurrentPlayBin->GetState() );

        switch( m_CurrentPlayBin->GetState() )
        {
            case guFADERPLAYBIN_STATE_FADEOUT_PAUSE :
            case guFADERPLAYBIN_STATE_PAUSED :
                State = guMEDIASTATE_PAUSED;
                break;

            case guFADERPLAYBIN_STATE_FADEOUT :
            case guFADERPLAYBIN_STATE_FADEOUT_STOP :
            case guFADERPLAYBIN_STATE_FADEIN :
            case guFADERPLAYBIN_STATE_PLAYING :
                State = guMEDIASTATE_PLAYING;
                break;

            case guFADERPLAYBIN_STATE_ERROR :
                State = guMEDIASTATE_ERROR;



            default :
                break;
        }
    }
    Unlock();
    return State;
}

// -------------------------------------------------------------------------------- //
long guMediaCtrl::Load( const wxString &uri, guFADERPLAYBIN_PLAYTYPE playtype, const int startpos )
{
    guLogDebug( wxT( "guMediaCtrl::Load %i" ), playtype );

    guFaderPlayBin * FaderPlaybin;
    long Result = 0;

#ifdef guSHOW_DUMPFADERPLAYBINS
    Lock();
    DumpFaderPlayBins( m_FaderPlayBins, m_CurrentPlayBin );
    Unlock();
#endif

    if( IsBuffering() )
    {
        playtype = guFADERPLAYBIN_PLAYTYPE_REPLACE;
    }

    switch( playtype )
    {
        case guFADERPLAYBIN_PLAYTYPE_AFTER_EOS :
        {
            Lock();
            if( m_CurrentPlayBin )
            {
                m_CurrentPlayBin->SetNextUri( uri );
                m_CurrentPlayBin->SetNextId( wxGetLocalTime() );
                Result = m_CurrentPlayBin->NextId();
                Unlock();
                return Result;
            }
            Unlock();
            break;
        }

        case guFADERPLAYBIN_PLAYTYPE_REPLACE :
        {
            guLogDebug( wxT( "Replacing the current track in the current playbin..." ) );
            Lock();
            if( m_CurrentPlayBin )
            {
                guLogDebug( wxT( "Id: %li  State: %i  Error: %i" ), m_CurrentPlayBin->GetId(), m_CurrentPlayBin->GetState(), m_CurrentPlayBin->ErrorCode() );
                //if( m_CurrentPlayBin->m_State == guFADERPLAYBIN_STATE_ERROR )
                if( !m_CurrentPlayBin->IsOk() )
                {
                    guLogDebug( wxT( "The current playbin has error...Removing it" ) );
                    m_CurrentPlayBin->m_State = guFADERPLAYBIN_STATE_PENDING_REMOVE;
                    ScheduleCleanUp();
                }
                else
                {
                    m_CurrentPlayBin->m_PlayType = guFADERPLAYBIN_PLAYTYPE_REPLACE;
                    m_CurrentPlayBin->m_State = guFADERPLAYBIN_STATE_WAITING;
                    m_CurrentPlayBin->Load( uri, true, startpos );
                    m_CurrentPlayBin->SetBuffering( false );
                    m_CurrentPlayBin->SetFaderVolume( 1.0 );
                    Result = m_CurrentPlayBin->GetId();
                    Unlock();
                    return Result;
                }
            }
            Unlock();
            break;
        }

        default :
            break;
    }

    FaderPlaybin = new guFaderPlayBin( this, uri, playtype, startpos );
    if( FaderPlaybin )
    {
        if( FaderPlaybin->IsOk() )
        {
            if( gst_element_set_state( FaderPlaybin->Playbin(), GST_STATE_PAUSED ) != GST_STATE_CHANGE_FAILURE )
            {
                Lock();
                m_FaderPlayBins.Insert( FaderPlaybin, 0 );
                Unlock();

                guMediaEvent event( guEVT_MEDIA_LOADED );
                event.SetInt( true );
                SendEvent( event );

                return FaderPlaybin->GetId();
            }
//            else
//            {
//                RemovePlayBin( FaderPlaybin );
//            }
        }
        delete FaderPlaybin;
    }
    return Result;
}

// -------------------------------------------------------------------------------- //
bool guMediaCtrl::Play( void )
{
    guLogDebug( wxT( "wxMediaCtrl::Play" ) );
    Lock();

    if( !m_FaderPlayBins.Count() )
    {
        Unlock();
        return false;
    }

#ifdef guSHOW_DUMPFADERPLAYBINS
    DumpFaderPlayBins( m_FaderPlayBins, m_CurrentPlayBin );
#endif
    guFaderPlayBin * FaderPlaybin = m_FaderPlayBins[ 0 ];
    guLogDebug( wxT( "CurrentFaderPlayBin State %i" ), FaderPlaybin->GetState() );

    Unlock();

    switch( FaderPlaybin->GetState() )
    {
        case guFADERPLAYBIN_STATE_FADEIN :
        case guFADERPLAYBIN_STATE_PLAYING :
        case guFADERPLAYBIN_STATE_FADEOUT :
        case guFADERPLAYBIN_STATE_FADEOUT_PAUSE :
        case guFADERPLAYBIN_STATE_FADEOUT_STOP :
        {
            FaderPlaybin->m_State = guFADERPLAYBIN_STATE_PLAYING;
            // Send event of change state to Playing
            guMediaEvent event( guEVT_MEDIA_CHANGED_STATE );
            event.SetInt( GST_STATE_PLAYING );
            SendEvent( event );
            break;
        }

        case guFADERPLAYBIN_STATE_PAUSED :
        {
            Lock();
            if( m_CurrentPlayBin != FaderPlaybin )
            {
                m_CurrentPlayBin = FaderPlaybin;
            }
            Unlock();
            FaderPlaybin->StartFade( 0.0, 1.0, ( !m_ForceGapless && m_FadeOutTime ) ? guFADERPLAYBIN_FAST_FADER_TIME : guFADERPLAYBIN_SHORT_FADER_TIME );
            FaderPlaybin->m_State = guFADERPLAYBIN_STATE_FADEIN;
            FaderPlaybin->Play();

            guMediaEvent event( guEVT_MEDIA_CHANGED_STATE );
            event.SetInt( GST_STATE_PLAYING );
            SendEvent( event );
            break;
        }

        case guFADERPLAYBIN_STATE_WAITING :
        case guFADERPLAYBIN_STATE_WAITING_EOS :
            return FaderPlaybin->StartPlay();

        default :
            break;
    }

    return false;
}

// -------------------------------------------------------------------------------- //
bool guMediaCtrl::Pause( void )
{
    guLogDebug( wxT( "***************************************************************************** guMediaCtrl::Pause" ) );

    guFaderPlayBin * FaderPlayBin = NULL;
	wxArrayPtrVoid  ToFade;

	bool            Done = FALSE;
	double          FadeOutStart = 1.0f;
	gint64          FadeOutTime;

	Lock();
	int Index;
	int Count = m_FaderPlayBins.Count();
	if( !Count )
	{
	    Unlock();
        return true;
	}
#ifdef guSHOW_DUMPFADERPLAYBINS
    DumpFaderPlayBins( m_FaderPlayBins, m_CurrentPlayBin );
#endif

	for( Index = 0; Index < Count; Index++ )
	{
        FaderPlayBin = m_FaderPlayBins[ Index ];

		switch( FaderPlayBin->m_State )
		{
            case guFADERPLAYBIN_STATE_WAITING :
            case guFADERPLAYBIN_STATE_WAITING_EOS :
                FaderPlayBin->m_State = guFADERPLAYBIN_STATE_PAUSED;
                //guLogDebug( wxT( "stream %s is not yet playing, can't pause" ), FaderPlayBin->m_Uri.c_str() );
                break;

            case guFADERPLAYBIN_STATE_PAUSED :
            case guFADERPLAYBIN_STATE_FADEOUT_PAUSE :
            case guFADERPLAYBIN_STATE_FADEOUT_STOP :
                //guLogDebug( wxT( "stream %s is already paused" ), FaderPlayBin->m_Uri.c_str() );
                Done = true;
                break;

            case guFADERPLAYBIN_STATE_FADEIN :
            case guFADERPLAYBIN_STATE_FADEOUT :
            case guFADERPLAYBIN_STATE_PLAYING :
                //guLogDebug( wxT( "pausing stream %s -> FADING_OUT_PAUSED" ), FaderPlayBin->m_Uri.c_str() );
                ToFade.Insert( FaderPlayBin, 0 );
                Done = true;
                break;

            case guFADERPLAYBIN_STATE_PENDING_REMOVE:
                //guLogDebug( wxT( "stream %s is done, can't pause" ), FaderPlayBin->m_Uri.c_str() );
                break;
		}

//		if( Done )
//			break;
	}

    Unlock();

	Count = ToFade.Count();
	for( Index = 0; Index < Count; Index++ )
	{
        FaderPlayBin = ( guFaderPlayBin * ) ToFade[ Index ];
        FadeOutTime = ( !m_ForceGapless && m_FadeOutTime ) ? guFADERPLAYBIN_FAST_FADER_TIME : guFADERPLAYBIN_SHORT_FADER_TIME;
		switch( FaderPlayBin->m_State )
		{
            case guFADERPLAYBIN_STATE_FADEOUT :
            case guFADERPLAYBIN_STATE_FADEIN :
                FadeOutStart = FaderPlayBin->GetFaderVolume();
                FadeOutTime = ( gint64 ) ( ( ( double ) guFADERPLAYBIN_FAST_FADER_TIME ) * FadeOutStart );
                //guLogDebug( wxT( "============== Fading Out a Fading In playbin =================" ) );

            case guFADERPLAYBIN_STATE_PLAYING:
            {
                FaderPlayBin->m_State = guFADERPLAYBIN_STATE_FADEOUT_PAUSE;
                FaderPlayBin->StartFade( FadeOutStart, 0.0f, FadeOutTime );
            }

            default:
                // shouldn't happen, but ignore it if it does
                break;
		}
	}

	if( !Done )
		guLogMessage( wxT( "couldn't find a stream to pause" ) );

#ifdef guSHOW_DUMPFADERPLAYBINS
    Lock();
    DumpFaderPlayBins( m_FaderPlayBins, m_CurrentPlayBin );
    Unlock();
#endif

    return Done;
}

// -------------------------------------------------------------------------------- //
bool guMediaCtrl::Stop( void )
{
    guLogDebug( wxT( "***************************************************************************** guMediaCtrl::Stop" ) );

    guFaderPlayBin * FaderPlayBin = NULL;
	wxArrayPtrVoid  ToFade;

	bool            Done = FALSE;
	double          FadeOutStart = 1.0f;
	gint64          FadeOutTime;

	Lock();
	int Index;
	int Count = m_FaderPlayBins.Count();
	if( !Count )
	{
	    Unlock();
        return true;
	}

#ifdef guSHOW_DUMPFADERPLAYBINS
    DumpFaderPlayBins( m_FaderPlayBins, m_CurrentPlayBin );
#endif

	for( Index = 0; Index < Count; Index++ )
	{
        FaderPlayBin = m_FaderPlayBins[ Index ];

		switch( FaderPlayBin->m_State )
		{
            case guFADERPLAYBIN_STATE_WAITING :
            case guFADERPLAYBIN_STATE_WAITING_EOS :
                FaderPlayBin->m_State = guFADERPLAYBIN_STATE_PENDING_REMOVE;
                Unlock();
                ScheduleCleanUp();
                Lock();
                //guLogDebug( wxT( "stream %s is not yet playing, can't pause" ), FaderPlayBin->m_Uri.c_str() );
                break;

            case guFADERPLAYBIN_STATE_PAUSED :
            case guFADERPLAYBIN_STATE_FADEOUT_PAUSE :
            case guFADERPLAYBIN_STATE_FADEOUT_STOP :
                //guLogDebug( wxT( "stream %s is already paused" ), FaderPlayBin->m_Uri.c_str() );
                FaderPlayBin->m_State = guFADERPLAYBIN_STATE_PENDING_REMOVE;
                Unlock();
                ScheduleCleanUp();
                Lock();
                break;

            case guFADERPLAYBIN_STATE_FADEIN :
            case guFADERPLAYBIN_STATE_FADEOUT :
            case guFADERPLAYBIN_STATE_PLAYING :
                //guLogDebug( wxT( "pausing stream %s -> FADING_OUT_PAUSED" ), FaderPlayBin->m_Uri.c_str() );
                ToFade.Insert( FaderPlayBin, 0 );
                Done = true;
                break;

            case guFADERPLAYBIN_STATE_PENDING_REMOVE:
                //guLogDebug( wxT( "stream %s is done, can't pause" ), FaderPlayBin->m_Uri.c_str() );
                break;
		}

//		if( Done )
//			break;
	}

    Unlock();

	Count = ToFade.Count();
	for( Index = 0; Index < Count; Index++ )
	{
        FaderPlayBin = ( guFaderPlayBin * ) ToFade[ Index ];
        FadeOutTime = ( !m_ForceGapless && m_FadeOutTime ) ? guFADERPLAYBIN_FAST_FADER_TIME : guFADERPLAYBIN_SHORT_FADER_TIME;
		switch( FaderPlayBin->m_State )
		{
            case guFADERPLAYBIN_STATE_FADEOUT :
            case guFADERPLAYBIN_STATE_FADEIN :
                FadeOutStart = FaderPlayBin->GetFaderVolume();
                FadeOutTime = ( gint64 ) ( ( ( double ) guFADERPLAYBIN_FAST_FADER_TIME ) * FadeOutStart );
                //guLogDebug( wxT( "============== Fading Out a Fading In playbin =================" ) );

            case guFADERPLAYBIN_STATE_PLAYING:
            {
                if( IsBuffering() )
                {
                    FaderPlayBin->m_State = guFADERPLAYBIN_STATE_PENDING_REMOVE;
                    ScheduleCleanUp();

                    guMediaEvent BufEvent( guEVT_MEDIA_BUFFERING );
                    BufEvent.SetInt( 100 );
                    SendEvent( BufEvent );

                    guMediaEvent event( guEVT_MEDIA_FADEOUT_FINISHED );
                    SendEvent( event );
                    FaderPlayBin->SetBuffering( false );
                }
                else
                {
                    FaderPlayBin->m_State = guFADERPLAYBIN_STATE_FADEOUT_STOP;
                    FaderPlayBin->StartFade( FadeOutStart, 0.0f, FadeOutTime );
                }
            }

            default:
                // shouldn't happen, but ignore it if it does
                break;
		}
	}

	if( !Done )
	{
        guMediaEvent event( guEVT_MEDIA_CHANGED_STATE );
        event.SetInt( GST_STATE_READY );
        SendEvent( event );

		guLogMessage( wxT( "couldn't find a stream to pause" ) );
	}

#ifdef guSHOW_DUMPFADERPLAYBINS
    Lock();
    DumpFaderPlayBins( m_FaderPlayBins, m_CurrentPlayBin );
    Unlock();
#endif

    return Done;
}

// -------------------------------------------------------------------------------- //
bool guMediaCtrl::Seek( wxFileOffset where, const bool accurate )
{
    guLogDebug( wxT( "guMediaCtrl::Seek( %lli )" ), where );
    bool Result = false;
    Lock();
    if( m_CurrentPlayBin )
    {
        Result = m_CurrentPlayBin->Seek( where, accurate );
    }
    Unlock();
    return Result;
}

// -------------------------------------------------------------------------------- //
void guMediaCtrl::UpdatedConfig( void )
{
    guConfig * Config  = ( guConfig * ) guConfig::Get();
    m_ForceGapless          = Config->ReadBool( wxT( "ForceGapless" ), false, wxT( "crossfader" ) );
    m_FadeOutTime           = Config->ReadNum( wxT( "FadeOutTime" ), 50, wxT( "crossfader" ) ) * 100;
    m_FadeInTime            = Config->ReadNum( wxT( "FadeInTime" ), 10, wxT( "crossfader" ) ) * 100;
    m_FadeInVolStart        = double( Config->ReadNum( wxT( "FadeInVolStart" ), 80, wxT( "crossfader" ) ) ) / 100.0;
    m_FadeInVolTriger       = double( Config->ReadNum( wxT( "FadeInVolTriger" ), 50, wxT( "crossfader" ) ) ) / 100.0;
    m_BufferSize            = Config->ReadNum( wxT( "BufferSize" ), 64, wxT( "general" ) );
    m_ReplayGainMode        = Config->ReadNum( wxT( "ReplayGainMode" ), 0, wxT( "general" ) );
    m_ReplayGainPreAmp      = double( Config->ReadNum( wxT( "ReplayGainPreAmp" ), 6, wxT( "general" ) ) );
    //m_ReplayGainFallback    = double( Config->ReadNum( wxT( "ReplayGainFallback" ), -6, wxT( "General" ) ) );
    m_OutputDevice          = Config->ReadNum( wxT( "OutputDevice" ), guOUTPUT_DEVICE_AUTOMATIC, wxT( "playback" ) );
    m_OutputDeviceName      = Config->ReadStr( wxT( "OutputDeviceName" ), wxEmptyString, wxT( "playback" ) );
}

// -------------------------------------------------------------------------------- //
void guMediaCtrl::ScheduleCleanUp( void )
{
    //Lock();
    if( !m_CleanUpId )
    {
        m_CleanUpId = g_timeout_add( 4000, GSourceFunc( cleanup_mediactrl ), this );
    }
    //Unlock();
}

// -------------------------------------------------------------------------------- //
void guMediaCtrl::FadeInStart( void )
{
    guLogDebug( wxT( "guMediaCtrl::FadeInStart" ) );

    Lock();
#ifdef guSHOW_DUMPFADERPLAYBINS
    DumpFaderPlayBins( m_FaderPlayBins, m_CurrentPlayBin );
#endif
    int Index;
    int Count = m_FaderPlayBins.Count();
    for( Index = 0; Index < Count; Index++ )
    {
        guFaderPlayBin * NextPlayBin = m_FaderPlayBins[ Index ];
        if( NextPlayBin->m_State == guFADERPLAYBIN_STATE_WAITING )
        {
            guLogDebug( wxT( "got fade-in-start for stream %s -> FADE_IN" ), NextPlayBin->m_Uri.c_str() );
            NextPlayBin->StartFade( m_FadeInVolStart, 1.0, m_FadeInTime );
            NextPlayBin->m_State = guFADERPLAYBIN_STATE_FADEIN;
            NextPlayBin->Play();

            guMediaEvent event( guEVT_MEDIA_CHANGED_STATE );
            event.SetInt( GST_STATE_PLAYING );
            SendEvent( event );

            guMediaEvent event2( guEVT_MEDIA_FADEIN_STARTED );
            event2.SetExtraLong( NextPlayBin->GetId() );
            SendEvent( event2 );

            m_CurrentPlayBin = NextPlayBin;
            break;
        }
    }
    Unlock();
}

// -------------------------------------------------------------------------------- //
void guMediaCtrl::FadeOutDone( guFaderPlayBin * faderplaybin )
{
    guLogDebug( wxT( "guMediaCtrl:FadeOutDone" ) );
    switch( faderplaybin->m_State )
    {
        case guFADERPLAYBIN_STATE_FADEOUT :
        {
            faderplaybin->m_State = guFADERPLAYBIN_STATE_PENDING_REMOVE;
            ScheduleCleanUp();

            guMediaEvent event( guEVT_MEDIA_FADEOUT_FINISHED );
            event.SetExtraLong( faderplaybin->GetId() );
            SendEvent( event );
            break;
        }

        case guFADERPLAYBIN_STATE_FADEOUT_PAUSE:
        {
            // try to seek back a bit to account for the fade
            gint64 Pos = -1;
            gst_element_query_position( faderplaybin->OutputSink(), GST_FORMAT_TIME, &Pos );
            if( Pos != -1 )
            {
                faderplaybin->m_PausePosition = Pos > guFADERPLAYBIN_FAST_FADER_TIME ? Pos - guFADERPLAYBIN_FAST_FADER_TIME : guFADERPLAYBIN_SHORT_FADER_TIME;
                //guLogDebug( wxT( "got fade-out-done for stream %s -> SEEKING_PAUSED [%" G_GINT64_FORMAT "]" ), FaderPlayBin->m_Uri.c_str(), FaderPlayBin->m_SeekTarget );
            }
            else
            {
                faderplaybin->m_PausePosition = wxNOT_FOUND;
                //guLogDebug( wxT( "got fade-out-done for stream %s -> PAUSED (position query failed)" ), FaderPlayBin->m_Uri.c_str() );
            }
            faderplaybin->m_State = guFADERPLAYBIN_STATE_PAUSED;
            faderplaybin->Pause();

            guMediaEvent event( guEVT_MEDIA_CHANGED_STATE );
            event.SetInt( GST_STATE_PAUSED );
            SendEvent( event );
            break;
        }

        case guFADERPLAYBIN_STATE_FADEOUT_STOP :
        {
            faderplaybin->m_PausePosition = 0;
            faderplaybin->m_State = guFADERPLAYBIN_STATE_STOPPED;
            faderplaybin->Stop();

            guMediaEvent event( guEVT_MEDIA_CHANGED_STATE );
            event.SetInt( GST_STATE_READY );
            SendEvent( event );
            break;
        }
    }
}

// -------------------------------------------------------------------------------- //
bool guMediaCtrl::EnableRecord( const wxString &recfile, const int format, const int quality )
{
    guLogDebug( wxT( "guMediaCtrl::EnableRecord" ) );
    Lock();
    m_IsRecording = ( m_CurrentPlayBin && m_CurrentPlayBin->EnableRecord( recfile, format, quality ) );
    Unlock();
    return  m_IsRecording;
}

// -------------------------------------------------------------------------------- //
void guMediaCtrl::DisableRecord( void )
{
    guLogDebug( wxT( "guMediaCtrl::DisableRecord" ) );
    m_IsRecording = false;
    Lock();
    if( m_CurrentPlayBin )
    {
        m_CurrentPlayBin->DisableRecord();
    }
    Unlock();
}

// -------------------------------------------------------------------------------- //
bool guMediaCtrl::SetRecordFileName( const wxString &filename )
{
    guLogDebug( wxT( "guMediaCtrl::SetRecordFileName  '%s'" ), filename.c_str() );

    Lock();
    bool Result = m_CurrentPlayBin && m_CurrentPlayBin->SetRecordFileName( filename );
    Unlock();
    return Result;
}

// -------------------------------------------------------------------------------- //
void guMediaCtrl::DoCleanUp( void )
{
    guLogDebug( wxT( "guMediaCtrl::DoCleanUp" ) );
	wxArrayPtrVoid ToDelete;

    Lock();
#ifdef guSHOW_DUMPFADERPLAYBINS
    DumpFaderPlayBins( m_FaderPlayBins, m_CurrentPlayBin );
#endif


	m_CleanUpId = 0;

	int Index;
	int Count = m_FaderPlayBins.Count();
	for( Index = Count - 1; Index >= 0; Index-- )
	{
	    guFaderPlayBin * FaderPlaybin = m_FaderPlayBins[ Index ];
		if( ( FaderPlaybin->m_State == guFADERPLAYBIN_STATE_PENDING_REMOVE ) ||
		    ( FaderPlaybin->m_State == guFADERPLAYBIN_STATE_ERROR ) )
		{
			ToDelete.Add( FaderPlaybin );
			m_FaderPlayBins.RemoveAt( Index );
			if( FaderPlaybin == m_CurrentPlayBin )
			{
			    m_CurrentPlayBin = NULL;
			}
		}
	}
	if( !m_FaderPlayBins.Count() )
	{
        guMediaEvent event( guEVT_MEDIA_CHANGED_STATE );
        event.SetInt( GST_STATE_READY );
        SendEvent( event );
	}

#ifdef guSHOW_DUMPFADERPLAYBINS
    DumpFaderPlayBins( m_FaderPlayBins, m_CurrentPlayBin );
#endif
	Unlock();

	Count = ToDelete.Count();
	for( Index = 0; Index < Count; Index++ )
	{
        guLogDebug( wxT( "Free stream %li" ), ( ( guFaderPlayBin * ) ToDelete[ Index ] )->GetId() );
		delete ( ( guFaderPlayBin * ) ToDelete[ Index ] );
	}
}

// -------------------------------------------------------------------------------- //
bool guMediaCtrl::Init()
{
    // Code taken from wxMediaCtrl class
    //Convert arguments to unicode if enabled
#if wxUSE_UNICODE
    int i;
    char * * argvGST = new char * [ wxTheApp->argc + 1 ];
    for( i = 0; i < wxTheApp->argc; i++ )
    {
        argvGST[ i ] = wxStrdupA( wxConvUTF8.cWX2MB( wxTheApp->argv[ i ] ) );
    }

    argvGST[ wxTheApp->argc ] = NULL;

    int argcGST = wxTheApp->argc;
#else
#define argcGST wxTheApp->argc
#define argvGST wxTheApp->argv
#endif

    //Really init gstreamer
    gboolean bInited;
    GError * error = NULL;
    bInited = gst_init_check( &argcGST, &argvGST, &error );

    // Cleanup arguments for unicode case
#if wxUSE_UNICODE
    for( i = 0; i < argcGST; i++ )
    {
        free( argvGST[ i ] );
    }

    delete [] argvGST;
#endif

    return bInited;
}




// -------------------------------------------------------------------------------- //
// guFaderPlayBin
// -------------------------------------------------------------------------------- //
guFaderPlayBin::guFaderPlayBin( guMediaCtrl * mediactrl, const wxString &uri, const int playtype, const int startpos )
{
	//guLogDebug( wxT( "creating new stream for %s" ), uri.c_str() );
    m_Player = mediactrl;
    m_Uri = uri;
    m_Id = wxGetLocalTimeMillis().GetLo();
    m_State = guFADERPLAYBIN_STATE_WAITING;
    m_ErrorCode = 0;
    m_IsFading = false;
    m_IsBuffering = false;
    m_EmittedStartFadeIn = false;
    m_PlayType = playtype;
    m_FaderTimeLine = NULL;
    m_AboutToFinishPending = false;
    m_LastFadeVolume = -1;
    m_StartOffset = startpos;
    m_SeekTimerId = 0;
    m_SettingRecordFileName = false;

    guLogDebug( wxT( "guFaderPlayBin::guFaderPlayBin (%li)  %i" ), m_Id, playtype );

    if( BuildOutputBin() && BuildPlaybackBin() )
    {
        //Load( uri, false );
        if( startpos )
        {
            m_SeekTimerId = g_timeout_add( 100, GSourceFunc( seek_timeout ), this );
        }
        SetVolume( m_Player->GetVolume() );
        SetEqualizer( m_Player->GetEqualizer() );
    }
}

// -------------------------------------------------------------------------------- //
guFaderPlayBin::~guFaderPlayBin()
{
    guLogDebug( wxT( "guFaderPlayBin::~guFaderPlayBin (%li) e: %i" ), m_Id, m_ErrorCode );
    //m_Player->RemovePlayBin( this );
    if( m_SeekTimerId )
    {
        g_source_remove( m_SeekTimerId );
    }

    if( m_Playbin )
    {
        GstStateChangeReturn ChangeState = gst_element_set_state( m_Playbin, GST_STATE_NULL );
//        typedef enum {
//          GST_STATE_CHANGE_FAILURE             = 0,
//          GST_STATE_CHANGE_SUCCESS             = 1,
//          GST_STATE_CHANGE_ASYNC               = 2,
//          GST_STATE_CHANGE_NO_PREROLL          = 3
//        } GstSt
        //guLogMessage( wxT( "Set To NULL: %i" ), ChangeState );
        if( ChangeState == GST_STATE_CHANGE_ASYNC )
        {
            gst_element_get_state( m_Playbin, NULL, NULL, GST_CLOCK_TIME_NONE );
        }
        gst_object_unref( GST_OBJECT( m_Playbin ) );
    }

    if( m_FaderTimeLine )
    {
        EndFade();
    }
    guLogDebug( wxT( "Finished destroying the playbin %li" ), m_Id );
}

// -------------------------------------------------------------------------------- //
bool guFaderPlayBin::BuildOutputBin( void )
{
    GstElement * outputsink;

    const char * ElementNames[] = {
        "autoaudiosink",
        "gconfaudiosink",
        "alsasink",
        "pulsesink",
        "osssink",
        NULL
    };

    int OutputDevice = m_Player->OutputDevice();
    if( ( OutputDevice >= guOUTPUT_DEVICE_AUTOMATIC ) && ( OutputDevice < guOUTPUT_DEVICE_OTHER ) )
    {
        outputsink = gst_element_factory_make( ElementNames[ OutputDevice ], "OutputSink" );
        if( IsValidElement( outputsink ) )
        {
            if( OutputDevice > guOUTPUT_DEVICE_GCONF )
            {
                wxString OutputDeviceName = m_Player->OutputDeviceName();
                if( !OutputDeviceName.IsEmpty() )
                {
                    g_object_set( outputsink, "device", ( const char * ) OutputDeviceName.mb_str( wxConvFile ), NULL );
                }
            }
            m_OutputSink = outputsink;
            return true;
        }
    }
    else if( OutputDevice == guOUTPUT_DEVICE_OTHER )
    {
        wxString OutputDeviceName = m_Player->OutputDeviceName();
        if( !OutputDeviceName.IsEmpty() )
        {
            GError * err = NULL;
            outputsink = gst_parse_launch( ( const char * ) OutputDeviceName.mb_str( wxConvFile ), &err );
            if( outputsink )
            {
                if( err )
                {
                    guLogMessage( wxT( "Error building output pipeline: '%s'" ), wxString( err->message, wxConvUTF8 ).c_str() );
                    g_error_free( err );
                }
                m_OutputSink = outputsink;
                return true;
            }
        }
    }

    guLogError( wxT( "The configured audio output sink is not valid. Autoconfiguring..." ) );

    int Index = 0;
    while( ElementNames[ Index ] )
    {
        outputsink = gst_element_factory_make( ElementNames[ Index ], "OutputSink" );
        if( IsValidElement( outputsink ) )
        {
            m_OutputSink = outputsink;
            return true;
        }
        Index++;
    }

    m_OutputSink = NULL;
    return false;
}

// -------------------------------------------------------------------------------- //
bool guFaderPlayBin::BuildPlaybackBin( void )
{
  m_Playbin = gst_element_factory_make( "playbin", "play" );
  if( IsValidElement( m_Playbin ) )
  {
    //m_Uri =
    g_object_set( m_Playbin, "uri", ( const char * ) m_Uri.mb_str( wxConvFile ), NULL );
    g_object_set( m_Playbin, "buffer-size", gint( m_Player->BufferSize() * 1024 ), NULL );
    //g_object_set( m_Playbin, "volume", 1.0, NULL );

    m_Playbackbin = gst_bin_new( "playbackbin" );
    if( IsValidElement( m_Playbackbin ) )
    {
      GstElement * converter = gst_element_factory_make( "audioconvert", "pb_audioconvert" );
      if( IsValidElement( converter ) )
      {
        GstElement * resample = gst_element_factory_make( "audioresample", "pb_audioresampler" );
        if( IsValidElement( resample ) )
        {
          m_ReplayGain = NULL;
          if( m_Player->m_ReplayGainMode )
          {
            m_ReplayGain = gst_element_factory_make( "rgvolume", "pb_rgvolume" );
          }

          if( !m_Player->m_ReplayGainMode || IsValidElement( m_ReplayGain ) )
          {
            if( m_ReplayGain )
            {
              g_object_set( G_OBJECT( m_ReplayGain ), "album-mode", gboolean( m_Player->m_ReplayGainMode - 1 ), NULL );
              g_object_set( G_OBJECT( m_ReplayGain ), "pre-amp", gdouble( m_Player->m_ReplayGainPreAmp ), NULL );
               //g_object_set( G_OBJECT( m_ReplayGain ), "fallback-gain", gdouble( -6 ), NULL );
            }

            m_FaderVolume = gst_element_factory_make( "volume", "fader_volume" );
            if( IsValidElement( m_FaderVolume ) )
            {
              if( m_PlayType == guFADERPLAYBIN_PLAYTYPE_CROSSFADE )
              {
                g_object_set( m_FaderVolume, "volume", gdouble( 0.0 ), NULL );
              }

              GstElement * level = gst_element_factory_make( "level", "pb_level" );
              if( IsValidElement( level ) )
              {
                g_object_set( level, "message", gboolean( true ), NULL );
                g_object_set( level, "interval", guint64( 100000000 ), NULL) ;
                g_object_set( level, "peak-falloff", gdouble( 6.0 ), NULL );
                g_object_set( level, "peak-ttl", guint64( 3 * 300000000 ), NULL );

                m_Volume = gst_element_factory_make( "volume", "pb_volume" );
                if( IsValidElement( m_Volume ) )
                {
                    m_Equalizer = gst_element_factory_make( "equalizer-10bands", "pb_equalizer" );
                    if( IsValidElement( m_Equalizer ) )
                    {
                        GstElement * limiter = gst_element_factory_make( "rglimiter", "pb_rglimiter" );
                        if( IsValidElement( limiter ) )
                        {
                            //g_object_set( G_OBJECT( limiter ), "enabled", TRUE, NULL );
                          GstElement * outconverter = gst_element_factory_make( "audioconvert", "pb_audioconvert2" );
                          if( IsValidElement( outconverter ) )
                          {
                             GstElement * outresample = gst_element_factory_make( "audioresample", "pb_audioresample2" );
                             if( IsValidElement( outresample ) )
                             {
                                m_Tee = gst_element_factory_make( "tee", "pb_tee" );
                                if( IsValidElement( m_Tee ) )
                                {
                                    GstElement * queue = gst_element_factory_make( "queue", "pb_queue" );
                                    if( IsValidElement( queue ) )
                                    {
                                        //g_object_set( queue, "max-size-time", guint64( 250000000 ), NULL );
                                        //g_object_set( queue, "max-size-time", 5 * GST_SECOND, "max-size-buffers", 0, "max-size-bytes", 0, NULL );

                                        if( m_ReplayGain )
                                        {
                                            gst_bin_add_many( GST_BIN( m_Playbackbin ), m_Tee, queue, converter, resample, m_FaderVolume, m_ReplayGain, level, m_Equalizer, limiter, m_Volume, outconverter, outresample, m_OutputSink, NULL );
                                            gst_element_link_many( m_Tee, queue, converter, resample, m_FaderVolume, m_ReplayGain, level, m_Equalizer, limiter, m_Volume, outconverter, outresample, m_OutputSink, NULL );
                                        }
                                        else
                                        {
                                            gst_bin_add_many( GST_BIN( m_Playbackbin ), m_Tee, queue, converter, resample, m_FaderVolume, level, m_Equalizer, limiter, m_Volume, outconverter, outresample, m_OutputSink, NULL );
                                            gst_element_link_many( m_Tee, queue, converter, resample, m_FaderVolume, level, m_Equalizer, limiter, m_Volume, outconverter, outresample, m_OutputSink, NULL );
                                        }

                                        GstPad * pad = gst_element_get_static_pad( m_Tee, "sink" );
                                        if( GST_IS_PAD( pad ) )
                                        {
                                            GstPad * ghostpad = gst_ghost_pad_new( "sink", pad );
                                            gst_element_add_pad( m_Playbackbin, ghostpad );
                                            gst_object_unref( pad );

                                            g_object_set( G_OBJECT( m_Playbin ), "audio-sink", m_Playbackbin, NULL );

                                            g_object_set( G_OBJECT( m_Playbin ), "flags", GST_PLAY_FLAG_AUDIO | GST_PLAY_FLAG_SOFT_VOLUME, NULL );

                                            g_signal_connect( G_OBJECT( m_Playbin ), "about-to-finish",
                                                G_CALLBACK( gst_about_to_finish ), ( void * ) this );
                                            //
                                            g_signal_connect( G_OBJECT( m_Playbin ), "audio-changed",
                                                G_CALLBACK( gst_audio_changed ), ( void * ) this );

                                            gst_bus_add_watch( gst_pipeline_get_bus( GST_PIPELINE( m_Playbin ) ),
                                                GstBusFunc( gst_bus_async_callback ), this );

                                            return true;
                                        }
                                        else
                                        {
                                            if( G_IS_OBJECT( pad ) )
                                                gst_object_unref( pad );
                                            guLogError( wxT( "Could not create the pad element" ) );
                                        }

                                        g_object_unref( queue );
                                    }
                                    else
                                    {
                                        guLogError( wxT( "Could not create the playback queue object" ) );
                                    }

                                    g_object_unref( m_Tee );
                                    m_Tee = NULL;
                                }
                                else
                                {
                                    guLogError( wxT( "Could not create the tee object" ) );
                                }
                                g_object_unref( outresample );
                             }
                             else
                             {
                               guLogError( wxT( "Could not create the outresample object" ) );
                             }
                             g_object_unref( outconverter );
                          }
                          else
                          {
                            guLogError( wxT( "Could not create the output outconvert object" ) );
                          }

                          g_object_unref( limiter );
                        }
                        else
                        {
                            guLogError( wxT( "Could not create the limiter object" ) );
                        }

                        g_object_unref( m_Equalizer );
                        m_Equalizer = NULL;
                    }
                    else
                    {
                        guLogError( wxT( "Could not create the equalizer object" ) );
                    }

                    g_object_unref( m_Volume );
                    m_Volume = NULL;
                }
                else
                {
                    guLogError( wxT( "Could not create the volume object" ) );
                }

                g_object_unref( level );
              }
              else
              {
                guLogError( wxT( "Could not create the level object" ) );
              }

              g_object_unref( m_FaderVolume );
            }
            else
            {
              guLogError( wxT( "Could not create the fader volume object" ) );
            }
            m_FaderVolume = NULL;

            g_object_unref( m_ReplayGain );
          }
          else
          {
            guLogError( wxT( "Could not create the replay gain object" ) );
          }
          m_ReplayGain = NULL;

          g_object_unref( resample );
        }
        else
        {
          guLogError( wxT( "Could not create the audioresample object" ) );
        }

        g_object_unref( converter );
      }
      else
      {
        guLogError( wxT( "Could not create the audioconvert object" ) );
      }

      g_object_unref( m_Playbackbin );
    }
    else
    {
      guLogError( wxT( "Could not create the gstreamer playbackbin." ) );
    }
    m_Playbackbin = NULL;

    gst_object_unref( m_Playbin );
  }
  else
  {
    guLogError( wxT( "Could not create the gstreamer playbin." ) );
  }
  m_Playbin = NULL;

  return false;
}

// -------------------------------------------------------------------------------- //
bool guFaderPlayBin::BuildRecordBin( const wxString &path, GstElement * encoder, GstElement * muxer )
{
    guLogDebug( wxT( "BuildRecordBin( '%s' )" ), path.c_str() );
    m_RecordBin = gst_bin_new( "gurb_recordbin" );
    if( IsValidElement( m_RecordBin ) )
    {
        GstElement * converter = gst_element_factory_make( "audioconvert", "gurb_audioconvert" );
        if( IsValidElement( converter ) )
        {
          GstElement * resample = gst_element_factory_make( "audioresample", "gurb_audioresample" );
          if( IsValidElement( resample ) )
          {
            m_FileSink = gst_element_factory_make( "filesink", "gurb_filesink" );
            if( IsValidElement( m_FileSink ) )
            {
                g_object_set( m_FileSink, "location", ( const char * ) path.mb_str( wxConvFile ), NULL );

                GstElement * queue = gst_element_factory_make( "queue", "gurb_queue" );
                if( IsValidElement( queue ) )
                {
                    // The bin contains elements that change state asynchronously and not as part of a state change in the entire pipeline.
                    g_object_set( m_RecordBin, "async-handling", gboolean( true ), NULL );

                    g_object_set( queue, "max-size-buffers", guint( 3 ), NULL );
                    //g_object_set( queue, "max-size-time", 10 * GST_SECOND, "max-size-buffers", 0, "max-size-bytes", 0, NULL );

                    if( muxer )
                    {
                        gst_bin_add_many( GST_BIN( m_RecordBin ), queue, converter, resample, encoder, muxer, m_FileSink, NULL );
                        gst_element_link_many( queue, converter, resample, encoder, muxer, m_FileSink, NULL );
                    }
                    else
                    {
                        gst_bin_add_many( GST_BIN( m_RecordBin ), queue, converter, resample, encoder, m_FileSink, NULL );
                        gst_element_link_many( queue, converter, resample, encoder, m_FileSink, NULL );
                    }

                    GstPad * pad = gst_element_get_static_pad( queue, "sink" );
                    if( GST_IS_PAD( pad ) )
                    {
                        m_RecordSinkPad = gst_ghost_pad_new( "sink", pad );
                        gst_element_add_pad( m_RecordBin, m_RecordSinkPad );
                        gst_object_unref( pad );

                        return true;
                    }
                    else
                    {
                        if( G_IS_OBJECT( pad ) )
                            gst_object_unref( pad );
                        guLogError( wxT( "Could not create the pad element" ) );
                    }

                    g_object_unref( queue );
                }
                else
                {
                    guLogError( wxT( "Could not create the playback queue object" ) );
                }

                g_object_unref( m_FileSink );
                m_FileSink = NULL;
            }
            else
            {
              guLogError( wxT( "Could not create the lame encoder object" ) );
            }
            g_object_unref( resample );
          }
          else
          {
            guLogError( wxT( "Could not create the lame resample object" ) );
          }
          g_object_unref( converter );
        }
        else
        {
            guLogError( wxT( "Could not create the record convert object" ) );
        }

        g_object_unref( m_RecordBin );
    }
    else
    {
        guLogError( wxT( "Could not create the recordbin object" ) );
    }
    m_RecordBin = NULL;

    return false;
}

// -------------------------------------------------------------------------------- //
bool guFaderPlayBin::DoStartSeek( void )
{
    guLogDebug( wxT( "DoStartSeek( %i )" ), m_StartOffset );
    m_SeekTimerId = 0;
    if( GST_IS_ELEMENT( m_Playbin ) )
    {
        return Seek( m_StartOffset, true );
    }
    return false;
}

// -------------------------------------------------------------------------------- //
void guFaderPlayBin::SendEvent( guMediaEvent &event )
{
    m_Player->SendEvent( event );
}

// -------------------------------------------------------------------------------- //
void guFaderPlayBin::SetBuffering( const bool isbuffering )
{
    if( m_IsBuffering != isbuffering )
    {
        m_IsBuffering = isbuffering;
        if( !isbuffering )
        {
            if( !m_PendingNewRecordName.IsEmpty() )
            {
                SetRecordFileName( m_PendingNewRecordName );
            }
        }
    }
}

// -------------------------------------------------------------------------------- //
bool guFaderPlayBin::SetVolume( double volume )
{
    guLogDebug( wxT( "guFaderPlayBin::SetVolume (%li)  %0.2f" ), m_Id, volume );
    g_object_set( m_Volume, "volume", gdouble( volume ), NULL );
    return true;
}

// -------------------------------------------------------------------------------- //
double guFaderPlayBin::GetFaderVolume( void )
{
    double RetVal;
    g_object_get( m_FaderVolume, "volume", &RetVal, NULL );
    return RetVal;
}

// -------------------------------------------------------------------------------- //
void guFaderPlayBin::FadeInStart( void )
{
    m_EmittedStartFadeIn = true;
    m_Player->FadeInStart();
}

// -------------------------------------------------------------------------------- //
void guFaderPlayBin::FadeOutDone( void )
{
     m_Player->FadeOutDone( this );
}

// -------------------------------------------------------------------------------- //
bool guFaderPlayBin::SetFaderVolume( double volume )
{
	const char * Message = NULL;
    //guLogMessage( wxT( "Set the VolEnd: %0.2f" ), volume );

    if( volume != m_LastFadeVolume )
    {
        //guLogDebug( wxT( "guFaderPlayBin::SetFaderVolume (%li)   %0.2f  %i" ), m_Id, volume, m_State == guFADERPLAYBIN_STATE_FADEIN );
        Lock();
        m_LastFadeVolume = volume;
        g_object_set( m_FaderVolume, "volume", gdouble( volume ), NULL );

        switch( m_State )
        {
            case guFADERPLAYBIN_STATE_FADEIN :
            {
                if( ( volume > 0.99 ) && m_IsFading )
                {
                    guLogDebug( wxT( "stream fully faded in (at %f) -> PLAYING state" ), volume );
                    m_IsFading = false;
                    m_State = guFADERPLAYBIN_STATE_PLAYING;
                }
                break;
            }

            case guFADERPLAYBIN_STATE_FADEOUT :
            case guFADERPLAYBIN_STATE_FADEOUT_PAUSE :
            case guFADERPLAYBIN_STATE_FADEOUT_STOP :
            {
                if( volume < 0.001 )
                {
                    //guLogDebug( wxT( "stream %s fully faded out (at %f)" ), faderplaybin->m_Uri.c_str(), Vol );
                    if( m_IsFading )
                    {
                        //Message = guFADERPLAYBIN_MESSAGE_FADEOUT_DONE;
    //                    m_State = guFADERPLAYBIN_STATE_PENDING_REMOVE;
    //                    guMediaEvent event( guEVT_MEDIA_FADEOUT_FINISHED );
    //                    event.SetExtraLong( GetId() );
    //                    SendEvent( event );
    //
    //                    m_Player->ScheduleCleanUp();
                        //m_Player->FadeOutDone( this );
                        Message = guFADERPLAYBIN_MESSAGE_FADEOUT_DONE;
                        //m_IsFading = false;
                    }
                }
                else if( !m_EmittedStartFadeIn && volume < ( m_Player->m_FadeInVolTriger + 0.001 ) )
                {
                    if( m_IsFading && m_State == guFADERPLAYBIN_STATE_FADEOUT )
                    {
                        //m_EmittedStartFadeIn = true;
                        //m_Player->FadeInStart();
                        Message = guFADERPLAYBIN_MESSAGE_FADEIN_START;
                    }
                }
                break;
            }
        }

        Unlock();
    }

	if( Message )
	{
		GstMessage * Msg;
		GstStructure * Struct;
		//guLogDebug( wxT( "posting %s message for stream %s" ), GST_TO_WXSTRING( Message ).c_str(), faderplaybin->m_Uri.c_str() );

		Struct = gst_structure_new( Message, NULL, NULL );
		Msg = gst_message_new_application( GST_OBJECT( m_Playbin ), Struct );
		gst_element_post_message( GST_ELEMENT( m_Playbin ), Msg );
	}

    return true;
}

// -------------------------------------------------------------------------------- //
bool guFaderPlayBin::SetEqualizer( const wxArrayInt &eqbands )
{
    if( m_Equalizer && ( eqbands.Count() == guEQUALIZER_BAND_COUNT ) )
    {
        int index;
        for( index = 0; index < guEQUALIZER_BAND_COUNT; index++ )
        {
            SetEqualizerBand( index, eqbands[ index ] );
        }
        return true;
    }
    return false;
}

// -------------------------------------------------------------------------------- //
void guFaderPlayBin::SetEqualizerBand( const int band, const int value )
{
    g_object_set( G_OBJECT( m_Equalizer ), wxString::Format( wxT( "band%u" ),
            band ).char_str(), gdouble( value / 10.0 ), NULL );
}

// -------------------------------------------------------------------------------- //
bool guFaderPlayBin::Load( const wxString &uri, const bool restart, const int startpos )
{
    guLogDebug( wxT( "guFaderPlayBin::Load (%li) %i" ), m_Id, restart );

    if( restart )
    {
        if( gst_element_set_state( m_Playbin, GST_STATE_READY ) == GST_STATE_CHANGE_FAILURE )
        {
            guLogDebug( wxT( "guFaderPlayBin::Load => Could not set state to ready..." ) );
            return false;
        }
        gst_element_set_state( m_Playbin, GST_STATE_NULL );
    }

    if( !gst_uri_is_valid( ( const char * ) uri.mb_str( wxConvFile ) ) )
    {
        guLogDebug( wxT( "guFaderPlayBin::Load => Invalid uri: '%s'" ), uri.c_str() );
        return false;
    }

    g_object_set( G_OBJECT( m_Playbin ), "uri", ( const char * ) uri.mb_str( wxConvFile ), NULL );

    if( restart )
    {
        if( gst_element_set_state( m_Playbin, GST_STATE_PAUSED ) == GST_STATE_CHANGE_FAILURE )
        {
            guLogDebug( wxT( "guFaderPlayBin::Load => Could not restore state to paused..." ) );
            return false;
        }
    }

    if( startpos )
    {
        m_StartOffset = startpos;
        m_SeekTimerId = g_timeout_add( 100, GSourceFunc( seek_timeout ), this );
    }

    guMediaEvent event( guEVT_MEDIA_LOADED );
    event.SetInt( restart );
    SendEvent( event );
    guLogDebug( wxT( "guFaderPlayBin::Load => Sent the loaded event..." ) );

    return true;
}

// -------------------------------------------------------------------------------- //
bool guFaderPlayBin::Play( void )
{
    guLogDebug( wxT( "guFaderPlayBin::Play (%li)" ), m_Id );
    return ( gst_element_set_state( m_Playbin, GST_STATE_PLAYING ) != GST_STATE_CHANGE_FAILURE );
}

// -------------------------------------------------------------------------------- //
bool guFaderPlayBin::StartPlay( void )
{
    guLogDebug( wxT( "guFaderPlayBin::StartPlay (%li)" ), m_Id );
    bool                Ret = true;
    bool                NeedReap = false;
    bool                Playing = false;
    guFaderPlayBinArray ToFade;

#ifdef guSHOW_DUMPFADERPLAYBINS
    m_Player->Lock();
    DumpFaderPlayBins( m_Player->m_FaderPlayBins, m_Player->m_CurrentPlayBin );
    m_Player->Unlock();
#endif


    switch( m_PlayType )
    {
        case guFADERPLAYBIN_PLAYTYPE_CROSSFADE :
        {
            guLogDebug( wxT( "About to start the faderplaybin in crossfade type" ) );
            m_Player->Lock();
            int Index;
            int Count = m_Player->m_FaderPlayBins.Count();
            for( Index = 0; Index < Count; Index++ )
            {
                guFaderPlayBin * FaderPlaybin = m_Player->m_FaderPlayBins[ Index ];

                if( FaderPlaybin == this )
                    continue;

                switch( FaderPlaybin->m_State )
                {
                    case guFADERPLAYBIN_STATE_FADEIN :
                    case guFADERPLAYBIN_STATE_PLAYING :
                        ToFade.Add( FaderPlaybin );
                        break;

                    case guFADERPLAYBIN_STATE_ERROR :
                    case guFADERPLAYBIN_STATE_PAUSED :
                    case guFADERPLAYBIN_STATE_STOPPED :
                    case guFADERPLAYBIN_STATE_WAITING :
                    case guFADERPLAYBIN_STATE_WAITING_EOS :
                        FaderPlaybin->m_State = guFADERPLAYBIN_STATE_PENDING_REMOVE;

                    case guFADERPLAYBIN_STATE_PENDING_REMOVE :
                        NeedReap = true;

                    default :
                        break;
                }

            }
            m_Player->Unlock();

            Count = ToFade.Count();
            for( Index = 0; Index < Count; Index++ )
            {
                double FadeOutStart = 1.0;
                int    FadeOutTime = m_Player->m_ForceGapless ? 0 : m_Player->m_FadeOutTime;

                guFaderPlayBin * FaderPlaybin = ToFade[ Index ];

                switch( FaderPlaybin->m_State )
                {
                    case guFADERPLAYBIN_STATE_FADEIN :
                        //g_object_get( FaderPlaybin->m_Playbin, "volume", &FadeOutStart, NULL );
                        FadeOutStart = FaderPlaybin->GetFaderVolume();
                        FadeOutTime = ( int ) ( ( ( double ) FadeOutTime ) * FadeOutStart );

                    case guFADERPLAYBIN_STATE_PLAYING :
                        FaderPlaybin->StartFade( FadeOutStart, 0.0, FadeOutTime );
                        FaderPlaybin->m_State = guFADERPLAYBIN_STATE_FADEOUT;
                        m_IsFading = true;
                        break;

                    default :
                        break;
                }
            }

            if( !m_IsFading )
            {
                guLogDebug( wxT( "There was not previous playing track in crossfade mode so play this playbin..." ) );
                SetFaderVolume( 1.0 ); //g_object_set( m_Playbin, "volume", 1.0, NULL );
                if( Play() )
                {
                    m_State = guFADERPLAYBIN_STATE_PLAYING;
                    m_Player->Lock();
                    m_Player->m_CurrentPlayBin = this;
                    m_Player->Unlock();

                    guMediaEvent event( guEVT_MEDIA_CHANGED_STATE );
                    event.SetInt( GST_STATE_PLAYING );
                    SendEvent( event );
                }
            }

            break;
        }

        case guFADERPLAYBIN_PLAYTYPE_AFTER_EOS :
        {
            Playing = false;
            int Index;
            m_Player->Lock();
            int Count = m_Player->m_FaderPlayBins.Count();
            for( Index = 0; Index < Count; Index++ )
            {
                guFaderPlayBin * FaderPlaybin = m_Player->m_FaderPlayBins[ Index ];

                if( FaderPlaybin == this )
                    continue;

                switch( FaderPlaybin->m_State )
                {
                    case guFADERPLAYBIN_STATE_PLAYING :
                    case guFADERPLAYBIN_STATE_FADEIN :
                    case guFADERPLAYBIN_STATE_FADEOUT_PAUSE :
                        //guLogDebug( wxT( "Stream %s already playing" ), FaderPlaybin->m_Uri.c_str() );
                        Playing = true;
                        break;

                    case guFADERPLAYBIN_STATE_PAUSED :
                        //guLogDebug( wxT( "stream %s is paused; replacing it" ), FaderPlaybin->m_Uri.c_str() );
                        FaderPlaybin->m_State = guFADERPLAYBIN_STATE_PENDING_REMOVE;

                    case guFADERPLAYBIN_STATE_PENDING_REMOVE :
                        NeedReap = true;
                        break;

                    default:
                        break;
                }
            }

            m_Player->Unlock();

            if( Playing )
            {
                m_State = guFADERPLAYBIN_STATE_WAITING_EOS;
            }
            else
            {
                if( Play() )
                {
                    m_State = guFADERPLAYBIN_STATE_PLAYING;
                    m_Player->Lock();
                    m_Player->m_CurrentPlayBin = this;
                    m_Player->Unlock();

                    guMediaEvent event( guEVT_MEDIA_CHANGED_STATE );
                    event.SetInt( GST_STATE_PLAYING );
                    SendEvent( event );
                }
            }

            break;
        }

        case guFADERPLAYBIN_PLAYTYPE_REPLACE :
        {
            int Index;
            m_Player->Lock();
            int Count = m_Player->m_FaderPlayBins.Count();
            for( Index = 0; Index < Count; Index++ )
            {
                guFaderPlayBin * FaderPlaybin = m_Player->m_FaderPlayBins[ Index ];

                if( FaderPlaybin == this )
                    continue;

                switch( FaderPlaybin->m_State )
                {
                    case guFADERPLAYBIN_STATE_PLAYING :
                    case guFADERPLAYBIN_STATE_PAUSED :
                    case guFADERPLAYBIN_STATE_FADEIN :
                    case guFADERPLAYBIN_STATE_PENDING_REMOVE :
                        // kill this one
                        //guLogDebug( wxT( "stopping stream %s (replaced by new stream)" ), FaderPlaybin->m_Uri.c_str() );
                        FaderPlaybin->m_State = guFADERPLAYBIN_STATE_PENDING_REMOVE;
                        NeedReap = true;
                        break;

                    default:
                        break;
                }
            }

            m_Player->Unlock();

            if( Play() )
            {
                m_State = guFADERPLAYBIN_STATE_PLAYING;
                m_Player->Lock();
                m_Player->m_CurrentPlayBin = this;
                m_Player->Unlock();

                guMediaEvent event( guEVT_MEDIA_CHANGED_STATE );
                event.SetInt( GST_STATE_PLAYING );
                SendEvent( event );
            }

            break;
        }
    }

    if( NeedReap )
    {
        m_Player->ScheduleCleanUp();
    }

    return Ret;
}

// -------------------------------------------------------------------------------- //
void guFaderPlayBin::AboutToFinish( void )
{
    guLogDebug( wxT( "guFaderPlayBin::AboutToFinish (%li)" ), m_Id );
    Load( m_NextUri, false );
    m_NextUri = wxEmptyString;
    m_AboutToFinishPending = true;
}

// -------------------------------------------------------------------------------- //
static bool reset_about_to_finish( guFaderPlayBin * faderplaybin )
{
    faderplaybin->ResetAboutToFinishPending();
    return false;
}


// -------------------------------------------------------------------------------- //
void guFaderPlayBin::AudioChanged( void )
{
    guLogDebug( wxT( "guFaderPlayBin::AudioChanged (%li)" ), m_Id );
    if( m_AboutToFinishPending )
    {
        if( m_NextId )
        {
            m_Id = m_NextId;
            m_NextId = 0;
        }

        guMediaEvent event( guEVT_MEDIA_CHANGED_STATE );
        event.SetInt( GST_STATE_PLAYING );
        SendEvent( event );
        //m_AboutToFinishPending = false;
        m_AboutToFinishPendingId = g_timeout_add( 3000, GSourceFunc( reset_about_to_finish ), this );
    }
}

// -------------------------------------------------------------------------------- //
bool guFaderPlayBin::StartFade( double volstart, double volend, int timeout )
{
    guLogDebug( wxT( "guFaderPlayBin::StartFade (%li) %0.2f, %0.2f, %i" ), m_Id, volstart, volend, timeout );
//    SetFaderVolume( volstart );

    m_EmittedStartFadeIn = false;
    m_IsFading = true;
    if( m_FaderTimeLine )
    {
//        guLogDebug( wxT( "Reversed the fader for %li" ), m_Id );
//        m_FaderTimeLine->ToggleDirection();
        EndFade();
    }

    m_FaderTimeLine = new guFaderTimeLine( timeout, NULL, this, volstart, volend );
    m_FaderTimeLine->SetDirection( volstart > volend ? guFaderTimeLine::Backward : guFaderTimeLine::Forward );
    m_FaderTimeLine->SetCurveShape( guTimeLine::LinearCurve );
    m_FaderTimeLine->Start();

    return true;
}

// -------------------------------------------------------------------------------- //
bool guFaderPlayBin::Pause( void )
{
    guLogDebug( wxT( "guFaderPlayBin::Pause (%li)" ), m_Id );
//    return ( gst_element_set_state( m_Playbin, GST_STATE_PAUSED ) != GST_STATE_CHANGE_FAILURE );
    g_timeout_add( 200, GSourceFunc( pause_timeout ), m_Playbin );
    return true;
}

// -------------------------------------------------------------------------------- //
bool guFaderPlayBin::Stop( void )
{
    guLogDebug( wxT( "guFaderPlayBin::Stop (%li)" ), m_Id );
    return ( gst_element_set_state( m_Playbin, GST_STATE_READY ) != GST_STATE_CHANGE_FAILURE );
}

// -------------------------------------------------------------------------------- //
bool guFaderPlayBin::Seek( wxFileOffset where, bool accurate )
{
    guLogDebug( wxT( "guFaderPlayBin::Seek (%li) %li )" ), m_Id, where );

    GstSeekFlags SeekFlags = GstSeekFlags( GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT );
    if( accurate )
        SeekFlags = GstSeekFlags( SeekFlags | GST_SEEK_FLAG_ACCURATE );

    return gst_element_seek_simple( m_Playbin, GST_FORMAT_TIME, SeekFlags, where * GST_MSECOND );
}

// -------------------------------------------------------------------------------- //
wxFileOffset guFaderPlayBin::Position( void )
{
    wxFileOffset Position;
    gst_element_query_position( m_OutputSink, GST_FORMAT_TIME, ( gint64 * ) &Position );
    return Position;
}

// -------------------------------------------------------------------------------- //
wxFileOffset guFaderPlayBin::Length( void )
{
    wxFileOffset Length;
    gst_element_query_duration( m_OutputSink, GST_FORMAT_TIME, ( gint64 * ) &Length );
    return Length;
}

// -------------------------------------------------------------------------------- //
bool guFaderPlayBin::EnableRecord( const wxString &recfile, const int format, const int quality )
{
    guLogDebug( wxT( "guFaderPlayBin::EnableRecord  %i %i '%s'" ), format, quality, recfile.c_str() );
    GstElement * Encoder = NULL;
    GstElement * Muxer = NULL;
    gint Mp3Quality[] = { 320, 192, 128, 96, 64 };
    gfloat OggQuality[] = { 0.9f, 0.7f, 0.5f, 0.3f, 0.1f };
    gint FlacQuality[] = { 8, 7, 5, 3, 1 };

    //
    switch( format )
    {
        case guRECORD_FORMAT_MP3  :
        {
            Encoder = gst_element_factory_make( "lamemp3enc", "rb_lame" );
            if( IsValidElement( Encoder ) )
            {
                g_object_set( Encoder, "bitrate", Mp3Quality[ quality ], NULL );

                Muxer = gst_element_factory_make( "xingmux", "rb_xingmux" );
                if( !IsValidElement( Muxer ) )
                {
                    guLogError( wxT( "Could not create the record xingmux object" ) );
                    Muxer = NULL;
                }
            }
            else
            {
                Encoder = NULL;
            }
            break;
        }

        case guRECORD_FORMAT_OGG  :
        {
            Encoder = gst_element_factory_make( "vorbisenc", "rb_vorbis" );
            if( IsValidElement( Encoder ) )
            {
                g_object_set( Encoder, "quality", OggQuality[ quality ], NULL );

                Muxer = gst_element_factory_make( "oggmux", "rb_oggmux" );
                if( !IsValidElement( Muxer ) )
                {
                    guLogError( wxT( "Could not create the record oggmux object" ) );
                    Muxer = NULL;
                }
            }
            else
            {
                Encoder = NULL;
            }
            break;
        }

        case guRECORD_FORMAT_FLAC :
        {
            Encoder = gst_element_factory_make( "flacenc", "rb_flac" );
            if( IsValidElement( Encoder ) )
            {
                g_object_set( Encoder, "quality", FlacQuality[ quality ], NULL );
            }
            else
            {
                Encoder = NULL;
            }
            break;
        }
    }

    if( Encoder )
    {
        if( BuildRecordBin( recfile, Encoder, Muxer ) )
        {
            GstPad * AddPad;
            GstPad * BlockPad;

            AddPad = gst_element_get_static_pad( m_Tee, "sink" );
            BlockPad = gst_pad_get_peer( AddPad );
            gst_object_unref( AddPad );

            //gst_pad_set_blocked_async( BlockPad, true, GstPadBlockCallback( add_record_element ), this );
			gst_pad_add_probe( BlockPad, GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM,
				GstPadProbeCallback( add_record_element ), this, NULL );

            return true;
        }
        else
        {
            guLogMessage( wxT( "Could not build the recordbin object." ) );
        }
    }
    else
    {
        guLogError( wxT( "Could not create the recorder encoder object" ) );
    }

    return false;
}

// -------------------------------------------------------------------------------- //
void guFaderPlayBin::DisableRecord( void )
{
    guLogDebug( wxT( "guFaderPlayBin::DisableRecord" ) );
    bool NeedBlock = true;

    GstPad * AddPad;
    GstPad * BlockPad;

    AddPad = gst_element_get_static_pad( m_Tee, "sink" );
    BlockPad = gst_pad_get_peer( AddPad );
    gst_object_unref( AddPad );

    if( NeedBlock )
    {
        //gst_pad_set_blocked_async( BlockPad, true, GstPadBlockCallback( remove_record_element ), this );
		gst_pad_add_probe( BlockPad, GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM,
			GstPadProbeCallback( remove_record_element ), this, NULL );
    }
    else
    {
        //remove_record_element( BlockPad, false, this );
    }
}

// -------------------------------------------------------------------------------- //
bool guFaderPlayBin::SetRecordFileName( const wxString &filename )
{
    if( filename == m_LastRecordFileName )
        return true;

    if( !m_RecordBin || m_SettingRecordFileName )
        return false;

    if( m_IsBuffering )
    {
        m_PendingNewRecordName = filename;
        return true;
    }

    guLogDebug( wxT( "guFaderPlayBin::SetRecordFileName %i" ), m_IsBuffering );
    m_SettingRecordFileName = true;
    m_LastRecordFileName = filename;
    m_PendingNewRecordName.Clear();

    // Once the pad is locked returns into SetRecordFileName()
    gst_pad_add_probe( m_TeeSrcPad, GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM,
        GstPadProbeCallback( record_locked ), this, NULL );

    return true;
}

// -------------------------------------------------------------------------------- //
bool guFaderPlayBin::SetRecordFileName( void )
{
//    m_RecordFileName = m_RecordPath + filename + m_RecordExt;
    if( !wxDirExists( wxPathOnly( m_LastRecordFileName ) ) )
        wxFileName::Mkdir( wxPathOnly( m_LastRecordFileName ), 0770, wxPATH_MKDIR_FULL );

    if( gst_element_set_state( m_RecordBin, GST_STATE_NULL ) == GST_STATE_CHANGE_FAILURE )
    {
        guLogMessage( wxT( "Could not reset recordbin state changing location" ) );
    }

    g_object_set( m_FileSink, "location", ( const char * ) m_LastRecordFileName.mb_str( wxConvFile ), NULL );

    if( gst_element_set_state( m_RecordBin, GST_STATE_READY ) == GST_STATE_CHANGE_FAILURE )
    {
        guLogMessage( wxT( "Could not restore recordbin state changing location" ) );
    }

    if( gst_element_set_state( m_RecordBin, GST_STATE_PLAYING ) == GST_STATE_CHANGE_FAILURE )
    {
        guLogMessage( wxT( "Could not set running recordbin changing location" ) );
    }

    m_SettingRecordFileName = false;

    return true;
}

// -------------------------------------------------------------------------------- //
void guFaderPlayBin::AddRecordElement( GstPad * pad )
{
    gst_element_set_state( m_RecordBin, GST_STATE_PAUSED );

    gst_bin_add( GST_BIN( m_Playbackbin ), m_RecordBin );
    m_TeeSrcPad = gst_element_get_request_pad( m_Tee, "src_%u" );

	gst_pad_link( m_TeeSrcPad, m_RecordSinkPad );

    gst_element_set_state( m_RecordBin, GST_STATE_PLAYING );
    gst_object_ref( m_RecordSinkPad );
}

// -------------------------------------------------------------------------------- //
void guFaderPlayBin::RemoveRecordElement( GstPad * pad )
{
    g_object_ref( m_RecordBin );
    gst_element_set_state( m_RecordBin, GST_STATE_PAUSED );

    gst_bin_remove( GST_BIN( m_Playbackbin ), m_RecordBin );

    gst_element_set_state( m_RecordBin, GST_STATE_NULL );
    g_object_unref( m_RecordBin );

    SetRecordBin( NULL );
}





// -------------------------------------------------------------------------------- //
// guFaderTimeLine
// -------------------------------------------------------------------------------- //
guFaderTimeLine::guFaderTimeLine( const int timeout, wxEvtHandler * parent, guFaderPlayBin * faderplaybin,
    double volstart, double volend ) :
    guTimeLine( timeout, parent )
{
    m_FaderPlayBin = faderplaybin;
    m_VolStart = volstart;
    m_VolEnd = volend;

    if( volstart > volend )
        m_VolStep = volstart - volend;
    else
        m_VolStep = volend - volstart;

    //guLogDebug( wxT( "Created the fader timeline for %i msecs %0.2f -> %0.2f (%0.2f)" ), timeout, volstart, volend, m_VolStep );
}

// -------------------------------------------------------------------------------- //
guFaderTimeLine::~guFaderTimeLine()
{
    //guLogDebug( wxT( "Destroyed the fader timeline" ) );
}

// -------------------------------------------------------------------------------- //
void guFaderTimeLine::ValueChanged( float value )
{
    if( m_Duration )
    {
        if( m_Direction == guTimeLine::Backward )
        {
            m_FaderPlayBin->SetFaderVolume( m_VolEnd + ( value * m_VolStep ) );
        }
        else
        {
            m_FaderPlayBin->SetFaderVolume( m_VolStart + ( value * m_VolStep ) );
        }
    }
}

// -------------------------------------------------------------------------------- //
void guFaderTimeLine::Finished( void )
{
    m_FaderPlayBin->SetFaderVolume( m_VolEnd );
    m_FaderPlayBin->EndFade();
}

// -------------------------------------------------------------------------------- //
static bool TimerUpdated( guFaderTimeLine * timeline )
{
    timeline->TimerEvent();
    return true;
}

// -------------------------------------------------------------------------------- //
int guFaderTimeLine::TimerCreate( void )
{
    return g_timeout_add( m_UpdateInterval, GSourceFunc( TimerUpdated ), this );
}




// -------------------------------------------------------------------------------- //
// guMediaRecordCtrl
// -------------------------------------------------------------------------------- //
guMediaRecordCtrl::guMediaRecordCtrl( guPlayerPanel * playerpanel, guMediaCtrl * mediactrl )
{
    m_PlayerPanel = playerpanel;
    m_MediaCtrl = mediactrl;
    m_Recording = false;
    m_FirstChange = false;

    UpdatedConfig();
}

// -------------------------------------------------------------------------------- //
guMediaRecordCtrl::~guMediaRecordCtrl()
{
}

// -------------------------------------------------------------------------------- //
void guMediaRecordCtrl::UpdatedConfig( void )
{
    guConfig * Config = ( guConfig * ) guConfig::Get();
    m_MainPath = Config->ReadStr( wxT( "Path" ), guPATH_DEFAULT_RECORDINGS, wxT( "record" ) );
    m_Format = Config->ReadNum( wxT( "Format" ), guRECORD_FORMAT_MP3, wxT( "record" ) );
    m_Quality = Config->ReadNum( wxT( "Quality" ), guRECORD_QUALITY_NORMAL, wxT( "record" ) );
    m_SplitTracks = Config->ReadBool( wxT( "Split" ), false, wxT( "record" ) );
    m_DeleteTracks = Config->ReadBool( wxT( "DeleteTracks" ), false, wxT( "record" ) );
    m_DeleteTime = Config->ReadNum( wxT( "DeleteTime" ), 55, wxT( "record" ) ) * 1000;

    if( !m_MainPath.EndsWith( wxT( "/" ) ) )
        m_MainPath += wxT( "/" );

    switch( m_Format )
    {
        case guRECORD_FORMAT_MP3 :
            m_Ext = wxT( ".mp3" );
            break;
        case guRECORD_FORMAT_OGG :
            m_Ext = wxT( ".ogg" );
            break;
        case guRECORD_FORMAT_FLAC :
            m_Ext = wxT( ".flac" );
            break;
    }

    //guLogDebug( wxT( "Record to '%s' %i, %i '%s'" ), m_MainPath.c_str(), m_Format, m_Quality, m_Ext.c_str() );
}

// -------------------------------------------------------------------------------- //
bool guMediaRecordCtrl::Start( const guTrack * track )
{
    guLogDebug( wxT( "guMediaRecordCtrl::Start" ) );
    m_TrackInfo = * track;

    if( m_TrackInfo.m_SongName.IsEmpty() )
        m_TrackInfo.m_SongName = wxT( "Record" );

    m_FileName = GenerateRecordFileName();

    wxFileName::Mkdir( wxPathOnly( m_FileName ), 0770, wxPATH_MKDIR_FULL );

    m_Recording = m_MediaCtrl->EnableRecord( m_FileName, m_Format, m_Quality );

    return m_Recording;
}

// -------------------------------------------------------------------------------- //
bool guMediaRecordCtrl::Stop( void )
{
    guLogDebug( wxT( "guMediaRecordCtrl::Stop" ) );
    if( m_Recording )
    {
        m_MediaCtrl->DisableRecord();
        m_Recording = false;

        SaveTagInfo( m_PrevFileName, &m_PrevTrack );
        m_PrevFileName = wxEmptyString;
    }
    return true;
}

// -------------------------------------------------------------------------------- //
wxString guMediaRecordCtrl::GenerateRecordFileName( void )
{
    wxString FileName = m_MainPath;

    if( !m_TrackInfo.m_AlbumName.IsEmpty() )
    {
        FileName += NormalizeField( m_TrackInfo.m_AlbumName );
    }
    else
    {
        wxURI Uri( m_TrackInfo.m_FileName );
        FileName += NormalizeField( Uri.GetServer() + wxT( "-" ) + Uri.GetPath() );
    }

    FileName += wxT( "/" );
    if( !m_TrackInfo.m_ArtistName.IsEmpty() )
    {
        FileName += NormalizeField( m_TrackInfo.m_ArtistName ) + wxT( " - " );
    }
    FileName += NormalizeField( m_TrackInfo.m_SongName ) + m_Ext;

    //guLogDebug( wxT( "The New Record Location is : '%s'" ), FileName.c_str() );
    return FileName;
}

// -------------------------------------------------------------------------------- //
void guMediaRecordCtrl::SplitTrack( void )
{
    guLogMessage( wxT( "guMediaRecordCtrl::SplitTrack" ) );

    m_FileName = GenerateRecordFileName();
    m_MediaCtrl->SetRecordFileName( m_FileName );

    SaveTagInfo( m_PrevFileName, &m_PrevTrack );

    m_PrevFileName = m_FileName;
    m_PrevTrack = m_TrackInfo;
}

// -------------------------------------------------------------------------------- //
bool guMediaRecordCtrl::SaveTagInfo( const wxString &filename, const guTrack * Track )
{
    bool RetVal = true;
    guTagInfo * TagInfo;

    if( !filename.IsEmpty() && wxFileExists( filename ) )
    {
        TagInfo = guGetTagInfoHandler( filename );

        if( TagInfo )
        {
            if( m_DeleteTracks )
            {
                TagInfo->Read();
                if( TagInfo->m_Length < m_DeleteTime )
                {
                    wxRemoveFile( filename );
                    delete TagInfo;
                    return true;
                }
            }

            TagInfo->m_AlbumName = Track->m_AlbumName;
            TagInfo->m_ArtistName = Track->m_ArtistName;
            TagInfo->m_GenreName = Track->m_GenreName;
            TagInfo->m_TrackName = Track->m_SongName;

            if( !( RetVal = TagInfo->Write( guTRACK_CHANGED_DATA_TAGS ) ) )
            {
                guLogError( wxT( "Could not set tags to the record track" ) );
            }

            delete TagInfo;
        }
    }
    return RetVal;
}

// -------------------------------------------------------------------------------- //
void guMediaRecordCtrl::SetTrack( const guTrack &track )
{
    guLogMessage( wxT( "guMediaRecordCtrl::SetTrack" ) );
    m_TrackInfo = track;
    SplitTrack();
}

// -------------------------------------------------------------------------------- //
void guMediaRecordCtrl::SetTrackName( const wxString &artistname, const wxString &trackname )
{
    bool NeedSplit = false;

    // If its the first file Set it so the tags are saved
    if( m_PrevFileName.IsEmpty() )
    {
        m_PrevFileName = m_FileName;
        m_PrevTrack = m_TrackInfo;
    }
    if( m_TrackInfo.m_ArtistName != artistname )
    {
        m_TrackInfo.m_ArtistName = artistname;
        NeedSplit = true;
    }
    if( m_TrackInfo.m_SongName != trackname )
    {
        m_TrackInfo.m_SongName = trackname;
        NeedSplit = true;
    }
    if( NeedSplit )
    {
        SplitTrack();
    }
}

// -------------------------------------------------------------------------------- //
void guMediaRecordCtrl::SetStation( const wxString &station )
{
    guLogMessage( wxT( "guMediaRecordCtrl::SetStation" ) );
    if( m_TrackInfo.m_AlbumName != station )
    {
        m_TrackInfo.m_AlbumName = station;
        SplitTrack();
    }
}

// -------------------------------------------------------------------------------- //
