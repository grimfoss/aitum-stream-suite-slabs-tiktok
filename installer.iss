#define MyAppName "aitum-stream-suite"
#define MyAppVersion "1.2.0"
#define MyAppPublisher "grimfoss"
#define MyAppURL "https://github.com/grimfoss/obs-aitum-stream-suite"

[Setup]
AppId={{11466163-4B1E-4793-9915-B6B6088B59F3}}
AppName="Aitum Stream Suite - Streamlabs TikTok"
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={code:GetDirName}
AppendDefaultDirName=no
DefaultGroupName="Aitum Stream Suite"
OutputBaseFilename={#MyAppName}-{#MyAppVersion}-windows-installer
Compression=lzma
SolidCompression=yes
DirExistsWarning=no
AllowNoIcons=yes
WizardStyle=modern
WizardResizable=yes
SetupIconFile=media\icon.ico

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
Source: "build_x64\RelWithDebInfo\aitum-stream-suite.dll"; DestDir: "{app}\obs-plugins\64bit"; Flags: ignoreversion
Source: "build_x64\RelWithDebInfo\aitum-stream-suite.pdb"; DestDir: "{app}\obs-plugins\64bit"; Flags: ignoreversion
Source: "data\locale\en-US.ini"; DestDir: "{app}\data\obs-plugins\aitum-stream-suite\locale"; Flags: ignoreversion

Source: "LICENSE"; Flags: dontcopy

[InstallDelete]
Type: files; Name: "{app}\obs-plugins\64bit\aitum-stream-suite.dll"
Type: files; Name: "{app}\obs-plugins\64bit\aitum-stream-suite.pdb"

[Icons]
Name: "{group}\{cm:ProgramOnTheWeb,{#MyAppName}}"; Filename: "{#MyAppURL}"

[Code]
function GetDirName(Value: string): string;
var
  InstallPath: string;
begin
  Result := ExpandConstant('{pf}\obs-studio');
  if RegQueryStringValue(HKLM32, 'SOFTWARE\OBS Studio', '', InstallPath) then
    Result := InstallPath;
  if RegQueryStringValue(HKLM64, 'SOFTWARE\OBS Studio', '', InstallPath) then
    Result := InstallPath;
end;

function NextButtonClick(PageId: Integer): Boolean;
var
    ObsFileName: string;
    ObsMS, ObsLS: Cardinal;
    ObsMajorVersion: Cardinal;
begin
    Result := True;
    if not (PageId = wpSelectDir) then
        exit;
    ObsFileName := ExpandConstant('{app}\bin\64bit\obs64.exe');
    if not FileExists(ObsFileName) then begin
        MsgBox('OBS Studio (bin\64bit\obs64.exe) does not seem to be installed in that folder. Please select the correct folder.', mbError, MB_OK);
        Result := False;
        exit;
    end;
    Result := GetVersionNumbers(ObsFileName, ObsMS, ObsLS);
    if not Result then begin
        MsgBox('Failed to read version from OBS Studio.', mbError, MB_OK);
        Result := False;
        exit;
    end;
    ObsMajorVersion := ObsMS shr 16;
    if ObsMajorVersion < 31 then begin
        MsgBox('OBS Studio 31.1.0 or newer is required.', mbError, MB_OK);
        Result := False;
        exit;
    end;
end;
