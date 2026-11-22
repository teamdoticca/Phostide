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
; No AppMutex: Inno's check only tells the user to close Phostide by hand (OK/Cancel).
; Setup closes it itself in PrepareToInstall / usUninstall (CloseRunningPhostide).
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
Source: "{#StageDir}\Themes.hlsl"; DestDir: "{app}"; Flags: ignoreversion
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
; Versioned ExplorerTAP copies the host loads into Explorer (unlocked after the
; uninstall shell restart). Settings live in %APPDATA% and are kept.
Type: filesandordirs; Name: "{localappdata}\Phostide\tap"
Type: dirifempty; Name: "{localappdata}\Phostide"

[Code]
function PostMessageW(hWnd: HWND; Msg, wParam, lParam: LongInt): BOOL;
  external 'PostMessageW@user32.dll stdcall';
function OpenEventW(dwDesiredAccess: DWORD; bInheritHandle: BOOL; lpName: String): THandle;
  external 'OpenEventW@kernel32.dll stdcall';
function CloseHandle(hObject: THandle): BOOL;
  external 'CloseHandle@kernel32.dll stdcall';

const
  SYNCHRONIZE = $00100000;

{ ExplorerTAP holds Local\Phostide.TAP.Ready open for Explorer's lifetime, so the
  event exists only while a TAP is mapped into the running shell (not after a
  reboot or an earlier shell restart). Keep in sync with kTapReadyEvent. }
function TapLoadedInExplorer: Boolean;
var
  H: THandle;
begin
  H := OpenEventW(SYNCHRONIZE, False, 'Local\Phostide.TAP.Ready');
  Result := H <> 0;
  if Result then
    CloseHandle(H);
end;

{ Version the loaded TAP stamped on bind (%TEMP%\Phostide\tap.version), or ''. }
function LoadedTapVersion: String;
var
  S: AnsiString;
begin
  Result := '';
  if LoadStringFromFile(ExpandConstant('{%TEMP}\Phostide\tap.version'), S) then
    Result := Trim(String(S));
end;

{ Install/upgrade: bounce only when Explorer still maps a TAP from another build.
  Same-version reinstall or no TAP loaded → the new host loads cleanly. }
function NeedsShellRestartForInstall: Boolean;
begin
  Result := TapLoadedInExplorer and (LoadedTapVersion <> '{#MyAppVersion}');
end;

{ Never Exec Phostide.exe here — builds before 0.1.4 ignore /exit and START the app,
  which spawned MessageBoxes behind the Setup wizard. }
{ Never MsgBox here — it can sit behind the wizard and look like a hang.
  Use the wizard/uninstall status line only (always visible in-window). }
procedure SetShellRestartStatus(const Msg: String);
begin
  if Assigned(WizardForm) then
  begin
    WizardForm.StatusLabel.Caption := Msg;
    WizardForm.StatusLabel.Update;
    WizardForm.BringToFront;
  end;
  if Assigned(UninstallProgressForm) then
  begin
    UninstallProgressForm.StatusLabel.Caption := Msg;
    UninstallProgressForm.StatusLabel.Update;
    UninstallProgressForm.BringToFront;
  end;
end;

{ Host single-instance mutex (Local\Phostide.SingleInstance; CheckForMutexes
  tries Local\ and Global\ itself). Held until the host process exits. }
function PhostideRunning: Boolean;
begin
  Result := (FindWindowByClassName('PhostideTray') <> 0) or
            CheckForMutexes('Phostide.SingleInstance');
end;

procedure CloseRunningPhostide;
var
  Wnd: HWND;
  I: Integer;
  ResultCode: Integer;
begin
  if not PhostideRunning then
    Exit;
  SetShellRestartStatus('Closing Phostide…');
  Wnd := FindWindowByClassName('PhostideTray');
  if Wnd <> 0 then
    PostMessageW(Wnd, 16, 0, 0); { WM_CLOSE — normal Exit, restores stock fill }

  { Wait for the process to really end (mutex released), not just the window:
    the host may be finishing an Explorer restart before it exits. }
  for I := 1 to 150 do
  begin
    if not PhostideRunning then
      Break;
    Sleep(100);
  end;

  { Last resort if the tray ignored WM_CLOSE. }
  if PhostideRunning then
    Exec('taskkill.exe', '/IM Phostide.exe /F', '', SW_HIDE,
         ewWaitUntilTerminated, ResultCode);
end;

procedure SleepWithStatus(TotalMs: Integer; const Msg: String);
var
  Waited: Integer;
begin
  Waited := 0;
  while Waited < TotalMs do
  begin
    SetShellRestartStatus(Msg);
    Sleep(250);
    Waited := Waited + 250;
  end;
end;

{ Explorer keeps ExplorerTAP.dll mapped after Phostide exits (XAML Diagnostics
  pins it). Updates must bounce the shell so the new TEMP copy can load. }
procedure RestartExplorerShell(const ForUninstall: Boolean);
var
  ResultCode: Integer;
  I: Integer;
begin
  if ForUninstall then
    SetShellRestartStatus(
      'Restarting Windows Explorer to unload the taskbar theme (desktop may flicker)…')
  else
    SetShellRestartStatus(
      'Restarting Windows Explorer so the taskbar theme can load (desktop may flicker)…');

  if FindWindowByClassName('Shell_TrayWnd') <> 0 then
  begin
    Exec('taskkill.exe', '/IM explorer.exe /F', '', SW_HIDE,
         ewWaitUntilTerminated, ResultCode);
    SleepWithStatus(1500,
      'Windows Explorer closed — starting it again…');
  end;

  Exec(ExpandConstant('{win}\explorer.exe'), '', '', SW_SHOWNORMAL, ewNoWait,
       ResultCode);

  { Wait until the tray exists, then settle so ixde can bind on first launch. }
  for I := 1 to 80 do
  begin
    if FindWindowByClassName('Shell_TrayWnd') <> 0 then
      Break;
    SetShellRestartStatus(
      'Waiting for the taskbar to come back…');
    Sleep(250);
  end;
  SleepWithStatus(8000,
    'Waiting for the taskbar to finish loading (this is normal)…');
  SetShellRestartStatus('Taskbar ready.');
end;

{ If closing Phostide interrupted its own shell bounce (or it was force-killed
  mid-restart), Explorer may be gone. Never finish Setup without a taskbar. }
procedure EnsureExplorerRunning;
var
  I: Integer;
  ResultCode: Integer;
begin
  for I := 1 to 12 do
  begin
    if FindWindowByClassName('Shell_TrayWnd') <> 0 then
      Exit;
    Sleep(250);
  end;
  SetShellRestartStatus('Starting Windows Explorer…');
  Exec(ExpandConstant('{win}\explorer.exe'), '', '', SW_SHOWNORMAL, ewNoWait,
       ResultCode);
  for I := 1 to 80 do
  begin
    if FindWindowByClassName('Shell_TrayWnd') <> 0 then
      Break;
    Sleep(250);
  end;
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
    { Files are on disk; bounce Explorer before [Run] starts Phostide — only
      when the running shell still has an older TAP mapped. }
    if NeedsShellRestartForInstall then
      RestartExplorerShell(False)
    else
      EnsureExplorerRunning;
  end;
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  if CurUninstallStep = usUninstall then
  begin
    CloseRunningPhostide;
    { Drop the in-shell TAP so uninstall leaves a stock taskbar (any version). }
    if TapLoadedInExplorer then
      RestartExplorerShell(True)
    else
      EnsureExplorerRunning;
  end;
end;
