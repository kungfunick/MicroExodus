// MXRobotActor.h — Phase 2C-Move Update
// Originally created: Phase 2A
// Updated: 2026-03-09 — Added selection/hover state, move-to-target, conditional name display
//
// ACharacter subclass representing a single robot in the world.
// Spawned by GameMode, bound to an FMXRobotProfile via BindToProfile().

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MXTypes.h"
#include "MXRobotActor.generated.h"

class UTextRenderComponent;
class UStaticMeshComponent;
class UMXAnimBridge;
struct FMXRobotProfile;

// ---------------------------------------------------------------------------
// Delegates
// ---------------------------------------------------------------------------

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRobotClicked, AMXRobotActor*, Robot, bool, bSelected);

// ---------------------------------------------------------------------------
// AMXRobotActor
// ---------------------------------------------------------------------------

UCLASS(BlueprintType, Blueprintable)
class MICROEXODUS_API AMXRobotActor : public ACharacter
{
    GENERATED_BODY()

public:

    AMXRobotActor();

protected:

    virtual void BeginPlay() override;

public:

    virtual void Tick(float DeltaTime) override;

    // -------------------------------------------------------------------------
    // Profile Binding
    // -------------------------------------------------------------------------

    /**
     * Bind this actor to a robot profile. Sets ID, name, and updates display.
     * @param InRobotId    The robot's GUID from the Identity module.
     * @param InRobotName  The robot's generated name.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Robot")
    void BindToProfile(const FGuid& InRobotId, const FString& InRobotName);

    /**
     * Bind this actor to a full robot profile. Populates all editor-visible identity fields.
     * @param Profile  The complete robot profile from the Identity module.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Robot")
    void BindToFullProfile(const FMXRobotProfile& Profile);

    /** The robot's unique identity. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MX|Robot|Identity")
    FGuid RobotId;

    /** The robot's display name. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MX|Robot|Identity")
    FString RobotName;

    // -------------------------------------------------------------------------
    // Generated Profile (visible in editor Details panel)
    // -------------------------------------------------------------------------

    /** Personality archetype description (e.g., "cautious optimist"). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MX|Robot|Profile")
    FString PersonalityDescription;

    /** Defining quirk that colors this robot's narrative voice. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MX|Robot|Profile")
    FString Quirk;

    /** Things this robot likes. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MX|Robot|Profile")
    TArray<FString> Likes;

    /** Things this robot dislikes. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MX|Robot|Profile")
    TArray<FString> Dislikes;

    /** Current role assignment. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MX|Robot|Profile")
    ERobotRole RobotRole = ERobotRole::None;

    /** Current level. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MX|Robot|Profile")
    int32 Level = 1;

    /** Currently displayed title. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MX|Robot|Profile")
    FString DisplayedTitle;

    /** Chassis color index. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MX|Robot|Profile")
    int32 ChassisColor = 0;

    /** Eye color index. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MX|Robot|Profile")
    int32 EyeColor = 0;

    // -------------------------------------------------------------------------
    // Selection & Hover (Phase 2C)
    // -------------------------------------------------------------------------

    /**
     * Set this robot's selection state. Called by UMXSelectionManager.
     * Updates visual indicators and name display visibility.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Robot|Selection")
    void SetSelected(bool bNewSelected);

    /**
     * Set this robot's hover state. Called by UMXSelectionManager.
     * Shows name on hover even when not selected.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Robot|Selection")
    void SetHovered(bool bNewHovered);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Robot|Selection")
    bool IsSelected() const { return bSelected; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Robot|Selection")
    bool IsHovered() const { return bHovered; }

    // -------------------------------------------------------------------------
    // Movement (Phase 2C-Move)
    // -------------------------------------------------------------------------

    /**
     * Command this robot to move toward a world-space target.
     * The robot will move each tick until it arrives within StopDistance.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Robot|Movement")
    void MoveToLocation(FVector Target);

    /**
     * Stop moving. Clears the current move target.
     */
    UFUNCTION(BlueprintCallable, Category = "MX|Robot|Movement")
    void StopMoving();

    /** Whether this robot currently has a movement target. */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Robot|Movement")
    bool HasMoveTarget() const { return bHasMoveTarget; }

    /** Get the current movement target (only valid if HasMoveTarget). */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MX|Robot|Movement")
    FVector GetMoveTarget() const { return MoveTargetLocation; }

    // -------------------------------------------------------------------------
    // Delegates
    // -------------------------------------------------------------------------

    UPROPERTY(BlueprintAssignable, Category = "MX|Robot")
    FOnRobotClicked OnRobotClicked;

    // -------------------------------------------------------------------------
    // Config
    // -------------------------------------------------------------------------

    /** Scale of the robot mesh. Default 0.20 = ~36cm tall. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Robot|Config")
    float RobotScale = 0.20f;

    /** Max movement speed in cm/s. Drives CMC MaxWalkSpeed. BS_Idle_Walk_Run thresholds: ~165=walk, ~375=run. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Robot|Config")
    float MoveSpeed = 400.0f;

    /** Distance from target at which the robot stops (cm). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Robot|Config")
    float StopDistance = 20.0f;

    /** Distance below which robots walk instead of run (cm). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Robot|Config")
    float WalkDistance = 150.0f;

    /** Rotation interpolation speed (degrees/sec effective). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Robot|Config")
    float RotationInterpSpeed = 8.0f;

    /** Selection ring color. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Robot|Config")
    FLinearColor SelectionColor = FLinearColor(0.0f, 0.85f, 0.25f, 1.0f);

    /** Name text color when selected. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Robot|Config")
    FColor SelectedNameColor = FColor(0, 220, 60, 255);

    /** Name text color when hovered (not selected). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Robot|Config")
    FColor HoveredNameColor = FColor(100, 200, 100, 200);

    /** Optional soft reference for skeletal mesh (C++ mesh assignment). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MX|Robot|Config")
    TSoftObjectPtr<USkeletalMesh> SkeletalMeshAsset;

    /** Animation bridge — reads CMC state and exposes to AnimBP. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MX|Robot|Animation")
    TObjectPtr<UMXAnimBridge> AnimBridge;


protected:

    // -------------------------------------------------------------------------
    // Components
    // -------------------------------------------------------------------------

    /** Name display — floats above robot. Hidden by default, shown on hover/select. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MX|Robot|Components")
    TObjectPtr<UTextRenderComponent> NameTextComponent;

    /** Selection ring indicator — visible only when selected. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MX|Robot|Components")
    TObjectPtr<UStaticMeshComponent> SelectionRingComponent;

private:

    // -------------------------------------------------------------------------
    // State
    // -------------------------------------------------------------------------

    bool bSelected = false;
    bool bHovered = false;
    bool bHasMoveTarget = false;
    FVector MoveTargetLocation = FVector::ZeroVector;

    // -------------------------------------------------------------------------
    // Internal
    // -------------------------------------------------------------------------

    /** Update name text visibility based on selection/hover state. */
    void UpdateNameVisibility();

    /** Update selection ring visibility. */
    void UpdateSelectionRing();

    /** Tick movement toward target. */
    void TickMovement(float DeltaTime);

    /** Billboard the name text toward the camera. */
    void TickNameBillboard();
};
