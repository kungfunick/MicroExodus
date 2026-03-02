// MXRobotCodec.cpp — Robot Factory Module v1.0
// Agent 14: Robot Factory

#include "MXRobotCodec.h"

// ---------------------------------------------------------------------------
// Crockford Base32 alphabet: 0-9 A-H J-K M-N P-T V-W X-Y (no I, L, O, U)
// ---------------------------------------------------------------------------

const TCHAR UMXRobotCodec::Alphabet[32] = {
    TEXT('0'), TEXT('1'), TEXT('2'), TEXT('3'), TEXT('4'), TEXT('5'), TEXT('6'), TEXT('7'),
    TEXT('8'), TEXT('9'), TEXT('A'), TEXT('B'), TEXT('C'), TEXT('D'), TEXT('E'), TEXT('F'),
    TEXT('G'), TEXT('H'), TEXT('J'), TEXT('K'), TEXT('M'), TEXT('N'), TEXT('P'), TEXT('Q'),
    TEXT('R'), TEXT('S'), TEXT('T'), TEXT('V'), TEXT('W'), TEXT('X'), TEXT('Y'), TEXT('Z'),
};

TCHAR UMXRobotCodec::EncodeBits(uint8 Value)
{
    return Alphabet[Value & 0x1F];
}

int32 UMXRobotCodec::DecodeBits(TCHAR Char)
{
    // Uppercase the char.
    if (Char >= TEXT('a') && Char <= TEXT('z'))
    {
        Char = Char - TEXT('a') + TEXT('A');
    }

    // Handle common substitutions (Crockford spec).
    if (Char == TEXT('I') || Char == TEXT('L')) Char = TEXT('1');
    if (Char == TEXT('O')) Char = TEXT('0');

    for (int32 i = 0; i < 32; ++i)
    {
        if (Alphabet[i] == Char) return i;
    }
    return -1; // Invalid character.
}

// ---------------------------------------------------------------------------
// GUID Folding: 128-bit → 80-bit
// ---------------------------------------------------------------------------

void UMXRobotCodec::FoldGuid(const FGuid& Id, uint64& OutHigh40, uint64& OutLow40)
{
    // XOR-fold the four 32-bit words with Knuth multiplicative mixing.
    // This reduces 128 bits to 80 while maintaining good distribution.

    const uint64 A = static_cast<uint64>(Id.A);
    const uint64 B = static_cast<uint64>(Id.B);
    const uint64 C = static_cast<uint64>(Id.C);
    const uint64 D = static_cast<uint64>(Id.D);

    // Mix A and C into the high 40 bits.
    uint64 High = (A ^ (C * 2654435761ULL)) & 0xFFFFFFFFULL;
    // Take 8 extra bits from B's upper byte for bits 32–39.
    High |= ((B >> 24) & 0xFFULL) << 32;
    OutHigh40 = High & 0xFFFFFFFFFFULL; // Mask to 40 bits.

    // Mix B and D into the low 40 bits.
    uint64 Low = (B ^ (D * 2246822519ULL)) & 0xFFFFFFFFULL;
    // Take 8 extra bits from A's upper byte for bits 32–39.
    Low |= ((A >> 24) & 0xFFULL) << 32;
    OutLow40 = Low & 0xFFFFFFFFFFULL; // Mask to 40 bits.
}

FGuid UMXRobotCodec::UnfoldToGuid(uint64 High40, uint64 Low40)
{
    // Create a synthetic GUID from the 80-bit code seed.
    // Not the original GUID — but deterministic from the same code.
    FGuid Result;
    Result.A = static_cast<uint32>(High40 & 0xFFFFFFFF);
    Result.B = static_cast<uint32>(Low40 & 0xFFFFFFFF);
    Result.C = static_cast<uint32>((High40 >> 8) ^ (Low40 * 31));
    Result.D = static_cast<uint32>((Low40 >> 8) ^ (High40 * 127));
    return Result;
}

// ---------------------------------------------------------------------------
// Encoding: GUID → Code
// ---------------------------------------------------------------------------

FMXRobotCode UMXRobotCodec::EncodeFromGuid(const FGuid& RobotId)
{
    FMXRobotCode Result;

    uint64 High40 = 0;
    uint64 Low40 = 0;
    FoldGuid(RobotId, High40, Low40);

    Result.SeedHigh = static_cast<int64>(High40);
    Result.SeedLow = static_cast<int64>(Low40);

    // Pack 80 bits into 16 Base32 chars (5 bits each).
    // Combine into a single 80-bit stream: High40 in upper bits, Low40 in lower.
    uint64 BitStream[2] = { High40, Low40 };

    TCHAR RawCode[CODE_LENGTH + 1];
    RawCode[CODE_LENGTH] = TEXT('\0');

    for (int32 i = 0; i < CODE_LENGTH; ++i)
    {
        // Extract 5 bits for this character.
        // Characters 0–7 come from High40, characters 8–15 from Low40.
        int32 BitOffset;
        uint64 Source;

        if (i < 8)
        {
            Source = High40;
            BitOffset = (7 - i) * 5; // MSB first.
        }
        else
        {
            Source = Low40;
            BitOffset = (15 - i) * 5;
        }

        const uint8 FiveBits = static_cast<uint8>((Source >> BitOffset) & 0x1F);
        RawCode[i] = EncodeBits(FiveBits);
    }

    Result.CodeString = FormatCode(FString(RawCode));
    Result.bValid = true;

    return Result;
}

// ---------------------------------------------------------------------------
// Decoding: Code string → Parsed code
// ---------------------------------------------------------------------------

FMXRobotCode UMXRobotCodec::DecodeFromString(const FString& Input)
{
    FMXRobotCode Result;
    Result.bValid = false;

    // Strip dashes, spaces, and other separators.
    FString Cleaned = Input.ToUpper();
    Cleaned.ReplaceInline(TEXT("-"), TEXT(""));
    Cleaned.ReplaceInline(TEXT(" "), TEXT(""));
    Cleaned.ReplaceInline(TEXT("."), TEXT(""));

    if (Cleaned.Len() != CODE_LENGTH)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("MXRobotCodec: Invalid code length %d (expected %d): '%s'"),
            Cleaned.Len(), CODE_LENGTH, *Input);
        return Result;
    }

    // Decode each character to 5 bits.
    uint64 High40 = 0;
    uint64 Low40 = 0;

    for (int32 i = 0; i < CODE_LENGTH; ++i)
    {
        const int32 Value = DecodeBits(Cleaned[i]);
        if (Value < 0)
        {
            UE_LOG(LogTemp, Warning,
                TEXT("MXRobotCodec: Invalid character '%c' at position %d in code '%s'"),
                Cleaned[i], i, *Input);
            return Result;
        }

        if (i < 8)
        {
            const int32 BitOffset = (7 - i) * 5;
            High40 |= (static_cast<uint64>(Value) << BitOffset);
        }
        else
        {
            const int32 BitOffset = (15 - i) * 5;
            Low40 |= (static_cast<uint64>(Value) << BitOffset);
        }
    }

    Result.SeedHigh = static_cast<int64>(High40 & 0xFFFFFFFFFFULL);
    Result.SeedLow = static_cast<int64>(Low40 & 0xFFFFFFFFFFULL);
    Result.CodeString = FormatCode(Cleaned);
    Result.bValid = true;

    UE_LOG(LogTemp, Log,
        TEXT("MXRobotCodec: Decoded code '%s' → High=%lld Low=%lld"),
        *Result.CodeString, Result.SeedHigh, Result.SeedLow);

    return Result;
}

// ---------------------------------------------------------------------------
// Seed Derivation
// ---------------------------------------------------------------------------

void UMXRobotCodec::DeriveSeeds(
    const FMXRobotCode& Code,
    int32& OutNameSeed,
    int32& OutPersonalitySeed,
    int32& OutAppearanceSeed)
{
    // Each subsystem gets an independent seed derived from the code's 80 bits.
    // Different prime multipliers ensure independence.
    const uint64 H = static_cast<uint64>(Code.SeedHigh);
    const uint64 L = static_cast<uint64>(Code.SeedLow);

    // Name seed: mix high bits.
    OutNameSeed = static_cast<int32>((H ^ (L * 6364136223846793005ULL)) & 0x7FFFFFFF);

    // Personality seed: mix low bits with different constant.
    OutPersonalitySeed = static_cast<int32>(((L >> 5) ^ (H * 1442695040888963407ULL)) & 0x7FFFFFFF);

    // Appearance seed: mix both halves.
    OutAppearanceSeed = static_cast<int32>(((H + L) * 2862933555777941757ULL >> 16) & 0x7FFFFFFF);
}

FGuid UMXRobotCodec::DeriveAssemblyGuid(const FMXRobotCode& Code)
{
    return UnfoldToGuid(
        static_cast<uint64>(Code.SeedHigh),
        static_cast<uint64>(Code.SeedLow));
}

// ---------------------------------------------------------------------------
// Validation & Formatting
// ---------------------------------------------------------------------------

bool UMXRobotCodec::IsValidCodeFormat(const FString& Input)
{
    FString Cleaned = Input.ToUpper();
    Cleaned.ReplaceInline(TEXT("-"), TEXT(""));
    Cleaned.ReplaceInline(TEXT(" "), TEXT(""));
    Cleaned.ReplaceInline(TEXT("."), TEXT(""));

    if (Cleaned.Len() != CODE_LENGTH) return false;

    for (int32 i = 0; i < Cleaned.Len(); ++i)
    {
        if (DecodeBits(Cleaned[i]) < 0) return false;
    }
    return true;
}

FString UMXRobotCodec::FormatCode(const FString& RawCode)
{
    // Already formatted?
    if (RawCode.Contains(TEXT("-"))) return RawCode;

    FString Clean = RawCode.ToUpper();
    Clean.ReplaceInline(TEXT("-"), TEXT(""));
    Clean.ReplaceInline(TEXT(" "), TEXT(""));

    if (Clean.Len() != CODE_LENGTH) return Clean;

    // Insert dashes: XXXX-XXXX-XXXX-XXXX
    return FString::Printf(TEXT("%s-%s-%s-%s"),
        *Clean.Mid(0, 4),
        *Clean.Mid(4, 4),
        *Clean.Mid(8, 4),
        *Clean.Mid(12, 4));
}
