; Bitemu - Inno Setup installer script
; Used by CI/CD pipeline to generate Windows .exe installer.
; Expects PyInstaller output in frontend\dist\bitemu\
;
; To build manually: iscc bitemu.iss

#define MyAppName "Bitemu"
#define MyAppPublisher "Bitemu Project"
#define MyAppURL "https://github.com/YOUR_USER/bitemu"
#define MyAppExeName "bitemu.exe"

; Version is read from VERSION file at repo root
#define FileHandle FileOpen("..\VERSION")
#define MyAppVersion Trim(FileRead(FileHandle))
#expr FileClose(FileHandle)

[Setup]
AppId={{E8B3F2A1-7C4D-4E9F-B6A8-1D2E3F4A5B6C}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
OutputDir=..\dist
OutputBaseFilename=bitemu-windows-x64-{#MyAppVersion}-setup
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
ArchitecturesInstallIn64BitMode=x64compatible

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"

[Files]
Source: "..\frontend\dist\bitemu\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "Launch Bitemu"; Flags: nowait postinstall skipifsilent
