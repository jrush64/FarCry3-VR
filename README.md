# Far Cry 3 VR

An unfinished but playable VR mod for Far Cry 3 (PC, DX11 renderer). It is a 32-bit
`d3d11.dll` proxy that puts the game into a PC VR headset through OpenXR, with real same-frame
stereo, head tracking and motion-controlled arms and weapons.

⚠️ Experimental, **unfinished** community mod. Not affiliated with Ubisoft. Use at your own risk.
Development has stopped, and the source is published so that others can pick it up. See
[What doesn't work yet](#what-doesnt-work-yet) and [`docs/TECHNICAL_NOTES.md`](docs/TECHNICAL_NOTES.md).

Licensed under [GPL-3.0](LICENSE).

## What it does

### Rendering and tracking
- **Same-frame geometric stereo.** The game's shaders are patched at load time (DXBC bytecode
  rewriting) so that each draw renders both eyes in the same frame from one simulation state.
  Lighting tiles, depth reconstruction and shadow passes are handled per eye. It is not alternate-eye
  rendering and not reprojection.
- **OpenXR output** to any runtime (SteamVR, Quest Link, Virtual Desktop, etc.), with steady frame
  pacing (each game frame is held for a whole number of headset refreshes, to avoid judder).
- **Head tracking.** Head rotation and position drive the view. The stick still turns the body. The
  view FOV is widened to the headset FOV, including in vehicles, through the engine's own FOV
  override.
- **Recentre**, comfortable start-up tracking, and a world-locked view through scripted animations
  (climbing, ziplines, ledges, takedowns) that hand camera control back cleanly afterwards.

### Motion controls
- **Controllers as a virtual Xbox pad** (ViGEm), merged with the controller tracking, so the whole game
  plays with motion controllers.
- **Motion-controlled arms.** Jason's first-person arms follow the controllers, with arm IK, captured
  native grip poses per weapon, and two-handed support grips.
- **Physical weapon handling** for most of the arsenal. Every weapon has a "Simple" mode that uses the
  game's own reload animation instead.
  - Magazine weapons (AK47, STG-90, A2000, MP5, FAMAS F1, BZ19, MK16, P416, Skorpion, 1911, 6P9,
    D50): eject, grab a fresh magazine from the body, insert it, and pull the charging handle or
    slide where the real gun needs it.
  - Shotguns: one-shell manual loading, and a physical pump for pump-action guns. SPAS-12 and 1887
    lever action have their own handling.
  - .44 revolver: cylinder out, tip up to dump the casings, speedloader, flick to close.
  - GM-94, flamethrower, RPG-7, flare gun and bow have weapon-specific handling.
  - Left-handed (mirrored) A2000. Aiming through physical scopes and holographic/reflex sights.
- **Throwables:** grenades (pull the pin, throw with arm velocity), Molotovs and a lighter, C4 and mines.
- **Machete melee** with blade contact and damage.
- **Body holsters and weapon access.** Draw from body positions, a hand-directed weapon wheel, the
  native four-weapon selection, and the weapon locker.
- **Vehicles:** grab and turn the car steering wheel with either or both hands. Stick
  throttle/brake is optional.
- **Wrist UI**, a headset notes panel, and an in-game **Insert menu** with per-weapon calibration
  pages (hand offsets, contact boxes, guides).

## What doesn't work yet

The most recent build's last headset feedback was "it's better" for the car steering, which still
needed too much physical wrist rotation. Open issues, roughly in priority order:

- **Vehicles.**
  - Hands are placed by the wrist joint rather than the closed fist, so they don't sit exactly on
    the wheel rim.
  - Steering needs a lot of rotation; the *Car steering sensitivity* setting (try 2.0–2.5×) is
    untested.
  - The jetski/ATV handlebars still use older grip rules that can drop your hands.
  - Entering a vehicle with the machete drawn can leave it unable to drive.
- **Camera / culling.** These cases can show sideways views or black-and-white culling for a
  moment:
  - looking backwards on a zipline;
  - turning during a container-climb animation;
  - mounting an ATV or jetski from the side.
- **Death / respawn.** After dying, the view and movement direction can come back rotated or
  swapped. Not root-caused.
- **Weapons:**
  - RPG-7 support-hand attach can rotate the launcher;
  - weapons added from the locker are untested in the headset;
  - D50 two-handed clipping;
  - arms stretch at full reach;
  - firing a two-handed shotgun can turn it sideways.
- **Stealth takedowns** work, but motion-controlled arms can disrupt their animation.
- **Molotov ignition** and native flame visuals are only partly done.
- **Performance and comfort** have not been tuned beyond what is described here.

Fixes are verified offline by a large regression suite (see `tests/`), but that does not prove
headset behaviour. Treat anything not listed as working in *What it does* as untested.

## Requirements

- Far Cry 3 (PC) running the **DX11** renderer.
- An OpenXR runtime and a PC VR headset with motion controllers.
- The [ViGEmBus](https://github.com/nefarius/ViGEmBus) driver installed (for the virtual pad).

## Building

See [BUILD.md](BUILD.md). In short: install Visual Studio 2022 with the C++ workload, then run
`scripts\build.bat`. It builds the DLL and runs a software stereo smoke test. All dependencies
are vendored.

## Installing

See [`release/README.txt`](release/README.txt).

## Layout

| Path | What's there |
|---|---|
| `src/` | Mod source. `stereo_proxy`/`stereo` (d3d11 proxy and frame), `dxbc_patch*` (shader patching), `xr_presenter` (OpenXR), `head_track`, `fov_boost`, `vr_pad` (controllers and ViGEm), `aim_probe`, and per-weapon/feature `.inl` units |
| `src/*_clips.inl` | Animation/pose data captured from the running game, used for weapon reload and grip animations |
| `tests/` | Offline regression tests and fixtures (software D3D11, no headset) |
| `scripts/` | `build.bat` (shipping DLL and smoke test), `vsenv32.bat`, `run_checks.ps1`, and `tests/` with one build script per test suite |
| `third_party/` | MinHook, Dear ImGui, ViGEmClient, OpenXR headers and loader, each with its license |
| `docs/` | `TECHNICAL_NOTES.md` |
| `release/` | End-user install notes and the 32-bit OpenXR loader |
| `builds/` | A prebuilt `d3d11.dll` |

## License

GPL-3.0 - see [LICENSE](LICENSE). Copyright (C) 2026 Halcyon.

Vendored dependencies keep their own licenses: MinHook (BSD-2-Clause), Dear ImGui (MIT),
ViGEmClient (MIT) and the OpenXR SDK headers and loader (Apache-2.0), each in its `third_party/`
folder.
