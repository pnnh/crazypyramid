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
	virtual void PossessedBy(AController* NewController) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Voxel Character")
	TObjectPtr<UCameraComponent> FirstPersonCamera;

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
