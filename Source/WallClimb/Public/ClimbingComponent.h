// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/Public/CollisionQueryParams.h"
#include "ClimbingComponent.generated.h"

namespace
{
	/** Reasons to leave the clambing state */
	UENUM()
	enum class EReasonToStopClimbing : uint8
	{
		StartFalling,
		StartHanging
	};
}

/** For later use in Animation state machine */
UENUM()
enum class EClimbDirection : uint8
{
	UPWARDS					UMETA(DisplayName = "Upwards"),
	LEFT					UMETA(DisplayName = "Left"),
	LEFT_ARROUND_CORNER		UMETA(DisplayName = "Left around corner"),
	RIGHT					UMETA(DisplayName = "Right"),
	RIGHT_AROUND_CORNER		UMETA(DisplayName = "Right around corner"),
	UP_ON					UMETA(DisplayName = "Up on"),
	IDLE					UMETA(DisplayName = "Idle hanging"),
	NONE					UMETA(DisplayName = "None")
};

UCLASS( Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class WALLCLIMB_API UClimbingComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UClimbingComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	/** Has to be triggered when the player presses Jump */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Event")
	void OnJumpPressed();

	/** Has to be triggered when the player releases Jump */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Event")
	void OnJumpReleased();

	/** Basic method to override the movement while hanging on a wall */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Event")
	void OnMoveRight(const float& Scale);

	/** Has to be triggered when the player presses Realese */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Event")
	void OnHangRelease();

	/** Has to be triggered on Character's OnLanded */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Event")
	void OnCharacterLanded();

	/** A signal method for the component to process the time of the animation being played on the Character's side */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Event")
	void OnLocationTransition();

	/** A signal method for the component to process the time of the animation stopped being played on the Character's side
		and to reset any possible freezes, that were applied for the animation time */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Event")
	void OnLocationTransitionFinished();

	/** A set of checks before starting a climb */
	bool CanStartClimbing();

	bool TickTrace(FHitResult& outHitResult);

	/** Prepare data, that will be used on StartClimbing */
	void ScanForClimbingData();

	void StartClimbing();

	void StopClimbing(const EReasonToStopClimbing InReason);

	void UpdateClimbing();

	void StartHanging();

	void StopHanging();

	void UpdateHanging(float InDeltaTime);	

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Climbing|Values", meta = (DisplayName = "Is Climbing"))
	bool IsClimbing;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Climbing|Values", meta = (DisplayName = "Is Hanging"))
	bool IsHanging;

	/** For animation state transition purpose */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Climbing|Values", meta = (DisplayName = "Climbing Direction"))
	EClimbDirection ClimbingDirection;

	/** A marker to define the height of the character's chest 
		TODO: Replace with USkeletalMeshSocket */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Climbing|Setup", meta = (DisplayName = "Chest bone socket Ref"))
	class UArrowComponent* ChestBoneSocket;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Climbing|Setup", meta = (DisplayName = "Capsule Component Ref"))
	class UCapsuleComponent* CapsuleComp;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Climbing|Setup", meta = (DisplayName = "Movement Component Ref"))
	class UCharacterMovementComponent* MovementComp;

	/** Max height that can be climbed */
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Climbing|Setup", meta = (DisplayName = "Max Climbing Distance"))
	float MaxClimbingDistance = 200;

	/** Speed with which a character moves on the walls vertically */
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Climbing|Setup", meta = (DisplayName = "Max Climbing Speed"))
	float MaxClimbingSpeed = 200.f;

	/** Speed with which a character moves on the walls sideways */
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Climbing|Setup", meta = (DisplayName = "Max Climbing Strafe Speed"))
	float MaxClimbingStrafeSpeed = 200.f;

	/** An angle of capturing a surface */
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Climbing|Setup", meta = (DisplayName = "Max Capture Angle", ClampMin = "0", ClampMax = "45"))
	float MaxSurfaceCaptureAngle = 45.f;

	/** Should be allowed by the Component's user, to specify when it must be activated. E.g. on sprinting or jumping */
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Climbing|Setup", meta = (DisplayName = "Is Climb On Hit Allowed"))
	bool IsClimbOnHitAllowed = false;

private:
	bool CanStartHanging;

	/** Resets on touching the groung, or hanging */
	bool HasAbilityToClimb;

	/** An actor to climb on*/
	AActor* ActorToClimbOn;

	/** Or height to climb */
	FVector LocationToGrab;

	bool IsLocationPotentiallyReachable;

	/** An object we've already climbed
		- Uses as a filter, so we don't try to hang on the same object */
	AActor* LastClimbedObject;
	
	/** A surface we are currently operating with */
	FVector CurrentSurfaceNormal;

	/** Filter for traces for objects */
	FCollisionObjectQueryParams ObjectsToTrace;

	/** To track the climbed distance */
	FVector ClimbingStartLocation;

	/** How far we've already climbed */
	float ClimbedDistance;

	/** To store initial data, retrieved from a hit on object to climb */
	FHitResult TickTraceHitResult;

private:

	/** Surface check*/
	bool IsClimbable(const FHitResult& HitResult);

	/** Climbing states check */
	bool IsOnTheWall();

	/** Updates the value of Climbed distance, returns MaxClimbingDistance > ClimbedDistance */
	bool UpdateClimbedDistance();

	/** Updates LocationToGrab. Returns if there is a reachable location to grab */
	bool GetLocationToGrab();

	bool UpwardTrace(TArray<FHitResult>& OutHitResult) const;

	bool FindClosestVerticalHit(const TArray<FHitResult>& InHitResults, FHitResult& HitResult) const;

	bool BoxContainsVector(const FVector& Origin, const FVector& Extent, const FVector& InVector) const;

	/** Reset values that define any climbing state, reset a climbing ability */
	void ResetClimbingStates();

	void MoveSideways(const float& Scale);
	
	bool CanMoveSidewaysToLocation(const FVector & InTargetLocation, FHitResult& OutHitResult) const;
};
