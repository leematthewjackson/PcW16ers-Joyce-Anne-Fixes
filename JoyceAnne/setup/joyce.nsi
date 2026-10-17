
Name "JOYCE"
OutFile "joycesetup.exe"
InstallDir $PROGRAMFILES\Joyce
;InstalDirRegKey HKLM "Software\jce@seasip\Joyce" "Install_Dir"

; Pages

Page components
Page directory
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

; Files to install

Section "Programs"
  SectionIn RO
  SetOutPath $INSTDIR
  File "../bin/xjoyce.exe"
  File "../bin/xanne.exe"
  File "../LibDsk/tools/dskid.exe"
  File "../LibDsk/tools/dskform.exe"
  File "../LibDsk/tools/dsktrans.exe"
  File "../setup/SDL.dll"
  File "../setup/pcw.ico"
  File "../setup/pcw16.ico"
  SetOutPath $INSTDIR\Lib
  File "../share/pcw.bmp"
  File "../share/beep_fm.wav"
  File "../share/pcw16.bmp"
  File "../share/pcwkey.bmp"
  File "../share/16power2.bmp"
  File "../share/16power3.bmp"
  SetOutPath $INSTDIR\Lib\Boot
  File "../share/bootfile.emj"
  SetOutPath $INSTDIR\Lib\Disks
  File "../share/utils.dsk"
  File "../share/utils16.dsk"
  File "../share/pcw16_cpm.dsk"
  File "../share/cpm2.dsk"
  File "../share/cpm86.dsk"
  SetOutPath $INSTDIR\Lib\Guest
  File "../guest/X86/BIOS/cpm86.sys"
  File "../guest/X86/BIOS/cpm86bios.bin"
  File "../guest/Z80/BIOS/os2ccp.prl"
  File "../guest/Z80/BIOS/os3bdos.prl"
  File "../guest/Z80/BIOS/cpm2bios.bin"
  SetOutPath $INSTDIR
  WriteRegStr HKLM SOFTWARE\jce@seasip\Joyce "Install_Dir" "$INSTDIR"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Joyce" "DisplayName" "Joyce" 
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Joyce" "UninstallString" "$INSTDIR\uninstall.exe" 
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Joyce" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Joyce" "NoRepair" 1
  WriteUninstaller "uninstall.exe"

; Start menu shortcuts
  CreateDirectory "$SMPROGRAMS\Joyce"
  CreateShortCut "$SMPROGRAMS\Joyce\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
  CreateShortCut "$SMPROGRAMS\Joyce\Start JOYCE (full screen).lnk" "$INSTDIR\xjoyce.exe" "-f" "$INSTDIR\pcw.ico" 0
  CreateShortCut "$SMPROGRAMS\Joyce\Start JOYCE (windowed).lnk" "$INSTDIR\xjoyce.exe" "" "$INSTDIR\pcw.ico" 0
  CreateShortCut "$SMPROGRAMS\Joyce\JOYCE Documentation.lnk" "$INSTDIR\Docs\joyce.pdf" "" "$INSTDIR\Docs\doc.ico" 0
  CreateShortCut "$SMPROGRAMS\Joyce\Start ANNE (full screen).lnk" "$INSTDIR\xanne.exe" "-f" "$INSTDIR\pcw16.ico" 0
  CreateShortCut "$SMPROGRAMS\Joyce\Start ANNE (windowed).lnk" "$INSTDIR\xanne.exe" "" "$INSTDIR\pcw16.ico" 0
  CreateShortCut "$SMPROGRAMS\Joyce\ANNE Documentation.lnk" "$INSTDIR\Docs\anne.pdf" "" "$INSTDIR\Docs\doc.ico" 0
SectionEnd

Section "Documentation"
  SetOutPath $INSTDIR\Docs
  File "../Docs/README"
  File "../Docs/edops.txt"
  File "../Docs/edops.pdf"
  File "../Docs/hardware.pdf"
  File "../Docs/hardware.txt"
  File "../Docs/joyce.pdf"
  File "../Docs/joyce.txt"
  File "../Docs/anne.pdf"
  File "../Docs/anne.txt"
  File "../setup/doc.ico"
SectionEnd


Section "Source code"
  SetOutPath $INSTDIR\Src
  File "../joyce-2.5.5.tar.gz"

SectionEnd

; Uninstaller
Section "Uninstall"
  ; Clean registry
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Joyce"
  DeleteRegKey HKLM SOFTWARE\jce@seasip\Joyce
  ; Remove programs
  Delete $INSTDIR\xjoyce.exe
  Delete $INSTDIR\xanne.exe
  Delete $INSTDIR\dskid.exe
  Delete $INSTDIR\dskform.exe
  Delete $INSTDIR\dsktrans.exe
  Delete $INSTDIR\SDL.dll
  Delete $INSTDIR\pcw.ico
  Delete $INSTDIR\pcw16.ico
  Delete $INSTDIR\Docs\README
  Delete $INSTDIR\Docs\edops.txt
  Delete $INSTDIR\Docs\edops.pdf
  Delete $INSTDIR\Docs\hardware.txt
  Delete $INSTDIR\Docs\hardware.pdf
  Delete $INSTDIR\Docs\joyce.txt
  Delete $INSTDIR\Docs\joyce.pdf
  Delete $INSTDIR\Docs\anne.txt
  Delete $INSTDIR\Docs\anne.pdf
  Delete $INSTDIR\Docs\doc.ico
  Delete $INSTDIR\Src\joyce-2.5.5.tar.gz
  Delete $INSTDIR\Lib\pcw.bmp
  Delete $INSTDIR\Lib\pcw16.bmp
  Delete $INSTDIR\Lib\pcwkey.bmp
  Delete $INSTDIR\Lib\16power2.bmp
  Delete $INSTDIR\Lib\16power3.bmp
  Delete $INSTDIR\Lib\Boot\bootfile.emj
  Delete $INSTDIR\Lib\Disks\utils.dsk
  Delete $INSTDIR\Lib\Disks\utils16.dsk
  Delete $INSTDIR\Lib\Disks\pcw16_cpm.dsk
  Delete $INSTDIR\Lib\Disks\cpm2.dsk
  Delete $INSTDIR\Lib\Disks\cpm86.dsk
  Delete $INSTDIR\Lib\Guest\cpm86.sys
  Delete $INSTDIR\Lib\Guest\cpm86bios.bin
  Delete $INSTDIR\Lib\Guest\os2ccp.prl
  Delete $INSTDIR\Lib\Guest\os3bdos.prl
  Delete $INSTDIR\Lib\Guest\cpm2bios.bin
  Delete $INSTDIR\uninstall.exe
  ; Remove shortcuts
  Delete "$SMPROGRAMS\Joyce\*.*"
  ; Remove directories
  RMDir "$SMPROGRAMS\Joyce"
  RMDir "$INSTDIR\Lib\Boot"
  RMDir "$INSTDIR\Lib\Disks"
  RMDir "$INSTDIR\Lib\Guest"
  RMDir "$INSTDIR\Lib"
  RMDir "$INSTDIR\Src"
  RMDir "$INSTDIR\Docs"
  RMDir "$INSTDIR"

SectionEnd

