#include "wxc_widget.h"

#include "allocator_mgr.h"
#include "aui_notebook_wrapper.h"
#include "bool_property.h"
#include "category_property.h"
#include "color_property.h"
#include "custom_control_wrapper.h"
#include "file_ficker_property.h"
#include "file_logger.h"
#include "font_property.h"
#include "import_from_wxFB.h"
#include "multi_strings_property.h"
#include "notebook_page_wrapper.h"
#include "StdToWX.h"
#include "winid_property.h"
#include "wx_collapsible_pane_pane_wrapper.h"
#include "wxc_bitmap_code_generator.h"
#include "wxc_settings.h"
#include "wxgui_defs.h"
#include "wxgui_helpers.h"
#include "xmlutils.h"

#include <wx/app.h>
#include <wx/regex.h>

size_t wxcWidget::s_objCounter = 0;
wxcWidget::SizerFlagsValueSet_t wxcWidget::s_sizerFlagsValue;
CustomControlTemplateMap_t wxcWidget::ms_customControlsUsed;
int wxcWidget::m_copyCounter = 0;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

wxcWidget::wxcWidget(int type)
    : m_parent(NULL)
    , m_type(type)
    , m_eventsMenu(NULL)
    , m_copyReason(CR_Copy)
{
    if(s_sizerFlagsValue.empty()) {
        s_sizerFlagsValue.insert(std::make_pair(wxALL, SZ_ALL));
        s_sizerFlagsValue.insert(std::make_pair(wxLEFT, SZ_LEFT));
        s_sizerFlagsValue.insert(std::make_pair(wxRIGHT, SZ_RIGHT));
        s_sizerFlagsValue.insert(std::make_pair(wxTOP, SZ_TOP));
        s_sizerFlagsValue.insert(std::make_pair(wxBOTTOM, SZ_BOTTOM));
        s_sizerFlagsValue.insert(std::make_pair(wxEXPAND, SZ_EXPAND));
        s_sizerFlagsValue.insert(std::make_pair(wxALIGN_CENTER, SZ_ALIGN_CENTER));
        s_sizerFlagsValue.insert(std::make_pair(wxALIGN_LEFT, SZ_ALIGN_LEFT));
        s_sizerFlagsValue.insert(std::make_pair(wxALIGN_CENTER_HORIZONTAL, SZ_ALIGN_CENTER_HORIZONTAL));
        s_sizerFlagsValue.insert(std::make_pair(wxALIGN_RIGHT, SZ_ALIGN_RIGHT));
        s_sizerFlagsValue.insert(
            std::make_pair(crafterALIGN_TOP, SZ_ALIGN_TOP)); // See the explanation inside enum SIZER_FLAG_VALUE
        s_sizerFlagsValue.insert(std::make_pair(wxALIGN_CENTER_VERTICAL, SZ_ALIGN_CENTER_VERTICAL));
        s_sizerFlagsValue.insert(std::make_pair(wxALIGN_BOTTOM, SZ_ALIGN_BOTTOM));
        s_sizerFlagsValue.insert(std::make_pair(wxRESERVE_SPACE_EVEN_IF_HIDDEN, SZ_RESERVE_SPACE_EVEN_IF_HIDDEN));
    }

    m_sizerItem.SetBorder(DEFAULT_SIZER_BORDER);
    m_sizerItem.SetProportion(DEFAULT_SIZER_PROPORTION);

    m_gbSpan = "1,1";
    m_gbPos = "0,0";

    // This will act as the separator between the basic styles and the control specific ones
    ADD_STYLE(0, false);
    ADD_STYLE(wxFULL_REPAINT_ON_RESIZE, false);
    ADD_STYLE(wxWANTS_CHARS, false);
    ADD_STYLE(wxTAB_TRAVERSAL, false);
    ADD_STYLE(wxBORDER_NONE, false);
    ADD_STYLE(wxCLIP_CHILDREN, false);
    ADD_STYLE(wxALWAYS_SHOW_SB, false);
    ADD_STYLE(wxBORDER_STATIC, false);
    ADD_STYLE(wxBORDER_SIMPLE, false);
    ADD_STYLE(wxBORDER_SUNKEN, false);
    ADD_STYLE(wxBORDER_THEME, false);
    ADD_STYLE(wxBORDER_RAISED, false);
    ADD_STYLE(wxHSCROLL, false);
    ADD_STYLE(wxVSCROLL, false);

    ADD_SIZER_FLAG(wxALL, true);
    ADD_SIZER_FLAG(wxLEFT, true);
    ADD_SIZER_FLAG(wxRIGHT, true);
    ADD_SIZER_FLAG(wxTOP, true);
    ADD_SIZER_FLAG(wxBOTTOM, true);
    ADD_SIZER_FLAG(wxEXPAND, false);
    ADD_SIZER_FLAG(wxALIGN_CENTER, false);
    ADD_SIZER_FLAG(wxALIGN_LEFT, false);
    ADD_SIZER_FLAG(wxALIGN_CENTER_HORIZONTAL, false);
    ADD_SIZER_FLAG(wxALIGN_RIGHT, false);
    ADD_SIZER_FLAG(wxALIGN_TOP, false);
    ADD_SIZER_FLAG(wxALIGN_CENTER_VERTICAL, false);
    ADD_SIZER_FLAG(wxALIGN_BOTTOM, false);

    if(GetType() == ID_WXSTATICBOXSIZER) {
        ADD_SIZER_FLAG(wxRESERVE_SPACE_EVEN_IF_HIDDEN, true);

    } else {
        ADD_SIZER_FLAG(wxRESERVE_SPACE_EVEN_IF_HIDDEN, false);
    }

    Add<CategoryProperty>(_("Common Settings"));
    Add<WinIdProperty>();
    Add<StringProperty>(PROP_SIZE,
                        "-1,-1",
                        _("The control's size. It is recommended to leave it as -1,-1 and let\n"
                          "the sizers calculate the best size for the window"));
    Add<StringProperty>(PROP_MINSIZE,
                        "-1,-1",
                        _("The control's minimum size, to indicate to the sizer layout "
                          "mechanism that this is the minimum required size"));
    Add<StringProperty>(PROP_NAME, "", _("C++ member name"));
    Add<MultiStringsProperty>(PROP_TOOLTIP, _("Tooltip"), "\\n", _("Tooltip text:"));
    Add<ColorProperty>(PROP_BG, "<Default>", _("Set the control's background colour"));
    Add<ColorProperty>(PROP_FG, "<Default>", _("Set the control's foreground colour"));
    Add<FontProperty>(PROP_FONT, "", _("Set the control's font"));

    Add<CategoryProperty>(_("Initial State"));
    Add<BoolProperty>(PROP_STATE_HIDDEN, false, _("Sets the control initial state to 'Hidden'"));
    Add<BoolProperty>(PROP_STATE_DISABLED, false, _("Sets the control initial state to 'Disabled'"));
    Add<BoolProperty>(PROP_HAS_FOCUS, false, _("This control should have keyboard focus"));

    Add<CategoryProperty>(_("Subclass"));
    Add<StringProperty>(
        PROP_SUBCLASS_NAME, "", _("The name of the derived class. Used both for C++ and XRC generated code."));
    Add<StringProperty>(
        PROP_SUBCLASS_INCLUDE, "", _("(C++ only) The name of any extra header file to be #included e.g. mydialog.hpp"));
    Add<StringProperty>(
        PROP_SUBCLASS_STYLE,
        "",
        _("Override the default class style with the content of this field.\nThe style should be | separated"));
    Add<CategoryProperty>(_("Control Specific Settings"));
}

wxcWidget::~wxcWidget()
{
    if(GetParent()) {
        GetParent()->RemoveChild(this);
    }
    DeleteAllChildren();
    wxDELETE(m_eventsMenu);
    m_properties.Clear();
    m_controlEvents.Clear();
}

void wxcWidget::DeleteAllChildren()
{
    // deleting a child will also remove it from the parent's m_children
    // this is why we use a temporary list here
    List_t tmpChildren;
    tmpChildren.insert(tmpChildren.end(), m_children.begin(), m_children.end());
    for (auto child : tmpChildren) {
        delete child;
    }
    m_children.clear();
}

wxString wxcWidget::GetWindowParent() const
{
    wxString parentName = "NULL";
    const NotebookPageWrapper* nbPage = dynamic_cast<const NotebookPageWrapper*>(this);

    wxcWidget* pParent = m_parent;
    if(nbPage && nbPage->GetNotebook()) {
        return nbPage->GetNotebook()->GetName();

    } else {
        while(pParent) {

            if(pParent->IsValidParent()) {
                if(pParent->IsTopWindow()) {
                    parentName = "this";

                } else {
                    parentName = pParent->GetName();
                }
                break;

            } else {
                pParent = pParent->GetParent();
            }
        }
    }

    if(pParent && pParent->GetType() == ID_WXCOLLAPSIBLEPANE_PANE) {
        const wxCollapsiblePanePaneWrapper* paneWin = dynamic_cast<const wxCollapsiblePanePaneWrapper*>(pParent);
        if(paneWin && paneWin->GetParent()) {
            parentName.Clear();
            parentName << paneWin->GetParent()->GetName() << "->GetPane()";
            return parentName;
        }
    }

    return parentName;
}

void wxcWidget::ChildrenXRC(wxString& text, XRC_TYPE type) const
{
    for (const auto* child : m_children) {
        wxString xrc;
        child->ToXRC(xrc, type);

        if (child->IsAuiPane()) {
            xrc = child->WrapInAuiPaneXRC(xrc);

        } else if (child->IsSizerItem()) {
            xrc = child->WrapInSizerXRC(xrc);
        }
        text << xrc;
    }
}

wxString wxcWidget::SizerFlags(const wxString& defaultFlags) const
{
    wxString s;
    MapStyles_t::ConstIterator iter = m_sizerFlags.Begin();
    for(; iter != m_sizerFlags.End(); iter++) {
        if(iter->second.is_set) {
            s << iter->second.style_name << "|";
        }
    }
    // For aesthetic reasons, remove the individual border flags if wxALL is present
    // First check it's there if it should be (it won't always be in legacy files)
    if(s.Contains("wxLEFT") && s.Contains("wxRIGHT") && s.Contains("wxTOP") && s.Contains("wxBOTTOM") &&
       !s.Contains("wxALL")) {
        s << "wxALL|";
    }
    if(s.Contains("wxALL")) {
        s.Replace("wxLEFT", "");
        s.Replace("wxRIGHT", "");
        s.Replace("wxTOP", "");
        s.Replace("wxBOTTOM", "");
    }
    // Cope with multiple or trailing separators
    s = wxCrafter::Join(wxCrafter::Split(s, "|"), "|");

    s.Trim().Trim(false);
    if(s.IsEmpty()) {
        s = defaultFlags;
    }
    return s;
}

wxString wxcWidget::WrapInSizerXRC(const wxString& objXRC) const
{
    bool isGBSizerItem = IsGridBagSizerItem();
    bool isSizer = (GetWidgetType() == TYPE_SIZER);
    bool isSpacer = (GetType() == ID_WXSPACER);

    // Minimum size is an object property, but XRC plonks it in sizeritem :/
    wxString minsize;
    wxSize minSize = wxCrafter::DecodeSize(PropertyString(PROP_MINSIZE));
    if(minSize != wxDefaultSize) {
        minsize = "<minsize>" + wxCrafter::EncodeSize(minSize) + "</minsize>";
    }

    // There's a backdoor way if IDing sizers/spacers: putting their 'name' in the sizeritem
    // See http://trac.wxwidgets.org/changeset/48718 and wxSizerItem::GetId and wxSizer::GetItemById
    wxString name;
    if((isSizer || isSpacer) && !GetName().empty()) {
        name << " name=\"" << wxCrafter::XMLEncode(GetName()) << "\"";
    }

    wxString strXRC;
    if(!isSpacer) {
        strXRC << "<object class=\"sizeritem\"" << name << " >";

    } else {
        strXRC << "<object class=\"spacer\"" << name << " >";

        if(GetSize() != wxSize(0, 0)) {
            strXRC << XRCSize();
        }
    }

    strXRC << "   <flag>" << SizerFlags("") << "</flag>";

    // Don't pointlessly output <border>0</border>
    if(m_sizerItem.GetBorder() > 0) {
        strXRC << "   <border>" << m_sizerItem.GetBorder() << "</border>";
    }

    strXRC << minsize;
    if(m_sizerItem.GetProportion() > 0) {
        strXRC << "   <option>" << m_sizerItem.GetProportion() << "</option>";
    }

    if(isGBSizerItem) {
        strXRC << "<cellpos>" << m_gbPos << "</cellpos>";
        strXRC << "<cellspan>" << m_gbSpan << "</cellspan>";
    }

    if(!isSpacer) {
        strXRC << objXRC;
    }

    strXRC << "</object>";
    return strXRC;
}

bool wxcWidget::HasMainSizer() const
{
    if(IsSizer()) {
        return false;
    }

    for (const auto* child : m_children) {
        if (child->IsSizer()) {
            return true;
        }
    }
    return false;
}

void wxcWidget::DoClearFlags(MapStyles_t& mp)
{
    MapStyles_t::Iterator iter = mp.Begin();
    for(; iter != mp.End(); iter++) {
        iter->second.is_set = false;
    }
}

void wxcWidget::AddProperty(std::unique_ptr<PropertyBase> prop)
{
    if (prop) {
        m_properties.PushBack(prop->GetLabel(), std::move(prop));

    } else {
        m_properties.PushBack("", NULL);
    }
}

wxString wxcWidget::StyleFlags(const wxString& deafultStyle) const
{
    wxString s;

    wxString subclassStyle = PropertyString(PROP_SUBCLASS_STYLE);
    if(!subclassStyle.IsEmpty()) {
        return subclassStyle;

    } else {
        for(auto style : m_styles) {
            wxString style_name = style.second.style_name;
            if(style.second.is_set) {
                if(style_name == "wxBORDER_SIMPLE") {
                    s << "get_border_simple_theme_aware_bit()|";
                } else {
                    s << style_name << "|";
                }
            }
        }

        if(s.EndsWith("|")) {
            s.RemoveLast();
        }

        if(s.IsEmpty()) {
            s = deafultStyle;
        }

        return s;
    }
}

void wxcWidget::DoAddSizerFlag(const wxString& name, WxStyleInfo info)
{
    if(!m_sizerFlags.Contains(name)) {
        m_sizerFlags.PushBack(name, info);
    } else {
        m_sizerFlags.Item(name) = info;
    }
}

bool wxcWidget::IsSizerItem() const { return GetParent() && GetParent()->IsSizer(); }

bool wxcWidget::IsGridBagSizerItem() const { return GetParent() && GetParent()->GetType() == ID_WXGRIDBAGSIZER; }

wxString wxcWidget::PropertyBool(const wxString& propname) const
{
    if(m_properties.Contains(propname)) {
        wxString value = m_properties.Item(propname)->GetValue();

        if(value == "1") {
            return "true";

        } else {
            return "false";
        }

    } else {
        return "false";
    }
}

wxString wxcWidget::PropertyFile(const wxString& propname) const
{
    if(m_properties.Contains(propname)) {
        wxString value = m_properties.Item(propname)->GetValue();

        // FIXME:: Expand codelite's macros here
        value.Replace("\\", "/");
        value = wxCrafter::XMLEncode(value);
        return value;

    } else {
        return "";
    }
}

wxString wxcWidget::PropertyString(const wxString& propname, const wxString& defaultValue) const
{
    if(m_properties.Contains(propname)) {
        wxString v = m_properties.Item(propname)->GetValue();
        v.Trim();
        if(v.IsEmpty()) {
            return defaultValue;
        }
        return v;

    } else {
        return defaultValue;
    }
}

void wxcWidget::DoSetPropertyStringValue(const wxString& propname, const wxString& value)
{
    if(m_properties.Contains(propname)) {
        // Delete the old one and replace it with a new property
        m_properties.Item(propname)->SetValue(value);
    }
}

void wxcWidget::DoTraverseAndGenCode(wxArrayString& headers, wxString& ctorCode, wxString& members,
                                     wxString& eventFunctions, wxString& eventConnectCode,
                                     wxStringMap_t& additionalFiles, wxString& dtorCode, wxString& extraFunctionsImpl,
                                     wxString& extraFunctionsDecl) const
{
    // Start by checking if this control is subclassed; if so, use its data, not the superclass's
    wxString subclass = PropertyString(PROP_SUBCLASS_NAME);
    wxString subinclude = PropertyString(PROP_SUBCLASS_INCLUDE);

    GetIncludeFile(headers);
    if(!subinclude.empty()) {
        // 'subinclude' will probably be just "myfoo.hpp", but check:
        if(!subinclude.Contains("#include")) {
            subinclude = "#include \"" + subinclude + "\"";
        }
        headers.Add(subinclude);
    }

    // Additional files generated by the controls (if any)
    GenerateAdditionalFiles(additionalFiles);

    // ctor
    ctorCode << DoGenerateCppCtorCode() << "\n";

    // dtor
    wxString dtor = DoGenerateCppDtorCode();
    if(dtor.IsEmpty() == false) {
        dtorCode << dtor << "\n";
    }

    wxArrayString existsFunctionsArr = wxCrafter::Split(eventFunctions, "\r\n");
    wxArrayString newEventFuncArr = wxCrafter::Split(DoGenerateEventStubs(), "\r\n");

    // Append the array
    existsFunctionsArr.insert(existsFunctionsArr.end(), newEventFuncArr.begin(), newEventFuncArr.end());

    // Remove duplicate entries
    existsFunctionsArr = wxCrafter::MakeUnique(existsFunctionsArr);

    // And finally convert the wxArrayString to wxString
    eventFunctions.Clear();
    eventFunctions << wxCrafter::Join(existsFunctionsArr, "\n");

    eventConnectCode << DoGenerateConnectCode();

    wxString memberCode = DoGenerateClassMember();
    if(memberCode.IsEmpty() == false) {
        members << memberCode << "\n";
    }

    for (const auto* child : m_children) {
        child->DoTraverseAndGenCode(headers,
                                    ctorCode,
                                    members,
                                    eventFunctions,
                                    eventConnectCode,
                                    additionalFiles,
                                    dtorCode,
                                    extraFunctionsImpl,
                                    extraFunctionsDecl);
        if (child->IsSizerItem()) {
            wxSize minSize = wxCrafter::DecodeSize(child->PropertyString(PROP_MINSIZE));
            if(minSize != wxDefaultSize) {
                if(ctorCode.Right(2) == "\n\n") {
                    ctorCode.RemoveLast(); // Otherwise the SetMinSize() feels lonely
                }
                ctorCode << child->GetName() << "->SetMinSize(wxSize(" << wxCrafter::EncodeSize(minSize) << "));\n\n";
            }
        }
    }

    // Allow the parent item to "close" the ctor (useful when we want to add calls like "p->Realize()" etc)
    wxString cppCtorEndCode = DoGenerateCppCtorCode_End();
    if(!cppCtorEndCode.IsEmpty()) {
        ctorCode.Trim();
        ctorCode << "\n" << cppCtorEndCode;
    }

    // Add any extra functions. atm the only example is an eventhandler for wxAuiToolBar dropdowns
    wxString extraCodeImpl, extraCodeDecl;
    DoGenerateExtraFunctions(extraCodeDecl,  // Implementation
                             extraCodeImpl); // Declarations
    if(!extraCodeImpl.IsEmpty()) {
        extraCodeImpl.Trim();
        extraFunctionsImpl << "\n" << extraCodeImpl;
        extraFunctionsDecl << "\n" << extraCodeDecl;
    }

    DoGenerateGetters(extraFunctionsDecl);
}

wxString wxcWidget::DoGenerateCppDtorCode() const
{
    wxString code = CppDtorCode();
    return code;
}

wxString wxcWidget::DoGenerateCppCtorCode() const
{
    wxString code = CppCtorCode();

    ////////////////////////////////////////////////////////////////////////////
    // Add extra code here (required in some unique cases, like wxToolbar, etc)
    ////////////////////////////////////////////////////////////////////////////

    if(GetType() == ID_WXSTDBUTTON) {
        // standard button
        wxString extraCode;
        extraCode << GetParent()->GetName() << "->AddButton(" << GetName() << ");\n";
        if(IsLastChild()) {
            extraCode << GetParent()->GetName() << "->Realize();\n";
        }
        code << extraCode;

    } else if(GetType() == ID_WXSPACER) {
        // Do nothing

    } else if(IsParentToolbar() && !IsToolBarTool()) {
        // Control is being added to the toolbar
        wxString tbCode;

        tbCode << GetWindowParent() << "->AddControl(" << GetName() << ");";
        code << tbCode << "\n";

        if(IsLastChild()) {
            code << GetWindowParent() << "->Realize();\n";
        }

    } else if(IsParentToolbar() && IsToolBarTool()) {
        if(IsLastChild()) {
            code << GetWindowParent() << "->Realize();\n";
        }

    } else if(IsGridBagSizerItem()) {

        wxString sizerCode;
        sizerCode << GetParent()->GetName() << "->Add(" << GetName() << ", "
                  << "wxGBPosition(" << m_gbPos << "), "
                  << "wxGBSpan(" << m_gbSpan << "), " << SizerFlags("0") << ", " << m_sizerItem.GetBorderScaled()
                  << ");";
        code << "\n" << sizerCode << "\n";

    } else if(IsAuiPane()) {
        wxString auiPaneCode;
        auiPaneCode << GetParent()->GetName() << "->AddPane(" << GetName() << ", " << m_auiPaneInfo.ToCppCode() << ");";
        code << "\n" << auiPaneCode << "\n";

        if(IsLastChild()) {
            code << GetParent()->GetName() << "->Update();\n";
        }

    } else if(IsSizerItem()) {
        wxString sizerCode;
        sizerCode << GetParent()->GetName() << "->Add(" << GetName() << ", " << m_sizerItem.GetProportion() << ", "
                  << SizerFlags("0") << ", " << m_sizerItem.GetBorderScaled() << ");";
        code << "\n" << sizerCode << "\n";
    }
    WrapInIfBlockIfNeeded(code);
    return code;
}

wxString wxcWidget::DoGenerateClassMember() const { return BaseDoGenerateClassMember(); }

bool wxcWidget::IsSizerFlagChecked(const wxString& style) const
{
    if(!m_sizerFlags.Contains(style)) {
        return false;
    }

    return m_sizerFlags.Item(style).is_set;
}

void wxcWidget::DoEnableStyle(wxcWidget::MapStyles_t& mp, const wxString& style, bool enable)
{
    if(mp.Contains(style)) {
        mp.Item(style).is_set = enable;
    }
}

void wxcWidget::Serialize(JSONElement& json) const
{
    json.addProperty("m_type", m_type);
    json.addProperty("proportion", m_sizerItem.GetProportion());
    json.addProperty("border", m_sizerItem.GetBorder());
    json.addProperty("gbSpan", m_gbSpan);
    json.addProperty("gbPosition", m_gbPos);

    JSONElement styles = JSONElement::createArray("m_styles");
    for (const auto& [_, styleInfo] : m_styles) {
        if (styleInfo.is_set) {
            styles.arrayAppend(styleInfo.style_name);
        }
    }
    json.append(styles);

    if(IsAuiPane()) {
        json.append(m_auiPaneInfo.ToJSON());
    }

    JSONElement sizerFlags = JSONElement::createArray("m_sizerFlags");
    for (const auto& [_, styleInfo] : m_sizerFlags) {
        if (styleInfo.is_set) {
            sizerFlags.arrayAppend(styleInfo.style_name);
        }
    }
    json.append(sizerFlags);

    JSONElement properties = JSONElement::createArray("m_properties");
    for (const auto& [_, property] : m_properties) {
        if (property) {
            properties.arrayAppend(property->Serialize());
        }
    }
    json.append(properties);

    JSONElement events = JSONElement::createArray("m_events");
    for (const auto& p : m_connectedEvents) {
        events.arrayAppend(p.second.ToJSON());
    }
    json.append(events);

    JSONElement children = JSONElement::createArray("m_children");
    List_t::const_iterator child_iter = m_children.begin();
    for(; child_iter != m_children.end(); child_iter++) {
        JSONElement child = JSONElement::createObject();
        (*child_iter)->Serialize(child);
        children.arrayAppend(child);
    }

    json.append(children);
}

void wxcWidget::UnSerialize(const JSONElement& json)
{
    m_sizerItem.SetBorder(json.namedObject("border").toInt(5));
    m_sizerItem.SetProportion(json.namedObject("proportion").toInt(0));
    m_gbSpan = json.namedObject("gbSpan").toString();
    m_gbPos = json.namedObject("gbPosition").toString();

    m_auiPaneInfo.FromJSON(json.namedObject("wxAuiPaneInfo"));

    // Unserialize the styles
    DoClearFlags(m_styles);
    DoClearFlags(m_sizerFlags);

    m_connectedEvents.Clear();

    JSONElement styles = json.namedObject("m_styles");
    int nCount = styles.arraySize();
    for(int i = 0; i < nCount; i++) {
        wxString styleName = styles.arrayItem(i).toString();
        EnableStyle(styleName, true);
    }

    JSONElement sizerFlags = json.namedObject("m_sizerFlags");
    nCount = sizerFlags.arraySize();
    for(int i = 0; i < nCount; i++) {
        wxString styleName = sizerFlags.arrayItem(i).toString();
        EnableSizerFlag(styleName, true);
    }

    // Unserialize the properties
    JSONElement properties = json.namedObject("m_properties");
    nCount = properties.arraySize();
    for(int i = 0; i < nCount; i++) {
        JSONElement jsonProp = properties.arrayItem(i);
        wxString propLabel = jsonProp.namedObject("m_label").toString();
        if(m_properties.Contains(propLabel)) {
            m_properties.Item(propLabel)->UnSerialize(jsonProp);
        }
    }

    // Unserialize the events
    JSONElement events = json.namedObject("m_events");
    nCount = events.arraySize();
    for(int i = 0; i < nCount; i++) {
        JSONElement jsonEvent = events.arrayItem(i);
        ConnectDetails details;
        details.FromJSON(jsonEvent);

        // Since wx295, the WebView event names were modified from *_WEB_VIEW_* to *_WEBVIEW_*
        // perfrom the name changes here
        if(details.GetEventName().Contains("_WEB_VIEW_")) {
            wxString new_name = details.GetEventName();
            new_name.Replace("_WEB_VIEW_", "_WEBVIEW_");
            details.SetEventName(new_name);
        }
        m_connectedEvents.PushBack(details.GetEventName(), details);
    }

    JSONElement children = json.namedObject("m_children");
    int nChildren = children.arraySize();
    for(int i = 0; i < nChildren; i++) {
        JSONElement child = children.arrayItem(i);
        wxcWidget* wrapper = Allocator::Instance()->CreateWrapperFromJSON(child);
        if(wrapper) {
            AddChild(wrapper);
        }
    }
}

void wxcWidget::LoadPropertiesFromXRC(const wxXmlNode* node)
{
    wxString value;

    // Start with 2 possible strings in node itself
    value = XmlUtils::ReadString(node, "name");
    if(!value.empty()) {
        SetName(value);
    }

    wxString subclass = XmlUtils::ReadString(node, "subclass");
    if(!subclass.empty()) {
        SetPropertyString(PROP_SUBCLASS_NAME, subclass);
    }

    wxXmlNode* propertynode = XmlUtils::FindFirstByTagName(node, "size");
    if(propertynode) {
        SetPropertyString(PROP_SIZE, propertynode->GetNodeContent());
    }

    /*    propertynode = XmlUtils::FindFirstByTagName(node, "pos");
        if (propertynode) {
           // wxC doesn't do positions
        }*/

    propertynode = XmlUtils::FindFirstByTagName(node, "tooltip");
    if(propertynode) {
        SetPropertyString(PROP_TOOLTIP, propertynode->GetNodeContent());
    }

    propertynode = XmlUtils::FindFirstByTagName(node, "font");
    if(propertynode) {
        SetPropertyString(PROP_FONT, wxCrafter::XRCToFontstring(propertynode));
    }

    propertynode = XmlUtils::FindFirstByTagName(node, "fg");
    if(propertynode) {
        SetPropertyString(PROP_FG, wxCrafter::ValueToColourString(propertynode->GetNodeContent()));
    }

    propertynode = XmlUtils::FindFirstByTagName(node, "bg");
    if(propertynode) {
        SetPropertyString(PROP_BG, wxCrafter::ValueToColourString(propertynode->GetNodeContent()));
    }

    propertynode = XmlUtils::FindFirstByTagName(node, "hidden");
    if(propertynode && propertynode->GetNodeContent() == "1") {
        SetPropertyString(PROP_STATE_HIDDEN, "1");
    }

    propertynode = XmlUtils::FindFirstByTagName(node, "enabled");
    if(propertynode && propertynode->GetNodeContent() == "0") {
        SetPropertyString(PROP_STATE_DISABLED, "1");
    }

    propertynode = XmlUtils::FindFirstByTagName(node, "focused");
    if(propertynode && propertynode->GetNodeContent() == "1") {
        SetPropertyString(PROP_HAS_FOCUS, "1");
    }

    // The Label property has a default, so we must clear it even if the node is absent
    PropertyBase* labelprop = GetProperty(PROP_LABEL);
    if(labelprop) {
        wxString labelvalue;
        propertynode = XmlUtils::FindFirstByTagName(node, "label");
        if(propertynode) {
            labelvalue = propertynode->GetNodeContent();
        }
        labelprop->SetValue(labelvalue);
    }

    propertynode = XmlUtils::FindFirstByTagName(node, "title");
    if(propertynode) {
        SetTitle(propertynode->GetNodeContent());
    }

    propertynode = XmlUtils::FindFirstByTagName(node, "style");
    if(propertynode) {
        wxString styles = propertynode->GetNodeContent();
        styles.Replace("wxRESIZE_BOX", "wxMAXIMIZE_BOX"); // Deprecated (removed?) in wx2.9 but still likely to be seen
        styles.Replace("wxSTATIC_BORDER", "wxBORDER_STATIC");
        styles.Replace("wxSIMPLE_BORDER", "wxBORDER_SIMPLE");
        styles.Replace("wxSUNKEN_BORDER", "wxBORDER_SUNKEN");
        styles.Replace("wxRAISED_BORDER", "wxBORDER_RAISED");
        styles.Replace("wxNO_BORDER", "wxBORDER_NONE");

        wxArrayString stylearray = wxCrafter::Split(styles, "|");

        DoClearFlags(m_styles); // otherwise the default ones will remain
        for(size_t n = 0; n < stylearray.GetCount(); ++n) {
            DoEnableStyle(m_styles, stylearray.Item(n), true);
        }
    }

    // Now get any events. wxFB's XRC output produces nothing. XRCed just produces:
    //    <XRCED> <events>EVT_LEFT_DOWN|EVT_CHAR</events> </XRCED>
    propertynode = XmlUtils::FindFirstByTagName(node, "XRCED");
    if(propertynode) {
        wxXmlNode* eventsnode = XmlUtils::FindFirstByTagName(propertynode, "events");
        if(eventsnode) {
            ImportEventsFromXRC(eventsnode->GetNodeContent());
        }
    }
}

void wxcWidget::LoadPropertiesFromwxSmith(const wxXmlNode* node)
{
    wxString value;

    // Start with 2 possible strings in node itself
    value = XmlUtils::ReadString(node, "name");
    if(!value.empty()) {
        SetName(value);
    }

    wxString subclass = XmlUtils::ReadString(node, "subclass");
    if(!subclass.empty()) {
        SetPropertyString(PROP_SUBCLASS_NAME, subclass);
    }

    wxXmlNode* propertynode = XmlUtils::FindFirstByTagName(node, "size");
    if(propertynode) {
        SetPropertyString(PROP_SIZE, propertynode->GetNodeContent());
    }

    /*    propertynode = XmlUtils::FindFirstByTagName(node, "pos");
        if (propertynode) {
           // wxC doesn't do positions
        }*/

    propertynode = XmlUtils::FindFirstByTagName(node, "tooltip");
    if(propertynode) {
        SetPropertyString(PROP_TOOLTIP, wxCrafter::ESCAPE(propertynode->GetNodeContent()));
    }

    propertynode = XmlUtils::FindFirstByTagName(node, "font");
    if(propertynode) {
        SetPropertyString(PROP_FONT, wxCrafter::XRCToFontstring(propertynode)); // wxS behaves like XRC here
    }

    propertynode = XmlUtils::FindFirstByTagName(node, "fg");
    if(propertynode) {
        SetPropertyString(PROP_FG, wxCrafter::ValueToColourString(propertynode->GetNodeContent())); // and here
    }

    propertynode = XmlUtils::FindFirstByTagName(node, "bg");
    if(propertynode) {
        SetPropertyString(PROP_BG, wxCrafter::ValueToColourString(propertynode->GetNodeContent()));
    }

    propertynode = XmlUtils::FindFirstByTagName(node, "hidden");
    if(propertynode && propertynode->GetNodeContent() == "1") {
        SetPropertyString(PROP_STATE_HIDDEN, "1");
    }

    propertynode = XmlUtils::FindFirstByTagName(node, "enabled");
    if(propertynode && propertynode->GetNodeContent() == "0") {
        SetPropertyString(PROP_STATE_DISABLED, "1");
    }

    propertynode = XmlUtils::FindFirstByTagName(node, "focused");
    if(propertynode && propertynode->GetNodeContent() == "1") {
        SetPropertyString(PROP_HAS_FOCUS, "1");
    }

    // The Label property has a default, so we must clear it even if the node is absent
    PropertyBase* labelprop = GetProperty(PROP_LABEL);
    if(labelprop) {
        wxString labelvalue;
        propertynode = XmlUtils::FindFirstByTagName(node, "label");
        if(propertynode) {
            labelvalue = wxCrafter::ESCAPE(propertynode->GetNodeContent());
        }
        labelprop->SetValue(labelvalue);
    }

    propertynode = XmlUtils::FindFirstByTagName(node, "title");
    if(propertynode) {
        SetTitle(wxCrafter::ESCAPE(propertynode->GetNodeContent()));
    }

    propertynode = XmlUtils::FindFirstByTagName(node, "style");
    if(propertynode) {
        wxString styles = propertynode->GetNodeContent();
        styles.Replace("wxRESIZE_BOX", "wxMAXIMIZE_BOX"); // Deprecated (removed?) in wx2.9 but still likely to be seen
        styles.Replace("wxSTATIC_BORDER", "wxBORDER_STATIC");
        styles.Replace("wxSIMPLE_BORDER", "wxBORDER_SIMPLE");
        styles.Replace("wxSUNKEN_BORDER", "wxBORDER_SUNKEN");
        styles.Replace("wxRAISED_BORDER", "wxBORDER_RAISED");
        styles.Replace("wxNO_BORDER", "wxBORDER_NONE");

        wxArrayString stylearray = wxCrafter::Split(styles, "|");

        DoClearFlags(m_styles); // otherwise the default ones will remain
        for(size_t n = 0; n < stylearray.GetCount(); ++n) {
            DoEnableStyle(m_styles, stylearray.Item(n), true);
        }
    }

    // Now get any events. wxSmith does:
    //	<handler function="OnTextCtrl1Text" entry="EVT_TEXT" />
    //	<handler function="OnTextCtrl1TextEnter" entry="EVT_TEXT_ENTER" />
    // i.e. possible multiple child nodes, so we can't use FindFirstByTagName()
    wxXmlNode* child = node->GetChildren();
    while(child) {
        if(child->GetName() == "handler") {
            ImportEventFromwxSmith(XmlUtils::ReadString(child, "entry"), XmlUtils::ReadString(child, "function"));
        }
        child = child->GetNext();
    }
}

void wxcWidget::LoadPropertiesFromwxFB(const wxXmlNode* node)
{
    wxString value;
    DoClearFlags(m_styles); // otherwise the default ones will remain. NB unlike fromXRC, do it here as we have 2 style
                            // attributes below

    // Unlike XRC, wxFB stores everything in <property name=foo>value</property> nodes
    wxXmlNode* propertynode = XmlUtils::FindNodeByName(node, "property", "name");
    if(propertynode) {
        value = propertynode->GetNodeContent();
        if(!value.empty()) {
            SetName(value);

            // use the name as the file name by default
            // ERAN: Do not set filename when importing from wxFB
            // if ( IsTopWindow() ) {
            //    SetFilename(value);
            // }

            // ERAN
            // When importing from wxFB, leave the 'Base Class Suffix' property empty
            if(IsTopWindow()) {
                if(m_properties.Contains(PROP_BASE_CLASS_SUFFIX)) {
                    m_properties.Item(PROP_BASE_CLASS_SUFFIX)->SetValue("");
                }
            }
        }
    }

    propertynode = XmlUtils::FindNodeByName(node, "property", "subclass");
    if(propertynode) {
        // Any contents will come as a pair: "classname; headername"
        wxString props = propertynode->GetNodeContent();
        if(!props.empty()) {
            wxArrayString arr = wxCrafter::Split(props, ";");
            wxString classname = arr.Item(0).Trim().Trim(false);
            if(!classname.empty()) {
                SetPropertyString(PROP_SUBCLASS_NAME, classname);
                if(arr.GetCount() > 1) {
                    SetPropertyString(PROP_SUBCLASS_INCLUDE, arr.Item(1).Trim().Trim(false));
                }
            }
        }
    }

    propertynode = XmlUtils::FindNodeByName(node, "property", "id");
    if(propertynode) {
        SetPropertyString(PROP_WINDOW_ID, propertynode->GetNodeContent());
    }

    propertynode = XmlUtils::FindNodeByName(node, "property", "size");
    if(propertynode) {
        SetPropertyString(PROP_SIZE, propertynode->GetNodeContent());
    }

    propertynode = XmlUtils::FindNodeByName(node, "property", "tooltip");
    if(propertynode) {
        SetPropertyString(PROP_TOOLTIP, wxCrafter::ESCAPE(propertynode->GetNodeContent()));
    }

    propertynode = XmlUtils::FindNodeByName(node, "property", "font");
    if(propertynode) {
        wxString fontasstring = wxCrafter::FBToFontstring(propertynode->GetNodeContent());
        if(!fontasstring.empty()) {
            SetPropertyString(PROP_FONT, fontasstring);
        }
    }

    propertynode = XmlUtils::FindNodeByName(node, "property", "fg");
    if(propertynode) {
        wxString value = propertynode->GetNodeContent();
        if(!value.empty()) {
            wxString col = wxCrafter::ValueToColourString(value);
            if(!col.empty()) {
                SetPropertyString(PROP_FG, col);
            }
        }
    }

    propertynode = XmlUtils::FindNodeByName(node, "property", "bg");
    if(propertynode) {
        wxString value = propertynode->GetNodeContent();
        if(!value.empty()) {
            wxString col = wxCrafter::ValueToColourString(propertynode->GetNodeContent());
            if(!col.empty()) {
                SetPropertyString(PROP_BG, col);
            }
        }
    }

    propertynode = XmlUtils::FindNodeByName(node, "property", "hidden");
    if(propertynode) {
        wxString value = propertynode->GetNodeContent();
        if(value == "1") {
            SetPropertyString(PROP_STATE_HIDDEN, "1");
        }
    }

    propertynode = XmlUtils::FindNodeByName(node, "property", "enabled");
    if(propertynode) {
        wxString value = propertynode->GetNodeContent();
        if(value == "0") {
            SetPropertyString(PROP_STATE_DISABLED, "1");
        }
    }

    // wxFB doesn't do Focused

    propertynode = XmlUtils::FindNodeByName(node, "property", "label");
    if(propertynode) {
        SetPropertyString(PROP_LABEL, wxCrafter::ESCAPE(propertynode->GetNodeContent()));
    }

    propertynode = XmlUtils::FindNodeByName(node, "property", "title");
    if(propertynode) {
        SetTitle(wxCrafter::ESCAPE(propertynode->GetNodeContent()));
    }

    propertynode = XmlUtils::FindNodeByName(node, "property", "minimum_size");
    if(propertynode) {
        SetPropertyString(PROP_MINSIZE, propertynode->GetNodeContent());
    }

    // Special 'two for the price of one' offer :)
    propertynode = XmlUtils::FindNodeByName(node, "property", "style"); // the derived-class styles
    if(propertynode) {
        value = propertynode->GetNodeContent();
    }
    propertynode = XmlUtils::FindNodeByName(node, "property", "window_style"); // the standard ones
    if(propertynode) {
        if(!value.empty()) {
            value << '|';
        }
        value << propertynode->GetNodeContent();
    }
    if(!value.empty()) {
        value.Replace("wxRESIZE_BOX", "wxMAXIMIZE_BOX"); // Deprecated (removed?) in wx2.9 but still likely to be seen
        value.Replace("wxSTATIC_BORDER", "wxBORDER_STATIC");
        value.Replace("wxSIMPLE_BORDER", "wxBORDER_SIMPLE");
        value.Replace("wxSUNKEN_BORDER", "wxBORDER_SUNKEN");
        value.Replace("wxRAISED_BORDER", "wxBORDER_RAISED");
        value.Replace("wxNO_BORDER", "wxBORDER_NONE");
        wxArrayString stylearray = wxCrafter::Split(value, "|");

        for(size_t n = 0; n < stylearray.GetCount(); ++n) {
            DoEnableStyle(m_styles, stylearray.Item(n), true);
        }
    }

    // Now get any events. wxFB lists all the control's events, whether set or not, so we must iterate
    //    <event name="OnChar"></event>
    //    <event name="OnButtonClick">whatever-the-user-typed</event>
    wxXmlNode* child = node->GetChildren();
    while(child) {
        if(child->GetName() == "event") {
            wxString value = child->GetNodeContent();
            if(!value.empty()) {
                ImportEventFromFB(XmlUtils::ReadString(child, "name"), value);
            }
        }
        child = child->GetNext();
    }
}

void wxcWidget::ImportEventsFromXRC(const wxString& events)
{
    // XRCed supplies any events as e.g. "EVT_LEFT_DOWN|EVT_CHAR"
    wxArrayString arr = wxCrafter::Split(events, "|");
    for(size_t n = 0; n < arr.GetCount(); ++n) {
        wxString eventname = arr.Item(n);
        wxString eventtype = ImportFromwxFB::GetEventtypeFromHandlerstub(
            eventname); // The 'FB' isn't a mistake; it's stored there for convenience
        if(!eventtype.empty()) {
            EventsDatabase& edb = Allocator::GetCommonEvents();
            if(edb.Exists(wxXmlResource::GetXRCID(eventtype))) {
                ConnectDetails eventDetails = edb.Item(wxXmlResource::GetXRCID(eventtype));
                eventDetails.SetFunctionNameAndSignature(eventDetails.GetEventClass() + "Handler");
                AddEvent(eventDetails);
            } else {
                EventsDatabase& edb = GetControlEvents();
                if(edb.Exists(wxXmlResource::GetXRCID(eventtype))) {
                    ConnectDetails eventDetails = edb.Item(wxXmlResource::GetXRCID(eventtype));
                    eventDetails.SetFunctionNameAndSignature(eventDetails.GetEventClass() + "Handler");
                    AddEvent(eventDetails);
                }
            }
        }
    }
}

void wxcWidget::ImportEventFromwxSmith(const wxString& eventname, const wxString& handlerstub)
{
    if(!eventname.empty()) {
        wxString eventtype = ImportFromwxFB::GetEventtypeFromHandlerstub(eventname);
        EventsDatabase& edb = Allocator::GetCommonEvents();
        if(edb.Exists(wxXmlResource::GetXRCID(eventtype))) {
            ConnectDetails eventDetails = edb.Item(wxXmlResource::GetXRCID(eventtype));
            eventDetails.SetFunctionNameAndSignature(handlerstub);
            AddEvent(eventDetails);
        } else {
            EventsDatabase& edb = GetControlEvents();
            if(edb.Exists(wxXmlResource::GetXRCID(eventtype))) {
                ConnectDetails eventDetails = edb.Item(wxXmlResource::GetXRCID(eventtype));
                eventDetails.SetFunctionNameAndSignature(handlerstub);
                AddEvent(eventDetails);
            }
        }
    }
}

void wxcWidget::ImportEventFromFB(const wxString& eventname, const wxString& handlerstub)
{
    if(!eventname.empty()) {
        wxString eventtype = ImportFromwxFB::GetEventtypeFromHandlerstub(eventname);
        EventsDatabase& edb = Allocator::GetCommonEvents();
        if(edb.Exists(wxXmlResource::GetXRCID(eventtype))) {
            ConnectDetails eventDetails = edb.Item(wxXmlResource::GetXRCID(eventtype));
            eventDetails.SetFunctionNameAndSignature(handlerstub);
            AddEvent(eventDetails);
        } else {
            EventsDatabase& edb = GetControlEvents();
            if(edb.Exists(wxXmlResource::GetXRCID(eventtype))) {
                ConnectDetails eventDetails = edb.Item(wxXmlResource::GetXRCID(eventtype));
                eventDetails.SetFunctionNameAndSignature(handlerstub);
                AddEvent(eventDetails);
            }
        }
    }
}

void wxcWidget::RemoveChild(wxcWidget* child)
{
    List_t::iterator iter = m_children.begin();
    for(; iter != m_children.end(); iter++) {
        if((*iter) == child) {
            m_children.erase(iter);
            break;
        }
    }
}

void wxcWidget::RemoveFromParent()
{
    if(m_parent) {
        m_parent->RemoveChild(this);
    }
    m_parent = NULL;
}

void wxcWidget::MoveDown()
{
    if(!m_parent) {
        return;
    }

    // Locate our pointer in the parent children list
    List_t& list = m_parent->m_children;
    List_t::iterator iter = list.begin();
    for(; iter != list.end(); iter++) {
        if((*iter) == this) {
            break;
        }
    }

    // Now move your element two positions forward
    int i = 0;
    List_t::iterator new_position = iter;
    for(; (new_position != list.end() && i < 2); new_position++) {
        i++;
    }

    list.insert(new_position, this);
    list.erase(iter);
}

void wxcWidget::MoveUp()
{
    if(!m_parent) {
        return;
    }

    // Locate our pointer in the parent children list
    List_t& list = m_parent->m_children;
    List_t::iterator iter = list.begin();
    for(; iter != list.end(); iter++) {
        if((*iter) == this) {
            break;
        }
    }

    // Now move your element two positions back (let's assume you have the elements in
    // the list to-do that amount of movement, but nevertheless,
    // we still check the list bounds and exit the loop if we hit the front of the list)
    int i = 0;
    List_t::iterator new_position = iter;
    for(; (new_position != list.begin() && i < 1); new_position--) {
        i++;
    }

    list.insert(new_position, this);
    list.erase(iter);
}

void wxcWidget::CopySizerAndAuiInfo(const wxcWidget* source, wxcWidget* target)
{
    for (const auto& sizer : source->m_sizerFlags) {
        if (target->m_sizerFlags.Contains(sizer.first)) {
            target->m_sizerFlags.Item(sizer.first) = sizer.second;
        }
    }
    target->m_auiPaneInfo = source->m_auiPaneInfo;
    target->m_sizerItem.SetProportion(source->m_sizerItem.GetProportion());
    target->m_sizerItem.SetBorder(source->m_sizerItem.GetBorder());
    target->m_gbPos = source->m_gbPos;
    target->m_gbSpan = source->m_gbSpan;
}

void wxcWidget::ReplaceWidget(wxcWidget* oldWidget, wxcWidget* newWidget)
{
    List_t tmp_children = oldWidget->m_children; // Naive copy

    // Use a tmp_children because 'Reparent' will cause the m_children
    // size to change so its not reliable to count on it
    for (auto child : tmp_children) {
        child->Reparent(newWidget);
    }

    // at this point oldWidget->m_children should be empty
    // Now whats left to be done is:
    // - disconnect oldWidget from its parent
    // - connect newWidget to the parent in the same position where oldWidget was
    if(oldWidget->GetParent()) {
        oldWidget->GetParent()->InsertBefore(newWidget, oldWidget);
        oldWidget->GetParent()->RemoveChild(oldWidget);
    }

    // ------------------------------------------
    // Copy the item's sizer flags
    // ------------------------------------------
    CopySizerAndAuiInfo(oldWidget, newWidget);
}

void wxcWidget::InsertWidgetInto(wxcWidget* oldWidget, wxcWidget* newWidget) // Used for Insert into new Sizer
{
    wxcWidget* parent = oldWidget->GetParent();
    wxCHECK_RET(parent, "An orphaned widget");

    parent->InsertBefore(newWidget, oldWidget);
    oldWidget->Reparent(newWidget);

    // ------------------------------------------
    // Copy the item's sizer flags
    // ------------------------------------------
    CopySizerAndAuiInfo(oldWidget, newWidget);
}

bool wxcWidget::CanMoveDown() const
{
    if(!m_parent) {
        return false;
    }

    // Locate our pointer in the parent children list
    List_t& list = m_parent->m_children;
    List_t::iterator iter = list.begin();
    for(; iter != list.end(); iter++) {
        if((*iter) == this) {
            break;
        }
    }

    if(iter == list.end()) {
        return false;
    }

    // Return true of this iterator is not the last one
    return (++iter) != list.end();
}

bool wxcWidget::CanMoveUp() const
{
    if(!m_parent) {
        return false;
    }

    // Locate our pointer in the parent children list
    List_t& list = m_parent->m_children;
    List_t::iterator iter = list.begin();
    for(; iter != list.end(); iter++) {
        if((*iter) == this) {
            break;
        }
    }

    if(iter == list.end()) {
        return false;
    }

    return (*iter) != *(list.begin());
}

wxcWidget*
wxcWidget::GetAdjacentSiblingSizer(bool* isAbove /*=NULL*/) const // Return an adjacent sibling that's a sizer, or NULL
{
    wxcWidget* adjacent = GetAdjacentSibling(true);
    if(adjacent && adjacent->IsSizer()) {
        if(isAbove) {
            *isAbove = true;
        }
        return adjacent;
    }
    adjacent = GetAdjacentSibling(false);
    if(adjacent && adjacent->IsSizer()) {
        if(isAbove) {
            *isAbove = false;
        }
        return adjacent;
    }
    return NULL;
}

wxcWidget*
wxcWidget::GetAdjacentSibling(bool previous) const // i.e. the previous (or next) control with the same parent
{
    if(!m_parent) {
        return NULL;
    }

    // Locate our pointer in the parent children list
    List_t& list = m_parent->m_children;
    List_t::iterator iter = list.begin();
    for(; iter != list.end(); ++iter) {
        if((*iter) == this) {
            break;
        }
    }

    if(iter == list.end()) {
        return NULL;
    }

    if(previous) {
        if(iter == list.begin()) {
            return NULL;
        }
        --iter;
    } else {
        if(++iter == list.end()) {
            return NULL;
        }
    }

    return *iter;
}

wxString wxcWidget::SizeAsString() const
{
    wxString parentStr = GetWindowParent();
    wxString size = PropertyString(PROP_SIZE);
    size.Trim().Trim(false);

    if(size.IsEmpty()) {
        size = "-1, -1";
    }
    size.Prepend("wxSize(").Append(")");
    if(!parentStr.IsEmpty() && (parentStr != "NULL")) {
        wxString dlgUnits;
        dlgUnits << "wxDLG_UNIT(" << parentStr << ", " << size << ")";
        size.swap(dlgUnits);
    }
    return size;
}

wxString wxcWidget::ValueAsString() const
{
    wxString value = PropertyString(PROP_VALUE);
    value.Prepend("wxT(\"").Append("\")");
    return value;
}

wxString wxcWidget::WindowID() const { return GetId(); }

wxString wxcWidget::CPPLabel() const { return wxCrafter::UNDERSCORE(PropertyString(PROP_LABEL)); }

wxString wxcWidget::Label() const
{
    wxString label = PropertyString(PROP_LABEL);
    label.Replace("&amp;", "&");
    return wxCrafter::UNDERSCORE(label);
}

wxString wxcWidget::Size() const
{
    wxString size = PropertyString(PROP_SIZE);
    size.Trim().Trim(false);

    if(size.IsEmpty()) {
        size = "-1, -1";
    }
    return size;
}

void wxcWidget::AddEvent(const ConnectDetails& eventDetails)
{
    if(m_connectedEvents.Contains(eventDetails.GetEventName())) {
        // Replace it
        m_connectedEvents.Item(eventDetails.GetEventName()) = eventDetails;

    } else {
        // Add it
        m_connectedEvents.PushBack(eventDetails.GetEventName(), eventDetails);
    }
}

void wxcWidget::RemoveEvent(const wxString& eventName) { m_connectedEvents.Remove(eventName); }

bool wxcWidget::HasEvent(const wxString& eventName) const { return m_connectedEvents.Contains(eventName); }

wxString wxcWidget::GetCppName() const
{
    if(IsTopWindow()) {
        return "this";
    } else {
        return GetName();
    }
}

wxString wxcWidget::CreateBaseclassName() const
{
    // Avoid creating MyFooBaseBaseClass
    wxString name = GetName().Trim().Trim(false);
    return name;
}

wxString wxcWidget::DoGenerateEventStubs() const
{
    wxString stubsCode;
    MapEvents_t::const_iterator iter = m_connectedEvents.begin();
    for(; iter != m_connectedEvents.end(); iter++) {
        ConnectDetails eventDetails = iter->second;
        if(eventDetails.GetFunctionNameAndSignature().IsEmpty()) {
            eventDetails.GenerateFunctionName(GetName());
        }

        if(eventDetails.GetNoBody()) {
            stubsCode << "virtual void " << eventDetails.GetFunctionNameAndSignature() << ";\n";
        } else {
            stubsCode << "virtual void " << eventDetails.GetFunctionNameAndSignature() << " { event.Skip(); }\n";
        }
    }
    WrapInIfBlockIfNeeded(stubsCode);
    return stubsCode;
}

wxString wxcWidget::DoGenerateConnectCode() const
{
    wxString bind_code;
    wxString scopeName = DoGetScopeName();

    MapEvents_t::const_iterator iter = m_connectedEvents.begin();
    for(; iter != m_connectedEvents.end(); iter++) {
        ConnectDetails eventDetails = iter->second;

        if(eventDetails.GetFunctionNameAndSignature().IsEmpty()) {
            eventDetails.GenerateFunctionName(GetName());
        }

        wxString funcNameOnly = eventDetails.GetFunctionNameAndSignature().BeforeFirst(wxT('('));
        bool isMenuItem = (GetType() == ID_WXMENUITEM);
        bool isToolbarItem = (GetType() == ID_WXTOOLBARITEM);
        bool isInfoBarButton = (GetType() == ID_WXINFOBARBUTTON);
        if(isMenuItem) {
            // This item is a menu item
            // dont generate code for separator item
            if(this->PropertyString(PROP_WINDOW_ID) != "wxID_SEPARATOR" &&
               wxCrafter::GetToolType(PropertyString(PROP_KIND)) != wxCrafter::TOOL_TYPE_SEPARATOR) {
                wxString menu_id = GetName() + "->GetId()";
                bind_code << "this->Bind(" << eventDetails.GetEventName() << ", "
                          << "&" << scopeName << "::" << funcNameOnly << ", "
                          << "this," << menu_id << ");\n";
            }

        } else if(isInfoBarButton) {
            // Toolbar item connect
            wxString menu_id = WindowID();
            bind_code << GetParent()->GetName() << "->Bind(" << eventDetails.GetEventName() << ", &" << scopeName
                      << "::" << funcNameOnly << ", "
                      << "this, " << menu_id << ");\n";

        } else if(isToolbarItem) {
            // Toolbar item connect
            bind_code << "this->Bind(" << eventDetails.GetEventName() << ", "
                      << "&" << scopeName << "::" << funcNameOnly << ", "
                      << "this, " << WindowID() << ");\n";

        } else {
            bind_code << GetCppName() << "->Bind(";
            if(!UseIdInConnect()) {
                bind_code << eventDetails.GetEventName() << ", "
                          << "&" << scopeName << "::" << funcNameOnly << ", "
                          << "this);\n";
            } else {
                bind_code << eventDetails.GetEventName() << ", "
                          << "&" << scopeName << "::" << funcNameOnly << ", "
                          << "this, " << GetId() << ");\n";
            }
        }
    }

    WrapInIfBlockIfNeeded(bind_code);
    return bind_code;
}

wxString wxcWidget::DoGetScopeName() const
{
    const wxcWidget* wrapper = this;
    do {
        if(wrapper->IsTopWindow()) {
            return wrapper->CreateBaseclassName();
        }
        wrapper = wrapper->GetParent();
    } while(wrapper);

    return "";
}

void wxcWidget::RegisterEvent(const wxString& eventName, const wxString& className, const wxString& description,
                              const wxString& handlerName /*=""*/, const wxString& functionNameAndSig /*=""*/,
                              bool noBody /*=false*/)
{
    m_controlEvents.Add(eventName, className, description, functionNameAndSig, noBody);
}

wxString wxcWidget::XRCPrefix(const wxString& class_name) const
{
    wxString text;
    wxString className = class_name.IsEmpty() ? GetWxClassName() : class_name;
    text << "<object class=\"" << className << "\" name=\"" << wxCrafter::XMLEncode(GetName()) << "\"";
    if(!PropertyString(PROP_SUBCLASS_NAME).empty()) {
        text << " subclass=\"" << wxCrafter::XMLEncode(PropertyString(PROP_SUBCLASS_NAME)) << "\"";
    }
    text << ">";
    return text;
}

wxString wxcWidget::XRCSuffix() const { return "</object>"; }

wxString wxcWidget::XRCStyle(bool forPreview) const
{
    wxString text;
    wxString style = StyleFlags();

    if(forPreview && style.IsEmpty()) {
        style << "wxSTAY_ON_TOP";

    } else if(forPreview) {
        style << "|wxSTAY_ON_TOP";
    }

    text << "<style>" << wxCrafter::XMLEncode(style) << "</style>";
    return text;
}

wxString wxcWidget::XRCSize(bool dontEmitDefault /*=true*/) const
{
    wxString text;
    if(!dontEmitDefault || GetSize() != wxSize(-1, -1)) {
        text << "<size>" << wxCrafter::XMLEncode(Size()) << "</size>";
    }
    return text;
}

wxString wxcWidget::XRCValue() const
{
    wxString text;
    wxString value = PropertyString(PROP_VALUE);
    if(!value.empty()) {
        // Don't use wxCrafter::CDATA here because of http://trac.wxwidgets.org/ticket/10552
        // which in < wx3 results in "foo" -> "\nfoo" and very strange-looking buttons!
        text << "<value>" << wxCrafter::XMLEncode(value) << "</value>";
    }
    return text;
}

wxString wxcWidget::XRCLabel() const
{
    wxString text;
    // Don't use wxCrafter::CDATA here because of http://trac.wxwidgets.org/ticket/10552
    // which in < wx3 results in "foo" -> "\nfoo" and very strange-looking buttons!
    text << "<label>" << wxCrafter::XMLEncode(PropertyString(PROP_LABEL)) << "</label>";
    return text;
}

wxString wxcWidget::XRCContentItems(bool ensureAtLeastOneEntry) const
{
    wxArrayString options = wxCrafter::Split(PropertyString(PROP_OPTIONS), ";");
    if(options.IsEmpty() && ensureAtLeastOneEntry) {
        options.Add("Dummy Option");
    }

    wxString text;
    text << "<content>";
    for(size_t i = 0; i < options.GetCount(); i++) {
        // Don't use wxCrafter::CDATA here because of http://trac.wxwidgets.org/ticket/10552
        // which in < wx3 results in "foo" -> "\nfoo" and very strange-looking buttons!
        text << "<item>" << wxCrafter::XMLEncode(options.Item(i)) << "</item>";
    }
    text << "</content>";
    return text;
}

wxString wxcWidget::XRCSelection() const
{
    wxString text;
    // Don't use wxCrafter::CDATA here because of http://trac.wxwidgets.org/ticket/10552
    // which in < wx3 results in "foo" -> "\nfoo" and very strange-looking buttons!
    text << "<selection>" << wxCrafter::XMLEncode(PropertyString(PROP_SELECTION)) << "</selection>";
    return text;
}

ConnectDetails wxcWidget::GetEventMetaData(const wxString& eventName) const
{
    if(!m_controlEvents.GetEvents().Contains(eventName)) {
        return ConnectDetails();
    }

    return m_controlEvents.GetEvents().Item(eventName);
}

ConnectDetails wxcWidget::GetEvent(const wxString& eventName) const
{
    if(m_connectedEvents.Contains(eventName)) {
        return m_connectedEvents.Item(eventName);
    }
    return ConnectDetails();
}

wxString wxcWidget::CPPStandardWxCtor(const wxString& defaultStyle) const
{
    // For subclassed controls we must use the subclass name
    wxString cpp;
    cpp << GetName() << " = new " << GetRealClassName() << "(" << GetWindowParent() << ", " << WindowID() << ", "
        << "wxDefaultPosition, " << SizeAsString() << ", " << StyleFlags(defaultStyle) << ");\n";
    cpp << CPPCommonAttributes();
    return cpp;
}

wxString wxcWidget::CPPStandardWxCtorWithValue(const wxString& defaultStyle) const
{
    // For subclassed controls we must use the subclass name
    wxString cpp;
    cpp << GetName() << " = new " << GetRealClassName() << "(" << GetWindowParent() << ", " << WindowID() << ", "
        << ValueAsString() << ", "
        << "wxDefaultPosition, " << SizeAsString() << ", " << StyleFlags(defaultStyle) << ");\n";

    cpp << CPPCommonAttributes();
    return cpp;
}

wxString wxcWidget::CPPStandardWxCtorWithLabel(const wxString& defaultStyle) const
{
    wxString cpp;
    cpp << GetName() << " = new " << GetRealClassName() << "(" << GetWindowParent() << ", " << WindowID() << ", "
        << Label() << ", "
        << "wxDefaultPosition, " << SizeAsString() << ", " << StyleFlags(defaultStyle) << ");\n";
    cpp << CPPCommonAttributes();
    return cpp;
}

wxString wxcWidget::XRCUnknown() const
{
    wxString xrc;
    xrc << "<object class=\"unknown\" name=\"" << GetName() << "\">";
    xrc << XRCSize() << XRCCommonAttributes() << XRCSuffix();
    return xrc;
}

wxString wxcWidget::XRCCommonAttributes() const
{
    wxString xrc;
    wxString colorname = PropertyString(PROP_BG);
    if(colorname != "<Default>") {
        xrc << "<bg>" << wxCrafter::GetColourForXRC(colorname) << "</bg>";
    }

    colorname = PropertyString(PROP_FG);
    if(colorname != "<Default>") {
        xrc << "<fg>" << wxCrafter::GetColourForXRC(colorname) << "</fg>";
    }

    wxString font = wxCrafter::FontToXRC(PropertyString(PROP_FONT));
    if(!font.IsEmpty()) {
        xrc << "<font>" << font << "</font>";
    }

    wxString tip = PropertyString(PROP_TOOLTIP);
    tip.Trim().Trim(false);

    if(!tip.IsEmpty()) {
        xrc << "<tooltip>" << wxCrafter::CDATA(tip) << "</tooltip>";
    }

    // Enable / Disable + Hidden support
    bool bHide = (PropertyBool(PROP_STATE_HIDDEN) == "true");
    bool bDisable = (PropertyBool(PROP_STATE_DISABLED) == "true");
    if(bHide) {
        xrc << "<hidden>1</hidden>";
    }

    if(bDisable) {
        xrc << "<enabled>0</enabled>";
    }

    bool bFocused = (PropertyBool(PROP_HAS_FOCUS) == "true");
    if(bFocused) {
        xrc << "<focused>1</focused>";
    }

    return xrc;
}

wxString wxcWidget::CPPCommonAttributes() const
{
    wxString cpp;
    wxString instanceName;
    if(IsTopWindow()) {
        instanceName = "this";
    } else {
        instanceName = GetName();
    }

    wxString colorname = wxCrafter::ColourToCpp(PropertyString(PROP_BG));
    if(colorname.IsEmpty() == false) {
        cpp << instanceName << "->SetBackgroundColour(" << colorname << ");\n";
    }

    colorname = wxCrafter::ColourToCpp(PropertyString(PROP_FG));
    if(colorname.IsEmpty() == false) {
        cpp << instanceName << "->SetForegroundColour(" << colorname << ");\n";
    }

    wxString fontMemberName = instanceName + "Font";
    wxString font = wxCrafter::FontToCpp(PropertyString(PROP_FONT), fontMemberName);
    if(font.IsEmpty() == false && font != "wxNullFont") {
        cpp << font;
        cpp << instanceName << "->SetFont(" << fontMemberName << ");\n";
    }

    wxString tip = PropertyString(PROP_TOOLTIP);
    tip.Trim().Trim(false);
    if(tip.IsEmpty() == false) {
        cpp << instanceName << "->SetToolTip(" << wxCrafter::UNDERSCORE(tip) << ");\n";
    }

    bool bHide = (PropertyBool(PROP_STATE_HIDDEN) == "true");
    if(bHide) {
        cpp << instanceName << "->Hide();\n";
    }

    bool bDisable = (PropertyBool(PROP_STATE_DISABLED) == "true");
    if(bDisable) {
        cpp << instanceName << "->Enable(false);\n";
    }

    bool bFocused = (PropertyBool(PROP_HAS_FOCUS) == "true");
    if(bFocused) {
        cpp << instanceName << "->SetFocus();\n";
    }

    return cpp;
}

void wxcWidget::SetParent(wxcWidget* parent) { this->m_parent = parent; }

CONTROL_TYPE wxcWidget::GetWidgetType(int type)
{
    // ADD_NEW_CONTROL
    switch(type) {
    case ID_WXREARRANGELIST:
    case ID_WXANIMATIONCTRL:
    case ID_WXBANNERWINDOW:
    case ID_WXBUTTON:
    case ID_WXBITMAPBUTTON:
    case ID_WXSTATICTEXT:
    case ID_WXTEXTCTRL:
    case ID_WXSTATICBITMAP:
    case ID_WXCOMBOBOX:
    case ID_WXCHOICE:
    case ID_WXLISTBOX:
    case ID_WXCHECKBOX:
    case ID_WXRADIOBOX:
    case ID_WXRADIOBUTTON:
    case ID_WXSTATICLINE:
    case ID_WXSLIDER:
    case ID_WXGAUGE:
    case ID_WXTREECTRL:
    case ID_WXHTMLWIN:
    case ID_WXRICHTEXT:
    case ID_WXCHECKLISTBOX:
    case ID_WXTOGGLEBUTTON:
    case ID_WXSEARCHCTRL:
    case ID_WXCOLORPICKER:
    case ID_WXFONTPICKER:
    case ID_WXFILEPICKER:
    case ID_WXDIRPICKER:
    case ID_WXDATEPICKER:
    case ID_WXCALEDARCTRL:
    case ID_WXSCROLLBAR:
    case ID_WXSPINCTRL:
    case ID_WXSPINBUTTON:
    case ID_WXHYPERLINK:
    case ID_WXGENERICDIRCTRL:
    case ID_WXCUSTOMCONTROL:
    case ID_WXSTC:
    case ID_WXDATAVIEWTREECTRL:
    case ID_WXCOMMANDLINKBUTTON:
    case ID_WXWEBVIEW:
    case ID_WXGLCANVAS:
    case ID_WXMEDIACTRL:
    case ID_WXBITMAPTOGGLEBUTTON:
    case ID_WXBITMAPCOMBOBOX:
    case ID_WXSIMPLEHTMLLISTBOX:
    case ID_WXACTIVITYINDICATOR:
    case ID_WXTIMEPICKERCTRL:
        return TYPE_CONTROL;

    case ID_WXGRID:
        return TYPE_GRID;

    case ID_WXGRIDCOL:
    case ID_WXGRIDROW:
        return TYPE_GRID_CHILD;

    case ID_WXRIBBONGALLERY:
        return TYPE_RIBBON_GALLERY;

    case ID_WXRIBBONGALLERYITME:
        return TYPE_RIBBON_GALLERY_ITEM;

    case ID_WXRIBBONTOOLBAR:
        return TYPE_RIBBON_TOOLBAR;

    case ID_WXRIBBONBUTTONBAR:
        return TYPE_RIBBON_BUTTONBAR;

    case ID_WXRIBBONBUTTON:
    case ID_WXRIBBONHYBRIDBUTTON:
    case ID_WXRIBBONTOGGLEBUTTON:
    case ID_WXRIBBONDROPDOWNBUTTON:
        return TYPE_RIBBON_BUTTON;

    case ID_WXRIBBONTOOL:
    case ID_WXRIBBONHYBRIDTOOL:
    case ID_WXRIBBONTOGGLETOOL:
    case ID_WXRIBBONDROPDOWNTOOL:
    case ID_WXRIBBONTOOLSEPARATOR:
        return TYPE_RIBBON_TOOL;

    case ID_WXRIBBONPANEL:
        return TYPE_RIBBON_PANEL;

    case ID_WXRIBBONPAGE:
        return TYPE_RIBBON_PAGE;

    case ID_WXRIBBONBAR:
        return TYPE_RIBBON_BAR;

    case ID_WXPROPERTYGRIDMANAGER:
        return TYPE_PG_MGR;

    case ID_WXPGPROPERTY:
        return TYPE_PG_PROPERTY;

    case ID_WXBITMAP:
        return TYPE_BITMAP;
    case ID_WXIMAGELIST:
        return TYPE_IMGLIST;

    case ID_WXAUIMANAGER:
        return TYPE_AUI_MGR;

    case ID_WXINFOBAR:
        return TYPE_INFO_BAR;

    case ID_WXINFOBARBUTTON:
        return TYPE_INFO_BAR_BUTTON;

    case ID_WXSTDDLGBUTTONSIZER:
        return TYPE_STD_BUTTON_SIZER;

    case ID_WXSTDBUTTON:
        return TYPE_STD_BUTTON;

    case ID_WXSTATUSBAR:
        return TYPE_STATUSABR;

    case ID_WXTOOLBAR:
        return TYPE_TOOLBAR;

    case ID_WXAUITOOLBAR:
    case ID_WXAUITOOLBARTOPLEVEL:
        return TYPE_AUITOOLBAR;

    case ID_WXTOOLBARITEM:
    case ID_WXTOOLBARITEM_SEPARATOR:
    case ID_WXTOOLBARITEM_STRETCHSPACE:
    case ID_WXAUITOOLBARLABEL:
    case ID_WXAUITOOLBARITEM_SPACE:
    case ID_WXAUITOOLBARITEM_STRETCHSPACE:
        return TYPE_TOOLITEM;

    case ID_WXMENUITEM:
        return TYPE_MENUITEM;

    case ID_WXMENUBAR:
        return TYPE_MENUBAR;

    case ID_WXMENU:
        return TYPE_MENU;

    case ID_WXSUBMENU:
        return TYPE_SUBMENU;

    case ID_WXSPACER:
        return TYPE_SPACER;

    case ID_WXLISTCTRL_COL:
        return TYPE_LIST_CTRL_COL;

    case ID_WXSPLITTERWINDOW:
        return TYPE_SPLITTER_WINDOW;

    case ID_WXLISTCTRL:
        return TYPE_LIST_CTRL;

    case ID_WXTREELISTCTRL:
        return TYPE_TREE_LIST_CTRL;

    case ID_WXTREELISTCTRLCOL:
        return TYPE_TREE_LIST_CTRL_COL;

    case ID_WXTIMER:
        return TYPE_TIMER;
    case ID_WXTASKBARICON:
        return TYPE_TASKBARICON;

    case ID_WXDATAVIEWLISTCTRL:
    case ID_WXDATAVIEWTREELISTCTRL:
        return TYPE_DV_WITH_COLUMNS;

    case ID_WXDATAVIEWCOL:
        return TYPE_DV_LISTCTRLCOL;

    case ID_WXPANEL_TOPLEVEL:
    case ID_WXDIALOG:
    case ID_WXPOPUPWINDOW:
        return TYPE_FORM;

    case ID_WXFRAME:
        return TYPE_FORM_FRAME;

    case ID_WXWIZARD:
        return TYPE_WIZARD;

    case ID_WXWIZARDPAGE:
        return TYPE_WIZARD_PAGE;

    case ID_WXBOXSIZER:
    case ID_WXFLEXGRIDSIZER:
    case ID_WXSTATICBOXSIZER:
    case ID_WXGRIDSIZER:
    case ID_WXGRIDBAGSIZER:
        return TYPE_SIZER;

    case ID_WXPANEL:
    case ID_WXSCROLLEDWIN:
        return TYPE_CONTAINER;

    case ID_WXCOLLAPSIBLEPANE:
        return TYPE_COLOLAPSIBLEPANE;

    case ID_WXCOLLAPSIBLEPANE_PANE:
        return TYPE_COLOLAPSIBLEPANE_PANE;

    case ID_WXNOTEBOOK:
    case ID_WXLISTBOOK:
    case ID_WXCHOICEBOOK:
    case ID_WXTREEBOOK:
    case ID_WXTOOLBOOK:
    case ID_WXAUINOTEBOOK:
    case ID_WXSIMPLEBOOK:
        return TYPE_BOOK;

    case ID_WXSPLITTERWINDOW_PAGE:
        return TYPE_SPLITTERWIN_PAGE;

    case ID_WXPANEL_NOTEBOOK_PAGE:
        return TYPE_NOTEBOOK_PAGE;

    default:
        return TYPE_UNKNOWN;
    }
}

CONTROL_TYPE wxcWidget::GetWidgetType() const { return GetWidgetType(this->m_type); }

bool wxcWidget::IsDirectOrIndirectChildOf(wxcWidget* p) const
{
    wxcWidget* parent = this->GetParent();
    while(parent) {
        if(parent == p) {
            return true;
        }
        parent = parent->GetParent();
    }
    return false;
}

void wxcWidget::InsertBefore(wxcWidget* item, wxcWidget* insertBefore)
{
    item->SetParent(this);
    List_t::iterator iter = m_children.begin();
    for(; iter != m_children.end(); iter++) {
        if((*iter) == insertBefore) {
            m_children.insert(iter, item);
            break;
        }
    }
}

void wxcWidget::InsertAfter(wxcWidget* item, wxcWidget* insertAfter)
{
    item->SetParent(this);
    bool inserted = false;
    bool insert_next = false;

    List_t::iterator iter = m_children.begin();
    for(; iter != m_children.end(); iter++) {

        if(insert_next) {
            m_children.insert(iter, item);
            inserted = true;
            break;

        } else if((*iter) == insertAfter) {
            insert_next = true;
        }
    }

    if(insert_next && !inserted) {
        m_children.push_back(item);
    }
}
void wxcWidget::FixPaths(const wxString& cwd)
{
    for (auto& prop : m_properties) {
        if (FilePickerProperty* pb = dynamic_cast<FilePickerProperty*>(prop.second.get())) {
            pb->FixPaths(cwd);
        }
    }

    // Fix the children properties as well
    for (auto child : m_children) {
        child->FixPaths(cwd);
    }
}

bool wxcWidget::IsParentToolbar() const
{
    return GetParent() && (GetParent()->GetType() == ID_WXTOOLBAR || GetParent()->GetType() == ID_WXAUITOOLBAR ||
                           GetParent()->GetType() == ID_WXAUITOOLBARTOPLEVEL);
}

bool wxcWidget::IsLastChild() const
{
    wxcWidget* parent = GetParent();
    if(!parent) {
        return false;
    }

    const List_t& siblings = parent->GetChildren();
    const wxcWidget* lastChild = siblings.back();
    return lastChild == this;
}

wxcWidget* wxcWidget::GetTopLevel() const
{
    const wxcWidget* parent = this;

    while(parent->GetParent()) {
        parent = parent->GetParent();
    }

    return const_cast<wxcWidget*>(parent);
}

bool wxcWidget::HasMenuBar() const
{
    for (const auto* child : GetChildren()) {
        if (child->GetType() == ID_WXMENUBAR) {
            return true;
        }
    }
    return false;
}

bool wxcWidget::HasStatusBar() const
{
    for (const auto* child : GetChildren()) {
        if (child->GetType() == ID_WXSTATUSBAR) {
            return true;
        }
    }
    return false;
}

bool wxcWidget::HasToolBar() const
{
    for (const auto* child : GetChildren()) {
        if (child->GetType() == ID_WXTOOLBAR) {
            return true;
        }
    }
    return false;
}

wxString wxcWidget::XRCBitmap(const wxString& label, const wxString& bitmap) const
{
    wxString file = bitmap;
    file.Trim().Trim(false);

    if(file.IsEmpty()) {
        return "";
    }

    wxString artId, clientId, sizeHint;
    wxString xrc;
    if(wxCrafter::IsArtProviderBitmap(file, artId, clientId, sizeHint)) {
        wxString artstring;
        artstring << " stock_id=\"" << artId << "\"";
        if(!clientId.empty()) {
            artstring << " stock_client=\"" << clientId << "\"";
            xrc << "<" << label << artstring << " />";
        }
    } else {
        xrc << "<" << label << ">" << file << "</" << label << ">";
    }
    return xrc;
}

wxString wxcWidget::XRCBitmap(const wxString& labelname) const
{
    wxString file = PropertyFile(PROP_BITMAP_PATH);
    file.Trim().Trim(false);

    if(file.IsEmpty()) {
        return "";
    }

    wxString artId, clientId, sizeHint;
    wxString xrc;
    if(wxCrafter::IsArtProviderBitmap(file, artId, clientId, sizeHint)) {
        wxString artstring;
        artstring << " stock_id=\"" << artId << "\"";
        if(!clientId.empty()) {
            artstring << " stock_client=\"" << clientId << "\"";
            xrc << "<" << labelname << artstring << " />";
        }
    } else {
        xrc << "<" << labelname << ">" << file << "</" << labelname << ">";
    }
    return xrc;
}

const wxcWidget* wxcWidget::FindChildByName(const wxString& name) const { return DoFindByName(this, name); }

const wxcWidget* wxcWidget::DoFindByName(const wxcWidget* parent, const wxString& name) const
{
    if(parent->GetName() == name) {
        return parent;
    }

    for (const auto child : parent->GetChildren()) {
        const wxcWidget* match = DoFindByName(child, name);
        if(match) {
            return match;
        }
    }
    return NULL;
}

const wxcWidget* wxcWidget::FindFirstDirectChildOfType(int type) const
{
    for (const auto* child : GetChildren()) {
        if (child->GetType() == type) {
            return child;
        }
    }
    return NULL;
}

wxSize wxcWidget::GetSize() const
{
    wxString sizeString = PropertyString(PROP_SIZE);
    wxSize sz = wxCrafter::DecodeSize(sizeString);
    return sz;
}

void wxcWidget::DoGetConnectedEventsRecursively(wxcWidget::Map_t& events, const wxcWidget* wb) const
{
    for (const auto& p : wb->m_connectedEvents) {
        wxString fooname = p.second.GetFunctionNameAndSignature().BeforeFirst(wxT('('));
        if(events.count(fooname) == 0) {
            events.insert(std::make_pair(fooname, p.second));
        }
    }

    for (const auto* child : wb->m_children) {
        DoGetConnectedEventsRecursively(events, child);
    }
}

wxcWidget::Map_t wxcWidget::GetConnectedEventsRecursively() const
{
    wxcWidget::Map_t events;
    DoGetConnectedEventsRecursively(events, this);
    return events;
}

PropertyBase* wxcWidget::GetProperty(const wxString& name)
{
    if(m_properties.Contains(name)) {
        return m_properties.Item(name).get();
    }
    return NULL;
}

void wxcWidget::DelProperty(const wxString& name)
{
    m_properties.Remove(name);
}

size_t wxcWidget::SizerFlagsAsInteger() const
{
    size_t flags = 0;
    for (const auto& [_, styleInfo] : m_sizerFlags) {
        if (styleInfo.is_set) {
            flags |= styleInfo.style_bit;
        }
    }
    return flags;
}

void wxcWidget::SetStyles(size_t value)
{
    for (const auto& [_, styleInfo] : m_styles) {
        EnableStyle(styleInfo.style_name, value & styleInfo.style_bit);
    }
}

void wxcWidget::EnableSizerFlag(const wxString& flag, bool enable)
{
    static const std::map<wxString, wxArrayString> s_antiGroup = {
        { "wxALIGN_LEFT", StdToWX::ToArrayString({ "wxALIGN_CENTER_HORIZONTAL", "wxALIGN_RIGHT" }) },
        { "wxALIGN_CENTER_HORIZONTAL", StdToWX::ToArrayString({ "wxALIGN_LEFT", "wxALIGN_RIGHT" }) },
        { "wxALIGN_RIGHT", StdToWX::ToArrayString({ "wxALIGN_LEFT", "wxALIGN_CENTER_HORIZONTAL" }) },
        { "wxALIGN_TOP", StdToWX::ToArrayString({ "wxALIGN_CENTER_VERTICAL", "wxALIGN_BOTTOM" }) },
        { "wxALIGN_CENTER_VERTICAL", StdToWX::ToArrayString({ "wxALIGN_TOP", "wxALIGN_BOTTOM" }) },
        { "wxALIGN_BOTTOM", StdToWX::ToArrayString({ "wxALIGN_TOP", "wxALIGN_CENTER_VERTICAL" }) }
    };

    if (m_sizerFlags.Contains(flag)) {
        m_sizerFlags.Item(flag).is_set = enable;

        if (enable) {
            // Uncheck all "grouped" flags
            if (auto iter = s_antiGroup.find(flag); iter != s_antiGroup.end()) {
                for (const auto& item : iter->second) {
                    EnableSizerFlag(item, false); // since it false, the recursion can not occur more than once
                }
            }
        }
    }
}

int wxcWidget::SizerFakeValue(int realValue)
{
    if(s_sizerFlagsValue.count(realValue) == 0) {
        return 0;
    }
    return s_sizerFlagsValue[realValue];
}

void wxcWidget::GenerateAdditionalFiles(wxStringMap_t& additionalFiles) const { wxUnusedVar(additionalFiles); }

bool wxcWidget::IsAuiPane() const { return GetParent() && GetParent()->GetType() == ID_WXAUIMANAGER; }

wxString wxcWidget::WrapInAuiPaneXRC(const wxString& objXRC) const { return m_auiPaneInfo.ToXRC(objXRC); }

size_t wxcWidget::StyleFlagsAsInteger() const
{
    size_t flags = 0;
    for (const auto& [_, styleInfo] : m_styles) {
        if (styleInfo.is_set) {
            flags |= styleInfo.style_bit;
        }
    }
    return flags;
}

int wxcWidget::PropertyInt(const wxString& propname, int defval) const
{
    if(m_properties.Contains(propname)) {
        wxString value = m_properties.Item(propname)->GetValue();
        return wxCrafter::ToNumber(value, defval);

    } else {
        return defval;
    }
}

bool wxcWidget::IsAuiManaged() const
{
    for (const auto* child : m_children) {
        if (child->GetType() == ID_WXAUIMANAGER) {
            return true;
        }
    }
    return false;
}

void wxcWidget::DoGetCustomControlsName(const wxcWidget* widget, wxArrayString& controls) const
{
    if(widget->GetType() == ID_WXCUSTOMCONTROL) {
        const CustomControlWrapper* cs = dynamic_cast<const CustomControlWrapper*>(widget);
        if(cs) {
            if(controls.Index(cs->GetTemplInfoName()) == wxNOT_FOUND) {
                controls.Add(cs->GetTemplInfoName());
            }
        }
    }

    for (const auto* child : widget->GetChildren()) {
        DoGetCustomControlsName(child, controls);
    }
}

void wxcWidget::GetCustomControlsName(wxArrayString& controls) const { DoGetCustomControlsName(this, controls); }

wxString wxcWidget::GetRealClassName() const
{
    wxString subclass = PropertyString(PROP_SUBCLASS_NAME);
    subclass.Trim().Trim(false);

    if(!subclass.IsEmpty()) {
        return subclass;
    } else {
        return GetWxClassName();
    }
}

bool wxcWidget::DoCheckNameUniqueness(const wxString& name, const wxcWidget* widget) const
{
    if(widget->GetRealName() == name) {
        return false;
    }

    for (const auto* child : widget->GetChildren()) {
        if (!DoCheckNameUniqueness(name, child)) {
            return false;
        }
    }
    return true;
}

bool wxcWidget::IsNameUnique(const wxString& name) const { return DoCheckNameUniqueness(name, this); }

wxcWidget* wxcWidget::Copy(enum DuplicatingOptions nametypesToChange, const std::set<wxString>& existingNames,
                           const wxString& chosenName, const wxString& chosenInheritedName,
                           const wxString& chosenFilename) const
{
    wxcWidget* widget = Clone();
    widget->DoDeepCopy(*this, nametypesToChange, existingNames, chosenName, chosenInheritedName, chosenFilename);
    DoCopyChildren(widget, nametypesToChange, existingNames);
    return widget;
}

void wxcWidget::DoCopyChildren(wxcWidget* widget, enum DuplicatingOptions nametypesToChange,
                               const std::set<wxString>& existingNames) const
{
    for (const auto* child : m_children) {
        wxcWidget* clone = child->Clone();
        clone->DoDeepCopy(*child, nametypesToChange, existingNames);
        widget->AddChild(clone);
        if (!child->GetChildren().empty()) {
            child->DoCopyChildren(clone, nametypesToChange, existingNames);
        }
    }
}

void wxcWidget::DoDeepCopy(const wxcWidget& rhs, enum DuplicatingOptions nametypesToChange,
                           const std::set<wxString>& existingNames, const wxString& chosenName,
                           const wxString& chosenInheritedName, const wxString& chosenFilename)
{
    for (const auto& prop : rhs.m_properties) {
        if (this->m_properties.Contains(prop.first)) {

            if (prop.first == PROP_NAME) {
                // The name should be different, unless we're just copying to the clipboard
                // or (optionally) unless we're duplicating a TLW or pasting into a different TLW
                wxString newname;
                if(!chosenName.empty()) { // Always use any supplied name
                    newname = chosenName;

                } else {
                    newname << prop.second->GetValue();
                    if(IsTopWindow() || existingNames.count(newname) ||
                       ((nametypesToChange & DO_renameAllChildren) ||
                        (nametypesToChange & DO_renameAllChildrenExceptUsernamed && wxIsdigit(newname.Last())))) {
                        newname << ++m_copyCounter; // Append a 'unique'ing number to the name e.g. foo12 -> foo1234
                    }
                }

                this->m_properties.Item(prop.first)->SetValue(newname);

            } else if (prop.first == PROP_FILE) {

                wxString filename;
                if(!chosenFilename.empty()) { // Always use any supplied name
                    filename = chosenFilename;
                } else {
                    filename << prop.second->GetValue();

                    if(!filename.empty() && (nametypesToChange != DO_renameNone)) {
                        filename << ++m_copyCounter;
                    }
                }

                // update the file name property
                this->m_properties.Item(prop.first)->SetValue(filename);

            } else if (prop.first == PROP_INHERITED_CLASS) {

                wxString classname;
                if(!chosenInheritedName.empty()) { // Always use any supplied name
                    classname = chosenInheritedName;
                } else {
                    classname << prop.second->GetValue();

                    if(!classname.empty() && (nametypesToChange != DO_renameNone)) {
                        // Don't increment here: it's sensible for classname to use the same suffix as filename
                        classname << m_copyCounter;
                    }
                }

                this->m_properties.Item(prop.first)->SetValue(classname);

            } else {
                this->m_properties.Item(prop.first)->SetValue(prop.second->GetValue());
            }
        }
    }

    if(nametypesToChange & DO_copyEventsToo) {
        // Doing a simple m_connectedEvents = rhs.m_connectedEvents (even using wxString::Clone) failed here as,
        // strangely, if ConnectDetails::m_functionNameAndSignature
        // of either duplicate was later changed, that change was shown in *both* controls in their EventsTableListView.
        // The generated code was still correct, though :/
        for (const auto& p : rhs.m_connectedEvents) {
            AddEvent(p.second);
        }
    }

    for (const auto& p : rhs.m_styles) {
        if (this->m_styles.Contains(p.first)) {
            this->m_styles.Item(p.first) = p.second;
        }
    }

    for (const auto& p : rhs.m_sizerFlags) {
        if (this->m_sizerFlags.Contains(p.first)) {
            this->m_sizerFlags.Item(p.first) = p.second;
        }
    }
    this->m_auiPaneInfo = rhs.m_auiPaneInfo;
    this->m_sizerItem.SetProportion(rhs.m_sizerItem.GetProportion());
    this->m_sizerItem.SetBorder(rhs.m_sizerItem.GetBorder());
    this->m_gbPos = rhs.m_gbPos;
    this->m_gbSpan = rhs.m_gbSpan;
}

wxString wxcWidget::DoGenerateCppCtorCode_End() const { return ""; }

wxString wxcWidget::GetId() const
{
    wxString winId = PropertyString(PROP_WINDOW_ID).Trim().Trim(false);
    static wxRegEx reXrcId("XRCID *\\(\"[^\"]*\"\\)");

    if(reXrcId.IsValid() && reXrcId.Matches(winId)) {
        /// this window id already contains XRCID("..") wrapper
        /// return it as it is
        return winId;
    }

    /// Not an XRCID string, check if it is one of the stock Id
    if(WinIdProperty::m_winIdSet.count(winId)) {
        return winId;
    }

    /// Is it a number?
    long nWinID = -1;
    if(winId.ToCLong(&nWinID)) {
        // a number
        return winId;
    }

    // A random string
    wxcCodeGeneratorHelper::Get().AddWindowId(winId);
    return winId;
}

void wxcWidget::Reparent(wxcWidget* parent)
{
    // detach us from the old parent
    if(GetParent()) {
        GetParent()->RemoveChild(this);
    }

    parent->AddChild(this);
}

void wxcWidget::StoreNames(std::set<wxString>& store)
{
    store.insert(GetName());

    for (auto child : m_children) {
        child->StoreNames(store);
    }
}

bool wxcWidget::HasStyle(int styleBit) const
{
    MapStyles_t::ConstIterator iter = m_styles.Begin();
    for(; iter != m_styles.End(); ++iter) {
        if(iter->second.style_bit == styleBit && iter->second.is_set) {
            return true;
        }
    }
    return false;
}

void wxcWidget::WrapInIfBlockIfNeeded(wxString& code) const
{
    if(!GetCondname().IsEmpty()) {
        wxCrafter::WrapInIfBlock(GetCondname(), code);
    }
}

void wxcWidget::SetIfBlockCond(const wxString& condname)
{
    m_condname = condname;

    // Set this condition to all this control events
    EventsDatabase::MapEvents_t& events = m_controlEvents.GetEvents();
    EventsDatabase::MapEvents_t::Iterator iter = events.Begin();
    for(; iter != events.End(); ++iter) {
        iter->second.SetIfBlock(m_condname);
    }
}

wxString wxcWidget::BaseDoGenerateClassMember() const
{
    wxString memberCode;

    if(KeepAsClassMember()) {

        // For subclasses controls we must use the subclass name
        wxString classname = GetRealClassName();
        if(!IsTopWindow() && !classname.IsEmpty()) {

            memberCode << "    " << classname << "* " << GetName() << ";";
            WrapInIfBlockIfNeeded(memberCode);
        }
    }
    return memberCode;
}

bool wxcWidget::IsParentAuiToolbar() const
{
    return GetParent() &&
           (GetParent()->GetType() == ID_WXAUITOOLBAR || GetParent()->GetType() == ID_WXAUITOOLBARTOPLEVEL);
}

void wxcWidget::DoGenerateGetters(wxString& decl) const
{
    if(KeepAsClassMember()) {
        switch(GetWidgetType()) {
        case TYPE_CONTROL:
        case TYPE_LIST_CTRL:
        case TYPE_CONTAINER:
        case TYPE_BOOK:
        case TYPE_WIZARD_PAGE:
        case TYPE_NOTEBOOK_PAGE:
        case TYPE_SPLITTERWIN_PAGE:
        case TYPE_SPLITTER_WINDOW:
        case TYPE_MENUBAR:
        case TYPE_TOOLBAR:
        case TYPE_STATUSABR:
        case TYPE_AUITOOLBAR:
        case TYPE_DV_WITH_COLUMNS:
        case TYPE_COLOLAPSIBLEPANE:
        case TYPE_INFO_BAR:
        case TYPE_AUI_MGR:
        case TYPE_TIMER:
        case TYPE_PG_MGR:
        case TYPE_RIBBON_BAR:
        case TYPE_GRID:
        case TYPE_TREE_LIST_CTRL:
        case TYPE_TASKBARICON: {
            if(!IsTopWindow()) {
                wxString code;
                wxString memberName(GetName());

                if(memberName.StartsWith("m_")) {
                    memberName.Remove(0, 2);

                } else if(memberName.StartsWith("_")) {
                    memberName.Remove(0, 1);
                }

                wxString firstChar = memberName.Mid(0, 1);
                firstChar.MakeUpper();
                memberName.replace(0, 1, firstChar);

                code << "    " << GetRealClassName() << "* Get" << memberName << "() { return " << GetName() << "; }\n";
                decl << code;
            }
        } break;
        default:
            break;
        }
    }
}

wxString wxcSizerItem::GetBorderScaled() const { return wxString() << "WXC_FROM_DIP(" << GetBorder() << ")"; }
