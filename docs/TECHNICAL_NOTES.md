# Technical notes

How the mod works and where to pick it up. Far Cry 3 is a 32-bit Dunia engine game. The mod is a
32-bit `d3d11.dll` proxy that loads next to the DX11 executable. Engine addresses quoted in the
source are RVAs into the current PC build. Native-call code paths check the build and refuse to
run on an unrecognised one.

## Source map

| File | Role |
|---|---|
| `stereo_proxy.cpp`, `main_state.cpp` | d3d11.dll export forwarding to the real system DLL; picks the game's main swapchain (the largest one); one log per process |
| `stereo.cpp` | The frame: device/context hooks, per-eye replay, eye matrices, arms/weapons/holster rendering. Most feature code is `#include`d into it from `*.inl` units named after the feature (and, historically, the build number that introduced them) |
| `dxbc_patch.cpp`, `dxbc_patch_pixel.cpp` | DXBC bytecode rewriting of the game's shaders, including the container hash (MD5 with non-standard padding) |
| `xr_presenter.cpp` | OpenXR session, swapchains, frame pacing, head pose relative to the body. Every OpenXR entry point is resolved at runtime, so a missing loader cannot stop the game |
| `head_track.cpp` | Injects look input through the game's own DirectInput mouse device and calibrates the camera's response per injected count (least-squares, 0/1-frame latency) |
| `aim_probe.cpp` | Native `CPawn` look access at the game's input seam, so aim follows the right controller |
| `fov_boost.cpp` | Widens the view to the headset FOV: the profile `FOVScaleFactor` on foot, and the engine's own `gfx_FOVOverride` in vehicles |
| `vr_pad.cpp` | Motion controllers to a ViGEm virtual Xbox 360 pad, including the chords (grip + button), weapon wheel and vehicle steering |
| `xr_holsters*.inl`, `handle_binding*.inl`, `car_*.inl`, `handlebar*.inl` | Body holsters, grabbable handles, car wheel and handlebars |
| `<weapon>.inl`, `<weapon>_clips.inl` | Per-weapon physical handling, and pose/animation clips captured from the game for that weapon |
| `aer_*.cpp`, `proxy.cpp`, `matrix_probe*.cpp`, `stereo_before.cpp`, `*_experiment.cpp` | Earlier experiments and probes, not part of the shipping DLL (see `scripts\tests\build.bat` and `build_aer.bat`) |

## Stereo: same frame, per-draw replay

The game renders one view. The mod makes it render two, inside the same frame:

1. **Shader patching.** When the game creates a vertex shader, `dxbc_patch` redirects its
   `SV_Position` writes through a temp and appends a multiply by an **eye matrix** read from a
   constant buffer added by the mod:
   `eyeMatrix = eyeProjection * eyeView * inverse(gameProjection)`. This takes whatever the game's
   shader produced to where that eye sees it. Pixel shaders that reconstruct position from depth
   (`$Globals.InvViewRotProjectionMatrix`) get the matching correction.
2. **Twin resources.** Every render target or depth buffer that differs between the eyes gets a
   *twin* for the right eye, created on demand and stored as private data on the D3D object itself
   (no pointer-keyed side tables). Views map to twin views the same way.
3. **Replay.** A draw or compute dispatch that reads or writes anything with a twin runs a second
   time for the right eye, with twins bound and the right-eye constant buffer. Clears, copies,
   `GenerateMips` and resolves are mirrored to twins. Shadow targets stay single, because they don't
   depend on the eye.
4. **Camera buffers.** The game's 896-byte camera constant buffer is recognised by content. A
   summary of what it holds is stored on the buffer and travels with `CopyResource`, because the
   game fills the buffers its draws bind by copying from another camera buffer.
5. **Deferred lighting tiles.** Lights are drawn as flat screen tiles (30×17 grid) whose lamps
   the CPU picks for the *game* camera. With head tracking the eye looks elsewhere, so the tiles are
   redrawn through the eye's homography. Tile-capable lighting shaders get a separate tile-test copy
   that is bound only for the mod's tile draws. A self-check turns that copy off if it ever discards
   samples the game would keep.
6. **Output.** Both eyes go to OpenXR. The desktop gets a side-by-side image, and after Present the
   left-eye image is put back, because the swapchain is a single blt-model buffer.

`scripts\build.bat` runs `stereo_smoke`, which exercises this whole path on WARP.

## Tracking, aiming and the body frame

- The game's camera is treated as the **body**. The stick turns it (yaw only). The head pose is
  measured against a reference taken when tracking starts or on recentre.
- **Aim follows the gun.** Each frame the game camera is pointed along the right controller.
  Everything the headset reports relative to the body is re-expressed relative to that camera
  (`Q` = body axes in camera axes), so the view stays on the head while the camera follows the gun.
- Scripted animations (climb, zipline, ledge, takedown, vehicle entry) own the camera. Before giving
  camera control back, the mod waits until the observed camera matches recent samples of the player.
- Frame pacing: each game frame is shown for the same whole number of headset refreshes (from the
  90th-percentile recent frame time), which removes 2-3-2-3 judder.

## Arms and weapons

- Jason's first-person arm bone palettes are rewritten from controller poses (two-bone IK). Grip
  and finger poses come from captured native animation clips (`*_clips.inl`).
- The real gun mesh is placed with the hand. Reload mechanics drive the **game's own** reload
  callbacks (ammo counts and the HUD stay native) when the physical step completes: magazine
  inserted, shell in the port, cylinder closed, and so on.
- Contact zones (magazine wells, ports, handles, holster volumes) are boxes and spheres, tunable
  per weapon in the Insert menu and saved to `fc3_stereo.ini`.

## Settings and logs

`%LOCALAPPDATA%\FarCry3VR\`: `fc3_stereo.ini`, per-process logs (`fc3_stereo_*.log`), headset notes,
appearance caches and optional eye dumps. Don't enumerate that folder recursively on a machine
that has run eye-capture builds; it can hold a very large number of files.

## Open work, with what is known

1. **Car hands on the rim.** Placement targets the wrist joint, not the closed-fist centre, so
   hands sit slightly inside or behind the rim. Fix: target the point where the rim passes through
   the closed fist (measure it once from a captured native grip pose) and offset along the hand's
   own axes, not the wheel's.
2. **Steering effort.** Steering is by polar angle around the hub, with full lock at 90° of hand
   travel. The *Car steering sensitivity* setting exists but is untuned.
3. **Handlebars (jetski/ATV)** still use the older eviction rules in `handlebar355.h`. Port the
   car's "sticky grip" rule (release only on the grip button or a context change) and its
   stale-sample pause.
4. **Culling and sideways views during animations** (zipline looking back, container climb,
   ATV/jetski mount). Hypothesis, not proven: the rendered XR view, the native frustum and the input
   orientation disagree during the animation-owned camera and its handoff back. Capture that
   transition before choosing a fix.
5. **Death/respawn** can leave the camera or control basis rotated (a camera/aim mismatch of about
   178° was logged). Investigate the camera/control basis and aim pointers through death, loading
   and respawn, and the XR recenter lifecycle.
6. **Weapons:** RPG-7 support-hand attach rotation, locker-added weapons not drawable (unverified
   fix), D50 support-hand clipping, arm over-stretch at full reach, two-handed shotgun turning
   sideways when fired.
7. **Stealth takedowns:** motion-controlled arms fight the native takedown animation. Hand arm and
   blade ownership back to the animation for its duration.
8. **Machete drawn when entering a vehicle** blocks driving (input ownership at vehicle entry).
9. **Molotov:** manual ignition and native flame visuals are incomplete.

## Testing

`tests\` holds offline suites that run on WARP with `FC3_STEREO_NO_XR=1`. They use fixtures
captured from real sessions (`*_fixture.inl`, `*_live_data.inl`). `scripts\run_checks.ps1` runs the
core set. Passing offline does not prove headset behaviour; every gameplay change still needs a
headset check.
