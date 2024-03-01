// Fill out your copyright notice in the Description page of Project Settings.


#include "TPSPlayer.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Components/ArrowComponent.h"

#include "Components/InputComponent.h"
#include "UObject/UnrealType.h"
#include "Engine/World.h"

// Sets default values
ATPSPlayer::ATPSPlayer()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Find Default Mapping Context
	static ConstructorHelpers::FObjectFinder<UInputMappingContext> DEFAULT_CONTEXT
		(TEXT("/Game/TPSPractice/Input/IMC_Default.IMC_Default"));
	if (DEFAULT_CONTEXT.Succeeded())
		DefaultContext = DEFAULT_CONTEXT.Object;

	// Find IA_Move
	static ConstructorHelpers::FObjectFinder<UInputAction> IA_MOVE
		(TEXT("/Game/TPSPractice/Input/Actions/IA_Move.IA_Move"));
	if (IA_MOVE.Succeeded())
		MoveAction = IA_MOVE.Object;

	// Find IA_Look
	static ConstructorHelpers::FObjectFinder<UInputAction> IA_LOOK
		(TEXT("/Game/TPSPractice/Input/Actions/IA_Look.IA_Look"));
	if (IA_LOOK.Succeeded())
		LookAction = IA_LOOK.Object;

	// Find IA_Jump
	static ConstructorHelpers::FObjectFinder<UInputAction> IA_JUMP
		(TEXT("/Game/TPSPractice/Input/Actions/IA_Jump.IA_Jump"));
	if (IA_JUMP.Succeeded())
		JumpAction = IA_JUMP.Object;

	// Find IA_Shoot
	static ConstructorHelpers::FObjectFinder<UInputAction> IA_SHOOT
		(TEXT("/Game/TPSPractice/Input/Actions/IA_Shoot.IA_Shoot"));
	if (IA_SHOOT.Succeeded())
		ShootAction = IA_SHOOT.Object;

	// Find BP_Bullet
	static ConstructorHelpers::FClassFinder<AActor> BP_BULLET
		(TEXT("/Game/TPSPractice/Blueprints/Bullet/BP_Bullet.BP_Bullet_C"));
	if (BP_BULLET.Succeeded())
		BulletClass = BP_BULLET.Class;
	else
		UE_LOG(LogTemp, Error, TEXT("BP_Bullet is not found"));
}

// Called when the game starts or when spawned
void ATPSPlayer::BeginPlay()
{
	Super::BeginPlay();

	// Add Mapping Context
	if (auto PlayerController = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<
			UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultContext, 0);
		}
	}

	
	// Find ShootArrowComponent
	auto Arrows = TArray<UArrowComponent*>();
	GetComponents<UArrowComponent*>(Arrows, true);
	for (auto Arrow : Arrows)
	{
		if (Arrow->GetName() == "ShootArrow")
		{
			ShootArrowComponent = Arrow;
			break;
		}
	}
}

// Called every frame
void ATPSPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void ATPSPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ATPSPlayer::Move);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ATPSPlayer::Look);

		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &ATPSPlayer::JumpStart);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ATPSPlayer::JumpEnd);

		EnhancedInputComponent->BindAction(ShootAction, ETriggerEvent::Triggered, this, &ATPSPlayer::Shoot);
	}
}

void ATPSPlayer::Move(const FInputActionValue& Value)
{
	auto MoveDirection = GetController()->GetControlRotation();
	MoveDirection.Pitch = 0;

	auto RightVector = FRotationMatrix(MoveDirection).GetScaledAxis(EAxis::Y);
	auto ForwardVector = FRotationMatrix(MoveDirection).GetScaledAxis(EAxis::X);

	auto Movement = Value.Get<FVector2D>();

	this->AddMovementInput(RightVector, Movement.X);
	this->AddMovementInput(ForwardVector, Movement.Y);
}

void ATPSPlayer::Look(const FInputActionValue& Value)
{
	auto Movement = Value.Get<FVector2D>();

	this->AddControllerPitchInput(Movement.Y);
	this->AddControllerYawInput(Movement.X);
}

void ATPSPlayer::JumpStart(const FInputActionValue& Value)
{
	Jump();
}

void ATPSPlayer::JumpEnd(const FInputActionValue& Value)
{
	StopJumping();
}

void ATPSPlayer::Shoot(const FInputActionValue& Value)
{
	if (!ensure(ShootArrowComponent))
	{
		UE_LOG(LogTemp, Error, TEXT("ShootArrowComponent is not set"));
		return;
	}

	if (!ensure(BulletClass))
	{
		UE_LOG(LogTemp, Error, TEXT("BulletClass is not set"));
		return;
	}
	
	auto ShootOrigin = ShootArrowComponent->GetComponentLocation();

	auto Rotation = GetController()->GetControlRotation();
	auto ShootDirection = FRotationMatrix(Rotation).GetScaledAxis(EAxis::X);

	for (int i = 0; i < 10; i++)
	{
		auto RandomDirection = FMath::VRandCone(ShootDirection, FMath::DegreesToRadians(10));
		auto RandomRotation = RandomDirection.Rotation();

		auto Bullet = GetWorld()->SpawnActor(BulletClass, &ShootOrigin, &RandomRotation);
		auto Impulse = FMath::RandRange(4500.f, 6000.f);
		Bullet->GetComponentByClass<UStaticMeshComponent>()->AddImpulse(Impulse * RandomDirection);
	}
}
