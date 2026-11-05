#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

// Rewrites a compiled SM4/SM5 vertex shader so its SV_Position output is
// re-projected for one eye: position' = eyeMatrix * position, where eyeMatrix
// takes the game camera's clip space to the eye's clip space (any eye
// position, orientation and field of view). It applies to every shader no
// matter which matrix the shader itself used.
namespace dxbc
{
// No Far Cry 3 vertex shader declares a constant buffer above slot 5, so the
// game never binds this slot. Unbound reads return zero = no shift.
constexpr uint32_t kEyeCbSlot = 13;

enum class Result
{
    Patched,
    NotVertexShader,
    Unsupported,
    Malformed,
};

// Vertex eye constant buffer, six float4:
//   [0..3] rows of (eyeMatrix - identity), applied to perspective vertices (w != 1)
//   [4]    offset added to screen-space vertices (w == 1)
//   [5]    (scale - 1) applied to screen-space vertices; [4] and [5] place the UI
Result PatchVertexShader(const void* bytecode, size_t size, std::vector<uint8_t>& out);
// Already divided particle positions still carry world depth; w==1 is not UI.
Result PatchProjectedVertexShader241(const void* bytecode, size_t size, std::vector<uint8_t>& out);

// The first-person arm shader: each vertex follows three transforms (upper arm,
// forearm, hand), weighted by where it sits along the arm in the mesh's own
// (bind) space: |x| of input v0 (int16, 1/16384 m per unit; elbow ~0.22 m,
// wrist ~0.59 m). Eye buffer rows:
//   [0..3]   hand      (eyeMatrix - identity)  -- also what every other shader reads
//   [6..9]   forearm
//   [10..13] upper arm
// When the three are equal the result is the ordinary patch.
Result PatchArmVertexShader(const void* bytecode, size_t size, std::vector<uint8_t>& out);
// Optional first-person wrist clip, applied AFTER the arm/stereo patch. cb12[0]
// contains enabled and squared bind-space wrist distance; zero/unbound preserves all vertices.
Result PatchHandsOnly287(const void* bytecode,size_t size,std::vector<uint8_t>& out);
constexpr uint32_t kHandsClipCbSlot287=12;
constexpr uint32_t kArmEyeRows = 14;
// Every patched shader declares the eye buffer this long. Rows 14/15: offset
// and (scale - 1) for flat vertices near the screen centre (the crosshair),
// which may be placed apart from the rest of the UI.
constexpr uint32_t kEyeRows = 16;

// Rewrites a pixel shader that rebuilds positions from depth through
// $Globals.InvViewRotProjectionMatrix. Those positions come out relative to the
// eye; everything else the game feeds the shader is relative to its own
// (centre) camera. The patch multiplies the matrix on the right by the
// eye-clip to game-clip matrix: row' = row + row * (thatMatrix - identity),
// with the columns of (thatMatrix - identity) in a constant buffer the mod
// binds at PS slot kEyeCbSlot (four float4). Unbound = zero = unchanged.
// Returns NotApplicable for pixel shaders that do not use that matrix.
enum class PixelResult
{
    Patched,
    NotApplicable,
    Unsupported,
    Malformed,
};
// [TILES] When the shader reads its clip vector into a temp and dp4s it with
// the matrix, and takes TEXCOORD0 in v1 (xyzw in the signature), a tile test is
// added in front: the pixel's game-screen position (eye clip -> game clip via
// the same columns) must lie inside the tile rectangle packed into v1.zw by the
// mod's tile geometry shader, or the pixel is discarded. The test only runs
// while cb13[4].x != 0 (the mod binds a 5-row buffer for tile draws only).
// Only added when wantTile is set (a separate shader the mod binds for its own
// tile draws; the game's shaders never carry it). *tileTest reports whether it
// was added.
PixelResult PatchPixelShader(const void* bytecode, size_t size, std::vector<uint8_t>& out,
                             bool* tileTest = nullptr, bool wantTile = false);

void ComputeChecksum(const uint8_t* data, size_t size, uint32_t hash[4]);
bool VerifyChecksum(const void* data, size_t size);
}
