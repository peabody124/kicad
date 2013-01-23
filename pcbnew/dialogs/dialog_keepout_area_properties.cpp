/**
 * @file dialog_keepout_area_properties.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
  * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <wx/wx.h>
#include <fctsys.h>
#include <appl_wxstruct.h>
#include <confirm.h>
#include <pcbnew.h>
#include <wxPcbStruct.h>
#include <zones.h>
#include <base_units.h>

#include <class_zone_settings.h>
#include <class_board.h>
#include <dialog_keepout_area_properties_base.h>

#include <wx/imaglist.h>    // needed for wx/listctrl.h, in wxGTK 2.8.12
#include <wx/listctrl.h>



/**
 * Class DIALOG_KEEPOUT_AREA_PROPERTIES
 * is the derived class from dialog_copper_zone_frame created by wxFormBuilder
 */
class DIALOG_KEEPOUT_AREA_PROPERTIES : public DIALOG_KEEPOUT_AREA_PROPERTIES_BASE
{
public:
    DIALOG_KEEPOUT_AREA_PROPERTIES( PCB_BASE_FRAME* aParent, ZONE_SETTINGS* aSettings );

private:
    PCB_BASE_FRAME* m_Parent;
    wxConfig*       m_Config;               ///< Current config
    ZONE_SETTINGS   m_zonesettings;
    ZONE_SETTINGS*  m_ptr;

    std::vector<int> m_LayerId;             ///< Handle the real layer number from layer
                                            ///< name position in m_LayerSelectionCtrl

    /**
     * Function initDialog
     * fills in the dialog controls using the current settings.
     */
    void initDialog();

    void OnOkClick( wxCommandEvent& event );
    void OnCancelClick( wxCommandEvent& event );
    void OnSize( wxSizeEvent& event );

    /**
     * Function AcceptOptionsForKeepOut
     * Test validity of options, and copy options in m_zonesettings, for keepout zones
     * @return bool - false if incorrect options, true if ok.
     */
    bool AcceptOptionsForKeepOut();

    /**
     * Function makeLayerBitmap
     * creates the colored rectangle bitmaps used in the layer selection widget.
     * @param aColor is the color to fill the rectangle with.
     */
    wxBitmap makeLayerBitmap( EDA_COLOR_T aColor );
};


#define LAYER_BITMAP_SIZE_X     20
#define LAYER_BITMAP_SIZE_Y     10

ZONE_EDIT_T InvokeKeepoutAreaEditor( PCB_BASE_FRAME* aCaller, ZONE_SETTINGS* aSettings )
{
    DIALOG_KEEPOUT_AREA_PROPERTIES dlg( aCaller, aSettings );

    ZONE_EDIT_T result = ZONE_EDIT_T( dlg.ShowModal() );

    return result;
}


DIALOG_KEEPOUT_AREA_PROPERTIES::DIALOG_KEEPOUT_AREA_PROPERTIES( PCB_BASE_FRAME* aParent, ZONE_SETTINGS* aSettings ) :
    DIALOG_KEEPOUT_AREA_PROPERTIES_BASE( aParent )
{
    m_Parent = aParent;
    m_Config = wxGetApp().GetSettings();

    m_ptr = aSettings;
    m_zonesettings = *aSettings;

    SetReturnCode( ZONE_ABORT );        // Will be changed on button OK ckick

    initDialog();

    GetSizer()->SetSizeHints( this );
}


void DIALOG_KEEPOUT_AREA_PROPERTIES::initDialog()
{
    BOARD* board = m_Parent->GetBoard();

    wxString msg;

    if( m_zonesettings.m_Zone_45_Only )
        m_OrientEdgesOpt->SetSelection( 1 );

    switch( m_zonesettings.m_Zone_HatchingStyle )
    {
    case CPolyLine::NO_HATCH:
        m_OutlineAppearanceCtrl->SetSelection( 0 );
        break;

    case CPolyLine::DIAGONAL_EDGE:
        m_OutlineAppearanceCtrl->SetSelection( 1 );
        break;

    case CPolyLine::DIAGONAL_FULL:
        m_OutlineAppearanceCtrl->SetSelection( 2 );
        break;
    }

    // Create one column in m_LayerSelectionCtrl
    wxListItem col0;
    col0.SetId( 0 );
    m_LayerSelectionCtrl->InsertColumn( 0, col0 );
    // Build copper layer list and append to layer widget
    int layerCount = board->GetCopperLayerCount();
    int layerNumber, itemIndex;
    EDA_COLOR_T layerColor;
    wxImageList* imageList = new wxImageList( LAYER_BITMAP_SIZE_X, LAYER_BITMAP_SIZE_Y );
    m_LayerSelectionCtrl->AssignImageList( imageList, wxIMAGE_LIST_SMALL );
    for( int ii = 0; ii < layerCount; ii++ )
    {
        layerNumber = LAYER_N_BACK;

        if( layerCount <= 1 || ii < layerCount - 1 )
            layerNumber = ii;
        else if( ii == layerCount - 1 )
            layerNumber = LAYER_N_FRONT;

        m_LayerId.insert( m_LayerId.begin(), layerNumber );

        msg = board->GetLayerName( layerNumber );
        layerColor = board->GetLayerColor( layerNumber );
        imageList->Add( makeLayerBitmap( layerColor ) );
        itemIndex = m_LayerSelectionCtrl->InsertItem( 0, msg, ii );

        if( m_zonesettings.m_CurrentZone_Layer == layerNumber )
            m_LayerSelectionCtrl->Select( itemIndex );
    }

    // Init keepout parameters:
    m_cbTracksCtrl->SetValue( m_zonesettings.GetDoNotAllowTracks() );
    m_cbViasCtrl->SetValue( m_zonesettings.GetDoNotAllowVias() );
    m_cbCopperPourCtrl->SetValue( m_zonesettings.GetDoNotAllowCopperPour() );
}

void DIALOG_KEEPOUT_AREA_PROPERTIES::OnCancelClick( wxCommandEvent& event )
{
    EndModal( ZONE_ABORT );
}

void DIALOG_KEEPOUT_AREA_PROPERTIES::OnOkClick( wxCommandEvent& event )
{
    if( AcceptOptionsForKeepOut() )
    {
        *m_ptr = m_zonesettings;
        EndModal( ZONE_OK );
    }
}


void DIALOG_KEEPOUT_AREA_PROPERTIES::OnSize( wxSizeEvent& event )
{
    Layout();

    // Set layer list column width to widget width minus a few pixels
    m_LayerSelectionCtrl->SetColumnWidth( 0, m_LayerSelectionCtrl->GetSize().x - 5 );
    event.Skip();
}

bool DIALOG_KEEPOUT_AREA_PROPERTIES::AcceptOptionsForKeepOut()
{
    // Init keepout parameters:
    m_zonesettings.SetIsKeepout( true );
    m_zonesettings.SetDoNotAllowTracks( m_cbTracksCtrl->GetValue() );
    m_zonesettings.SetDoNotAllowVias( m_cbViasCtrl->GetValue() );
    m_zonesettings.SetDoNotAllowCopperPour( m_cbCopperPourCtrl->GetValue() );

    // Test for not allowed items: should have at least one item not allowed:
    if( ! m_zonesettings.GetDoNotAllowTracks() &&
        ! m_zonesettings.GetDoNotAllowVias() &&
        ! m_zonesettings.GetDoNotAllowCopperPour() )
        {
            DisplayError( NULL,
                          _("Tracks, vias and pads are allowed. The keepout is useless" ) );
            return false;
        }

    // Get the layer selection for this zone
    int ii = m_LayerSelectionCtrl->GetFirstSelected();

    if( ii < 0 )
    {
        DisplayError( NULL, _( "No layer selected." ) );
        return false;
    }

    m_zonesettings.m_CurrentZone_Layer = m_LayerId[ii];
    switch( m_OutlineAppearanceCtrl->GetSelection() )
    {
    case 0:
        m_zonesettings.m_Zone_HatchingStyle = CPolyLine::NO_HATCH;
        break;

    case 1:
        m_zonesettings.m_Zone_HatchingStyle = CPolyLine::DIAGONAL_EDGE;
        break;

    case 2:
        m_zonesettings.m_Zone_HatchingStyle = CPolyLine::DIAGONAL_FULL;
        break;
    }

    if( m_Config )
    {
        m_Config->Write( ZONE_NET_OUTLINES_HATCH_OPTION_KEY,
                         (long) m_zonesettings.m_Zone_HatchingStyle );
    }

    if( m_OrientEdgesOpt->GetSelection() == 0 )
        m_zonesettings.m_Zone_45_Only = false;
    else
        m_zonesettings.m_Zone_45_Only = true;

    m_zonesettings.m_ZonePriority = 0; //m_PriorityLevelCtrl->GetValue();

    return true;
}

wxBitmap DIALOG_KEEPOUT_AREA_PROPERTIES::makeLayerBitmap( EDA_COLOR_T aColor )
{
    wxBitmap    bitmap( LAYER_BITMAP_SIZE_X, LAYER_BITMAP_SIZE_Y );
    wxBrush     brush;
    wxMemoryDC  iconDC;

    iconDC.SelectObject( bitmap );
    brush.SetColour( MakeColour( aColor ) );
    brush.SetStyle( wxSOLID );
    iconDC.SetBrush( brush );
    iconDC.DrawRectangle( 0, 0, LAYER_BITMAP_SIZE_X, LAYER_BITMAP_SIZE_Y );

    return bitmap;
}