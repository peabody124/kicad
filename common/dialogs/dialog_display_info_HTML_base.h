///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __dialog_display_info_HTML_base__
#define __dialog_display_info_HTML_base__

#include <wx/intl.h>

#include <wx/html/htmlwin.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_DISPLAY_HTML_TEXT_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_DISPLAY_HTML_TEXT_BASE : public wxDialog 
{
	private:
	
	protected:
		wxButton* m_buttonClose;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnHTMLLinkClicked( wxHtmlLinkEvent& event ){ event.Skip(); }
		virtual void OnCloseButtonClick( wxCommandEvent& event ){ event.Skip(); }
		
	
	public:
		wxHtmlWindow* m_htmlWindow;
		DIALOG_DISPLAY_HTML_TEXT_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 431,120 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~DIALOG_DISPLAY_HTML_TEXT_BASE();
	
};

#endif //__dialog_display_info_HTML_base__
