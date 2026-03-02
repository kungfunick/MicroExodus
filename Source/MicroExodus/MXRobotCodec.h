// MXRobotCodec.h — Robot Factory Module v1.0
// Created: 2026-03-01
// Agent 14: Robot Factory
// Deterministic bidirectional conversion: FGuid ↔ shareable robot code.
//
// Code format: XXXX-XXXX-XXXX-XXXX (16 chars, Base32-Crockford, 4 groups of 4)
//   Example: "7G4K-2MNP-QR9T-FJ6W"
//
// Why Base32-Crockford?
//   - No ambiguous chars (0/O, 1/I/L excluded)
//   - Case-insensitive (always uppercase)
//   - 16 chars × 5 bits = 80 bits — matches the 80 bits we need from the GUID
//   - Human-readable, easy to type on mobile/console
//
// Encoding:
//   The 128-bit GUID is reduced to 80 bits via XOR-folding:
//     bits[0..39]  = GUID.A XOR GUID.C (with bit mixing)
//     bits[40..79] = GUID.B XOR GUID.D (with bit mixing)
//   This is NOT reversible to the original GUID. Instead, we encode enough
//   information to deterministically regenerate the same robot:
//     - Same name (seeded from code)
//     - Same personality (seeded from code)
//     - Same appearance/colors (seeded from code)
//     - Same modular parts (seeded from code)
//
//   The code IS a seed — it doesn't store the GUID, it IS the identity.
//   When a fresh robot is created, its GUID is random but its code is derived.
//   When a robot is replicated from a code, a new GUID is assigned but all
//   visual/personality generation uses the code as seed → identical result.
//
// Change Log:
//   v1.0 — Initial implementation

#pragma once

#include "CoreMinimal.h"
#include "MXRobotCodec.generated.h"

// Forward declarations
struct FMXRobotProfile;

// ---------------------------------------------------------------------------
// FMXRobotCode — the parsed result of a code string
// ---------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct MICROEXODUS_API FMXRobotCode
{
    GENERATED_BODY()

    /** The formatted code string (e.g., "7G4K-2MNP-QR9T-FJ6W"). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString CodeString;

    /** The 80-bit seed packed into two uint64s (low 40 bits each). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int64 SeedHigh = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int64 SeedLow = 0;

    /** Whether this code was successfully parsed/generated. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bValid = false;
};

// ---------------------------------------------------------------------------
// UMXRobotCodec
// ---------------------------------------------------------------------------

/**
 * UMXRobotCodec
 * Stateless utility: GUID → code, code → seed, seed → deterministic robot attributes.
 *
 * Usage flow:
 *   Creating a robot's code:
 *     FMXRobotCode Code = UMXRobotCodec::EncodeFromGuid(Robot.robot_id);
 *     Robot.code = Code.CodeString; // Store or display
 *
 *   Replicating from a friend's code:
 *     FMXRobotCode Code = UMXRobotCodec::DecodeFromString("7G4K-2MNP-QR9T-FJ6W");
 *     if (Code.bValid)
 *     {
 *         int32 NameSeed, PersonalitySeed, AppearanceSeed;
 *         UMXRobotCodec::DeriveSeeds(Code, NameSeed, PersonalitySeed, AppearanceSeed);
 *         // Use seeds to generate identical name, personality, appearance
 *     }
 */
UCLASS(BlueprintType)
class MICROEXODUS_API UMXRobotCodec : public UObject
{
    GENERATED_BODY()

public:

    // -------------------------------------------------------------------------
    // Encoding: GUID → Code
    // -------------------------------------------------------------------------

    /**
     * Generate a shareable code from a robot's GUID.
     * Deterministic: same GUID always produces the same code.
     * @param RobotId  The robot's unique identifier.
     * @return         The encoded robot code.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Factory|Code")
    static FMXRobotCode EncodeFromGuid(const FGuid& RobotId);

    // -------------------------------------------------------------------------
    // Decoding: Code string → Parsed code
    // -------------------------------------------------------------------------

    /**
     * Parse a code string entered by the player.
     * Accepts formats: "7G4K-2MNP-QR9T-FJ6W", "7G4K2MNPQR9TFJ6W", "7g4k-2mnp-qr9t-fj6w"
     * @param Input  The raw input string.
     * @return       Parsed code with bValid indicating success.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Factory|Code")
    static FMXRobotCode DecodeFromString(const FString& Input);

    // -------------------------------------------------------------------------
    // Seed derivation: Code → deterministic seeds for robot generation
    // -------------------------------------------------------------------------

    /**
     * Derive independent seeds for each robot generation subsystem.
     * Each seed is deterministic from the code — identical codes produce
     * identical robots on any player's machine.
     *
     * @param Code               The parsed robot code.
     * @param OutNameSeed        Seed for the name generator.
     * @param OutPersonalitySeed Seed for personality archetype + likes/dislikes.
     * @param OutAppearanceSeed  Seed for chassis color, eye color, antenna style.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Factory|Code")
    static void DeriveSeeds(const FMXRobotCode& Code,
                            int32& OutNameSeed,
                            int32& OutPersonalitySeed,
                            int32& OutAppearanceSeed);

    /**
     * Derive a GUID-compatible seed for the assembly system.
     * Returns a synthetic FGuid that, when passed to UMXRobotAssembler::GenerateRecipe,
     * produces the same modular part selection as the original robot.
     * @param Code  The parsed robot code.
     * @return      A synthetic GUID for assembly seeding.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Factory|Code")
    static FGuid DeriveAssemblyGuid(const FMXRobotCode& Code);

    // -------------------------------------------------------------------------
    // Validation
    // -------------------------------------------------------------------------

    /**
     * Check if a code string looks syntactically valid (correct length, valid chars).
     * Does NOT check if the code matches an existing robot.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Factory|Code")
    static bool IsValidCodeFormat(const FString& Input);

    /**
     * Format a raw 16-char code into grouped format with dashes.
     * "7G4K2MNPQR9TFJ6W" → "7G4K-2MNP-QR9T-FJ6W"
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Factory|Code")
    static FString FormatCode(const FString& RawCode);

private:

    // -------------------------------------------------------------------------
    // Base32 Crockford alphabet (excludes I, L, O, U)
    // -------------------------------------------------------------------------

    static constexpr int32 CODE_LENGTH = 16;
    static constexpr int32 BITS_PER_CHAR = 5;

    /** Crockford Base32 encoding alphabet. */
    static const TCHAR Alphabet[32];

    /** Encode a 5-bit value (0–31) to a Crockford character. */
    static TCHAR EncodeBits(uint8 Value);

    /** Decode a Crockford character to a 5-bit value. Returns -1 on invalid. */
    static int32 DecodeBits(TCHAR Char);

    /** XOR-fold a 128-bit GUID into 80 bits (two uint64, low 40 bits each). */
    static void FoldGuid(const FGuid& Id, uint64& OutHigh40, uint64& OutLow40);

    /** Unfold 80 bits back into a synthetic GUID (for assembly seeding). */
    static FGuid UnfoldToGuid(uint64 High40, uint64 Low40);
};
