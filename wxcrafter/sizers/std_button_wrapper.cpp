#include "std_button_wrapper.h"

#include "StdToWX.h"
#include "allocator_mgr.h"
#include "bool_property.h"
#include "choice_property.h"
#include "multi_strings_property.h"
#include "wxgui_defs.h"
#include "xmlutils.h"

StdButtonWrapper::StdButtonWrapper()
    : wxcWidget(ID_WXSTDBUTTON)
{
    m_properties.Clear();
    const wxArrayString ids = StdToWX::ToArrayString({ "wxID_OK", "wxID_YES", "wxID_SAVE", "wxID_APPLY", "wxID_CLOSE",
                                                       "wxID_NO", "wxID_CANCEL", "wxID_HELP", "wxID_CONTEXT_HELP" });

    RegisterEvent(wxT("wxEVT_COMMAND_BUTTON_CLICKED"), wxT("wxCommandEvent"),
                  _("Process a wxEVT_COMMAND_BUTTON_CLICKED event, when the button is clicked."),
                  wxT("wxCommandEventHandler"));

    Add<CategoryProperty>(_("Standard wxButton"));
    Add<ChoiceProperty>(PROP_WINDOW_ID, ids, 0, _("Button ID"));
    Add<StringProperty>(PROP_NAME, wxT(""), _("C++ member name"));
    Add<MultiStringsProperty>(PROP_TOOLTIP, _("Tooltip"), wxT("\\n"), _("Tooltip text:"));
    Add<BoolProperty>(PROP_DEFAULT_BUTTON, false, wxT("Make this button the default button"));
    m_namePattern = "m_button";
    SetName(GenerateName());
}

wxcWidget* StdButtonWrapper::Clone() const { return new StdButtonWrapper(); }

wxString StdButtonWrapper::CppCtorCode() const
{
    wxString code;
    code << CPPStandardWxCtorWithLabel("0");
    if(PropertyString(PROP_DEFAULT_BUTTON) == wxT("1")) { code << GetName() << wxT("->SetDefault();\n"); }
    code << CPPCommonAttributes();
    return code;
}

void StdButtonWrapper::GetIncludeFile(wxArrayString& headers) const { headers.Add(wxT("#include <wx/button.h>")); }

wxString StdButtonWrapper::GetWxClassName() const { return "wxButton"; }

void StdButtonWrapper::ToXRC(wxString& text, XRC_TYPE type) const
{
    text << "<object class=\"button\">" << wxT("<object class=\"") << GetWxClassName() << wxT("\" name=\"") << GetId()
         << wxT("\">") << XRCLabel() << XRCStyle() << XRCSize() << XRCCommonAttributes() << wxT("<default>")
         << PropertyString(PROP_DEFAULT_BUTTON) << wxT("</default>") << XRCSuffix() // wxButton
         << XRCSuffix();                                                            // button
}

void StdButtonWrapper::LoadPropertiesFromXRC(const wxXmlNode* node)
{
    // First call the base-class for the standard things
    wxcWidget::LoadPropertiesFromXRC(node);

    wxXmlNode* propertynode = XmlUtils::FindFirstByTagName(node, wxT("default"));
    if(propertynode) { SetPropertyString(PROP_DEFAULT_BUTTON, propertynode->GetNodeContent()); }
}

// wxFB doesn't do anything here
