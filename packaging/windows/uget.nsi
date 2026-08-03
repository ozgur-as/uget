; uGet Windows installer (NSIS)
;
; Not built directly -- packaging/windows/make-dist.sh assembles the staging
; tree and invokes makensis with the defines below:
;
;   VERSION    version string, e.g. "2.3.0"
;   STAGEDIR   directory whose contents become $INSTDIR (bin/, share/, lib/)
;   OUTFILE    installer path to write
;   LICENSE    license text shown on the license page
;   ICON       application icon
;
; makensis runs on Linux (Fedora's mingw64-nsis), so the whole Windows chain
; -- cross-compile and packaging both -- stays on Linux.

; 64-bit Unicode installer, matching the 64-bit application. This also picks
; the amd64 stub, which is the only one Fedora's mingw64-nsis ships.
Target amd64-unicode
SetCompressor /SOLID lzma

!include "MUI2.nsh"
!include "FileFunc.nsh"

!define APPNAME    "uGet"
!define PUBLISHER  "uGet Project"
!define EXENAME    "uget-gtk.exe"
!define UNINSTKEY  "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}"

Name "${APPNAME} ${VERSION}"
OutFile "${OUTFILE}"
InstallDir "$PROGRAMFILES64\${APPNAME}"
; Reuse the previous location when upgrading.
InstallDirRegKey HKLM "Software\${APPNAME}" "InstallDir"
RequestExecutionLevel admin

VIProductVersion "${VERSION}.0"
VIAddVersionKey "ProductName"     "${APPNAME}"
VIAddVersionKey "FileDescription" "uGet Download Manager Installer"
VIAddVersionKey "FileVersion"     "${VERSION}"
VIAddVersionKey "ProductVersion"  "${VERSION}"
VIAddVersionKey "LegalCopyright"  "GNU LGPL-2.1-or-later"

!define MUI_ICON   "${ICON}"
!define MUI_UNICON "${ICON}"
!define MUI_ABORTWARNING

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "${LICENSE}"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES

!define MUI_FINISHPAGE_RUN "$INSTDIR\bin\${EXENAME}"
!define MUI_FINISHPAGE_RUN_TEXT "Run ${APPNAME}"
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

; ---------------------------------------------------------------------------

Section "${APPNAME}" SecMain
	SectionIn RO
	SetOutPath "$INSTDIR"
	File /r "${STAGEDIR}\*.*"

	; An installed copy stores its config in the user profile. The portable
	; zip ships a "uget-portable-mode" marker instead; never create it here.
	Delete "$INSTDIR\bin\uget-portable-mode"

	CreateDirectory "$SMPROGRAMS\${APPNAME}"
	CreateShortCut "$SMPROGRAMS\${APPNAME}\${APPNAME}.lnk" "$INSTDIR\bin\${EXENAME}"
	CreateShortCut "$DESKTOP\${APPNAME}.lnk" "$INSTDIR\bin\${EXENAME}"

	WriteRegStr HKLM "Software\${APPNAME}" "InstallDir" "$INSTDIR"

	; Add/Remove Programs
	WriteRegStr HKLM "${UNINSTKEY}" "DisplayName"     "${APPNAME} Download Manager"
	WriteRegStr HKLM "${UNINSTKEY}" "DisplayVersion"  "${VERSION}"
	WriteRegStr HKLM "${UNINSTKEY}" "Publisher"       "${PUBLISHER}"
	WriteRegStr HKLM "${UNINSTKEY}" "DisplayIcon"     "$INSTDIR\bin\${EXENAME}"
	WriteRegStr HKLM "${UNINSTKEY}" "UninstallString" "$INSTDIR\uninstall.exe"
	WriteRegStr HKLM "${UNINSTKEY}" "InstallLocation" "$INSTDIR"
	WriteRegDWORD HKLM "${UNINSTKEY}" "NoModify" 1
	WriteRegDWORD HKLM "${UNINSTKEY}" "NoRepair" 1

	${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
	IntFmt $0 "0x%08X" $0
	WriteRegDWORD HKLM "${UNINSTKEY}" "EstimatedSize" "$0"

	WriteUninstaller "$INSTDIR\uninstall.exe"
SectionEnd

; ---------------------------------------------------------------------------

Section "Uninstall"
	Delete "$SMPROGRAMS\${APPNAME}\${APPNAME}.lnk"
	RMDir  "$SMPROGRAMS\${APPNAME}"
	Delete "$DESKTOP\${APPNAME}.lnk"

	Delete "$INSTDIR\uninstall.exe"
	RMDir /r "$INSTDIR\bin"
	RMDir /r "$INSTDIR\share"
	RMDir /r "$INSTDIR\lib"
	; Only if nothing the user added is left behind.
	RMDir "$INSTDIR"

	DeleteRegKey HKLM "${UNINSTKEY}"
	DeleteRegKey HKLM "Software\${APPNAME}"
SectionEnd
