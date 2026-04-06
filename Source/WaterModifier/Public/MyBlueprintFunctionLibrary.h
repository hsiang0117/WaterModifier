// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraComponent.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MyBlueprintFunctionLibrary.generated.h"

/**
 * 
 */
USTRUCT(BlueprintType)
struct FIntVector2d
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	int X;

	UPROPERTY(BlueprintReadWrite)
	int Y;
};

UCLASS()
class WATERMODIFIER_API UMyBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
	UFUNCTION(BlueprintCallable, Category="MyBlueprintFunctionLibrary")
	static UTexture2D* MyImportFileAsTexture2D(const FString& ImagePath);

	UFUNCTION(BlueprintCallable, Category="MyBlueprintFunctionLibrary")
	static UTexture2D* CreateTextureFromWatermaskArray(const FString& FilePath, const int size);
	
	UFUNCTION(Blueprintable, Category="MyBlueprintFunctionLibrary")
	static int GetWatermaskPos(const FString& FilePath);

	UFUNCTION(Blueprintable, Category="MyBlueprintFunctionLibrary")
	static TArray<uint8> ReadWatermask(const FString& FilePath);
	
	UFUNCTION(BlueprintCallable, Category="MyBlueprintFunctionLibrary")
	static bool CheckMapRootPath(const FString& MapRootPath);

	UFUNCTION(BlueprintCallable, Category="MyBlueprintFunctionLibrary")
	static bool CheckTerrainRootPath(const FString& TerrainRootPath);
	
	UFUNCTION(BlueprintCallable, Category="MyBlueprintFunctionLibrary")
	static int ReadMapMaxLod(const FString& MapRootPath);

	UFUNCTION(BlueprintCallable,Category="MyBlueprintFunctionLibrary")
	static FIntVector2d GetTileStartNumber(const FString& MapRootPath, int CurrentLod);
	
	UFUNCTION(BlueprintCallable, Category="MyBlueprintFunctionLibrary")
	static FIntVector2d GetTileNumbersByLod(const FString& MapRootPath, int CurrentLod);

	UFUNCTION(BlueprintCallable, Category="MyBlueprintFunctionLibrary")
	static TArray<double> GetUnitsList(const FString& MapRootPath);

	UFUNCTION(BlueprintCallable, Category="MyBlueprintFunctionLibrary")
	static FVector2D GetCurrentCameraCoordinates(FVector CameraLocation, FIntVector2d StartXY, const int TileSize, int CurrentLod, TArray<double> UnitsList);

	UFUNCTION(BlueprintCallable, Category="MyBlueprintFunctionLibrary")
	static bool IsLodChanged(const int CurrentLod);
	
	UFUNCTION(BlueprintCallable, Category="MyBlueprintFunctionLibrary")
	static FVector2D GetNewPositionAfterLodChange(const int CurrentLod, const int TileSize, FVector2D CameraCoordinates, TArray<double> UnitsList, FIntVector2d StartXY);

	UFUNCTION(BlueprintCallable,Category="MyBlueprintFunctionLibrary")
	static FString GetBottomLeft(UCameraComponent* CameraComponent, const int TileSize, FIntVector2d TileStartNumber);

	UFUNCTION(BlueprintCallable, Category="MyBlueprintFunctionLibrary")
	static FString GetOffsetFromTopLeft(UCameraComponent* CameraComponent, const int TileSize);
	
	UFUNCTION(BlueprintCallable, Category = "File")
	static bool ReadFile(FString FilePath, FString& OutContent);
};