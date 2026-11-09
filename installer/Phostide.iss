; Phostide — per-user Windows installer (Inno Setup 6)
; Compile via scripts/package-release.ps1 (ISCC).

#define MyAppName "Phostide"
#ifndef MyAppVersion
  #define MyAppVersion "0.1.0"
#endif
#define MyAppPublisher "Doticca"
#define MyAppURL "https://phostide.doticca.com"
#define MyAppExeName "Phostide.exe"

#ifndef StageDir
  #define StageDir "..\out\stage\Phostide-{#MyAppVersion}-win-x64"
#endif
#ifndef OutputDir
  #define OutputDir "..\out"
#endif
#ifndef SetupIcon
  #define SetupIcon "..\assets\brand\setup-phostide.ico"
#endif

[Setup]
AppId={{A7C3E9F1-4B2D-4E8A-9C1F-6D5E0B8A2F31}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={localappdata}\Programs\{#MyAppName}
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
LicenseFile=..\LICENSE
OutputDir={#OutputDir}
OutputBaseFilename=Phostide-Setup-{#MyAppVersion}-win-x64
SetupIconFile={#SetupIcon}
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
UninstallDisplayIcon={app}\{#MyAppExeName}
CloseApplications=yes
RestartApplications=no
VersionInfoVersion={#MyAppVersion}.0
VersionInfoCompany={#MyAppPublisher}
VersionInfoProductName={#MyAppName}
VersionInfoCopyright=Copyright (C) 2026 {#MyAppPublisher}

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "{#StageDir}\Phostide.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#StageDir}\ExplorerTAP.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#StageDir}\LICENSE"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#StageDir}\README.md"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
Type: filesandordirs; Name: "{app}"
