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
; force = Restart Manager closes us without a stuck "applications still running" prompt.
CloseApplications=force
RestartApplications=no
; Same folder + AppId on upgrade so Windows keeps treating this as one app.
UsePreviousAppDir=yes
; Do not prefix Local\ — Inno tries Local\ and Global\ itself.
AppMutex=Phostide.SingleInstance
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
; AppUserModelID must stay stable so taskbar / tray prefs survive updates.
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; AppUserModelID: "Doticca.Phostide"
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon; AppUserModelID: "Doticca.Phostide"

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
Type: filesandordirs; Name: "{app}"

[Code]
function PostMessageW(hWnd: HWND; Msg, wParam, lParam: LongInt): BOOL;
  external 'PostMessageW@user32.dll stdcall';

{ Never Exec Phostide.exe here — builds before 0.1.4 ignore /exit and START the app,
  which spawned MessageBoxes behind the Setup wizard. }
procedure CloseRunningPhostide;
var
  Wnd: HWND;
  I: Integer;
  ResultCode: Integer;
begin
  Wnd := FindWindowByClassName('PhostideTray');
  if Wnd <> 0 then
  begin
    PostMessageW(Wnd, 16, 0, 0); { WM_CLOSE }
    for I := 1 to 50 do
    begin
      if FindWindowByClassName('PhostideTray') = 0 then
        Break;
      Sleep(100);
    end;
  end;

  { Last resort if the tray ignored WM_CLOSE. }
  if FindWindowByClassName('PhostideTray') <> 0 then
    Exec('taskkill.exe', '/IM Phostide.exe /F', '', SW_HIDE,
         ewWaitUntilTerminated, ResultCode);
end;

{ Explorer keeps ExplorerTAP.dll mapped after Phostide exits (XAML Diagnostics
  pins it). Updates must bounce the shell so the new TEMP copy can load. }
procedure RestartExplorerShell;
var
  ResultCode: Integer;
  I: Integer;
begin
  if FindWindowByClassName('Shell_TrayWnd') <> 0 then
  begin
    Exec('taskkill.exe', '/IM explorer.exe /F', '', SW_HIDE,
         ewWaitUntilTerminated, ResultCode);
    Sleep(1500);
  end;

  Exec(ExpandConstant('{win}\explorer.exe'), '', '', SW_SHOWNORMAL, ewNoWait,
       ResultCode);

  { Wait until the tray exists, then settle so ixde can bind on first launch. }
  for I := 1 to 80 do
  begin
    if FindWindowByClassName('Shell_TrayWnd') <> 0 then
      Break;
    Sleep(250);
  end;
  Sleep(8000);
end;

function PrepareToInstall(var NeedsRestart: Boolean): String;
begin
  Result := '';
  NeedsRestart := False;
  CloseRunningPhostide;
  if Assigned(WizardForm) then
    WizardForm.BringToFront;
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssPostInstall then
  begin
    { Files are on disk; bounce Explorer before [Run] starts Phostide. }
    RestartExplorerShell;
  end;
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  if CurUninstallStep = usUninstall then
  begin
    CloseRunningPhostide;
    { Drop the in-shell TAP so uninstall leaves a stock taskbar. }
    RestartExplorerShell;
  end;
end;
