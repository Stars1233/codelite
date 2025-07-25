#include "timer_wrapper.h"

#include "allocator_mgr.h"
#include "bool_property.h"
#include "category_property.h"
#include "int_property.h"
#include "wxgui_helpers.h"

TimerWrapper::TimerWrapper()
    : wxcWidget(ID_WXTIMER)
{
    m_styles.Clear();
    m_properties.Clear();
    m_sizerFlags.Clear();

    SetPropertyString(_("Common Settings"), "wxTimer");
    Add<CategoryProperty>(_("wxTimer"));
    Add<StringProperty>(PROP_NAME, "", _("Control name"));
    Add<IntProperty>(PROP_INTERVAL, 1000, _("Sets the current interval for the timer (in milliseconds)"));
    Add<BoolProperty>(PROP_START_TIMER, true, _("Start the timer"));
    Add<BoolProperty>(PROP_ONE_SHOT_TIMER,
                      false,
                      _("A one shot timer - sets whether the timer event is called repeatedly or only once"));
    RegisterEvent("wxEVT_TIMER", "wxTimerEvent", _("Process a timer event"));

    m_namePattern = "m_timer";
    SetName(GenerateName());
}

wxcWidget* TimerWrapper::Clone() const { return new TimerWrapper(); }

wxString TimerWrapper::CppCtorCode() const
{
    int interval = wxCrafter::ToNumber(PropertyString(PROP_INTERVAL), 1000);
    wxString cppCode;
    cppCode << GetName() << " = new " << GetRealClassName() << ";\n";
    if(PropertyBool(PROP_START_TIMER) == "true") {
        cppCode << GetName() << "->Start(" << interval << ", " << PropertyBool(PROP_ONE_SHOT_TIMER) << ");\n";
    }
    return cppCode;
}

void TimerWrapper::GetIncludeFile(wxArrayString& headers) const { headers.Add("#include <wx/timer.h>"); }

wxString TimerWrapper::GetWxClassName() const { return "wxTimer"; }

void TimerWrapper::ToXRC(wxString& text, XRC_TYPE type) const
{
    // Not supported
    wxUnusedVar(text);
    wxUnusedVar(type);
}

wxString TimerWrapper::CppDtorCode() const
{
    wxString cppCode;
    cppCode << "    " << GetName() << "->Stop();\n";
    cppCode << "    wxDELETE( " << GetName() << " );\n";
    return cppCode;
}
