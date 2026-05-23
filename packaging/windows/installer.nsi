; Nounours Player NSIS Installer
; Usage: makensis -DVERSION=x.y.z -DDEPLOY_DIR="C:\path\deploy" -DICO_PATH="C:\path\logo.ico" -DOUT_DIR="C:\output" installer.nsi

!include "MUI2.nsh"

Name "Nounours Player"
OutFile "${OUT_DIR}\nounoursplayer-${VERSION}-win-setup.exe"
InstallDir "$PROGRAMFILES64\Nounours Player"
InstallDirRegKey HKCU "Software\NounoursPlayer" ""
RequestExecutionLevel admin

!define MUI_ICON "${ICO_PATH}"
!define MUI_UNICON "${ICO_PATH}"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

Section "Install"
    SetOutPath "$INSTDIR"
    File /r "${DEPLOY_DIR}\*"

    CreateShortCut "$DESKTOP\Nounours Player.lnk" "$INSTDIR\nounours-player.exe" "" "$INSTDIR\nounours-player.exe" 0

    CreateDirectory "$SMPROGRAMS\Nounours Player"
    CreateShortCut "$SMPROGRAMS\Nounours Player\Nounours Player.lnk" "$INSTDIR\nounours-player.exe" "" "$INSTDIR\nounours-player.exe" 0

    WriteUninstaller "$INSTDIR\Uninstall.exe"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\NounoursPlayer" "DisplayName" "Nounours Player"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\NounoursPlayer" "UninstallString" "$INSTDIR\Uninstall.exe"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\NounoursPlayer" "DisplayIcon" "$INSTDIR\nounours-player.exe"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\NounoursPlayer" "DisplayVersion" "${VERSION}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\NounoursPlayer" "Publisher" "Nounours Player"
SectionEnd

Section "Uninstall"
    RMDir /r "$INSTDIR"
    Delete "$DESKTOP\Nounours Player.lnk"
    Delete "$SMPROGRAMS\Nounours Player\Nounours Player.lnk"
    RMDir "$SMPROGRAMS\Nounours Player"
    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\NounoursPlayer"
SectionEnd
