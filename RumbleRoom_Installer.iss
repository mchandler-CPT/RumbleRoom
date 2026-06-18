[Setup]
; Generated a brand-new unique GUID specifically for RumbleRoom to prevent registry clashes
AppId={{4AE23307-5077-40C9-9AC0-F6D676D07A85}
AppName=RumbleRoom
AppVersion=1.0.0-beta
AppPublisher=bdEnergy / Mark Chandler
PrivilegesRequired=admin
; 64-bit hosts must install VST3 under native Common Files (not WOW64 Program Files (x86)).
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64
OutputDir=Output
OutputBaseFilename=RumbleRoom_Setup
DefaultDirName={autopf}\bdEnergy\RumbleRoom
DefaultGroupName=RumbleRoom
DisableDirPage=yes
DisableProgramGroupPage=yes
Compression=zip
SolidCompression=no
VersionInfoCompany=bdEnergy / Mark Chandler
VersionInfoDescription=RumbleRoom Analog Space Modulator Installer
VersionInfoVersion=1.0.0.0
WizardStyle=modern

; Optional: uncomment and point to your icon if available.
; SetupIconFile=assets\RumbleRoom.ico

[Tasks]
Name: "desktopmanual"; Description: "Create a Desktop Shortcut for the README manual"; GroupDescription: "Additional Icons:"

[Files]
; VST3 bundle (recursive copy from RumbleRoom-specific staging area)
Source: "installer_dist\RumbleRoom.vst3\*"; DestDir: "{commoncf}\VST3\RumbleRoom.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs

; Presets
Source: "installer_dist\Presets\*"; DestDir: "{userappdata}\bdEnergy\RumbleRoom\Presets"; Flags: ignoreversion recursesubdirs createallsubdirs skipifsourcedoesntexist

; Optional PDF manual
Source: "installer_dist\README.pdf"; DestDir: "{userappdata}\bdEnergy\RumbleRoom"; Flags: ignoreversion skipifsourcedoesntexist

[Icons]
; Desktop shortcut to the RumbleRoom manual PDF
Name: "{autodesktop}\RumbleRoom README"; Filename: "{userappdata}\bdEnergy\RumbleRoom\README.pdf"; Tasks: desktopmanual
