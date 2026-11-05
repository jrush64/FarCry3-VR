# Building Far Cry 3 VR

## Prerequisites

- Windows 10/11.
- Visual Studio 2022 (any edition, or the standalone Build Tools) with the **"Desktop development
  with C++"** workload. The scripts find it via `vswhere.exe` and use the **x86** (32-bit) toolset,
  because Far Cry 3 is a 32-bit game.
- Nothing else. MinHook, Dear ImGui, ViGEmClient and the OpenXR headers are vendored in
  `third_party/`. The OpenXR loader is loaded at runtime, not linked.

## Build

```
scripts\build.bat
```

This compiles the mod into `build\stereo\d3d11.dll`. It then builds and runs `stereo_smoke.exe`,
which loads the DLL against a software (WARP) D3D11 device without opening a headset and checks
that both eyes render in one frame. Expected last line:

```
PASS: 51 exports; both eyes rendered in one frame; disparity matches; shadow target left single.
```

## Tests

`scripts\tests\` holds one build-and-run script per offline suite (weapons, reloads, holsters,
vehicles, menus, rendering, etc.). Most of them link against the object files from
`scripts\build.bat`, so run that first. `scripts\run_checks.ps1` runs the core regression set
used before each release candidate.

A few GPU/UI tests read captured assets from a folder named by the `FC3VR_TEST_DATA` environment
variable. `vehicle_camera279_test` reads the game's own `FC3_d3d11.dll` from the path in
`FC3VR_GAME_DLL`. Without those, these tests skip or fail.

## Deploying

Copy into `<Far Cry 3>\bin\` (next to `farcry3_d3d11.exe`), with the game closed:

- `build\stereo\d3d11.dll`
- `release\openxr_loader.dll`: the 32-bit Khronos OpenXR loader

Settings, logs and the headset notes live in `%LOCALAPPDATA%\FarCry3VR\` (`fc3_stereo.ini`).

## Third-party components

| Component | Location | License |
|---|---|---|
| MinHook | `third_party/minhook` | BSD 2-Clause |
| Dear ImGui | `third_party/imgui` | MIT |
| ViGEmClient | `third_party/vigemclient` | MIT |
| OpenXR SDK headers + loader (x86) | `third_party/openxr` | Apache 2.0 |
