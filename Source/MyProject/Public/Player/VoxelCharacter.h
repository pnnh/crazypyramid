// T016/T017/T020/T023/T024/T026/T027: First-person gravity/collision character - movement,
// look, and the break/place/cycle-block-type interactions.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Voxel/VoxelTypes.h"
#include "VoxelCharacter.generated.h"

class UCameraComponent;
class UInputMappingContext;
class UInputAction;
class UInputComponent;
class UStaticMeshComponent;
class AVoxelWorld;
struct FInputActionValue;

UCLASS()
class MYPROJECT_API AVoxelCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AVoxelCharacter();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Voxel Character")
	TObjectPtr<UCameraComponent> FirstPersonCamera;

	// Bare-handed first-person arms, attached under FirstPersonCamera - one on each side of the
	// view. Static meshes only - no skeleton/animation; idle sway is a per-frame sine offset
	// applied in Tick (see research.md in 002-first-person-visuals for why). The left arm reuses
	// the same mesh mirrored via negative Y scale rather than a second asset.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Voxel Character")
	TObjectPtr<UStaticMeshComponent> RightArmMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Voxel Character")
	TObjectPtr<UStaticMeshComponent> LeftArmMeshComponent;

	// Relative transforms each arm sways around, set once in BeginPlay so Tick can offset from
	// them every frame without drifting.
	FVector RightArmRestLocation = FVector::ZeroVector;
	FRotator RightArmRestRotation = FRotator::ZeroRotator;
	FVector LeftArmRestLocation = FVector::ZeroVector;
	FRotator LeftArmRestRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, Category = "Voxel Character|Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditAnywhere, Category = "Voxel Character|Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, Category = "Voxel Character|Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditAnywhere, Category = "Voxel Character|Input")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditAnywhere, Category = "Voxel Character|Input")
	TObjectPtr<UInputAction> BreakAction;

	UPROPERTY(EditAnywhere, Category = "Voxel Character|Input")
	TObjectPtr<UInputAction> PlaceAction;

	UPROPERTY(EditAnywhere, Category = "Voxel Character|Input")
	TObjectPtr<UInputAction> CycleBlockTypeAction;

	// Max distance (cm) the player can reach to break/place a block.
	UPROPERTY(EditAnywhere, Category = "Voxel Character")
	float InteractReachCm = 500.f;

private:
	EVoxelBlockType SelectedBlockType = EVoxelBlockType::Grass;
	TWeakObjectPtr<AVoxelWorld> CachedVoxelWorld;

	void HandleMove(const FInputActionValue& Value);
	void HandleLook(const FInputActionValue& Value);
	void HandleBreak(const FInputActionValue& Value);
	void HandlePlace(const FInputActionValue& Value);
	void HandleCycleBlockType(const FInputActionValue& Value);

	bool TraceForTargetVoxel(FVector& OutHitLocation, FVector& OutHitNormal) const;
	AVoxelWorld* FindVoxelWorld() const;
};
