[Setup]
AppId={{CCA7592C-91D7-4637-BC71-E1DD50185E66}
AppName=QFTbx
AppVersion={#AppVersion}
AppVerName=QFTbx {#AppVersion} beta
AppPublisher=Isaac Martínez Forte, Joaquín Cervera López
AppPublisherURL=https://github.com/Isaac-Martinez-Forte/QFTbx
AppSupportURL=https://github.com/Isaac-Martinez-Forte/QFTbx/issues
DefaultDirName={autopf}\QFTbx
DefaultGroupName=QFTbx
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
LicenseFile={#SourcePath}\..\..\LICENSE
SetupIconFile={#SourcePath}\..\..\Resources\Icon\qftbx.ico
UninstallDisplayIcon={app}\QFTbx.exe
OutputBaseFilename=QFTbx-{#AppVersion}-beta-windows-x86_64-setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "{#SourceDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{autoprograms}\QFTbx"; Filename: "{app}\QFTbx.exe"
Name: "{autodesktop}\QFTbx"; Filename: "{app}\QFTbx.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\QFTbx.exe"; Description: "{cm:LaunchProgram,QFTbx}"; Flags: nowait postinstall skipifsilent
