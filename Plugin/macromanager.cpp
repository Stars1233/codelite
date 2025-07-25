//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
// copyright            : (C) 2014 Eran Ifrah
// file name            : macromanager.cpp
//
// -------------------------------------------------------------------------
// A
//              _____           _      _     _ _
//             /  __ \         | |    | |   (_) |
//             | /  \/ ___   __| | ___| |    _| |_ ___
//             | |    / _ \ / _  |/ _ \ |   | | __/ _ )
//             | \__/\ (_) | (_| |  __/ |___| | ||  __/
//              \____/\___/ \__,_|\___\_____/_|\__\___|
//
//                                                  F i l e
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

#include "macromanager.h"

#include "IWorkspace.h"
#include "FileSystemWorkspace/clFileSystemWorkspace.hpp"
#include "build_config.h"
#include "clWorkspaceManager.h"
#include "environmentconfig.h"
#include "globals.h"
#include "imanager.h"
#include "project.h"
#include "workspace.h"
#include "wxStringHash.h"
#include "fileutils.h"

#include <wx/regex.h>

MacroManager::MacroManager() {}

MacroManager::~MacroManager() {}

MacroManager* MacroManager::Instance()
{
    static MacroManager ms_instance;
    return &ms_instance;
}

wxString MacroManager::Expand(const wxString& expression, IManager* manager, const wxString& project,
                              const wxString& confToBuild)
{
    return DoExpand(expression, manager, project, true, confToBuild);
}

wxString MacroManager::Replace(const wxString& inString, const wxString& variableName, const wxString& replaceWith,
                               bool bIgnoreCase)
{
    size_t flags = wxRE_DEFAULT;
    if(bIgnoreCase) {
        flags |= wxRE_ICASE;
    }

    wxString strRe1;
    wxString strRe2;
    wxString strRe3;
    wxString strRe4;

    strRe1 << "\\$\\((" << variableName << ")\\)";
    strRe2 << "\\$\\{(" << variableName << ")\\}";
    strRe3 << "\\$(" << variableName << ")";
    strRe4 << "%(" << variableName << ")%";

    wxRegEx reOne(strRe1, flags);   // $(variable)
    wxRegEx reTwo(strRe2, flags);   // ${variable}
    wxRegEx reThree(strRe3, flags); // $variable
    wxRegEx reFour(strRe4, flags);  // %variable%

    wxString result = inString;
    if(reOne.Matches(result)) {
        reOne.ReplaceAll(&result, replaceWith);
    }

    if(reTwo.Matches(result)) {
        reTwo.ReplaceAll(&result, replaceWith);
    }

    if(reThree.Matches(result)) {
        reThree.ReplaceAll(&result, replaceWith);
    }

    if(reFour.Matches(result)) {
        reFour.ReplaceAll(&result, replaceWith);
    }
    return result;
}

bool MacroManager::FindVariable(const wxString& inString, wxString& name, wxString& fullname)
{
    size_t flags = wxRE_DEFAULT | wxRE_ICASE;

    wxString strRe1;
    wxString strRe2;
    wxString strRe3;
    wxString strRe4;

    strRe1 << "\\$\\(("
           << "[a-z_0-9]+"
           << ")\\)";
    strRe2 << "\\$\\{("
           << "[a-z_0-9]+"
           << ")\\}";
    strRe3 << "\\$("
           << "[a-z_0-9]+"
           << ")";
    strRe4 << "%("
           << "[a-z_0-9]+"
           << ")%";

    wxRegEx reOne(strRe1, flags);   // $(variable)
    wxRegEx reTwo(strRe2, flags);   // ${variable}
    wxRegEx reThree(strRe3, flags); // $variable
    wxRegEx reFour(strRe4, flags);  // %variable%

    if(reOne.Matches(inString)) {
        name = reOne.GetMatch(inString, 1);
        fullname = reOne.GetMatch(inString);
        return true;
    }

    if(reTwo.Matches(inString)) {
        name = reTwo.GetMatch(inString, 1);
        fullname = reTwo.GetMatch(inString);
        return true;
    }

    if(reThree.Matches(inString)) {
        name = reThree.GetMatch(inString, 1);
        fullname = reThree.GetMatch(inString);
        return true;
    }

    if(reFour.Matches(inString)) {
        name = reFour.GetMatch(inString, 1);
        fullname = reFour.GetMatch(inString);
        return true;
    }
    return false;
}

namespace
{
// the below list are macros supported by CodeLite
const std::unordered_set<wxString> CODELITE_MACROS = {
    "WorkspaceName",
    "WorkspaceConfiguration",
    "WorkspacePath",
    "WorkspaceRealPath",
    "OutputDirectory",
    "ProjectOutputFile",
    "OutputFile",
    "ProjectWorkingDirectory",
    "ProjectRunWorkingDirectory",
    "ProjectPath",
    "ProjectName",
    "IntermediateDirectory",
    "ConfigurationName",
    "OutDir",
    "ProjectFiles",
    "ProjectFilesAbs",
    "CurrentFileName",
    "CurrentFilePath",
    "CurrentFileExt",
    "CurrentFileFullName",
    "CurrentFileFullPath",
    "CurrentFileRelPath",
    "CurrentSelection",
    "CurrentSelectionRange",
    "Program",
    "User",
    "Date",
    "CodeLitePath",
    "CC",
    "CFLAGS",
    "CXX",
    "CXXFLAGS",
    "LDFLAGS",
    "AS",
    "ASFLAGS",
    "RES",
    "RESFLAGS",
    "AR",
    "MAKE",
    "IncludePath",
    "LibraryPath",
    "ResourcePath",
    "LinkLibraries",
    "SSH_AccountName",
    "SSH_Host",
    "SSH_User",
};
} // namespace

wxString MacroManager::ExpandNoEnv(const wxString& expression, const wxString& project, const wxString& confToBuild)
{
    return DoExpand(expression, NULL, project, false, confToBuild);
}

wxString MacroManager::DoExpand(const wxString& expression, IManager* manager, const wxString& project, bool applyEnv,
                                const wxString& confToBuild)
{
    wxString expandedString(expression);
    clCxxWorkspace* workspace = nullptr;

    if(!manager) {
        manager = clGetManager();
    }

    wxString wspName;
    wxString wspConfig;
    wxString wspPath;
    wxString program_to_run;

    // ssh macros
    wxString ssh_host;
    wxString account_name;
    wxString ssh_user;

    auto w = clWorkspaceManager::Get().GetWorkspace();
    if(w && w->IsRemote()) {
        wxString ssh_account = w->GetSshAccount();
        auto account = SSHAccountInfo::LoadAccount(ssh_account);
        ssh_host = account.GetHost();
        account_name = ssh_account;
        ssh_user = account.GetUsername();
    }

    if(clCxxWorkspaceST::Get()->IsOpen()) {
        wspName = clCxxWorkspaceST::Get()->GetName();
        wspConfig = clCxxWorkspaceST::Get()->GetSelectedConfig()
                        ? clCxxWorkspaceST::Get()->GetSelectedConfig()->GetName()
                        : wxString();
        wspPath = clCxxWorkspaceST::Get()->GetDir();
        workspace = clCxxWorkspaceST::Get();
    } else if(clFileSystemWorkspace::Get().IsOpen()) {
        wspName = clFileSystemWorkspace::Get().GetName();
        if(clFileSystemWorkspace::Get().GetSettings().GetSelectedConfig()) {
            program_to_run = clFileSystemWorkspace::Get().GetSettings().GetSelectedConfig()->GetExecutable();
            wspConfig = clFileSystemWorkspace::Get().GetSettings().GetSelectedConfig()->GetName();
        }
        wspPath = clFileSystemWorkspace::Get().GetDir();
    } else if(clWorkspaceManager::Get().IsWorkspaceOpened()) {
        wspPath = clWorkspaceManager::Get().GetWorkspace()->GetDir();
        wspName = clWorkspaceManager::Get().GetWorkspace()->GetName();
    }

    wxString wspRealPath = FileUtils::RealPath(wspPath, true);

    size_t retries = 0;
    wxString dummyname, dummfullname;
    while((retries < 5) && FindVariable(expandedString, dummyname, dummyname)) {
        ++retries;
        DollarEscaper de(expandedString);
        expandedString.Replace("$(WorkspaceName)", wspName);
        expandedString.Replace("$(WorkspaceConfiguration)", wspConfig);
        expandedString.Replace("$(WorkspacePath)", wspPath);
        expandedString.Replace("$(WorkspaceRealPath)", wspRealPath);

        if(workspace) {
            ProjectPtr proj = workspace->GetProject(project);
            if(proj) {
                wxString prjBuildWd;
                wxString prjRunWd;
                wxString project_name(proj->GetName());

                // make sure that the project name does not contain any spaces
                project_name.Replace(" ", "_");

                BuildConfigPtr bldConf = workspace->GetProjBuildConf(proj->GetName(), confToBuild);
                if(bldConf) {
                    bool isCustom = bldConf->IsCustomBuild();
                    expandedString.Replace("$(OutputDirectory)", bldConf->GetOutputDirectory());
                    expandedString.Replace("$(ProjectOutputFile)", bldConf->GetOutputFileName());
                    // An alias
                    expandedString.Replace("$(OutputFile)", bldConf->GetOutputFileName());
                    expandedString.Replace("$(Program)", bldConf->GetCommand());

                    // When custom build project, use the working directory set in the
                    // custom build tab, otherwise use the project file's path
                    prjBuildWd = isCustom ? bldConf->GetCustomBuildWorkingDir() : proj->GetFileName().GetPath();
                    prjRunWd = bldConf->GetWorkingDirectory();
                }

                expandedString.Replace("$(ProjectWorkingDirectory)", prjBuildWd);
                expandedString.Replace("$(ProjectRunWorkingDirectory)", prjRunWd);
                expandedString.Replace("$(ProjectPath)", proj->GetFileName().GetPath());
                expandedString.Replace("$(WorkspacePath)", workspace->GetWorkspaceFileName().GetPath());
                expandedString.Replace("$(ProjectName)", project_name);

                if(bldConf) {
                    expandedString.Replace("$(IntermediateDirectory)", bldConf->GetIntermediateDirectory());
                    expandedString.Replace("$(ConfigurationName)", bldConf->GetName());
                    expandedString.Replace("$(OutDir)", bldConf->GetIntermediateDirectory());

                    // Compiler-related variables
                    wxString cFlags = bldConf->GetCCompileOptions();
                    cFlags.Replace(";", " ");
                    expandedString.Replace("$(CC)", bldConf->GetCompiler()->GetTool("CC"));
                    expandedString.Replace("$(CFLAGS)", cFlags);

                    wxString cxxFlags = bldConf->GetCompileOptions();
                    cxxFlags.Replace(";", " ");
                    expandedString.Replace("$(CXX)", bldConf->GetCompiler()->GetTool("CXX"));
                    expandedString.Replace("$(CXXFLAGS)", cxxFlags);

                    wxString ldFlags = bldConf->GetLinkOptions();
                    ldFlags.Replace(";", " ");
                    expandedString.Replace("$(LDFLAGS)", ldFlags);

                    wxString asFlags = bldConf->GetAssmeblerOptions();
                    asFlags.Replace(";", " ");
                    expandedString.Replace("$(AS)", bldConf->GetCompiler()->GetTool("AS"));
                    expandedString.Replace("$(ASFLAGS)", asFlags);

                    wxString resFlags = bldConf->GetResCompileOptions();
                    resFlags.Replace(";", " ");
                    expandedString.Replace("$(RES)", bldConf->GetCompiler()->GetTool("ResourceCompiler"));
                    expandedString.Replace("$(RESFLAGS)", resFlags);

                    expandedString.Replace("$(AR)", bldConf->GetCompiler()->GetTool("AR"));

                    expandedString.Replace("$(MAKE)", bldConf->GetCompiler()->GetTool("MAKE"));

                    expandedString.Replace("$(IncludePath)", bldConf->GetIncludePath());
                    expandedString.Replace("$(LibraryPath)", bldConf->GetLibPath());
                    expandedString.Replace("$(ResourcePath)", bldConf->GetResCmpIncludePath());
                    expandedString.Replace("$(LinkLibraries)", bldConf->GetLibraries());
                }

                if(expandedString.Find("$(ProjectFiles)") != wxNOT_FOUND) {
                    expandedString.Replace("$(ProjectFiles)", proj->GetFilesAsString(false));
                }

                if(expandedString.Find("$(ProjectFilesAbs)") != wxNOT_FOUND) {
                    expandedString.Replace("$(ProjectFilesAbs)", proj->GetFilesAsString(true));
                }
            }
        }

        if(!program_to_run.empty()) {
            expandedString.Replace("$(Program)", program_to_run);
        }

        if(manager) {
            IEditor* editor = manager->GetActiveEditor();
            if(editor) {
                wxFileName fn(editor->GetRemotePathOrLocal());

                expandedString.Replace("$(CurrentFileName)", fn.GetName());

                wxString fpath(fn.GetPath());

                // build the relative path
                wxString rel_path = fn.GetFullPath();
                if(w) {
                    wxFileName _f(fn);
                    _f.MakeRelativeTo(w->GetDir());
                    rel_path = _f.GetFullPath(w->IsRemote() ? wxPATH_UNIX : wxPATH_NATIVE);
                }

                fpath.Replace("\\", "/");
                expandedString.Replace("$(CurrentFilePath)", fpath);
                expandedString.Replace("$(CurrentFileExt)", fn.GetExt());
                expandedString.Replace("$(CurrentFileFullName)", fn.GetFullName());
                expandedString.Replace("$(CurrentFileRelPath)", rel_path);

                wxString ffullpath(fn.GetFullPath());
                ffullpath.Replace("\\", "/");
                expandedString.Replace("$(CurrentFileFullPath)", ffullpath);
                expandedString.Replace("$(CurrentSelection)", editor->GetSelection());
                if(expandedString.Find("$(CurrentSelectionRange)") != wxNOT_FOUND) {
                    int start = editor->GetSelectionStart(), end = editor->GetSelectionEnd();

                    wxString output = wxString::Format("%i:%i", start, end);
                    expandedString.Replace("$(CurrentSelectionRange)", output);
                }
            }
        }

        // exapand common macros
        wxDateTime now = wxDateTime::Now();
        expandedString.Replace("$(User)", wxGetUserId());
        expandedString.Replace("$(Date)", now.FormatDate());

        // ssh related
        expandedString.Replace("$(SSH_AccountName)", account_name);
        expandedString.Replace("$(SSH_Host)", ssh_host);
        expandedString.Replace("$(SSH_User)", ssh_user);

        if(manager && applyEnv) {
            expandedString.Replace("$(CodeLitePath)", manager->GetInstallDirectory());

            // Apply the environment and expand the variables
            EnvSetter es(NULL, NULL, project, confToBuild);
            expandedString = manager->GetEnv()->ExpandVariables(expandedString, false);
        } else if(applyEnv) {
            expandedString = EnvironmentConfig::Instance()->ExpandVariables(expandedString, false);
        }
    }
    return expandedString;
}

bool MacroManager::IsCodeLiteMacro(const wxString& macroname) const { return CODELITE_MACROS.count(macroname) != 0; }

wxString MacroManager::ExpandFileMacros(const wxString& expression, const wxString& filepath)
{
    wxString wsp_dir;
    bool is_remote = false;
    auto workspace = clWorkspaceManager::Get().GetWorkspace();
    if(workspace) {
        is_remote = workspace->IsRemote();
        wsp_dir = workspace->GetDir();
    }

    // replace file macros
    wxString filepath_relative;
    wxString fullname;
    wxString filedir;
    wxString fullpath = filepath;

    wxFileName fn{ filepath };
    if(!wsp_dir.empty()) {
        fn.MakeRelativeTo(wsp_dir);
    }

    filepath_relative = fn.GetFullPath(is_remote ? wxPATH_UNIX : wxPATH_NATIVE);
    fullname = fn.GetFullName();
    filedir = fn.GetPath(is_remote ? wxPATH_UNIX : wxPATH_NATIVE);

    EnvSetter env;
    wxString tmp_expr = expression;
    tmp_expr.Replace("$(CurrentFileName)", fn.GetName());
    tmp_expr.Replace("$(CurrentFilePath)", filedir);
    tmp_expr.Replace("$(CurrentFileExt)", fn.GetExt());
    tmp_expr.Replace("$(CurrentFileFullName)", fullname);
    tmp_expr.Replace("$(CurrentFileFullPath)", fullpath);
    tmp_expr.Replace("$(CurrentFileRelPath)", filepath_relative);
    return tmp_expr;
}

#if 1 // DEPRECATED

#include "procutils.h"

static wxString DoExpandAllVariables(const wxString& expression,
                                     clCxxWorkspace* workspace,
                                     const wxString& projectName,
                                     const wxString& confToBuild,
                                     const wxString& fileName)
{
    wxString errMsg;
    wxString output(expression);

    size_t retries = 0;
    wxString dummyname, dummfullname;
    while ((retries < 5) && MacroManager::Instance()->FindVariable(output, dummyname, dummyname)) {
        ++retries;
        DollarEscaper de(output);
        if (workspace) {
            output.Replace("$(WorkspaceName)", workspace->GetName());
            output.Replace("$(WorkspaceConfiguration)",
                           workspace->GetSelectedConfig() ? workspace->GetSelectedConfig()->GetName() : "");
            ProjectPtr proj = workspace->FindProjectByName(projectName, errMsg);
            if (proj) {
                wxString project_name(proj->GetName());

                // make sure that the project name does not contain any spaces
                project_name.Replace(" ", "_");

                BuildConfigPtr bldConf = workspace->GetProjBuildConf(proj->GetName(), confToBuild);
                output.Replace("$(ProjectPath)", proj->GetFileName().GetPath());
                output.Replace("$(WorkspacePath)", workspace->GetWorkspaceFileName().GetPath());
                output.Replace("$(ProjectName)", project_name);

                if (bldConf) {
                    output.Replace("$(ConfigurationName)", bldConf->GetName());
                    output.Replace("$(OutputDirectory)", bldConf->GetOutputDirectory());
                    output.Replace("$(OutputFile)", bldConf->GetOutputFileName());

                    // the IntermediateDirectory variable is special, since it can contains
                    // other variables in it.
                    wxString id(bldConf->GetIntermediateDirectory());

                    // Substitute all macros from $(IntermediateDirectory)
                    id.Replace("$(ProjectPath)", proj->GetFileName().GetPath());
                    id.Replace("$(WorkspacePath)", workspace->GetWorkspaceFileName().GetPath());
                    id.Replace("$(ProjectName)", project_name);
                    id.Replace("$(ConfigurationName)", bldConf->GetName());

                    output.Replace("$(IntermediateDirectory)", id);
                    output.Replace("$(OutDir)", id);

                    // Compiler-related variables

                    wxString cFlags = bldConf->GetCCompileOptions();
                    cFlags.Replace(";", " ");
                    output.Replace("$(CC)", bldConf->GetCompiler()->GetTool("CC"));
                    output.Replace("$(CFLAGS)", cFlags);

                    wxString cxxFlags = bldConf->GetCompileOptions();
                    cxxFlags.Replace(";", " ");
                    output.Replace("$(CXX)", bldConf->GetCompiler()->GetTool("CXX"));
                    output.Replace("$(CXXFLAGS)", cxxFlags);

                    wxString ldFlags = bldConf->GetLinkOptions();
                    ldFlags.Replace(";", " ");
                    output.Replace("$(LDFLAGS)", ldFlags);

                    wxString asFlags = bldConf->GetAssmeblerOptions();
                    asFlags.Replace(";", " ");
                    output.Replace("$(AS)", bldConf->GetCompiler()->GetTool("AS"));
                    output.Replace("$(ASFLAGS)", asFlags);

                    wxString resFlags = bldConf->GetResCompileOptions();
                    resFlags.Replace(";", " ");
                    output.Replace("$(RES)", bldConf->GetCompiler()->GetTool("ResourceCompiler"));
                    output.Replace("$(RESFLAGS)", resFlags);

                    output.Replace("$(AR)", bldConf->GetCompiler()->GetTool("AR"));

                    output.Replace("$(MAKE)", bldConf->GetCompiler()->GetTool("MAKE"));

                    output.Replace("$(IncludePath)", bldConf->GetIncludePath());
                    output.Replace("$(LibraryPath)", bldConf->GetLibPath());
                    output.Replace("$(ResourcePath)", bldConf->GetResCmpIncludePath());
                    output.Replace("$(LinkLibraries)", bldConf->GetLibraries());
                }

                if (output.Find("$(ProjectFiles)") != wxNOT_FOUND) {
                    output.Replace("$(ProjectFiles)", proj->GetFilesAsString(false));
                }
                if (output.Find("$(ProjectFilesAbs)") != wxNOT_FOUND) {
                    output.Replace("$(ProjectFilesAbs)", proj->GetFilesAsString(true));
                }
            }
        }

        if (fileName.IsEmpty() == false) {
            wxFileName fn(fileName);

            output.Replace("$(CurrentFileName)", fn.GetName());

            wxString fpath(fn.GetPath());
            fpath.Replace("\\", "/");
            output.Replace("$(CurrentFilePath)", fpath);
            output.Replace("$(CurrentFileExt)", fn.GetExt());

            wxString ffullpath(fn.GetFullPath());
            ffullpath.Replace("\\", "/");
            output.Replace("$(CurrentFileFullPath)", ffullpath);
            output.Replace("$(CurrentFileFullName)", fn.GetFullName());
        }

        // exapnd common macros
        wxDateTime now = wxDateTime::Now();
        output.Replace("$(User)", wxGetUserId());
        output.Replace("$(Date)", now.FormatDate());

        if (workspace) {
            output.Replace("$(CodeLitePath)", workspace->GetStartupDir());
        }

        // call the environment & workspace variables expand function
        output = EnvironmentConfig::Instance()->ExpandVariables(output, true);
    }
    return output;
}

// This functions accepts expression and expand all variables in it
wxString ExpandAllVariables(const wxString& expression,
                            clCxxWorkspace* workspace,
                            const wxString& projectName,
                            const wxString& selConf,
                            const wxString& fileName)
{
    // add support for backticks commands
    wxString tmpExp;
    wxString noBackticksExpression;
    for (size_t i = 0; i < expression.Length(); i++) {
        if (expression.GetChar(i) == '`') {
            // found a backtick, loop over until we found the closing backtick
            wxString backtick;
            bool found(false);
            i++;
            for (; i < expression.Length(); i++) {
                if (expression.GetChar(i) == '`') {
                    found = true;
                    i++;
                    break;
                }
                backtick << expression.GetChar(i);
            }

            if (!found) {
                // dont replace anything
                clLogMessage("Syntax error in expression: " + expression + ": expecting '`'");
                return expression;
            } else {
                // expand the backtick statement
                wxString expandedBacktick = DoExpandAllVariables(backtick, workspace, projectName, selConf, fileName);

                // execute the backtick
                wxArrayString output;
                ProcUtils::SafeExecuteCommand(expandedBacktick, output);

                // concatenate the array into sAssign To:pace delimited string
                backtick.Clear();
                for (size_t xx = 0; xx < output.GetCount(); xx++) {
                    backtick << output.Item(xx).Trim().Trim(false) << " ";
                }

                // and finally concatente the result of the backtick command back to the expression
                tmpExp << backtick;
            }
        } else {
            tmpExp << expression.GetChar(i);
        }
    }

    return DoExpandAllVariables(tmpExp, workspace, projectName, selConf, fileName);
}

wxString ExpandVariables(const wxString& expression, ProjectPtr proj, IEditor* editor, const wxString& filename)
{
    wxString project_name(proj->GetName());
    wxString file = filename;
    if (file.IsEmpty() && editor) {
        file = editor->GetFileName().GetFullPath();
    }
    return ExpandAllVariables(expression, clCxxWorkspaceST::Get(), project_name, wxEmptyString, file);
}

#endif

