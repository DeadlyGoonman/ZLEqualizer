; Inno Setup script for the ZL Equalizer fork (Windows VST3).
; Build: ISCC /DBundleDir=<path to "ZL Equalizer 2.vst3" folder> /DAppVersion=fork-N /DOutName=<exe name> /O<outdir> fork_installer.iss
#ifndef BundleDir
  #error BundleDir not defined
#endif
#ifndef AppVersion
  #define AppVersion "dev"
#endif
#ifndef OutName
  #define OutName "ZLEqualizer-installer"
#endif

[Setup]
AppId={{6F1D3E52-8A4B-4C7E-9B0D-5E2A7C1F4D93}
AppName=ZL Equalizer 2 (fork)
AppVersion={#AppVersion}
AppPublisher=DeadlyGoonman
DefaultDirName={commoncf64}\VST3\ZL Equalizer 2.vst3
DisableDirPage=yes
DisableProgramGroupPage=yes
DisableReadyPage=yes
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputBaseFilename={#OutName}
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
SetupIconFile=installer.ico
UninstallDisplayName=ZL Equalizer 2 (fork) VST3

[Files]
Source: "{#BundleDir}\*"; DestDir: "{app}"; Flags: recursesubdirs createallsubdirs ignoreversion

[Messages]
FinishedLabel=ZL Equalizer 2 (fork) {#AppVersion} was installed to the VST3 folder. Close your DAW before installing, then rescan plugins.
