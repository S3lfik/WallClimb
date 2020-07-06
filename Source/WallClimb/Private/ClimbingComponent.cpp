// Fill out your copyright notice in the Description page of Project Settings.


#include "ClimbingComponent.h"
#include "Engine/World.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/SkinnedMeshComponent.h"

// Sets default values for this component's properties
UClimbingComponent::UClimbingComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	ObjectsToTrace = FCollisionObjectQueryParams::AllStaticObjects;
	
}

// Called when the game starts
void UClimbingComponent::BeginPlay()
{
	Super::BeginPlay();

	auto Owner = GetOwner();
	if (!(Owner))
	{
		UE_LOG(LogTemp, Error, TEXT("[%s] Use of unintialized pointers."), *FString(__FUNCTION__));
		return;
	}
	auto Comps = Owner->GetComponentsByTag(UArrowComponent::StaticClass(), "TraceArrow");
	if (Comps.Num() > 0)
	{
		ChestBoneSocket = Cast<UArrowComponent>(Comps[0]);
	}

	MovementComp = Cast<UCharacterMovementComponent>(Owner->GetComponentByClass(UCharacterMovementComponent::StaticClass()));
	
}

void UClimbingComponent::OnMoveRight_Implementation()
{
	// For later
}

void UClimbingComponent::OnHangRelease_Implementation()
{
	// For later
}

void UClimbingComponent::OnJumpPressed_Implementation()
{
	// For later
}

void UClimbingComponent::OnJumpReleased_Implementation()
{
	// For later
}


// Called every frame
void UClimbingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	/* TODO: Tick freeze for UpdateClimbingData to be added, for when climbing is not needed */
	UpdateClimbingData();
}

bool UClimbingComponent::IsClimbable(const FHitResult & HitResult)
{
	auto Owner = GetOwner();

	if (!(Owner && MovementComp))
	{
		UE_LOG(LogTemp, Error, TEXT("[%s] Use of unintialized pointers."), *FString(__FUNCTION__));
		return false;
	}

	/* Walkable surface check*/
	if (MovementComp->IsWalkable(HitResult))
	{
		return false;
	}

	/* Negative slope check*/
	if ((HitResult.ImpactNormal.Z > -0.05f))
	{
		return false;
	}

	auto ActorFwdVec = Owner->GetActorForwardVector();
	float Dot = FVector2D::DotProduct(FVector2D(ActorFwdVec.X, ActorFwdVec.Y), 
		FVector2D(HitResult.ImpactNormal.X, HitResult.ImpactNormal.Y));
	
	/* Climable tilt check */
	if ((FMath::Acos(Dot) > (180.f - MaxSurfaceCaptureAngle)))
	{
		return false;
	}

	return true;
}

void UClimbingComponent::OnCharacterLanded_Implementation()
{
	// For later
}

bool UClimbingComponent::CanStartClimbing()
{
	// Check if we are allowed to climb, have something to climb and not climbing already
	return HasAbilityToClimb && IsClimbOnHitAllowed 
		&& ( ActorToClimbOn != nullptr) && !IsOnTheWall() ;
}

bool UClimbingComponent::IsOnTheWall()
{
	return IsClimbing || IsHanging;
}

bool UClimbingComponent::TickTrace(FHitResult & outHitResult)
{
	auto Owner = GetOwner();

	if (!(Owner && CapsuleComp))
	{
		UE_LOG(LogTemp, Error, TEXT("[%s] Use of unintialized pointers."), *FString(__FUNCTION__));
		return false;
	}

	float CapsuleHalfHeight, CapsuleRadius;
	auto ActorRot = Owner->GetActorRotation().Quaternion();
	auto ActorLoc = Owner->GetActorLocation();
	auto ActorForwardVector = Owner->GetActorForwardVector();

	CapsuleComp->GetScaledCapsuleSize(CapsuleRadius, CapsuleHalfHeight);

	FCollisionShape CollisionBox = FCollisionShape::MakeBox(FVector(CapsuleRadius, CapsuleRadius, CapsuleHalfHeight * 0.75f));

	FVector TraceCenter = ActorLoc + (ActorForwardVector * (CapsuleRadius * 1.5f));
	
	/* TODO: Should be branched with Line trace if IsHanging later. */
	return GetWorld()->SweepSingleByObjectType(outHitResult, TraceCenter, TraceCenter, ActorRot, ObjectsToTrace, CollisionBox);
}

void UClimbingComponent::UpdateClimbingData()
{
	FHitResult HitResult;

	if (! TickTrace(HitResult))
	{
		/* TODO: Get rid of this ugly work around */
		if (!IsHanging)
		{
			ActorToClimbOn = nullptr;
			CurrentSurfaceNormal = FVector::ZeroVector;
		}
	}

	ActorToClimbOn = HitResult.GetActor();
	CurrentSurfaceNormal = HitResult.ImpactNormal;
}
