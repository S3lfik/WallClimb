// Fill out your copyright notice in the Description page of Project Settings.


#include "ClimbingComponent.h"
#include "Engine/World.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/SkinnedMeshComponent.h"
#include "GameFramework/Character.h"
#include "Engine/Public/DrawDebugHelpers.h"

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
	CapsuleComp = Cast<UCapsuleComponent>(Owner->GetComponentByClass(UCapsuleComponent::StaticClass()));

	HasAbilityToClimb = true;
	IsClimbing = false;
	IsHanging = false;
}

void UClimbingComponent::OnMoveRight_Implementation(const float& Scale)
{
	MoveSideways(Scale);
}

void UClimbingComponent::OnHangRelease_Implementation()
{
	if (IsHanging)
	{
		StopHanging();
	}
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

	/* TODO: A Freeze should be considered for the ScanForClimbingData */
	ScanForClimbingData();
	if (!IsClimbing)
	{
		StartClimbing();
	}
	else if (IsClimbing)
	{
		UpdateClimbing();
	}
	
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
	//if ((HitResult.ImpactNormal.Z > -0.05f))
	//{
	//	return false;
	//}

	auto ActorFwdVec = Owner->GetActorForwardVector();
	float Dot = FVector2D::DotProduct(FVector2D(ActorFwdVec.X, ActorFwdVec.Y), 
		FVector2D(HitResult.ImpactNormal.X, HitResult.ImpactNormal.Y));
	
	/* Check the capture angle */
	float temp = FMath::Cos(FMath::DegreesToRadians(180.f - MaxSurfaceCaptureAngle));

	if (Dot > temp)
	{
		return false;		
	}

	return true;
}

void UClimbingComponent::OnCharacterLanded_Implementation()
{
	ResetClimbingStates();
}

void UClimbingComponent::OnLocationTransition_Implementation()
{
	// For later
}

void UClimbingComponent::OnLocationTransitionFinished_Implementation()
{
	// For later
}

bool UClimbingComponent::CanStartClimbing()
{
	// Check if we are allowed to climb, have something to climb and not climbing already
	return HasAbilityToClimb && IsClimbOnHitAllowed 
		&& (TickTraceHitResult.Actor != nullptr) && !IsOnTheWall() ;
}

bool UClimbingComponent::IsOnTheWall()
{
	return IsClimbing || IsHanging;
}

bool UClimbingComponent::UpdateClimbedDistance()
{
	auto Owner = GetOwner();

	if (!(Owner))
	{
		UE_LOG(LogTemp, Error, TEXT("[%s] Use of unintialized pointers."), *FString(__FUNCTION__));
		return false;
	}

	ClimbedDistance = (ClimbingStartLocation - Owner->GetActorLocation()).Size();

	return MaxClimbingDistance > ClimbedDistance;
}

bool UClimbingComponent::GetLocationToGrab()
{
	TArray<FHitResult> VerticalHitResults;
	FHitResult ClosestGrabableHit;
	bool HitRetrieved = false;
	

	if (UpwardTrace(VerticalHitResults))
	{
		HitRetrieved = FindClosestVerticalHit(VerticalHitResults, ClosestGrabableHit);
	}

	if (HitRetrieved)
	{
		LocationToGrab = ClosestGrabableHit.ImpactPoint;
		return true;
	}

	return false;
}

bool UClimbingComponent::FindClosestVerticalHit(const TArray<FHitResult>& InHitResults, 
	FHitResult & HitResult) const
{
	if (!(ChestBoneSocket))
	{
		UE_LOG(LogTemp, Error, TEXT("[%s] Use of unintialized pointers."), *FString(__FUNCTION__));
		return false;
	}

	if (InHitResults.Num() == 0)
	{
		return false;
	}

	for (int32 i = InHitResults.Num() - 1; i >= 0; --i)
	{
		FVector ActorBoundsOrigin, ActorBoundsExtent;
		InHitResults[i].GetActor()->GetActorBounds(false, ActorBoundsOrigin, ActorBoundsExtent);
		const float ComparisonTollerance = 1.f;

		// check that ImpactPoint is on the top of the Actor's box, not inside
		if ((ActorBoundsOrigin + ActorBoundsExtent).Z - InHitResults[i].ImpactPoint.Z > ComparisonTollerance)
		{
			continue;
		}

		if (InHitResults[i].ImpactPoint.Z >= ChestBoneSocket->GetComponentLocation().Z)
		{
			HitResult = InHitResults[i];
			return true;
		}
	}

	return false;
}

bool UClimbingComponent::UpwardTrace(TArray<FHitResult>& OutHitResults) const
{
	auto Owner = GetOwner();
	if (!(Owner && ChestBoneSocket && CapsuleComp))
	{
		UE_LOG(LogTemp, Error, TEXT("[%s] Use of unintialized pointers."), *FString(__FUNCTION__));
		return false;
	}

	// Trace top-down
	
	// TODO: think it over again later, what location to take as a base.
	FVector TraceEnd = TickTraceHitResult.ImpactPoint + (TickTraceHitResult.ImpactNormal * -1.f * CapsuleComp->GetScaledCapsuleRadius());

	FVector TraceBegin = TraceEnd +
		FVector(0.f, 0.f, 1.f) * (MaxClimbingDistance - ClimbedDistance);

	DrawDebugLine(GetWorld(), TraceBegin, TraceEnd, FColor::Blue, false, 5.f, 0, 2.f);
	FCollisionQueryParams Params(FName("UpwardTrace"), false, Owner);
	return GetWorld()->LineTraceMultiByChannel(OutHitResults, TraceBegin, TraceEnd, ECC_WorldStatic, Params);
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

	// TODO: This sensor should be rethinked
	FCollisionShape CapsuleCollision = FCollisionShape::MakeCapsule(CapsuleRadius * 1.1, CapsuleHalfHeight * 0.75f);	
	FCollisionQueryParams Params(FName("TickTrace"), false, Owner);

	//FVector TraceCenter = ActorLoc + (ActorForwardVector * (CapsuleRadius * 1.5f));

	return GetWorld()->SweepSingleByChannel(outHitResult, ActorLoc, ActorLoc, ActorRot, ECC_WorldStatic, CapsuleCollision, Params);
}

void UClimbingComponent::ScanForClimbingData()
{
	// If climbing is not allowed by the user and we are not on the wall, no need to update any data for climbing
	if (!IsClimbOnHitAllowed)
	{
		return;
	}
	if (IsOnTheWall())
	{
		return;
	}

	FHitResult HitResult;

	if (! TickTrace(HitResult))
	{
		/* TODO: Get rid of this ugly work around */
		if (!IsHanging)
		{
			ActorToClimbOn = nullptr;
			CurrentSurfaceNormal = FVector::ZeroVector;
		}
		return;
	}

 	if (IsClimbable(HitResult))
	{
		ActorToClimbOn = HitResult.GetActor();
		CurrentSurfaceNormal = HitResult.ImpactNormal;
		TickTraceHitResult = std::move(HitResult);
	}
	else
	{
		TickTraceHitResult = FHitResult();
	}
}

void UClimbingComponent::StartClimbing()
{
	auto Owner = GetOwner();

	if (!(Owner && MovementComp))
	{
		UE_LOG(LogTemp, Error, TEXT("[%s] Use of unintialized pointers."), *FString(__FUNCTION__));
		return;
	}

	if (!CanStartClimbing())
	{
		return;
	}
	
	IsClimbing = true;
	HasAbilityToClimb = false;

	// by default we assume anything is potentially grabable
	IsLocationPotentiallyReachable = true;
	
	LocationToGrab = FVector::ZeroVector;
	MovementComp->SetMovementMode(EMovementMode::MOVE_Flying);
	MovementComp->GravityScale = 0.f;

	FVector LaunchVelocity = (CurrentSurfaceNormal * -1.f) + FVector(0.f, 0.f, 1.f);
	LaunchVelocity.Normalize();
	LaunchVelocity *= MaxClimbingSpeed;

	ClimbingStartLocation = Owner->GetActorLocation();

	ACharacter* Character = Cast<ACharacter>(Owner);
	if (Character)
	{
		Character->LaunchCharacter(LaunchVelocity, true, true);
	}
}

void UClimbingComponent::StopClimbing(const EReasonToStopClimbing InReason)
{
	auto Owner = GetOwner();
	if (!(Owner && MovementComp))
	{
		UE_LOG(LogTemp, Error, TEXT("[%s] Use of unintialized pointers."), *FString(__FUNCTION__));
		return;
	}
	// Wipe all the climbing related data.

	IsClimbing = false;
	LocationToGrab = FVector::ZeroVector;
	IsLocationPotentiallyReachable = true;
	ClimbedDistance = 0.f;

	switch (InReason)
	{
	case EReasonToStopClimbing::StartFalling:
		MovementComp->GravityScale = 1.f;
   		MovementComp->SetMovementMode(EMovementMode::MOVE_Walking);
		break;
	case EReasonToStopClimbing::StartHanging:
		StartHanging();
		break;
	}
}

void UClimbingComponent::UpdateClimbing()
{
	auto Owner = GetOwner();

	if (!(Owner && ChestBoneSocket))
	{
		UE_LOG(LogTemp, Error, TEXT("[%s] Use of unintialized pointers."), *FString(__FUNCTION__));
		return;
	}

	// check if grab location not Zero, but potentially is reachable
	if (LocationToGrab == FVector::ZeroVector &&
		IsLocationPotentiallyReachable)
	{
		IsLocationPotentiallyReachable = GetLocationToGrab();
	}

	if (LocationToGrab != FVector::ZeroVector)
	{
		if (ChestBoneSocket->GetComponentLocation().Z >= LocationToGrab.Z)
		{
			// Compensate Tick location update step
			FVector DeltaLocation = FVector(0.f, 0.f, ChestBoneSocket->GetComponentLocation().Z - LocationToGrab.Z);
			Owner->AddActorWorldOffset(DeltaLocation, false, nullptr, ETeleportType::TeleportPhysics);

			// Stop climbing
			StopClimbing(EReasonToStopClimbing::StartHanging);
		}
	}
	
	if (!UpdateClimbedDistance())
	{
		StopClimbing(EReasonToStopClimbing::StartFalling);
	}	
}

void UClimbingComponent::StartHanging()
{
	HasAbilityToClimb = true;
	IsHanging = true;
	MovementComp->StopMovementImmediately();
}

void UClimbingComponent::StopHanging()
{
	if (!(MovementComp))
	{
		UE_LOG(LogTemp, Error, TEXT("[%s] Use of unintialized pointers."), *FString(__FUNCTION__));
		return;
	}

	IsHanging = false;
	MovementComp->SetMovementMode(EMovementMode::MOVE_Walking);
	MovementComp->GravityScale = 1.f;
}

void UClimbingComponent::UpdateHanging(float InDeltaTime)
{
	// most probably the best place to override the tick trace with 
	// a bit cheaper trace to update current values. E.g. CurrentSurfaceNormal
}

bool UClimbingComponent::BoxContainsVector(const FVector& Origin, const FVector& Extent, const FVector& InVector) const
{
	FVector NegativeExtent = Extent * -1.f;
	FVector LocalSpaceVector = Origin - InVector;

	bool InsideX = ((LocalSpaceVector.X > NegativeExtent.X) && (LocalSpaceVector.X < Extent.X)) ? true : false;
	bool InsideY = ((LocalSpaceVector.Y > NegativeExtent.Y) && (LocalSpaceVector.Y < Extent.Y)) ? true : false;
	bool InsideZ = ((LocalSpaceVector.Z > NegativeExtent.Z) && (LocalSpaceVector.Z < Extent.Z)) ? true : false;

	return InsideX && InsideY && InsideZ;
}

void UClimbingComponent::ResetClimbingStates()
{
	IsClimbing = false;
	IsHanging = false;
	HasAbilityToClimb = true;

	// Just in case of immergency use, try reset movement component to walking
	if (!(MovementComp))
	{
		UE_LOG(LogTemp, Error, TEXT("[%s] Use of unintialized pointers."), *FString(__FUNCTION__));
		return;
	}

	if (MovementComp->MovementMode != EMovementMode::MOVE_Walking)
	{
		MovementComp->SetMovementMode(EMovementMode::MOVE_Walking);
		MovementComp->GravityScale = 1.f;
	}	
}

void UClimbingComponent::MoveSideways(const float& Scale)
{
	if (!IsHanging)
	{
		return;
	}

	if (FMath::Abs(Scale) < 0.1f)
	{
		return;
	}

	auto Owner = GetOwner();
	
	if (!(Owner))
	{
		UE_LOG(LogTemp, Error, TEXT("[%s] Use of unintialized pointers."), *FString(__FUNCTION__));
		return;
	}

	FVector SurfaceRightVector = CurrentSurfaceNormal;
	SurfaceRightVector = FVector::CrossProduct(SurfaceRightVector, FVector::UpVector);
	SurfaceRightVector.Normalize();

	FVector MoveVelocity = SurfaceRightVector * Scale * GetWorld()->DeltaTimeSeconds * MaxClimbingStrafeSpeed;

	FVector NextLocation = FMath::VInterpTo(Owner->GetActorLocation(), Owner->GetActorLocation() + MoveVelocity, GetWorld()->DeltaTimeSeconds, 0.f);
	FHitResult NewLocationHitResult;
	if (CanMoveSidewaysToLocation(NextLocation, NewLocationHitResult))
	{
		Owner->SetActorLocation(NextLocation);
		// In case of curved surfaces
		CurrentSurfaceNormal = NewLocationHitResult.ImpactNormal;
	}
	else
	{
		// climb around the corner
	}
}

bool UClimbingComponent::CanMoveSidewaysToLocation(const FVector & InTargetLocation, FHitResult& OutHitResult) const
{
	auto Owner = GetOwner();
	if (!(CapsuleComp && Owner))
	{
		UE_LOG(LogTemp, Error, TEXT("[%s] Use of unintialized pointers."), *FString(__FUNCTION__));
		return false;
	}

	FHitResult HitResult;
	FVector Offset = (CurrentSurfaceNormal * -1.f) *
		(CapsuleComp->GetScaledCapsuleRadius() * 1.1f);
	FCollisionQueryParams Params(FName("MoveSidewaysTrace"), false, Owner);
	
	DrawDebugLine(GetWorld(), InTargetLocation, InTargetLocation + Offset, FColor::Purple, false, 0.1f, 0, 2.f);
	return GetWorld()->LineTraceSingleByChannel(HitResult, InTargetLocation, InTargetLocation + Offset, ECC_WorldStatic, Params);
}
