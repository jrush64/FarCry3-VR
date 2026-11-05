================================================================================
  FAR CRY 3 VR  (experimental, unfinished)
================================================================================

WHAT YOU NEED

  - Far Cry 3 for PC, using the DX11 renderer (farcry3_d3d11.exe).
  - A PC VR headset with motion controllers and an OpenXR runtime (SteamVR,
    Meta Quest Link, Virtual Desktop, ...) set as the active OpenXR runtime.
  - The ViGEmBus driver: https://github.com/nefarius/ViGEmBus/releases
    (the mod presents your motion controllers to the game as an Xbox pad).

INSTALLING

  1. Close the game.

  2. Open the game's  bin  folder (in Steam: right click Far Cry 3, Manage,
     Browse local files, then open  bin).

  3. If there is already a d3d11.dll there, rename it to keep a backup.

  4. Copy these files into  bin :

         d3d11.dll           (from the builds folder)
         openxr_loader.dll   (from this folder)

  5. Start your VR runtime, then start the game in DX11 mode.

  Settings and logs are stored in  %LOCALAPPDATA%\FarCry3VR

USING IT

  Insert         opens the VR menu: per-weapon calibration, reload modes
                 (Manual / Simple), vehicle steering sensitivity, etc.
  Left grip + Y  opens the weapon wheel (point with your hand).
  Each weapon's reload steps are described in its Insert menu page.

UNINSTALLING

  Delete d3d11.dll and openxr_loader.dll from  bin  (and restore any d3d11.dll
  you renamed). Delete %LOCALAPPDATA%\FarCry3VR to remove settings.

THIRD-PARTY

  openxr_loader.dll is the Khronos OpenXR loader, Apache License 2.0.
  See openxr_loader.LICENSE.txt.
