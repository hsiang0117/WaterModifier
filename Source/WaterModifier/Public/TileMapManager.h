// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Camera/CameraComponent.h"
#include "TileMapManager.generated.h"

USTRUCT(BlueprintType)
struct FTileByIndex
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	int X;

	UPROPERTY(BlueprintReadWrite)
	int Y;
};

USTRUCT(BlueprintType)
struct FTileToRender
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FString FilePath;

	UPROPERTY(BlueprintReadWrite)
	float LoactionX;

	UPROPERTY(BlueprintReadWrite)
	float LoactionY;

	UPROPERTY(BlueprintReadWrite)
	FTileByIndex TileIndex;
};

UENUM(BlueprintType)
enum class EExcutePinNodes: uint8
{
	OnRangeChange,
	OnLodChange,
	NoChange
};

UCLASS()
class WATERMODIFIER_API ATileMapManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATileMapManager();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

private:
	UFUNCTION(BlueprintCallable,Category="TileMapManager",meta=(ExpandEnumAsExecs="Result"))
	static void Tick(EExcutePinNodes& Result,UCameraComponent* CameraComponent,int CurrentLod, int TileSize);
	
	UFUNCTION(BlueprintCallable,Category="TileMapManager")
	static FIntVector4 GetBottomLeftAndTopRight(UCameraComponent* CameraComponent, int TileSize);

	UFUNCTION(BlueprintCallable,Category="TileMapManager")
	static TArray<FIntPoint> GetUnionMinusFirst(FIntPoint Square1BottomLeft, FIntPoint Square1TopRight,
												 FIntPoint Square2BottomLeft, FIntPoint Square2TopRight);

	UFUNCTION(BlueprintCallable,Category="TileMapManager")
	static TArray<FIntPoint> GetIntersection(FIntPoint Square1BottomLeft, FIntPoint Square1TopRight,
												  FIntPoint Square2BottomLeft, FIntPoint Square2TopRight);
};
