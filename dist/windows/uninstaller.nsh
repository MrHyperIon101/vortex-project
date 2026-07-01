Section "un.$(remove_files)" ;"un.Remove files"
  SectionIn RO

; Remove files and uninstaller
  Delete "$INSTDIR\vortex.exe"
  Delete "$INSTDIR\vortex.pdb"
  Delete "$INSTDIR\qt.conf"
  Delete "$INSTDIR\uninst_core.exe"
  Delete "$INSTDIR\uninst.exe"

  ; Remove directories used
  RMDir /r "$INSTDIR\translations"
  RMDir /r "$INSTDIR\sqldrivers"
  RMDir /r "$INSTDIR\networkinformation"
  RMDir /r "$INSTDIR\platforms"
  RMDir /r "$INSTDIR\iconengines"
  RMDir /r "$INSTDIR\imageformats"
  RMDir /r "$INSTDIR\styles"
  RMDir /r "$INSTDIR\tls"
  RMDir "$INSTDIR"
SectionEnd

Section "un.$(remove_shortcuts)" ;"un.Remove shortcuts"
  SectionIn RO
; Remove shortcuts, if any
  RMDir /r "$SMPROGRAMS\Vortex"
  Delete "$DESKTOP\Vortex.lnk"
SectionEnd

Section "un.$(remove_registry)" ;"un.Remove registry keys"
  SectionIn RO
  ; Remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Vortex"
  DeleteRegKey HKLM "Software\Vortex"
  ; Remove ProgIDs
  DeleteRegKey HKLM "Software\Classes\Vortex.File.Torrent"
  DeleteRegKey HKLM "Software\Classes\Vortex.Url.Magnet"
  System::Call 'shell32::SHChangeNotify(i ${SHCNE_ASSOCCHANGED}, i ${SHCNF_IDLIST}, p 0, p 0)'
SectionEnd

Section "un.$(remove_firewall)" UNSEC_FIREWALL ;

  DetailPrint $(remove_firewallinfo)
  nsisFirewallW::RemoveAuthorizedApplication "$INSTDIR\vortex.exe"

SectionEnd

Section /o "un.$(remove_conf)" UNSEC_CONF ;"un.Remove configuration files"

  !insertmacro UAC_AsUser_Call Function un.remove_conf_user ${UAC_SYNCREGISTERS}|${UAC_SYNCOUTDIR}|${UAC_SYNCINSTDIR}

SectionEnd

Function un.remove_conf_user

  System::Call 'shell32::SHGetKnownFolderPath(g "${FOLDERID_RoamingAppData}", i 0, p 0, *w . r1) i . r0'
  RMDir /r "$1\Vortex"
  System::Call 'ole32::CoTaskMemFree(p r1)'

FunctionEnd

Section /o "un.$(remove_cache)" UNSEC_CACHE

  !insertmacro UAC_AsUser_Call Function un.remove_cache_user ${UAC_SYNCREGISTERS}|${UAC_SYNCOUTDIR}|${UAC_SYNCINSTDIR}

SectionEnd

Function un.remove_cache_user

  System::Call 'shell32::SHGetKnownFolderPath(g "${FOLDERID_LocalAppData}", i 0, p 0, *w . r1) i . r0'
  RMDir /r "$1\Vortex"
  System::Call 'ole32::CoTaskMemFree(p r1)'

FunctionEnd

;--------------------------------
;Uninstaller Functions

Function un.onInit

  !insertmacro Init "uninstaller"
  !insertmacro MUI_UNGETLANGUAGE

  ${If} ${Silent}
    ClearErrors
    ${GetOptions} $CMDLINE "/REMOVE_FIREWALL=" $R0
    ${IfNot} ${Errors}
      ${If} $R0 == "1"
        !insertmacro SelectSection ${UNSEC_FIREWALL}
      ${Else}
        !insertmacro UnselectSection ${UNSEC_FIREWALL}
      ${EndIf}
    ${EndIf}

    ClearErrors
    ${GetOptions} $CMDLINE "/REMOVE_CONF=" $R0
    ${IfNot} ${Errors}
      ${If} $R0 == "1"
        !insertmacro SelectSection ${UNSEC_CONF}
      ${Else}
        !insertmacro UnselectSection ${UNSEC_CONF}
      ${EndIf}
    ${EndIf}

    ClearErrors
    ${GetOptions} $CMDLINE "/REMOVE_CACHE=" $R0
    ${IfNot} ${Errors}
      ${If} $R0 == "1"
        !insertmacro SelectSection ${UNSEC_CACHE}
      ${Else}
        !insertmacro UnselectSection ${UNSEC_CACHE}
      ${EndIf}
    ${EndIf}
  ${EndIf}

FunctionEnd

Function un.check_instance

  check:
  FindProcDLL::FindProc "vortex.exe"
  StrCmp $R0 "1" 0 notfound
  MessageBox MB_RETRYCANCEL|MB_ICONEXCLAMATION $(uninst_warning) /SD IDCANCEL IDRETRY check IDCANCEL canceled

  canceled:
  SetErrorLevel 15618 # WinError.h: `ERROR_PACKAGES_IN_USE`
  Abort

  notfound:

FunctionEnd

Function un.onUninstSuccess
  SetErrorLevel 0
FunctionEnd
