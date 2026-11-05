#include "dxbc_patch.h"

#include <cstring>

namespace dxbc
{
namespace
{
// ---- MD5 core (the container hash is MD5 with non-standard final padding) --

inline uint32_t Rol(uint32_t v, int s) { return (v << s) | (v >> (32 - s)); }

void Md5Transform(uint32_t state[4], const uint8_t block[64])
{
    static const uint32_t k[64] = {
        0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee, 0xf57c0faf, 0x4787c62a,
        0xa8304613, 0xfd469501, 0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
        0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821, 0xf61e2562, 0xc040b340,
        0x265e5a51, 0xe9b6c7aa, 0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
        0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed, 0xa9e3e905, 0xfcefa3f8,
        0x676f02d9, 0x8d2a4c8a, 0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
        0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70, 0x289b7ec6, 0xeaa127fa,
        0xd4ef3085, 0x04881d05, 0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
        0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039, 0x655b59c3, 0x8f0ccc92,
        0xffeff47d, 0x85845dd1, 0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
        0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391 };
    static const int r[64] = {
        7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
        5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20,
        4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
        6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21 };

    uint32_t w[16];
    std::memcpy(w, block, 64);
    uint32_t a = state[0], b = state[1], c = state[2], d = state[3];
    for (int i = 0; i < 64; ++i)
    {
        uint32_t f;
        int g;
        if (i < 16)      { f = (b & c) | (~b & d); g = i; }
        else if (i < 32) { f = (d & b) | (~d & c); g = (5 * i + 1) & 15; }
        else if (i < 48) { f = b ^ c ^ d;          g = (3 * i + 5) & 15; }
        else             { f = c ^ (b | ~d);       g = (7 * i) & 15; }
        const uint32_t temp = d;
        d = c;
        c = b;
        b = b + Rol(a + f + k[i] + w[g], r[i]);
        a = temp;
    }
    state[0] += a; state[1] += b; state[2] += c; state[3] += d;
}

// ---- tokenized program format ---------------------------------------------

constexpr uint32_t kOpAdd = 0;
constexpr uint32_t kOpAnd = 1;
constexpr uint32_t kOpDp4 = 17;
constexpr uint32_t kOpItof = 43;
constexpr uint32_t kOpLt = 49;
constexpr uint32_t kOpMul = 56;
constexpr uint32_t kSaturate = 0x2000;
constexpr uint32_t kOpMad = 50;
constexpr uint32_t kOpMin = 51;
constexpr uint32_t kOpLabel = 44;
constexpr uint32_t kOpCustomData = 53;
constexpr uint32_t kOpMov = 54;
constexpr uint32_t kOpMovc = 55;
constexpr uint32_t kOpNe = 57;
constexpr uint32_t kOpRet = 62;
constexpr uint32_t kOpRetc = 63;
constexpr uint32_t kOpDclConstantBuffer = 89;
constexpr uint32_t kOpDclTemps = 104;
constexpr uint32_t kOpDclGlobalFlags = 106;
constexpr uint32_t kOpInterfaceCall = 120;

constexpr uint32_t kOperandTemp = 0;
constexpr uint32_t kOperandOutput = 2;
constexpr uint32_t kOperandImmediate32 = 4;
constexpr uint32_t kOperandImmediate64 = 5;

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

// Walks one operand starting at t[i]. Rewrites a write to the position output
// register into a write to a temp. Returns false on a layout it cannot parse.
bool WalkOperand(uint32_t* t, size_t& i, size_t end, uint32_t posReg,
                 uint32_t posTemp, int depth)
{
    if (i >= end || depth > 3)
        return false;
    const size_t tokenIndex = i;
    const uint32_t token = t[i++];
    if (token & 0x80000000u)
    {
        for (;;)
        {
            if (i >= end) return false;
            const uint32_t extended = t[i++];
            if (!(extended & 0x80000000u)) break;
        }
    }

    const uint32_t componentMode = token & 3;
    const uint32_t type = (token >> 12) & 0xff;
    const uint32_t dimension = (token >> 20) & 3;

    if (type == kOperandImmediate32 || type == kOperandImmediate64)
    {
        const uint32_t values = componentMode == 1 ? 1 : componentMode == 2 ? 4 : 0;
        i += values * (type == kOperandImmediate64 ? 2 : 1);
        return i <= end;
    }

    for (uint32_t d = 0; d < dimension; ++d)
    {
        const uint32_t representation = (token >> (22 + 3 * d)) & 7;
        switch (representation)
        {
        case 0: // immediate32
            if (d == 0 && type == kOperandOutput && dimension == 1)
            {
                if (i >= end) return false;
                if (t[i] == posReg)
                {
                    t[tokenIndex] = (token & ~(0xffu << 12)) | (kOperandTemp << 12);
                    t[i] = posTemp;
                }
            }
            i += 1;
            break;
        case 1: // immediate64
            i += 2;
            break;
        case 2: // relative
            if (!WalkOperand(t, i, end, posReg, posTemp, depth + 1)) return false;
            break;
        case 3: // immediate32 + relative
            i += 1;
            if (!WalkOperand(t, i, end, posReg, posTemp, depth + 1)) return false;
            break;
        case 4: // immediate64 + relative
            i += 2;
            if (!WalkOperand(t, i, end, posReg, posTemp, depth + 1)) return false;
            break;
        default:
            return false;
        }
        if (i > end) return false;
    }
    return true;
}

constexpr uint32_t Op(uint32_t opcode, uint32_t length) { return opcode | (length << 24); }
// 4-component temp, single-component write mask
constexpr uint32_t TempDst(uint32_t component) { return 0x00100002u | ((1u << component) << 4); }
// 4-component temp, single-component select
constexpr uint32_t TempSrc(uint32_t component) { return 0x0010000Au | (component << 4); }
}

void ComputeChecksum(const uint8_t* data, size_t size, uint32_t hash[4])
{
    uint32_t state[4] = { 0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476 };
    if (size < 20)
    {
        std::memcpy(hash, state, 16);
        return;
    }
    const uint8_t* body = data + 20;
    const uint32_t length = static_cast<uint32_t>(size - 20);
    const uint32_t bits = length * 8;
    const uint32_t full = length & ~63u;
    for (uint32_t offset = 0; offset < full; offset += 64)
        Md5Transform(state, body + offset);

    const uint32_t last = length - full;
    const uint8_t* tail = body + full;
    uint8_t block[64];
    if (last >= 56)
    {
        std::memset(block, 0, 64);
        std::memcpy(block, tail, last);
        block[last] = 0x80;
        Md5Transform(state, block);

        std::memset(block, 0, 64);
        std::memcpy(block, &bits, 4);
        const uint32_t trailer = (bits >> 2) | 1;
        std::memcpy(block + 60, &trailer, 4);
        Md5Transform(state, block);
    }
    else
    {
        std::memset(block, 0, 64);
        std::memcpy(block, &bits, 4);
        if (last)
            std::memcpy(block + 4, tail, last);
        block[4 + last] = 0x80;
        const uint32_t trailer = (bits >> 2) | 1;
        std::memcpy(block + 60, &trailer, 4);
        Md5Transform(state, block);
    }
    std::memcpy(hash, state, 16);
}

bool VerifyChecksum(const void* data, size_t size)
{
    if (!data || size < 32)
        return false;
    uint32_t hash[4];
    ComputeChecksum(static_cast<const uint8_t*>(data), size, hash);
    return std::memcmp(hash, static_cast<const uint8_t*>(data) + 4, 16) == 0;
}

Result PatchVertexShaderImpl(const void* bytecode, size_t size, std::vector<uint8_t>& out, bool arm);

Result PatchVertexShader(const void* bytecode, size_t size, std::vector<uint8_t>& out)
{
    return PatchVertexShaderImpl(bytecode, size, out, false);
}

Result PatchArmVertexShader(const void* bytecode, size_t size, std::vector<uint8_t>& out)
{
    return PatchVertexShaderImpl(bytecode, size, out, true);
}

namespace
{
uint32_t FloatBits(float v)
{
    uint32_t bits = 0;
    std::memcpy(&bits, &v, 4);
    return bits;
}
}

Result PatchVertexShaderImpl(const void* bytecode, size_t size, std::vector<uint8_t>& out, bool arm)
{
    const uint8_t* src = static_cast<const uint8_t*>(bytecode);
    if (!src || size < 32 || std::memcmp(src, "DXBC", 4) != 0)
        return Result::Malformed;

    uint32_t totalSize = 0, chunkCount = 0;
    std::memcpy(&totalSize, src + 24, 4);
    std::memcpy(&chunkCount, src + 28, 4);
    if (totalSize > size || chunkCount == 0 || chunkCount > 64 ||
        32 + 4 * static_cast<size_t>(chunkCount) > totalSize)
        return Result::Malformed;

    struct Chunk { uint32_t tag; uint32_t size; const uint8_t* data; };
    Chunk chunks[64];
    int shaderChunk = -1;
    int outputChunk = -1;
    for (uint32_t c = 0; c < chunkCount; ++c)
    {
        uint32_t offset = 0;
        std::memcpy(&offset, src + 32 + 4 * c, 4);
        if (offset + 8 > totalSize)
            return Result::Malformed;
        std::memcpy(&chunks[c].tag, src + offset, 4);
        std::memcpy(&chunks[c].size, src + offset + 4, 4);
        chunks[c].data = src + offset + 8;
        if (offset + 8 + static_cast<uint64_t>(chunks[c].size) > totalSize)
            return Result::Malformed;
        if (chunks[c].tag == 0x52444853 /*SHDR*/ || chunks[c].tag == 0x58454853 /*SHEX*/)
            shaderChunk = static_cast<int>(c);
        if (chunks[c].tag == 0x4E47534F /*OSGN*/)
            outputChunk = static_cast<int>(c);
    }
    if (shaderChunk < 0 || chunks[shaderChunk].size < 8)
        return Result::Malformed;

    const size_t tokenCount = chunks[shaderChunk].size / 4;
    std::vector<uint32_t> tokens(tokenCount);
    std::memcpy(tokens.data(), chunks[shaderChunk].data, tokenCount * 4);
    if ((tokens[0] >> 16) != 1)
        return Result::NotVertexShader;
    if (tokens[1] != tokenCount)
        return Result::Malformed;
    if (outputChunk < 0 || chunks[outputChunk].size < 8)
        return Result::Unsupported;

    // Find the register that carries SV_Position.
    uint32_t posReg = 0xffffffffu;
    {
        const uint8_t* og = chunks[outputChunk].data;
        uint32_t elements = 0;
        std::memcpy(&elements, og, 4);
        if (8 + 24 * static_cast<uint64_t>(elements) > chunks[outputChunk].size)
            return Result::Malformed;
        for (uint32_t e = 0; e < elements; ++e)
        {
            uint32_t systemValue = 0, reg = 0;
            uint8_t mask = 0;
            std::memcpy(&systemValue, og + 8 + 24 * e + 8, 4);
            std::memcpy(&reg, og + 8 + 24 * e + 16, 4);
            mask = og[8 + 24 * e + 20];
            if (systemValue == 1)
            {
                if (mask != 0xf)
                    return Result::Unsupported;
                posReg = reg;
            }
        }
    }
    if (posReg == 0xffffffffu)
        return Result::Unsupported;

    // First pass: validate the layout and locate the edit points.
    size_t temps = 0;
    size_t tempsIndex = 0;          // index of the dcl_temps count token, 0 = none
    size_t firstCode = 0;           // first non-declaration instruction
    size_t retIndex = 0;
    size_t afterGlobalFlags = 2;
    bool usesEyeSlot = false;
    for (size_t i = 2; i < tokenCount;)
    {
        const uint32_t op = tokens[i] & 0x7ff;
        const uint32_t length = InstructionLength(&tokens[i], tokenCount - i);
        if (length == 0 || i + length > tokenCount)
            return Result::Malformed;

        if (IsDeclaration(op))
        {
            if (firstCode != 0)
                return Result::Unsupported;     // declaration after code
            if (op == kOpDclTemps && length == 2)
            {
                temps = tokens[i + 1];
                tempsIndex = i + 1;
            }
            if (op == kOpDclConstantBuffer && length == 4 && tokens[i + 2] == kEyeCbSlot)
                usesEyeSlot = true;
            if (op == kOpDclGlobalFlags && i == 2)
                afterGlobalFlags = i + length;
        }
        else
        {
            if (firstCode == 0)
                firstCode = i;
            if (op == kOpRetc || op == kOpLabel || op == kOpInterfaceCall)
                return Result::Unsupported;
            if (op == kOpRet)
            {
                if (retIndex != 0 || i + length != tokenCount)
                    return Result::Unsupported; // only a single trailing ret
                retIndex = i;
            }
        }
        i += length;
    }
    if (firstCode == 0 || retIndex == 0 || usesEyeSlot)
        return Result::Unsupported;

    const uint32_t posTemp = static_cast<uint32_t>(temps);
    const uint32_t delta = posTemp + 1;
    const uint32_t mask = posTemp + 2;
    const uint32_t flat = posTemp + 3;
    const uint32_t weights = posTemp + 4;    // arm only
    const uint32_t row = posTemp + 5;        // arm only
    const uint32_t extraTemps = 6u;

    // Second pass: redirect every write to the position register into posTemp.
    for (size_t i = firstCode; i < tokenCount;)
    {
        const uint32_t length = InstructionLength(&tokens[i], tokenCount - i);
        const size_t end = i + length;
        size_t cursor = i + 1;
        uint32_t opcodeToken = tokens[i];
        while (opcodeToken & 0x80000000u)
        {
            if (cursor >= end) return Result::Malformed;
            opcodeToken = tokens[cursor++];
        }
        while (cursor < end)
        {
            if (!WalkOperand(tokens.data(), cursor, end, posReg, posTemp, 0))
                return Result::Unsupported;
        }
        if (cursor != end)
            return Result::Unsupported;
        i = end;
    }

    // Assemble the new token stream.
    std::vector<uint32_t> patched;
    patched.reserve(tokenCount + 64);
    patched.insert(patched.end(), tokens.begin(), tokens.begin() + afterGlobalFlags);
    // dcl_constantbuffer cb13[6], immediateIndexed
    patched.push_back(Op(kOpDclConstantBuffer, 4));
    patched.push_back(0x00208E46u);
    patched.push_back(kEyeCbSlot);
    patched.push_back(kEyeRows);
    if (tempsIndex != 0)
    {
        patched.insert(patched.end(), tokens.begin() + afterGlobalFlags,
                       tokens.begin() + firstCode);
        patched[tempsIndex + 4] = static_cast<uint32_t>(temps + extraTemps);
    }
    else
    {
        patched.insert(patched.end(), tokens.begin() + afterGlobalFlags,
                       tokens.begin() + firstCode);
        patched.push_back(Op(kOpDclTemps, 2));
        patched.push_back(extraTemps);
    }
    patched.insert(patched.end(), tokens.begin() + firstCode, tokens.begin() + retIndex);

    if (arm)
    {
        // weights.x = (bind x)^2 in metres^2
        // itof weights.x, v0.x
        patched.push_back(Op(kOpItof, 5));
        patched.push_back(TempDst(0)); patched.push_back(weights);
        patched.push_back(0x0010100Au); patched.push_back(0);
        // mul weights.x, weights.x, l(1/16384)
        patched.push_back(Op(kOpMul, 7));
        patched.push_back(TempDst(0)); patched.push_back(weights);
        patched.push_back(TempSrc(0)); patched.push_back(weights);
        patched.push_back(0x00004001u); patched.push_back(FloatBits(1.0f / 16384.0f));
        // mul weights.x, weights.x, weights.x
        patched.push_back(Op(kOpMul, 7));
        patched.push_back(TempDst(0)); patched.push_back(weights);
        patched.push_back(TempSrc(0)); patched.push_back(weights);
        patched.push_back(TempSrc(0)); patched.push_back(weights);
        // Ramps: past the elbow (|x| 0.36..0.40 m) and past the wrist (0.56..0.60 m).
        const float elbowA = 0.36f * 0.36f, elbowB = 0.40f * 0.40f;
        const float wristA = 0.56f * 0.56f, wristB = 0.60f * 0.60f;
        // mad_sat weights.y, weights.x, l(k), l(c)      0 = upper arm .. 1 = forearm or further
        patched.push_back(Op(kOpMad, 9) | kSaturate);
        patched.push_back(TempDst(1)); patched.push_back(weights);
        patched.push_back(TempSrc(0)); patched.push_back(weights);
        patched.push_back(0x00004001u); patched.push_back(FloatBits(1.0f / (elbowB - elbowA)));
        patched.push_back(0x00004001u); patched.push_back(FloatBits(-elbowA / (elbowB - elbowA)));
        // mad_sat weights.z, weights.x, l(k), l(c)      1 = hand
        patched.push_back(Op(kOpMad, 9) | kSaturate);
        patched.push_back(TempDst(2)); patched.push_back(weights);
        patched.push_back(TempSrc(0)); patched.push_back(weights);
        patched.push_back(0x00004001u); patched.push_back(FloatBits(1.0f / (wristB - wristA)));
        patched.push_back(0x00004001u); patched.push_back(FloatBits(-wristA / (wristB - wristA)));
        // mad weights.w, weights.y, l(-1), l(1)          upper arm weight
        patched.push_back(Op(kOpMad, 9));
        patched.push_back(TempDst(3)); patched.push_back(weights);
        patched.push_back(TempSrc(1)); patched.push_back(weights);
        patched.push_back(0x00004001u); patched.push_back(FloatBits(-1.0f));
        patched.push_back(0x00004001u); patched.push_back(FloatBits(1.0f));
        // mad weights.y, weights.z, l(-1), weights.y      forearm weight
        patched.push_back(Op(kOpMad, 9));
        patched.push_back(TempDst(1)); patched.push_back(weights);
        patched.push_back(TempSrc(2)); patched.push_back(weights);
        patched.push_back(0x00004001u); patched.push_back(FloatBits(-1.0f));
        patched.push_back(TempSrc(1)); patched.push_back(weights);
        for (uint32_t c = 0; c < 4; ++c)
        {
            // mul row, cb13[10 + c], weights.wwww      upper arm
            patched.push_back(Op(kOpMul, 8));
            patched.push_back(0x001000F2u); patched.push_back(row);
            patched.push_back(0x00208E46u); patched.push_back(kEyeCbSlot); patched.push_back(10 + c);
            patched.push_back(0x00100FF6u); patched.push_back(weights);
            // mad row, cb13[6 + c], weights.yyyy, row      forearm
            patched.push_back(Op(kOpMad, 10));
            patched.push_back(0x001000F2u); patched.push_back(row);
            patched.push_back(0x00208E46u); patched.push_back(kEyeCbSlot); patched.push_back(6 + c);
            patched.push_back(0x00100556u); patched.push_back(weights);
            patched.push_back(0x00100E46u); patched.push_back(row);
            // mad row, cb13[c], weights.zzzz, row          hand
            patched.push_back(Op(kOpMad, 10));
            patched.push_back(0x001000F2u); patched.push_back(row);
            patched.push_back(0x00208E46u); patched.push_back(kEyeCbSlot); patched.push_back(c);
            patched.push_back(0x00100AA6u); patched.push_back(weights);
            patched.push_back(0x00100E46u); patched.push_back(row);
            // dp4 delta.c, pos, row
            patched.push_back(Op(kOpDp4, 7));
            patched.push_back(TempDst(c)); patched.push_back(delta);
            patched.push_back(0x00100E46u); patched.push_back(posTemp);
            patched.push_back(0x00100E46u); patched.push_back(row);
        }
    }
    else
    {
    // ne mask.x, pos.w, l(1.0)               perspective vertex?
    patched.push_back(Op(kOpNe, 7));
    patched.push_back(TempDst(0)); patched.push_back(mask);
    patched.push_back(TempSrc(3)); patched.push_back(posTemp);
    patched.push_back(0x00004001u); patched.push_back(0x3F800000u);
    // dp4 delta.c, pos.xyzw, cb13[c].xyzw     delta = (eyeMatrix - identity) * pos
    for (uint32_t c = 0; c < 4; ++c)
    {
        patched.push_back(Op(kOpDp4, 8));
        patched.push_back(TempDst(c)); patched.push_back(delta);
        patched.push_back(0x00100E46u); patched.push_back(posTemp);
        patched.push_back(0x00208E46u); patched.push_back(kEyeCbSlot); patched.push_back(c);
    }
    // mad flat.xyzw, pos.xyzw, cb13[5].xyzw, cb13[4].xyzw     flat vertices: scale and offset (UI panel)
    patched.push_back(Op(kOpMad, 11));
    patched.push_back(0x001000F2u); patched.push_back(flat);
    patched.push_back(0x00100E46u); patched.push_back(posTemp);
    patched.push_back(0x00208E46u); patched.push_back(kEyeCbSlot); patched.push_back(5);
    patched.push_back(0x00208E46u); patched.push_back(kEyeCbSlot); patched.push_back(4);
    // Flat vertices near the centre of the screen (|x| < 0.06, |y| < 0.1):
    // the crosshair, which gets its own placement (rows 14 and 15).
    // lt weights.x, pos.x, l(0.06)
    patched.push_back(Op(kOpLt, 7));
    patched.push_back(TempDst(0)); patched.push_back(weights);
    patched.push_back(TempSrc(0)); patched.push_back(posTemp);
    patched.push_back(0x00004001u); patched.push_back(FloatBits(0.06f));
    // lt weights.y, l(-0.06), pos.x
    patched.push_back(Op(kOpLt, 7));
    patched.push_back(TempDst(1)); patched.push_back(weights);
    patched.push_back(0x00004001u); patched.push_back(FloatBits(-0.06f));
    patched.push_back(TempSrc(0)); patched.push_back(posTemp);
    // lt weights.z, pos.y, l(0.1)
    patched.push_back(Op(kOpLt, 7));
    patched.push_back(TempDst(2)); patched.push_back(weights);
    patched.push_back(TempSrc(1)); patched.push_back(posTemp);
    patched.push_back(0x00004001u); patched.push_back(FloatBits(0.1f));
    // lt weights.w, l(-0.1), pos.y
    patched.push_back(Op(kOpLt, 7));
    patched.push_back(TempDst(3)); patched.push_back(weights);
    patched.push_back(0x00004001u); patched.push_back(FloatBits(-0.1f));
    patched.push_back(TempSrc(1)); patched.push_back(posTemp);
    // and weights.x, weights.x, weights.y / and weights.z, weights.z, weights.w / and weights.x, weights.x, weights.z
    const uint32_t ands[3][3] = { { 0, 0, 1 }, { 2, 2, 3 }, { 0, 0, 2 } };
    for (const auto& a : ands)
    {
        patched.push_back(Op(kOpAnd, 7));
        patched.push_back(TempDst(a[0])); patched.push_back(weights);
        patched.push_back(TempSrc(a[1])); patched.push_back(weights);
        patched.push_back(TempSrc(a[2])); patched.push_back(weights);
    }
    // mad row.xyzw, pos.xyzw, cb13[15].xyzw, cb13[14].xyzw
    patched.push_back(Op(kOpMad, 11));
    patched.push_back(0x001000F2u); patched.push_back(row);
    patched.push_back(0x00100E46u); patched.push_back(posTemp);
    patched.push_back(0x00208E46u); patched.push_back(kEyeCbSlot); patched.push_back(15);
    patched.push_back(0x00208E46u); patched.push_back(kEyeCbSlot); patched.push_back(14);
    // movc flat.xyzw, weights.xxxx, row.xyzw, flat.xyzw
    patched.push_back(Op(kOpMovc, 9));
    patched.push_back(0x001000F2u); patched.push_back(flat);
    patched.push_back(0x00100006u); patched.push_back(weights);
    patched.push_back(0x00100E46u); patched.push_back(row);
    patched.push_back(0x00100E46u); patched.push_back(flat);
    // movc delta.xyzw, mask.xxxx, delta.xyzw, flat.xyzw
    patched.push_back(Op(kOpMovc, 9));
    patched.push_back(0x001000F2u); patched.push_back(delta);
    patched.push_back(0x00100006u); patched.push_back(mask);
    patched.push_back(0x00100E46u); patched.push_back(delta);
    patched.push_back(0x00100E46u); patched.push_back(flat);
    }
    // add delta.xyzw, pos.xyzw, delta.xyzw
    patched.push_back(Op(kOpAdd, 7));
    patched.push_back(0x001000F2u); patched.push_back(delta);
    patched.push_back(0x00100E46u); patched.push_back(posTemp);
    patched.push_back(0x00100E46u); patched.push_back(delta);
    // The sky sits exactly on the far plane (depth 1). Turned to the eye's view,
    // rounding puts parts of it a hair beyond, and the GPU drops them. Keep every
    // vertex in front of the eye at depth <= 1:
    // min mask.y, delta.z, delta.w
    patched.push_back(Op(kOpMin, 7));
    patched.push_back(TempDst(1)); patched.push_back(mask);
    patched.push_back(TempSrc(2)); patched.push_back(delta);
    patched.push_back(TempSrc(3)); patched.push_back(delta);
    // lt mask.z, l(0.0), delta.w
    patched.push_back(Op(kOpLt, 7));
    patched.push_back(TempDst(2)); patched.push_back(mask);
    patched.push_back(0x00004001u); patched.push_back(0x00000000u);
    patched.push_back(TempSrc(3)); patched.push_back(delta);
    // movc delta.z, mask.z, mask.y, delta.z
    patched.push_back(Op(kOpMovc, 9));
    patched.push_back(TempDst(2)); patched.push_back(delta);
    patched.push_back(TempSrc(2)); patched.push_back(mask);
    patched.push_back(TempSrc(1)); patched.push_back(mask);
    patched.push_back(TempSrc(2)); patched.push_back(delta);
    // mov oPos.xyzw, delta.xyzw
    patched.push_back(Op(kOpMov, 5));
    patched.push_back(0x001020F2u); patched.push_back(posReg);
    patched.push_back(0x00100E46u); patched.push_back(delta);

    // Captured shaders whose additional outputs are projections of SV_Position.
    // Geometry already moved to the eye above. Its depth/refraction coordinates
    // must move with it. Exact bytecode hashes keep all other shaders untouched.
    uint64_t screenHash = 14695981039346656037ull;
    for (size_t i = 0; i < size; ++i) { screenHash ^= src[i]; screenHash *= 1099511628211ull; }
    const bool particle=screenHash==0xF753F9846F8728BAull;
    const bool interior = screenHash == 0x2287E89CC64747EDull;
    const bool water = screenHash == 0xE01EBD2966163B82ull || screenHash == 0xF23C874C31A8D0A0ull;
    const bool riverDiffuse = screenHash == 0x19F9BF97F64AD98Dull;
    const bool river = screenHash == 0x97ADE3799CCB9D8Dull || riverDiffuse;
    if (interior || water || river || particle)
    {
        const uint32_t output = particle?5:riverDiffuse ? 6 : (river ? 7 : (interior ? 2 : 4));
        const uint32_t viewport = river ? 0 : 1;
        // flat.xy = eyeClip.w * _DepthTextureOffset.xy
        patched.push_back(Op(kOpMul, 8));
        patched.push_back(0x00100032u); patched.push_back(flat);
        patched.push_back(TempSrc(3)); patched.push_back(delta);
        patched.push_back(0x00208046u); patched.push_back(viewport); patched.push_back(25);
        // oScreen.xy = eyeClip.xy * _DepthTextureScale.xy + flat.xy
        patched.push_back(Op(kOpMad, 10));
        patched.push_back(0x00102032u); patched.push_back(output);
        patched.push_back(0x00100046u); patched.push_back(delta);
        patched.push_back(0x00208046u); patched.push_back(viewport); patched.push_back(24);
        patched.push_back(0x00100046u); patched.push_back(flat);
        // oScreen.z = eyeClip.w (perspective divide in the pixel shader)
        patched.push_back(Op(kOpMov, 5));
        patched.push_back(0x00102042u); patched.push_back(output);
        patched.push_back(TempSrc(3)); patched.push_back(delta);
        if (interior)
        {
            // PS compares this against normalized LINEAR eye depth, not game depth.
            patched.push_back(Op(kOpMul, 8));
            patched.push_back(0x00102082u); patched.push_back(1);
            patched.push_back(TempSrc(3)); patched.push_back(delta);
            patched.push_back(0x0020803Au); patched.push_back(1); patched.push_back(36);
        }
        else if (water)
        {
            // o6 = ((eyeX+eyeW)/2, (eyeW-eyeY)/2, eyeW).
            // o5 belongs to a separate, mono reflection camera; preserve it.
            patched.push_back(Op(kOpMul, 10));
            patched.push_back(0x00100032u); patched.push_back(flat);
            patched.push_back(TempSrc(3)); patched.push_back(delta);
            patched.push_back(0x00004002u);
            patched.push_back(FloatBits(.5f)); patched.push_back(FloatBits(.5f));
            patched.push_back(0); patched.push_back(0);
            patched.push_back(Op(kOpMad, 12));
            patched.push_back(0x00102032u); patched.push_back(6);
            patched.push_back(0x00100046u); patched.push_back(delta);
            patched.push_back(0x00004002u);
            patched.push_back(FloatBits(.5f)); patched.push_back(FloatBits(-.5f));
            patched.push_back(0); patched.push_back(0);
            patched.push_back(0x00100046u); patched.push_back(flat);
            patched.push_back(Op(kOpMov, 5));
            patched.push_back(0x00102042u); patched.push_back(6);
            patched.push_back(TempSrc(3)); patched.push_back(delta);
        }
    }

    patched.insert(patched.end(), tokens.begin() + retIndex, tokens.end());
    patched[1] = static_cast<uint32_t>(patched.size());

    // Rebuild the container around the new shader chunk.
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
        if (isShader)
            std::memcpy(out.data() + cursor + 8, patched.data(), chunkSize);
        else
            std::memcpy(out.data() + cursor + 8, chunks[c].data, chunkSize);
        cursor += 8 + chunkSize;
    }

    uint32_t hash[4];
    ComputeChecksum(out.data(), out.size(), hash);
    std::memcpy(out.data() + 4, hash, 16);
    return Result::Patched;
}
}
