Var uninstallerPath

Section "-hidden"

    ;Search if Vortex is already installed.
    FindFirst $0 $1 "$uninstallerPath\uninst.exe"
    FindClose $0
    StrCmp $1 "" done

    ;Run the uninstaller of the previous install.
    DetailPrint $(inst_unist)
    ExecWait '"$uninstallerPath\uninst.exe" /S _?=$uninstallerPath'
    Delete "$uninstallerPath\uninst.exe"
    RMDir "$uninstallerPath"

    done:

SectionEnd


Section $(inst_qbt_req) ;"Vortex (required)"

  SectionIn RO

  ; Set output path to the installation directory.
  SetOutPath $INSTDIR
  ; Put files there
  File "${QBT_DIST_DIR}\vortex.exe"
  File "qt.conf"
  File /nonfatal /r /x "${QBT_DIST_DIR}\vortex.exe" "${QBT_DIST_DIR}\*.*"

  ; Write the installation path into the registry
  WriteRegStr HKLM "Software\Vortex" "InstallLocation" "$INSTDIR"

  ; Register Vortex as possible default program for .torrent files and magnet links
  WriteRegStr HKLM "Software\Vortex\Capabilities" "ApplicationDescription" "A BitTorrent client in Qt"
  WriteRegStr HKLM "Software\Vortex\Capabilities" "ApplicationName" "Vortex"
  WriteRegStr HKLM "Software\Vortex\Capabilities\FileAssociations" ".torrent" "Vortex.File.Torrent"
  WriteRegStr HKLM "Software\Vortex\Capabilities\UrlAssociations" "magnet" "Vortex.Url.Magnet"
  WriteRegStr HKLM "Software\RegisteredApplications" "Vortex" "Software\Vortex\Capabilities"
  ; Register Vortex ProgIDs
  WriteRegStr HKLM "Software\Classes\Vortex.File.Torrent" "" "Torrent File"
  WriteRegStr HKLM "Software\Classes\Vortex.File.Torrent\DefaultIcon" "" '"$INSTDIR\vortex.exe",1'
  WriteRegStr HKLM "Software\Classes\Vortex.File.Torrent\shell\open\command" "" '"$INSTDIR\vortex.exe" "%1"'
  WriteRegStr HKLM "Software\Classes\Vortex.Url.Magnet" "" "Magnet URI"
  WriteRegStr HKLM "Software\Classes\Vortex.Url.Magnet\DefaultIcon" "" '"$INSTDIR\vortex.exe",1'
  WriteRegStr HKLM "Software\Classes\Vortex.Url.Magnet\shell\open\command" "" '"$INSTDIR\vortex.exe" "%1"'

  WriteRegStr HKLM "Software\Classes\.torrent" "Content Type" "application/x-bittorrent"
  WriteRegStr HKLM "Software\Classes\magnet" "" "URL:Magnet URI"
  WriteRegStr HKLM "Software\Classes\magnet" "Content Type" "application/x-magnet"
  WriteRegStr HKLM "Software\Classes\magnet" "URL Protocol" ""

  System::Call 'shell32::SHChangeNotify(i ${SHCNE_ASSOCCHANGED}, i ${SHCNF_IDLIST}, p 0, p 0)'

  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Vortex" "DisplayName" "Vortex"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Vortex" "UninstallString" '"$INSTDIR\uninst.exe"'
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Vortex" "DisplayIcon" '"$INSTDIR\vortex.exe",0'
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Vortex" "Publisher" "Vortex Project"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Vortex" "URLInfoAbout" "https://vortexproject.in"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Vortex" "HelpLink" "https://vortexproject.in"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Vortex" "URLUpdateInfo" "https://vortexproject.in"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Vortex" "DisplayVersion" "${QBT_VERSION}"
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Vortex" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Vortex" "NoRepair" 1
  WriteUninstaller "uninst.exe"
  ${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
  IntFmt $0 "0x%08X" $0
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Vortex" "EstimatedSize" "$0"

SectionEnd

; Optional section (can be disabled by the user)
Section /o $(inst_desktop) ;"Create Desktop Shortcut"

  CreateShortCut "$DESKTOP\Vortex.lnk" "$INSTDIR\vortex.exe"

SectionEnd

Section $(inst_startmenu) ;"Create Start Menu Shortcut"

  CreateDirectory "$SMPROGRAMS\Vortex"
  CreateShortCut "$SMPROGRAMS\Vortex\Vortex.lnk" "$INSTDIR\vortex.exe"
  CreateShortCut "$SMPROGRAMS\Vortex\$(inst_uninstall_link_description).lnk" "$INSTDIR\uninst.exe"

SectionEnd

Section /o $(inst_startup) ;"Start Vortex on Windows start up"

  !insertmacro UAC_AsUser_Call Function inst_startup_user ${UAC_SYNCREGISTERS}|${UAC_SYNCOUTDIR}|${UAC_SYNCINSTDIR}

SectionEnd

Function inst_startup_user

  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Run" "Vortex" "$INSTDIR\vortex.exe"

FunctionEnd

Section $(inst_firewall)

  DetailPrint $(inst_firewallinfo)
  nsisFirewallW::AddAuthorizedApplication "$INSTDIR\vortex.exe" "Vortex"

SectionEnd

Section $(inst_pathlimit) ;"Disable Windows path length limit (260 character MAX_PATH limitation, requires Windows 10 1607 or later)"

  WriteRegDWORD HKLM "SYSTEM\CurrentControlSet\Control\FileSystem" "LongPathsEnabled" 1

SectionEnd

;--------------------------------

Function .onInit

  !insertmacro Init "installer"
  !insertmacro MUI_LANGDLL_DISPLAY

  ${IfNot} ${AtLeastWaaS} 1809 ; Windows 10 (1809) / Windows Server 2019. Min supported version by Qt6
    MessageBox MB_OK|MB_ICONEXCLAMATION $(inst_requires_win10) /SD IDOK
    SetErrorLevel 1654 # WinError.h: `ERROR_INSTALL_REJECTED`
    Abort
  ${EndIf}

  ${IfNot} ${RunningX64}
    MessageBox MB_OK|MB_ICONEXCLAMATION $(inst_requires_64bit) /SD IDOK
    SetErrorLevel 1654 # WinError.h: `ERROR_INSTALL_REJECTED`
    Abort
  ${EndIf}

  ; check installer and current system architecture
  ${If} "${QBT_CPU_ARCH}" == "arm64"
  ${AndIf} ${IsNativeAMD64}  ; installing arm64 binary on x64
    MessageBox MB_OK|MB_ICONEXCLAMATION $(inst_arch_mismatch_arm64_on_x64) /SD IDOK
    SetErrorLevel 1654 # WinError.h: `ERROR_INSTALL_REJECTED`
    Abort
  ${EndIf}

  ;Search if Vortex is already installed.
  FindFirst $0 $1 "$INSTDIR\uninst.exe"
  FindClose $0
  StrCmp $1 "" done

  ;Copy old value to var so we can call the correct uninstaller
  StrCpy $uninstallerPath $INSTDIR

  ;Ask user to Upgrade, Reinstall, or Cancel
  MessageBox MB_YESNOCANCEL|MB_ICONQUESTION "Vortex is already installed at $INSTDIR.$\n$\nDo you want to Upgrade the existing installation?$\n$\nClick Yes to Upgrade (installs over the old version, preserving settings).$\nClick No to Reinstall (fully uninstalls the old version first).$\nClick Cancel to abort." /SD IDYES IDYES upgrade IDNO reinstall
  Goto cancel

  upgrade:
    StrCpy $uninstallerPath ""
    Goto done

  reinstall:
    Goto done

  cancel:
    Quit

  done:

FunctionEnd

Function check_instance

  check:
  FindProcDLL::FindProc "vortex.exe"
  StrCmp $R0 "1" 0 notfound
  MessageBox MB_RETRYCANCEL|MB_ICONEXCLAMATION $(inst_warning) /SD IDCANCEL IDRETRY check IDCANCEL canceled

  canceled:
  SetErrorLevel 15618 # WinError.h: `ERROR_PACKAGES_IN_USE`
  Abort

  notfound:

FunctionEnd

Function PageFinishRun

  !insertmacro UAC_AsUser_ExecShell "" "$INSTDIR\vortex.exe" "" "" ""

FunctionEnd

Function .onInstSuccess
  SetErrorLevel 0
FunctionEnd
