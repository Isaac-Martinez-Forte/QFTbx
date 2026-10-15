[Setup]
AppId={{CCA7592C-91D7-4637-BC71-E1DD50185E66}
AppName=QFTbx
AppVersion={#AppVersion}
AppVerName=QFTbx {#AppLabel}
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
OutputBaseFilename=QFTbx-{#AppLabel}-windows-x86_64-setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
ChangesAssociations=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "associate"; Description: "{cm:AssocFileExtension,QFTbx,.qft}"

[Files]
Source: "{#SourceDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Registry]
Root: HKA; Subkey: "Software\Classes\.qft"; ValueType: string; ValueName: ""; ValueData: "QFTbx.Project"; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\QFTbx.Project"; ValueType: string; ValueName: ""; ValueData: "QFTbx project"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\QFTbx.Project\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\QFTbx.exe,0"; Tasks: associate
Root: HKA; Subkey: "Software\Classes\QFTbx.Project\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\QFTbx.exe"" ""%1"""; Tasks: associate

[Icons]
Name: "{autoprograms}\QFTbx"; Filename: "{app}\QFTbx.exe"
Name: "{autodesktop}\QFTbx"; Filename: "{app}\QFTbx.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\QFTbx.exe"; Description: "{cm:LaunchProgram,QFTbx}"; Flags: nowait postinstall skipifsilent
