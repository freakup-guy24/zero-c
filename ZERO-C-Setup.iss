#define ProductName "ZERO C"
#define ProductVersion "1.0.1"
#define ProductPublisher "ZERO C Project"
#define AppExe "ZERO-C.exe"
#define StageDir "build"
#ifndef CompilerDir
#define CompilerDir "GCC"
#endif

[Setup]
AppId={{F2A490F8-9662-4B93-9BB5-ABEE1F5CF591}
AppName={#ProductName}
AppVersion={#ProductVersion}
AppPublisher={#ProductPublisher}
DefaultDirName={localappdata}\Programs\ZERO C
DefaultGroupName={#ProductName}
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
LicenseFile={#StageDir}\THIRD-PARTY-NOTICES.txt
OutputDir=dist
OutputBaseFilename=ZERO-C-Setup-{#ProductVersion}
SetupIconFile=zero-c.ico
UninstallDisplayIcon={app}\{#AppExe}
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
WizardSizePercent=110
CloseApplications=yes
RestartApplications=no
Uninstallable=yes
VersionInfoVersion=1.0.1.0
VersionInfoCompany={#ProductPublisher}
VersionInfoDescription=ZERO C Setup
VersionInfoProductName={#ProductName}
VersionInfoProductVersion={#ProductVersion}
ChangesAssociations=no

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; GroupDescription: "Shortcuts:"

[Files]
Source: "{#StageDir}\{#AppExe}"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#StageDir}\README.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#StageDir}\THIRD-PARTY-NOTICES.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#StageDir}\compat\conio.h"; DestDir: "{app}\C\TC\compat"; Flags: ignoreversion
Source: "{#CompilerDir}\*"; DestDir: "{app}\GCC\mingw64"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{autoprograms}\ZERO C"; Filename: "{app}\{#AppExe}"; WorkingDir: "{app}"
Name: "{autodesktop}\ZERO C"; Filename: "{app}\{#AppExe}"; WorkingDir: "{app}"; Tasks: desktopicon
Name: "{autoprograms}\ZERO C Guide"; Filename: "{app}\README.txt"

[Run]
Filename: "{app}\{#AppExe}"; Description: "Launch ZERO C"; Flags: postinstall nowait skipifsilent
