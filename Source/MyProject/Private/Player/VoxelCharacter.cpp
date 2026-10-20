#include "Player/VoxelCharacter.h"
#include "Voxel/VoxelWorld.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "Kismet/GameplayStatics.h"

AVoxelCharacter::AVoxelCharacter()
{
	bUseControllerRotationYaw = true;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->bOrientRotationToMovement = false;

		// Voxel terrain's collision is the chunk's complex (triangle-mesh) collision rather than
		// a simple shape. The rounded capsule-bottom floor sweep CharacterMovement uses by
		// default can fail to register "grounded" reliably against flat, grid-aligned complex
		// collision, leaving the character stuck in Falling (heavy air-control, broken jump).
		// A flat-based floor check is the standard fix for blocky/voxel ground.
		Movement->bUseFlatBaseForFloorChecks = true;
	}

	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
	FirstPersonCamera->bUsePawnControlRotation = true;
}

void AVoxelCharacter::BeginPlay()
{
	Super::BeginPlay();

	FirstPersonCamera->SetRelativeLocation(FVector(0.f, 0.f, BaseEyeHeight));

	CachedVoxelWorld = Cast<AVoxelWorld>(UGameplayStatics::GetActorOfClass(GetWorld(), AVoxelWorld::StaticClass()));
}

void AVoxelCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// Must happen here, not in BeginPlay: for a GameMode-spawned default pawn, BeginPlay runs
	// before Possess(), so GetController() is still null at that point and the mapping context
	// would silently never be added - no key would ever do anything despite correct bindings.
	if (APlayerController* PC = Cast<APlayerController>(NewController))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
			}
		}
	}
}

void AVoxelCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (MoveAction)
		{
			EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AVoxelCharacter::HandleMove);
		}
		if (LookAction)
		{
			EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &AVoxelCharacter::HandleLook);
		}
		if (JumpAction)
		{
			EnhancedInput->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
			EnhancedInput->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
		}
		if (BreakAction)
		{
			EnhancedInput->BindAction(BreakAction, ETriggerEvent::Started, this, &AVoxelCharacter::HandleBreak);
		}
		if (PlaceAction)
		{
			EnhancedInput->BindAction(PlaceAction, ETriggerEvent::Started, this, &AVoxelCharacter::HandlePlace);
		}
		if (CycleBlockTypeAction)
		{
			EnhancedInput->BindAction(CycleBlockTypeAction, ETriggerEvent::Started, this, &AVoxelCharacter::HandleCycleBlockType);
		}
	}
}

void AVoxelCharacter::HandleMove(const FInputActionValue& Value)
{
	// X = forward/back, Y = right/left - matches the IMC_VoxelDefault key mapping, where W/S
	// land on the raw X component (no swizzle needed) and A/D are swizzled onto Y.
	const FVector2D MoveVector = Value.Get<FVector2D>();

	if (Controller)
	{
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
		const FVector ForwardDir = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDir = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(ForwardDir, MoveVector.X);
		AddMovementInput(RightDir, MoveVector.Y);
	}
}

void AVoxelCharacter::HandleLook(const FInputActionValue& Value)
{
	const FVector2D LookVector = Value.Get<FVector2D>();
	AddControllerYawInput(LookVector.X);
	AddControllerPitchInput(-LookVector.Y); // macOS trackpad reports Y with the opposite sign of what "mouse up = look up" expects
}

bool AVoxelCharacter::TraceForTargetVoxel(FVector& OutHitLocation, FVector& OutHitNormal) const
{
	if (!FirstPersonCamera)
	{
		return false;
	}

	const FVector Start = FirstPersonCamera->GetComponentLocation();
	const FVector End = Start + FirstPersonCamera->GetForwardVector() * InteractReachCm;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	FHitResult Hit;
	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
	{
		OutHitLocation = Hit.ImpactPoint;
		OutHitNormal = Hit.ImpactNormal;
		return true;
	}

	return false;
}

AVoxelWorld* AVoxelCharacter::FindVoxelWorld() const
{
	return CachedVoxelWorld.Get();
}

void AVoxelCharacter::HandleBreak(const FInputActionValue&)
{
	FVector HitLocation, HitNormal;
	if (!TraceForTargetVoxel(HitLocation, HitNormal))
	{
		return;
	}

	if (AVoxelWorld* World = FindVoxelWorld())
	{
		World->TryBreakBlock(HitLocation, HitNormal);
	}
}

void AVoxelCharacter::HandlePlace(const FInputActionValue&)
{
	FVector HitLocation, HitNormal;
	if (!TraceForTargetVoxel(HitLocation, HitNormal))
	{
		return;
	}

	if (AVoxelWorld* World = FindVoxelWorld())
	{
		World->TryPlaceBlock(HitLocation, HitNormal, SelectedBlockType, this);
	}
}

void AVoxelCharacter::HandleCycleBlockType(const FInputActionValue&)
{
	const TArray<EVoxelBlockType> Placeable = VoxelBlockRules::GetPlaceableBlockTypes();
	if (Placeable.Num() == 0)
	{
		return;
	}

	const int32 CurrentIndex = Placeable.IndexOfByKey(SelectedBlockType);
	const int32 NextIndex = (CurrentIndex == INDEX_NONE) ? 0 : (CurrentIndex + 1) % Placeable.Num();
	SelectedBlockType = Placeable[NextIndex];
}
