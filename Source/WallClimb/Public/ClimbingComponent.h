// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ClimbingComponent.generated.h"

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

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Event")
	void OnMoveRight();

	/** Has to be triggered when the player presses Realese */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Event")
	void OnHangRelease();

	/** Has to be triggered on Character's OnLanded */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Event")
	void OnCharacterLanded();

	/** A set of checks before starting a climb */
	bool CanStartClimbing();

	/** Climbing states check */
	bool IsOnTheWall();

	bool TickTrace(FHitResult& outHitResult);

	void UpdateClimbingData();

	void StartClimbing();

	void StopClimbing();

	void UpdateClimbing(float DeltaTime);

	void StartHanging();

	void StopHanging();

	void UpdateHanging(float DeltaTime);

	

	void LocationTransition();

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Climbing|Values", meta = (DisplayName = "Is Climbing"))
	bool IsClimbing;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Climbing|Values", meta = (DisplayName = "Is Climbing"))
	bool IsHanging;

	/** For animation state transition purpose */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Climbing|Values", meta = (DisplayName = "Climbing Direction"))
	EClimbDirection ClimbingDirection;

	/** A marker to define the height of the character's chest 
		TODO: Replace with USkeletalMeshSocket */
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Climbing|Setup", meta = (DisplayName = "Chest bone socket Ref"))
	class UArrowComponent* ChestBoneSocket;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Climbing|Setup", meta = (DisplayName = "Capsule Component Ref"))
	class UCapsuleComponent* CapsuleComp;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Climbing|Setup", meta = (DisplayName = "Movement Component Ref"))
	class UCharacterMovementComponent* MovementComp;

	/** Max height that can be climbed */
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Climbing|Setup", meta = (DisplayName = "Max Climbing Distance"))
	float MaxClimbingDistance = 200;

	/** An angle of capturing a surface */
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Climbing|Setup", meta = (DisplayName = "Max Capture Angle", ClampMin = "0", ClampMax = "45"))
	float MaxSurfaceCaptureAngle = 45.f;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Climbing|Setup", meta = (DisplayName = "Climb on hit allowed"))
	bool IsClimbOnHitAllowed;

private:
	bool CanStartHanging;

	/** Resets on touching the groung, or hanging */
	bool HasAbilityToClimb;

	/** An actor to climb on*/
	AActor* ActorToClimbOn;
	
	/** A surface we are currently operating with */
	FVector CurrentSurfaceNormal;

	/** Filter for traces for objects */
	FCollisionObjectQueryParams ObjectsToTrace;

private:

	bool IsClimbable(const FHitResult& HitResult);
};
