// MXTestFloorGenerator.h — Phase 2C-Move: Procedural Test Floor
// Created: 2026-03-09
// Generates a simple grid floor with collision for the spawn test level.
// Replaces the manual Plane mesh (which lacked collision and required GravityScale=0).
// Future: integrate with UMXProceduralGen for room-based layouts.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MXTestFloorGenerator.generated.h"

class UStaticMeshComponent;

// ---------------------------------------------------------------------------
// AMXTestFloorGenerator
// ---------------------------------------------------------------------------

/**
 * AMXTestFloorGenerator
 * Spawns a grid of floor tiles with collision for testing.
 * Each tile is a flattened cube (built-in engine mesh) with collision enabled.
 *
 * Usage: Place in level or spawn from GameMode. Call GenerateFloor() to build.
 *
 * Future phases will replace this with UMXProceduralGen room-based layouts.
 * This actor exists to provide a proper collision surface so robots can use
 * CharacterMovementComponent normally (no GravityScale hack).
 */
UCLASS(BlueprintType, Blueprintable)
class MICROEXODUS_API AMXTestFloorGenerator : public AActor
{
    GENERATED_BODY()

public:

    AMXTestFloorGenerator();

    virtual void BeginPlay() override;

    // -------------------------------------------------------------------------
    // Generation
    // -------------------------------------------------------------------------

    /**
     * Generate the floor grid. Safe to call multiple times (clears previous).
     */
    UFUNCTION(BlueprintCallable, Category = "MX|TestFloor")
    void GenerateFloor();

    /**
     * Clear all generated floor tiles.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|TestFloor")
    void ClearFloor();

    /**
     * Get the world-space center of the generated floor.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|TestFloor")
    FVector GetFloorCenter() const;

    /**
     * Get the world-space bounds of the generated floor.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|TestFloor")
    FBox GetFloorBounds() const;

    /**
     * Get a random walkable position on the floor surface.
     * @param Margin  Inset from edges in cm.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|TestFloor")
    FVector GetRandomFloorPosition(float Margin = 50.0f) const;

    // -------------------------------------------------------------------------
    // Config
    // -------------------------------------------------------------------------

    /** Number of tiles along X axis. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|TestFloor|Config")
    int32 GridSizeX = 10;

    /** Number of tiles along Y axis. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|TestFloor|Config")
    int32 GridSizeY = 10;

    /** Size of each tile in cm. Tiles are square. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|TestFloor|Config")
    float TileSize = 200.0f;

    /** Tile thickness in cm. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|TestFloor|Config")
    float TileThickness = 10.0f;

    /** Gap between tiles in cm (0 = flush). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|TestFloor|Config")
    float TileGap = 2.0f;

    /** Z position of the floor surface (top of tiles). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|TestFloor|Config")
    float FloorZ = 0.0f;

    /** Whether to auto-generate on BeginPlay. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|TestFloor|Config")
    bool bAutoGenerate = true;

    /** Base color for floor tiles. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|TestFloor|Config")
    FLinearColor FloorColor = FLinearColor(0.15f, 0.15f, 0.18f, 1.0f);

    /** Alternate color for checkerboard pattern. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|TestFloor|Config")
    FLinearColor AltFloorColor = FLinearColor(0.12f, 0.12f, 0.14f, 1.0f);

    /** Whether to use checkerboard pattern. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|TestFloor|Config")
    bool bCheckerboard = true;

private:

    /** Spawned tile components. */
    UPROPERTY()
    TArray<UStaticMeshComponent*> FloorTiles;

    /** Cached floor bounds. */
    FBox CachedBounds;

    /** Reference to the cube mesh (loaded once). */
    UPROPERTY()
    UStaticMesh* CubeMesh = nullptr;

    /** Material instances for floor tiles. */
    UPROPERTY()
    UMaterialInstanceDynamic* FloorMaterial = nullptr;

    UPROPERTY()
    UMaterialInstanceDynamic* AltFloorMaterial = nullptr;

    /** Load the engine cube mesh. */
    void LoadCubeMesh();

    /** Create material instances. */
    void CreateMaterials();
};
