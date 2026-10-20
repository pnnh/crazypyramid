#include "Player/VoxelCharacter.h"
#include "Voxel/VoxelWorld.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"

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

		// Default JumpZVelocity (420) only clears ~90uu - just under a single 100uu voxel - so
		// a 1-block-deep pit is sometimes impossible to escape. Raised to comfortably clear it.
		Movement->JumpZVelocity = 550.f;
	}

	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
	FirstPersonCamera->bUsePawnControlRotation = true;

	// The mesh's local +X axis runs wrist-to-hand (see research.md), so identity rotation already
	// points each arm forward into the screen; only a slight inward/upward tilt is added so the
	// hands rise toward the center of the view, FPS-style.
	RightArmMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightArmMeshComponent"));
	RightArmMeshComponent->SetupAttachment(FirstPersonCamera);
	RightArmMeshComponent->SetOnlyOwnerSee(true); // never visible from a third-person/spectator view
	RightArmMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision); // belt-and-braces: the
	// aim trace already ignores this actor's components entirely (see research.md in
	// 002-first-person-visuals), this just means the arm can't physically collide with anything
	RightArmMeshComponent->SetCastShadow(false);
	RightArmMeshComponent->SetRelativeLocation(FVector(20.f, 24.f, -20.f)); // forward, right, down - lower-right of view
	RightArmMeshComponent->SetRelativeRotation(FRotator(10.f, -6.f, 0.f)); // tilt hand up, barely inward

	LeftArmMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftArmMeshComponent"));
	LeftArmMeshComponent->SetupAttachment(FirstPersonCamera);
	LeftArmMeshComponent->SetOnlyOwnerSee(true);
	LeftArmMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	LeftArmMeshComponent->SetCastShadow(false);
	LeftArmMeshComponent->SetRelativeLocation(FVector(20.f, -24.f, -20.f)); // mirrored - lower-left of view
	LeftArmMeshComponent->SetRelativeRotation(FRotator(10.f, 6.f, 0.f));
	LeftArmMeshComponent->SetRelativeScale3D(FVector(1.f, -1.f, 1.f)); // mirror the same mesh into a left arm

	static ConstructorHelpers::FObjectFinder<UStaticMesh> ArmMeshFinder(TEXT("/Game/VoxelSandbox/Meshes/SM_FirstPersonArm.SM_FirstPersonArm"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ArmMaterialFinder(TEXT("/Game/VoxelSandbox/Materials/M_FirstPersonArm.M_FirstPersonArm"));
	for (UStaticMeshComponent* Arm : { RightArmMeshComponent, LeftArmMeshComponent })
	{
		if (ArmMeshFinder.Succeeded())
		{
			Arm->SetStaticMesh(ArmMeshFinder.Object);
		}
		if (ArmMaterialFinder.Succeeded())
		{
			Arm->SetMaterial(0, ArmMaterialFinder.Object);
		}
	}
}

void AVoxelCharacter::BeginPlay()
{
	Super::BeginPlay();

	FirstPersonCamera->SetRelativeLocation(FVector(0.f, 0.f, BaseEyeHeight));

	if (RightArmMeshComponent)
	{
		RightArmRestLocation = RightArmMeshComponent->GetRelativeLocation();
		RightArmRestRotation = RightArmMeshComponent->GetRelativeRotation();
	}
	if (LeftArmMeshComponent)
	{
		LeftArmRestLocation = LeftArmMeshComponent->GetRelativeLocation();
		LeftArmRestRotation = LeftArmMeshComponent->GetRelativeRotation();
	}

	CachedVoxelWorld = Cast<AVoxelWorld>(UGameplayStatics::GetActorOfClass(GetWorld(), AVoxelWorld::StaticClass()));
}

void AVoxelCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Idle "breathing" sway - the only motion the spec asks for (no skeleton/animation).
	const float Time = GetWorld()->GetTimeSeconds();
	const float SwayZ = FMath::Sin(Time * 1.5f) * 0.6f; // cm
	const float SwayPitch = FMath::Sin(Time * 1.5f) * 0.6f; // degrees
	const FVector SwayOffset(0.f, 0.f, SwayZ);
	const FRotator SwayRotation(SwayPitch, 0.f, 0.f);

	if (RightArmMeshComponent)
	{
		RightArmMeshComponent->SetRelativeLocation(RightArmRestLocation + SwayOffset);
		RightArmMeshComponent->SetRelativeRotation(RightArmRestRotation + SwayRotation);
	}
	if (LeftArmMeshComponent)
	{
		LeftArmMeshComponent->SetRelativeLocation(LeftArmRestLocation + SwayOffset);
		LeftArmMeshComponent->SetRelativeRotation(LeftArmRestRotation + SwayRotation);
	}
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
			// BreakAction carries an InputTriggerHold (see IA_Break asset) so this only fires
			// once the player has held the button down for a short duration, Minecraft-style,
			// instead of breaking instantly on press.
			EnhancedInput->BindAction(BreakAction, ETriggerEvent::Triggered, this, &AVoxelCharacter::HandleBreak);
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
