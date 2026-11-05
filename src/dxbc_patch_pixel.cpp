#include "dxbc_patch.h"

#include <cstring>

// Pixel shader side of the stereo patch. See PatchPixelShader in dxbc_patch.h.
namespace dxbc
{
namespace
{
constexpr uint32_t kOpLabel = 44;
constexpr uint32_t kOpAnd = 1;
constexpr uint32_t kOpDiscard = 13;
constexpr uint32_t kOpDiv = 14;
constexpr uint32_t kOpFtou = 28;
constexpr uint32_t kOpLt = 49;
constexpr uint32_t kOpMad = 50;
constexpr uint32_t kOpNe = 57;
constexpr uint32_t kOpOr = 60;
constexpr uint32_t kOpUshr = 85;
constexpr uint32_t kOpUtof = 86;
constexpr uint32_t kOpDclInputPs = 98;
constexpr uint32_t kOperandInput = 1;
constexpr uint32_t kOpAdd = 0;
constexpr uint32_t kOpDp4 = 17;
constexpr uint32_t kOpCustomData = 53;
constexpr uint32_t kOpDclConstantBuffer = 89;
constexpr uint32_t kOpDclTemps = 104;
constexpr uint32_t kOpDclGlobalFlags = 106;
constexpr uint32_t kOpInterfaceCall = 120;

constexpr uint32_t kOperandTemp = 0;
constexpr uint32_t kOperandImmediate32 = 4;
constexpr uint32_t kOperandImmediate64 = 5;
constexpr uint32_t kOperandConstantBuffer = 8;

struct ContainerChunk { uint32_t tag; uint32_t size; const uint8_t* data; };

constexpr uint32_t Op(uint32_t opcode, uint32_t length) { return opcode | (length << 24); }

bool IsDeclaration(uint32_t op)
{
    return (op >= 88 && op <= 106) || (op >= 143 && op <= 163) ||
           (op >= 113 && op <= 116) || op == kOpCustomData;
}

uint32_t InstructionLength(const uint32_t* t, size_t remaining)
{
    const uint32_t op = t[0] & 0x7ff;
    if (op == kOpCustomData)
        return remaining >= 2 ? t[1] : 0;
    return (t[0] >> 24) & 0x7f;
}

bool ReadString(const uint8_t* base, uint32_t size, uint32_t offset, const char* expect)
{
    const size_t length = std::strlen(expect) + 1;
    return offset + length <= size && std::memcmp(base + offset, expect, length) == 0;
}

// Finds $Globals.InvViewRotProjectionMatrix (used) in a RDEF chunk.
// Returns false when the shader does not use it.
bool FindUnprojectMatrix(const uint8_t* rdef, uint32_t size, uint32_t& slot, uint32_t& firstRegister)
{
    if (size < 28) return false;
    uint32_t cbCount, cbOffset, bindCount, bindOffset, version;
    std::memcpy(&cbCount, rdef, 4);
    std::memcpy(&cbOffset, rdef + 4, 4);
    std::memcpy(&bindCount, rdef + 8, 4);
    std::memcpy(&bindOffset, rdef + 12, 4);
    std::memcpy(&version, rdef + 16, 4);
    const uint32_t variableSize = (version & 0xffff) >= 0x500 ? 40 : 24;

    bool found = false;
    for (uint32_t c = 0; c < cbCount && !found; ++c)
    {
        const uint32_t entry = cbOffset + c * 24;
        if (entry + 24 > size) return false;
        uint32_t name, variableCount, variableOffset;
        std::memcpy(&name, rdef + entry, 4);
        std::memcpy(&variableCount, rdef + entry + 4, 4);
        std::memcpy(&variableOffset, rdef + entry + 8, 4);
        if (!ReadString(rdef, size, name, "$Globals")) continue;
        for (uint32_t v = 0; v < variableCount; ++v)
        {
            const uint32_t variable = variableOffset + v * variableSize;
            if (variable + 16 > size) return false;
            uint32_t variableName, start, flags;
            std::memcpy(&variableName, rdef + variable, 4);
            std::memcpy(&start, rdef + variable + 4, 4);
            std::memcpy(&flags, rdef + variable + 12, 4);
            if (!(flags & 2)) continue;     // not used by this shader
            if (!ReadString(rdef, size, variableName, "InvViewRotProjectionMatrix")) continue;
            if (start % 16 != 0) return false;
            firstRegister = start / 16;
            found = true;
            break;
        }
    }
    if (!found) return false;

    for (uint32_t b = 0; b < bindCount; ++b)
    {
        const uint32_t entry = bindOffset + b * 32;
        if (entry + 32 > size) return false;
        uint32_t name, type, bindPoint;
        std::memcpy(&name, rdef + entry, 4);
        std::memcpy(&type, rdef + entry + 4, 4);
        std::memcpy(&bindPoint, rdef + entry + 20, 4);
        if (type == 0 && ReadString(rdef, size, name, "$Globals"))
        {
            slot = bindPoint;
            return true;
        }
    }
    return false;
}

// Copies one operand from t[i] to out. A top-level read of cb<slot>[first+0..3]
// becomes a read of temp (tempBase + row). Returns false on an unknown layout.
bool CopyOperand(const uint32_t* t, size_t& i, size_t end, std::vector<uint32_t>& out,
                 uint32_t slot, uint32_t first, uint32_t tempBase, int depth, int& replaced)
{
    if (i >= end || depth > 3) return false;
    const size_t start = i;
    const uint32_t token = t[i++];
    size_t extendedEnd = i;
    if (token & 0x80000000u)
    {
        for (;;)
        {
            if (i >= end) return false;
            const uint32_t extended = t[i++];
            if (!(extended & 0x80000000u)) break;
        }
        extendedEnd = i;
    }
    const uint32_t componentMode = token & 3;
    const uint32_t type = (token >> 12) & 0xff;
    const uint32_t dimension = (token >> 20) & 3;

    if (type == kOperandImmediate32 || type == kOperandImmediate64)
    {
        const uint32_t values = componentMode == 1 ? 1 : componentMode == 2 ? 4 : 0;
        i += values * (type == kOperandImmediate64 ? 2 : 1);
        if (i > end) return false;
        out.insert(out.end(), t + start, t + i);
        return true;
    }

    if (depth == 0 && type == kOperandConstantBuffer && dimension == 2 &&
        ((token >> 22) & 0x3f) == 0 && i + 2 <= end &&
        t[i] == slot && t[i + 1] >= first && t[i + 1] < first + 4)
    {
        // same component selection, now a 1-D temp register
        uint32_t rewritten = token & ~((0xffu << 12) | (3u << 20) | (0x1ffu << 22));
        rewritten |= (kOperandTemp << 12) | (1u << 20);
        out.push_back(rewritten);
        out.insert(out.end(), t + start + 1, t + extendedEnd);
        out.push_back(tempBase + (t[i + 1] - first));
        i += 2;
        ++replaced;
        return true;
    }

    out.insert(out.end(), t + start, t + extendedEnd);
    for (uint32_t d = 0; d < dimension; ++d)
    {
        const uint32_t representation = (token >> (22 + 3 * d)) & 7;
        switch (representation)
        {
        case 0:
            if (i + 1 > end) return false;
            out.push_back(t[i++]);
            break;
        case 1:
            if (i + 2 > end) return false;
            out.push_back(t[i++]);
            out.push_back(t[i++]);
            break;
        case 2:
            if (!CopyOperand(t, i, end, out, slot, first, tempBase, depth + 1, replaced)) return false;
            break;
        case 3:
            if (i + 1 > end) return false;
            out.push_back(t[i++]);
            if (!CopyOperand(t, i, end, out, slot, first, tempBase, depth + 1, replaced)) return false;
            break;
        case 4:
            if (i + 2 > end) return false;
            out.push_back(t[i++]);
            out.push_back(t[i++]);
            if (!CopyOperand(t, i, end, out, slot, first, tempBase, depth + 1, replaced)) return false;
            break;
        default:
            return false;
        }
    }
    return true;
}
}

namespace
{
// ---- [TILES] tile test tokens ----------------------------------------------
uint32_t Swz(uint32_t x, uint32_t y, uint32_t z, uint32_t w) { return x | (y << 2) | (z << 4) | (w << 6); }
void DstTemp(std::vector<uint32_t>& o, uint32_t reg, uint32_t mask) { o.push_back(0x00100002u | (mask << 4)); o.push_back(reg); }
void SrcTemp(std::vector<uint32_t>& o, uint32_t reg, uint32_t swz) { o.push_back(0x00100006u | (swz << 4)); o.push_back(reg); }
void SrcInput(std::vector<uint32_t>& o, uint32_t reg, uint32_t swz) { o.push_back(0x00100006u | (swz << 4) | (kOperandInput << 12)); o.push_back(reg); }
void SrcCb(std::vector<uint32_t>& o, uint32_t slot, uint32_t row, uint32_t swz) { o.push_back(0x00200006u | (swz << 4) | (kOperandConstantBuffer << 12)); o.push_back(slot); o.push_back(row); }
void SrcImm(std::vector<uint32_t>& o, uint32_t a, uint32_t b, uint32_t c, uint32_t d) { o.push_back(0x00004002u); o.push_back(a); o.push_back(b); o.push_back(c); o.push_back(d); }
uint32_t F(float f) { uint32_t u; std::memcpy(&u, &f, 4); return u; }
// Instruction: opcode + operands assembled in body (length filled in).
void Emit(std::vector<uint32_t>& o, uint32_t op, const std::vector<uint32_t>& body, uint32_t extra = 0)
{
    o.push_back(op | extra | (static_cast<uint32_t>(body.size() + 1) << 24));
    o.insert(o.end(), body.begin(), body.end());
}

// Length of the operand at t[i] (0 = cannot parse).
size_t OperandLength(const uint32_t* t, size_t i, size_t end)
{
    std::vector<uint32_t> scratch;
    int replaced = 0;
    size_t cursor = i;
    if (!CopyOperand(t, cursor, end, scratch, 0xffffffffu, 0, 0, 0, replaced)) return 0;
    return cursor - i;
}

// Appends the tile test. C = clip temp register (xyzw). U, T, R, S = new temps.
void EmitTileTest(std::vector<uint32_t>& o, uint32_t clip, uint32_t base)
{
    const uint32_t U = base + 4, T = base + 5, R = base + 6, S = base + 7;
    const uint32_t XYZW = Swz(0, 1, 2, 3);
    std::vector<uint32_t> b;
    // U.xy = v1.zw + 0.5 ; U.xy = ftou ; U.zw = U.xy >> 10 (min) ; U.xy &= 1023 (max)
    b.clear(); DstTemp(b, U, 0x3); SrcInput(b, 1, Swz(2, 3, 2, 2)); SrcImm(b, F(0.5f), F(0.5f), F(0.5f), F(0.5f)); Emit(o, kOpAdd, b);
    b.clear(); DstTemp(b, U, 0x3); SrcTemp(b, U, Swz(0, 1, 0, 0)); Emit(o, kOpFtou, b);
    b.clear(); DstTemp(b, U, 0xC); SrcTemp(b, U, Swz(0, 0, 0, 1)); SrcImm(b, 10, 10, 10, 10); Emit(o, kOpUshr, b);
    b.clear(); DstTemp(b, U, 0x3); SrcTemp(b, U, Swz(0, 1, 0, 0)); SrcImm(b, 1023, 1023, 1023, 1023); Emit(o, kOpAnd, b);
    b.clear(); DstTemp(b, U, 0xF); SrcTemp(b, U, XYZW); Emit(o, kOpUtof, b);
    const float k = 2.0f / 1023.0f;
    // U = (maxx, maxy, minx, miny) in game NDC
    b.clear(); DstTemp(b, U, 0xF); SrcTemp(b, U, XYZW); SrcImm(b, F(k), F(k), F(k), F(k)); SrcImm(b, F(-1), F(-1), F(-1), F(-1)); Emit(o, kOpMad, b);
    // T = X * C = C + sum_c cb13[c] * C.c  (game clip of this pixel's point)
    for (uint32_t c = 0; c < 4; ++c)
    {
        b.clear(); DstTemp(b, T, 0xF); SrcCb(b, kEyeCbSlot, c, XYZW); SrcTemp(b, clip, Swz(c, c, c, c));
        if (c == 0) SrcTemp(b, clip, XYZW); else SrcTemp(b, T, XYZW);
        Emit(o, kOpMad, b);
    }
    b.clear(); DstTemp(b, T, 0x3); SrcTemp(b, T, Swz(0, 1, 0, 0)); SrcTemp(b, T, Swz(3, 3, 3, 3)); Emit(o, kOpDiv, b);
    // outside: ndc < min (not a border edge) or ndc > max (not a border edge)
    b.clear(); DstTemp(b, R, 0x3); SrcTemp(b, T, Swz(0, 1, 0, 0)); SrcTemp(b, U, Swz(2, 3, 2, 2)); Emit(o, kOpLt, b);
    b.clear(); DstTemp(b, R, 0xC); SrcTemp(b, U, Swz(0, 0, 0, 1)); SrcTemp(b, T, Swz(0, 0, 0, 1)); Emit(o, kOpLt, b);
    b.clear(); DstTemp(b, S, 0x3); SrcImm(b, F(-0.999f), F(-0.999f), F(-0.999f), F(-0.999f)); SrcTemp(b, U, Swz(2, 3, 2, 2)); Emit(o, kOpLt, b);
    b.clear(); DstTemp(b, S, 0xC); SrcTemp(b, U, Swz(0, 0, 0, 1)); SrcImm(b, F(0.999f), F(0.999f), F(0.999f), F(0.999f)); Emit(o, kOpLt, b);
    b.clear(); DstTemp(b, R, 0xF); SrcTemp(b, R, XYZW); SrcTemp(b, S, XYZW); Emit(o, kOpAnd, b);
    b.clear(); DstTemp(b, R, 0x3); SrcTemp(b, R, Swz(0, 1, 0, 0)); SrcTemp(b, R, Swz(2, 3, 2, 2)); Emit(o, kOpOr, b);
    b.clear(); DstTemp(b, R, 0x1); SrcTemp(b, R, Swz(0, 0, 0, 0)); SrcTemp(b, R, Swz(1, 1, 1, 1)); Emit(o, kOpOr, b);
    b.clear(); DstTemp(b, S, 0x1); SrcCb(b, kEyeCbSlot, 4, Swz(0, 0, 0, 0)); SrcImm(b, 0, 0, 0, 0); Emit(o, kOpNe, b);
    b.clear(); DstTemp(b, R, 0x1); SrcTemp(b, R, Swz(0, 0, 0, 0)); SrcTemp(b, S, Swz(0, 0, 0, 0)); Emit(o, kOpAnd, b);
    b.clear(); b.push_back(0x0010000Au); b.push_back(R); // temp, select x
    Emit(o, kOpDiscard, b, 1u << 18);  // discard_nz
}
}

#include "particle_depth254.inl"

PixelResult PatchPixelShader(const void* bytecode, size_t size, std::vector<uint8_t>& out, bool* tileTest, bool wantTile)
{
    if (tileTest) *tileTest = false;
    const uint8_t* src = static_cast<const uint8_t*>(bytecode);
    if (!src || size < 32 || std::memcmp(src, "DXBC", 4) != 0)
        return PixelResult::Malformed;
    uint32_t totalSize = 0, chunkCount = 0;
    std::memcpy(&totalSize, src + 24, 4);
    std::memcpy(&chunkCount, src + 28, 4);
    if (totalSize > size || chunkCount == 0 || chunkCount > 64 ||
        32 + 4 * static_cast<size_t>(chunkCount) > totalSize)
        return PixelResult::Malformed;

    ContainerChunk chunks[64];
    int shaderChunk = -1;
    int rdefChunk = -1;
    for (uint32_t c = 0; c < chunkCount; ++c)
    {
        uint32_t offset = 0;
        std::memcpy(&offset, src + 32 + 4 * c, 4);
        if (offset + 8 > totalSize) return PixelResult::Malformed;
        std::memcpy(&chunks[c].tag, src + offset, 4);
        std::memcpy(&chunks[c].size, src + offset + 4, 4);
        chunks[c].data = src + offset + 8;
        if (offset + 8 + static_cast<uint64_t>(chunks[c].size) > totalSize)
            return PixelResult::Malformed;
        if (chunks[c].tag == 0x52444853 || chunks[c].tag == 0x58454853) shaderChunk = static_cast<int>(c);
        if (chunks[c].tag == 0x46454452 /*RDEF*/) rdefChunk = static_cast<int>(c);
    }
    if (shaderChunk < 0 || chunks[shaderChunk].size < 8) return PixelResult::Malformed;
    if (rdefChunk < 0) return PixelResult::NotApplicable;

    const size_t tokenCount = chunks[shaderChunk].size / 4;
    std::vector<uint32_t> tokens(tokenCount);
    std::memcpy(tokens.data(), chunks[shaderChunk].data, tokenCount * 4);
    if ((tokens[0] >> 16) != 0)             // pixel shaders only
        return PixelResult::NotApplicable;
    if (tokens[1] != tokenCount) return PixelResult::Malformed;

    uint32_t slot = 0, first = 0;
    // River depth is sampled in eye UV, but thickness/caustics use GAME-view
    // positions. Convert the sampled point back before its inverse projection.
    // Exact bytecode identity also pins the UV convention and matrix registers.
    uint64_t shaderHash = 14695981039346656037ull;
    for (size_t i = 0; i < size; ++i) { shaderHash ^= src[i]; shaderHash *= 1099511628211ull; }
    if(shaderHash==0xB42B55CCCF14B3CEull)return PatchParticleDepth254(src,chunks,chunkCount,shaderChunk,tokens,out,true);
    if(shaderHash==0xC19BFBC792F0F843ull||shaderHash==0xBA81B5F7771569B2ull||shaderHash==0x057EE039AB2B7A3Eull||shaderHash==0xE20ABD31F16921EAull||shaderHash==0x83A6FCAF6854B4EDull||shaderHash==0x6771A60187C7067Full)
        return PatchParticleDepth254(src,chunks,chunkCount,shaderChunk,tokens,out);
    // Native camera blur follows the aim/game camera, including controller-only
    // turns. Pin its final sampling velocity to zero for stereo VR. Preserve
    // the original centre sample, mask weighting, colour and alpha math.
    // Exact hash + instruction guard: do not affect other post-processing.
    if (shaderHash == 0xA577E6A5E9ECCB71ull)
    {
        const uint32_t velocity[] = { Op(56,7), 0x00100032u,1,
                                     0x00100AA6u,1,0x00100046u,1 };
        size_t at = 0;
        for (size_t i = 2; i < tokenCount;)
        {
            const uint32_t length = InstructionLength(&tokens[i], tokenCount-i);
            if (!length || i+length>tokenCount) return PixelResult::Malformed;
            if (length == 7 && std::memcmp(&tokens[i],velocity,sizeof(velocity)) == 0)
            {
                if (at) return PixelResult::Unsupported;
                at = i;
            }
            i += length;
        }
        if (!at) return PixelResult::Unsupported;
        // mov r1.xy, l(0), followed by two nops: keep the container layout.
        const uint32_t stationary[] = { Op(54,5),0x00100032u,1,0x00004001u,0,
                                       Op(58,1),Op(58,1) };
        out.assign(src,src+size);
        const size_t offset = static_cast<size_t>(chunks[shaderChunk].data-src)+at*4;
        std::memcpy(out.data()+offset,stationary,sizeof(stationary));
        uint32_t checksum[4]; ComputeChecksum(out.data(),out.size(),checksum);
        std::memcpy(out.data()+4,checksum,sizeof(checksum));
        return PixelResult::Patched;
    }
    const bool riverDiffuse = shaderHash == 0xB5850815B1E93395ull || shaderHash == 0xF9BD64B91671E5FEull;
    // Cave pond: viewport CB1, projected eye depth in v6, native view position
    // retained in v3, and a separate mono reflection projector in CB2.
    const bool cave256=shaderHash==0xF8F31518956C98DBull;
    const bool riverReflection = shaderHash == 0x5E139F4BB30305E5ull || shaderHash == 0xA1F37111CC1926D5ull || cave256;
    const bool river = riverDiffuse || riverReflection;
    const bool ocean254 = shaderHash==0x919B2333D357EBB2ull || shaderHash==0xFFD1D36205DC7757ull;
    const bool waterEdge254=shaderHash==0x6CEEF5F8E48054A5ull;
    const bool waterSurface254=ocean254||shaderHash==0x3E11D1C4732A23E3ull||waterEdge254;
    const bool water254=waterSurface254||shaderHash==0x22ECA6952F53DBD7ull;
    const uint32_t waterViewport254=waterEdge254?0:1,waterInput254=ocean254?4:waterEdge254?2:6;
    if (river) { slot = riverDiffuse ? 0 : 1; first = 16; }
    else if(water254){slot=waterViewport254;first=16;}
    else if (!FindUnprojectMatrix(chunks[rdefChunk].data, chunks[rdefChunk].size, slot, first))
        return PixelResult::NotApplicable;

    size_t temps = 0, tempsIndex = 0, firstCode = 0, afterGlobalFlags = 2;
    for (size_t i = 2; i < tokenCount;)
    {
        const uint32_t op = tokens[i] & 0x7ff;
        const uint32_t length = InstructionLength(&tokens[i], tokenCount - i);
        if (length == 0 || i + length > tokenCount) return PixelResult::Malformed;
        if (IsDeclaration(op))
        {
            if (firstCode != 0) return PixelResult::Unsupported;
            if (op == kOpDclTemps && length == 2) { temps = tokens[i + 1]; tempsIndex = i + 1; }
            if (op == kOpDclConstantBuffer && length == 4 && tokens[i + 2] == kEyeCbSlot)
                return PixelResult::Unsupported;
            if (op == kOpDclGlobalFlags && i == 2) afterGlobalFlags = i + length;
        }
        else
        {
            if (firstCode == 0) firstCode = i;
            if (op == kOpLabel || op == kOpInterfaceCall) return PixelResult::Unsupported;
        }
        i += length;
    }
    if (firstCode == 0) return PixelResult::Unsupported;
    const uint32_t tempBase = static_cast<uint32_t>(temps);

    // [TILES] can this shader take the tile test? v1 declared linear, TEXCOORD0
    // in register 1 with xyzw in the signature, and a dp4 of a clip temp with
    // the matrix.
    size_t v1Decl = 0;
    for (size_t i = 2; i < firstCode;)
    {
        const uint32_t length = InstructionLength(&tokens[i], tokenCount - i);
        if ((tokens[i] & 0x7ff) == kOpDclInputPs && length == 3 &&
            ((tokens[i + 1] >> 12) & 0xff) == kOperandInput && tokens[i + 2] == 1)
            v1Decl = i + 1;
        i += length;
    }
    int isgnChunk = -1;
    size_t isgnReg1 = 0;
    for (uint32_t c = 0; c < chunkCount; ++c)
        if (chunks[c].tag == 0x4E475349u /*ISGN*/) isgnChunk = static_cast<int>(c);
    if (isgnChunk >= 0 && chunks[isgnChunk].size >= 8)
    {
        uint32_t count = 0;
        std::memcpy(&count, chunks[isgnChunk].data, 4);
        for (uint32_t e = 0; e < count && 8 + 24 * (e + 1) <= chunks[isgnChunk].size; ++e)
        {
            const uint8_t* el = chunks[isgnChunk].data + 8 + 24 * e;
            uint32_t reg = 0;
            std::memcpy(&reg, el + 16, 4);
            if (reg == 1 && (el[20] & 0xC) == 0xC) isgnReg1 = 8 + 24 * e;
        }
    }
    size_t tileAt = 0;          // instruction index to put the test in front of
    uint32_t tileClip = 0;
    if (v1Decl && isgnReg1)
    {
        for (size_t i = firstCode; i < tokenCount && !tileAt;)
        {
            const uint32_t length = InstructionLength(&tokens[i], tokenCount - i);
            const size_t end = i + length;
            if ((tokens[i] & 0x7ff) == kOpDp4 && !(tokens[i] & 0x80000000u))
            {
                size_t cursor = i + 1;
                const size_t dstLen = OperandLength(tokens.data(), cursor, end);
                if (dstLen)
                {
                    cursor += dstLen;
                    size_t a = cursor, aLen = OperandLength(tokens.data(), a, end);
                    size_t b = a + aLen, bLen = aLen ? OperandLength(tokens.data(), b, end) : 0;
                    auto isMatrix = [&](size_t at) {
                        return ((tokens[at] >> 12) & 0xff) == kOperandConstantBuffer && ((tokens[at] >> 20) & 3) == 2 &&
                               !(tokens[at] & 0x80000000u) && tokens[at + 1] == slot &&
                               tokens[at + 2] >= first && tokens[at + 2] < first + 4;
                    };
                    auto isClip = [&](size_t at, size_t len) {
                        return len == 2 && ((tokens[at] >> 12) & 0xff) == kOperandTemp && ((tokens[at] >> 20) & 3) == 1 &&
                               (tokens[at] & 0xFFF) == 0xE46;   // 4 components, .xyzw
                    };
                    if (aLen && bLen && b + bLen == end)
                    {
                        if (isMatrix(b) && isClip(a, aLen)) { tileAt = i; tileClip = tokens[a + 1]; }
                        else if (isMatrix(a) && isClip(b, bLen)) { tileAt = i; tileClip = tokens[b + 1]; }
                    }
                }
            }
            i = end;
        }
    }
    const bool addTile = !river && !water254 && wantTile && tileAt != 0;

    std::vector<uint32_t> patched;
    patched.reserve(tokenCount + 64);
    patched.insert(patched.end(), tokens.begin(), tokens.begin() + afterGlobalFlags);
    patched.push_back(Op(kOpDclConstantBuffer, 4));
    patched.push_back(0x00208E46u);
    patched.push_back(kEyeCbSlot);
    patched.push_back(addTile ? 5 : 4);
    patched.insert(patched.end(), tokens.begin() + afterGlobalFlags, tokens.begin() + firstCode);
    const uint32_t extraTemps = addTile ? 8 : (riverReflection||waterSurface254 ? 6 : 4);
    if (tempsIndex != 0)
    {
        patched[tempsIndex + 4] = static_cast<uint32_t>(temps + extraTemps);
    }
    else
    {
        patched.push_back(Op(kOpDclTemps, 2));
        patched.push_back(extraTemps);
    }
    if (addTile)    // v1.xy -> v1.xyzw (the declaration moved by 4 tokens)
        patched[v1Decl + 4] = (patched[v1Decl + 4] & ~0xF0u) | 0xF0u;

    // row'[i] = row[i] + row[i] * (eyeMatrix - identity), the matrix columns in cb13[0..3]
    for (uint32_t row = 0; row < 4; ++row)
    {
        for (uint32_t c = 0; c < 4; ++c)
        {
            patched.push_back(Op(kOpDp4, 9));
            patched.push_back(0x00100002u | ((1u << c) << 4)); patched.push_back(tempBase + row);
            patched.push_back(0x00208E46u); patched.push_back(slot); patched.push_back(first + row);
            patched.push_back(0x00208E46u); patched.push_back(kEyeCbSlot); patched.push_back(c);
        }
        patched.push_back(Op(kOpAdd, 8));
        patched.push_back(0x001000F2u); patched.push_back(tempBase + row);
        patched.push_back(0x00100E46u); patched.push_back(tempBase + row);
        patched.push_back(0x00208E46u); patched.push_back(slot); patched.push_back(first + row);
        if (river||water254)
        {
            // Input is (u,v,depth,1), not clip NDC. Postmultiply the row by
            // UV->NDC: (2u-1,1-2v,depth,1). Do w before overwriting x/y.
            patched.push_back(Op(kOpMad, 9));
            patched.push_back(0x00100082u); patched.push_back(tempBase + row);
            patched.push_back(0x00100006u); patched.push_back(tempBase + row);
            patched.push_back(0x00004001u); patched.push_back(0xBF800000u);
            patched.push_back(0x00100FF6u); patched.push_back(tempBase + row);
            patched.push_back(Op(kOpAdd, 7));
            patched.push_back(0x00100082u); patched.push_back(tempBase + row);
            patched.push_back(0x00100556u); patched.push_back(tempBase + row);
            patched.push_back(0x00100FF6u); patched.push_back(tempBase + row);
            patched.push_back(Op(56 /*mul*/, 10));
            patched.push_back(0x00100032u); patched.push_back(tempBase + row);
            patched.push_back(0x00100046u); patched.push_back(tempBase + row);
            patched.push_back(0x00004002u);
            patched.push_back(0x40000000u); patched.push_back(0xC0000000u);
            patched.push_back(0); patched.push_back(0);
        }
    }

    if (riverReflection||waterSurface254)
    {
        // The reflection is rendered by the GAME camera into a mono texture.
        // Its edge-fade/fallback uses screen position too; raw eye SV_Position
        // makes the reflected dark scenery slide with head rotation. Recover
        // the game pixel of THIS water surface, not the depth behind the water.
        const uint32_t C = tempBase + 4, G = tempBase + 5;
        const uint32_t all = Swz(0,1,2,3);
        std::vector<uint32_t> b;
        const uint32_t surface=waterSurface254?waterInput254:cave256?6:7,viewport=waterSurface254?waterViewport254:1;
        b.clear(); DstTemp(b,C,3); SrcInput(b,surface,Swz(0,1,0,0)); SrcInput(b,surface,Swz(2,2,2,2)); Emit(patched,kOpDiv,b);
        b.clear(); DstTemp(b,C,3); SrcTemp(b,C,all); SrcImm(b,F(2),F(-2),0,0); SrcImm(b,F(-1),F(1),0,0); Emit(patched,kOpMad,b);
        // InvProjection row3: homogeneous w = 1 / positive view distance.
        b.clear(); DstTemp(b,C,4); SrcImm(b,F(1),F(1),F(1),F(1)); SrcInput(b,surface,Swz(2,2,2,2)); Emit(patched,kOpDiv,b);
        b.clear(); DstTemp(b,C,4); SrcCb(b,viewport,19,Swz(3,3,3,3)); SrcImm(b,F(-1),F(-1),F(-1),F(-1)); SrcTemp(b,C,Swz(2,2,2,2)); Emit(patched,kOpMad,b);
        b.clear(); DstTemp(b,C,4); SrcTemp(b,C,Swz(2,2,2,2)); SrcCb(b,viewport,19,Swz(2,2,2,2)); Emit(patched,kOpDiv,b);
        b.clear(); DstTemp(b,C,8); SrcImm(b,F(1),F(1),F(1),F(1)); Emit(patched,54 /*mov*/,b);
        for(uint32_t c=0;c<4;++c) {
            b.clear(); DstTemp(b,G,15); SrcCb(b,kEyeCbSlot,c,all); SrcTemp(b,C,Swz(c,c,c,c)); SrcTemp(b,c?G:C,all); Emit(patched,kOpMad,b);
        }
        if(waterSurface254){
            // Native thickness terms compare native view distances. Keep the
            // depth-sampling UV divide in eye space, but restore surface-w for
            // subsequent native water-thickness ratios and near fades.
            b.clear();DstTemp(b,G,8);SrcTemp(b,G,Swz(3,3,3,3));SrcInput(b,surface,Swz(2,2,2,2));Emit(patched,56,b);
        }
        if(!waterSurface254){
        b.clear(); DstTemp(b,G,3); SrcTemp(b,G,all); SrcTemp(b,G,Swz(3,3,3,3)); Emit(patched,kOpDiv,b);
        b.clear(); DstTemp(b,G,3); SrcTemp(b,G,all); SrcImm(b,F(.5f),F(-.5f),0,0); SrcImm(b,F(.5f),F(.5f),0,0); Emit(patched,kOpMad,b);
        b.clear(); DstTemp(b,G,3); SrcTemp(b,G,all); SrcCb(b,1,37,Swz(0,1,0,0)); Emit(patched,56 /*mul*/,b);
        }
    }

    int replaced = 0;
    for (size_t i = firstCode; i < tokenCount;)
    {
        const uint32_t length = InstructionLength(&tokens[i], tokenCount - i);
        const size_t end = i + length;
        const uint32_t op = tokens[i] & 0x7ff;
        if (addTile && i == tileAt)
            EmitTileTest(patched, tileClip, tempBase);
        const size_t opcodeAt = patched.size();
        size_t cursor = i;
        uint32_t opcodeToken = tokens[cursor++];
        patched.push_back(opcodeToken);
        while (opcodeToken & 0x80000000u)
        {
            if (cursor >= end) return PixelResult::Malformed;
            opcodeToken = tokens[cursor++];
            patched.push_back(opcodeToken);
        }
        if (op == kOpCustomData)
        {
            patched.insert(patched.end(), tokens.begin() + cursor, tokens.begin() + end);
        }
        else
        {
            unsigned operandNumber=0;bool screenDivide=false;
            while (cursor < end)
            {
                // Only rewrite the two reflection-edge reads of SV_Position.
                // Preserve swizzles and the negate modifier on the first read.
                const size_t originalAt = cursor, outputAt = patched.size();
                if (!CopyOperand(tokens.data(), cursor, end, patched, slot, first, tempBase, 0, replaced))
                    return PixelResult::Unsupported;
                const uint32_t operand = tokens[originalAt];
                const bool surfaceInput=waterSurface254&&((operand>>12)&255)==kOperandInput&&tokens[cursor-1]==waterInput254;
                if(operandNumber==1)screenDivide=op==kOpDiv&&surfaceInput;
                if(surfaceInput&&!screenDivide&&operandNumber>0){
                    const uint32_t select=(operand>>2)&3;
                    if((select==1&&((operand>>4)&255)==0xaa)||(select==2&&((operand>>4)&3)==2)){
                        patched[outputAt]&=~(255u<<12);patched.back()=tempBase+5;
                        if(select==1)patched[outputAt]=(patched[outputAt]&~(255u<<4))|(255u<<4);
                        else patched[outputAt]=(patched[outputAt]&~(3u<<4))|(3u<<4);
                    }
                }
                if (riverReflection && ((operand >> 12) & 0xff) == kOperandInput &&
                    ((operand >> 20) & 3) == 1 && ((operand >> 22) & 7) == 0 &&
                    tokens[cursor - 1] == 0)
                {
                    patched[outputAt] &= ~(0xffu << 12);
                    patched.back() = tempBase + 5;
                }
                ++operandNumber;
            }
            if (cursor != end) return PixelResult::Unsupported;
            const uint32_t newLength = static_cast<uint32_t>(patched.size() - opcodeAt);
            if (newLength > 0x7f) return PixelResult::Unsupported;
            patched[opcodeAt] = (patched[opcodeAt] & ~(0x7fu << 24)) | (newLength << 24);
        }
        i = end;
    }
    if (replaced == 0) return PixelResult::Unsupported;
    patched[1] = static_cast<uint32_t>(patched.size());

    const uint32_t newChunkSize = static_cast<uint32_t>(patched.size() * 4);
    size_t newTotal = 32 + 4 * static_cast<size_t>(chunkCount);
    for (uint32_t c = 0; c < chunkCount; ++c)
        newTotal += 8 + (static_cast<int>(c) == shaderChunk ? newChunkSize : chunks[c].size);
    out.assign(newTotal, 0);
    std::memcpy(out.data(), src, 32);
    const uint32_t total32 = static_cast<uint32_t>(newTotal);
    std::memcpy(out.data() + 24, &total32, 4);
    size_t cursor = 32 + 4 * static_cast<size_t>(chunkCount);
    for (uint32_t c = 0; c < chunkCount; ++c)
    {
        const uint32_t offset = static_cast<uint32_t>(cursor);
        std::memcpy(out.data() + 32 + 4 * c, &offset, 4);
        const bool isShader = static_cast<int>(c) == shaderChunk;
        const uint32_t chunkSize = isShader ? newChunkSize : chunks[c].size;
        std::memcpy(out.data() + cursor, &chunks[c].tag, 4);
        std::memcpy(out.data() + cursor + 4, &chunkSize, 4);
        std::memcpy(out.data() + cursor + 8,
                    isShader ? static_cast<const void*>(patched.data())
                             : static_cast<const void*>(chunks[c].data), chunkSize);
        if (addTile && static_cast<int>(c) == isgnChunk)
            out[cursor + 8 + isgnReg1 + 21] |= 0x0C;     // TEXCOORD0 now reads zw too
        cursor += 8 + chunkSize;
    }
    uint32_t hash[4];
    ComputeChecksum(out.data(), out.size(), hash);
    std::memcpy(out.data() + 4, hash, 16);
    if (tileTest) *tileTest = addTile;
    return PixelResult::Patched;
}
}
