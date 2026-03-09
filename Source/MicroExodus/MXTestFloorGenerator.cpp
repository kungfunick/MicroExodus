// MXTestFloorGenerator.cpp — Phase 2C-Move: Procedural Test Floor
// Created: 2026-03-09

#include "MXTestFloorGenerator.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

AMXTestFloorGenerator::AMXTestFloorGenerator()
{
    PrimaryActorTick.bCanEverTick = false;

    // Create a root component.
    USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);
}

// ---------------------------------------------------------------------------
// BeginPlay
// ---------------------------------------------------------------------------

void AMXTestFloorGenerator::BeginPlay()
{
    Super::BeginPlay();

    if (bAutoGenerate)
    {
        GenerateFloor();
    }
}

// ---------------------------------------------------------------------------
// GenerateFloor
// ---------------------------------------------------------------------------

void AMXTestFloorGenerator::GenerateFloor()
{
    ClearFloor();
    LoadCubeMesh();
    CreateMaterials();

    if (!CubeMesh)
    {
        UE_LOG(LogTemp, Error, TEXT("MXTestFloorGenerator: Failed to load cube mesh."));
        return;
    }

    const float Step = TileSize + TileGap;

    // Center the grid on the actor's location.
    const float OffsetX = -(GridSizeX * Step) * 0.5f + Step * 0.5f;
    const float OffsetY = -(GridSizeY * Step) * 0.5f + Step * 0.5f;

    // The cube mesh is 100x100x100 by default. Scale to desired tile size.
    // X and Y scale = TileSize / 100. Z scale = TileThickness / 100.
    const FVector TileScale(TileSize / 100.0f, TileSize / 100.0f, TileThickness / 100.0f);

    // Position Z: top of tile at FloorZ means tile center is at FloorZ - TileThickness/2.
    const float TileCenterZ = FloorZ - TileThickness * 0.5f;

    for (int32 X = 0; X < GridSizeX; ++X)
    {
        for (int32 Y = 0; Y < GridSizeY; ++Y)
        {
            FName CompName = *FString::Printf(TEXT("Tile_%d_%d"), X, Y);
            UStaticMeshComponent* Tile = NewObject<UStaticMeshComponent>(this, CompName);
            if (!Tile) continue;

            Tile->SetStaticMesh(CubeMesh);
            Tile->SetWorldScale3D(TileScale);

            FVector TilePos(
                GetActorLocation().X + OffsetX + X * Step,
                GetActorLocation().Y + OffsetY + Y * Step,
                TileCenterZ
            );
            Tile->SetWorldLocation(TilePos);

            // Collision — critical for CharacterMovementComponent.
            Tile->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
            Tile->SetCollisionResponseToAllChannels(ECR_Block);
            Tile->SetCollisionObjectType(ECC_WorldStatic);

            // Material — checkerboard pattern.
            bool bAlt = bCheckerboard && ((X + Y) % 2 == 1);
            if (bAlt && AltFloorMaterial)
            {
                Tile->SetMaterial(0, AltFloorMaterial);
            }
            else if (FloorMaterial)
            {
                Tile->SetMaterial(0, FloorMaterial);
            }

            Tile->AttachToComponent(GetRootComponent(),
                FAttachmentTransformRules::KeepWorldTransform);
            Tile->RegisterComponent();

            FloorTiles.Add(Tile);
        }
    }

    // Cache bounds.
    const float HalfExtentX = (GridSizeX * Step) * 0.5f;
    const float HalfExtentY = (GridSizeY * Step) * 0.5f;
    CachedBounds = FBox(
        FVector(GetActorLocation().X - HalfExtentX, GetActorLocation().Y - HalfExtentY, FloorZ - TileThickness),
        FVector(GetActorLocation().X + HalfExtentX, GetActorLocation().Y + HalfExtentY, FloorZ)
    );

    UE_LOG(LogTemp, Log,
        TEXT("MXTestFloorGenerator: Generated %d tiles (%dx%d, %.0fcm each, surface at Z=%.0f)."),
        FloorTiles.Num(), GridSizeX, GridSizeY, TileSize, FloorZ);
}

// ---------------------------------------------------------------------------
// ClearFloor
// ---------------------------------------------------------------------------

void AMXTestFloorGenerator::ClearFloor()
{
    for (UStaticMeshComponent* Tile : FloorTiles)
    {
        if (Tile)
        {
            Tile->DestroyComponent();
        }
    }
    FloorTiles.Empty();
}

// ---------------------------------------------------------------------------
// Queries
// ---------------------------------------------------------------------------

FVector AMXTestFloorGenerator::GetFloorCenter() const
{
    return FVector(GetActorLocation().X, GetActorLocation().Y, FloorZ);
}

FBox AMXTestFloorGenerator::GetFloorBounds() const
{
    return CachedBounds;
}

FVector AMXTestFloorGenerator::GetRandomFloorPosition(float Margin) const
{
    const float Step = TileSize + TileGap;
    const float HalfX = (GridSizeX * Step) * 0.5f - Margin;
    const float HalfY = (GridSizeY * Step) * 0.5f - Margin;

    return FVector(
        GetActorLocation().X + FMath::RandRange(-HalfX, HalfX),
        GetActorLocation().Y + FMath::RandRange(-HalfY, HalfY),
        FloorZ
    );
}

// ---------------------------------------------------------------------------
// Private Helpers
// ---------------------------------------------------------------------------

void AMXTestFloorGenerator::LoadCubeMesh()
{
    if (CubeMesh) return;

    CubeMesh = LoadObject<UStaticMesh>(nullptr,
        TEXT("/Engine/BasicShapes/Cube.Cube"));

    if (!CubeMesh)
    {
        UE_LOG(LogTemp, Error,
            TEXT("MXTestFloorGenerator: Could not load /Engine/BasicShapes/Cube."));
    }
}

void AMXTestFloorGenerator::CreateMaterials()
{
    // Use the engine's basic material as a base.
    UMaterial* BaseMat = LoadObject<UMaterial>(nullptr,
        TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

    if (!BaseMat)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("MXTestFloorGenerator: Could not load BasicShapeMaterial. Tiles will use default."));
        return;
    }

    FloorMaterial = UMaterialInstanceDynamic::Create(BaseMat, this);
    if (FloorMaterial)
    {
        FloorMaterial->SetVectorParameterValue(TEXT("Color"), FloorColor);
    }

    AltFloorMaterial = UMaterialInstanceDynamic::Create(BaseMat, this);
    if (AltFloorMaterial)
    {
        AltFloorMaterial->SetVectorParameterValue(TEXT("Color"), AltFloorColor);
    }
}
