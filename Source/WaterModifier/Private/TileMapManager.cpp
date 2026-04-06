// Fill out your copyright notice in the Description page of Project Settings.


#include "TileMapManager.h"

// Sets default values
ATileMapManager::ATileMapManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ATileMapManager::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ATileMapManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

static FVector LastCameraLocation;
static int LastLod = -1;
static float LastOrthoWidth = 1024;
//每帧调用，检查相机所覆盖范围是否改变。
void ATileMapManager::Tick(EExcutePinNodes& Result, UCameraComponent* CameraComponent, int CurrentLod, int TileSize)
{
	Result = EExcutePinNodes::NoChange;
	FVector CameraLocation = CameraComponent->GetComponentLocation();
	int OrthoWidth = CameraComponent->OrthoWidth;
	
	int LastBottomLeftX = floor((LastCameraLocation.X - LastOrthoWidth/2) / TileSize);
	int CurrentBottomLeftX = floor((CameraLocation.X - OrthoWidth/2) / TileSize);
	int LastBottomLeftY = floor((LastCameraLocation.Z - LastOrthoWidth/2) / TileSize);
	int CurrentBottomLeftY = floor((CameraLocation.Z - OrthoWidth/2) / TileSize);
	int LastTopRightX = floor((LastCameraLocation.X + LastOrthoWidth/2) / TileSize);
	int CurrentTopRightX = floor((CameraLocation.X + OrthoWidth/2) / TileSize);
	int LastTopRightY = floor((LastCameraLocation.Z + LastOrthoWidth/2) / TileSize);
	int CurrentTopRightY = floor((CameraLocation.Z + OrthoWidth/2) / TileSize);
	LastCameraLocation = CameraLocation;
	LastOrthoWidth = OrthoWidth;
	if(LastBottomLeftX!=CurrentBottomLeftX||LastBottomLeftY!=CurrentBottomLeftY||
		LastTopRightX!=CurrentTopRightX||LastTopRightY!=CurrentTopRightY)
	{
		Result=EExcutePinNodes::OnRangeChange;
	}
	if (LastLod!=CurrentLod)
	{
		LastLod=CurrentLod;
		Result=EExcutePinNodes::OnLodChange;
	}
}

FIntVector4 ATileMapManager::GetBottomLeftAndTopRight(UCameraComponent* CameraComponent, int TileSize)
{
	FIntVector4 BottomLeftAndTopRight;

	FVector CameraLocation = CameraComponent->GetComponentLocation();
	int OrthoWidth = CameraComponent->OrthoWidth;
	BottomLeftAndTopRight.X = floor((CameraLocation.X - OrthoWidth/2)/TileSize);
	BottomLeftAndTopRight.Y = floor((CameraLocation.Z - OrthoWidth/2)/TileSize);
	BottomLeftAndTopRight.Z = floor((CameraLocation.X + OrthoWidth/2)/TileSize);
	BottomLeftAndTopRight.W = floor((CameraLocation.Z + OrthoWidth/2)/TileSize);
	
	return BottomLeftAndTopRight;
}

TArray<FIntPoint> ATileMapManager::GetUnionMinusFirst(FIntPoint Square1BottomLeft, FIntPoint Square1TopRight,
	FIntPoint Square2BottomLeft, FIntPoint Square2TopRight)
{
	TArray<FIntPoint> Result;
	// 遍历第二个正方形内所有整数点（闭区间）
	for (int32 X = Square2BottomLeft.X; X <= Square2TopRight.X; X++)
	{
		for (int32 Y = Square2BottomLeft.Y; Y <= Square2TopRight.Y; Y++)
		{
			// 若该点不在第一个正方形内，则加入结果
			if (!(X >= Square1BottomLeft.X && X <= Square1TopRight.X &&
				  Y >= Square1BottomLeft.Y && Y <= Square1TopRight.Y))
			{
				Result.Add(FIntPoint(X, Y));
			}
		}
	}
	return Result;
}

TArray<FIntPoint> ATileMapManager::GetIntersection(FIntPoint Square1BottomLeft, FIntPoint Square1TopRight,
	FIntPoint Square2BottomLeft, FIntPoint Square2TopRight)
{
	TArray<FIntPoint> Result;
	// 遍历第一个正方形内所有整数点（闭区间）
	for (int32 X = Square1BottomLeft.X; X <= Square1TopRight.X; X++)
	{
		for (int32 Y = Square1BottomLeft.Y; Y <= Square1TopRight.Y; Y++)
		{
			// 若该点不在第二个正方形内，则加入结果
			if (X >= Square2BottomLeft.X && X <= Square2TopRight.X &&
				  Y >= Square2BottomLeft.Y && Y <= Square2TopRight.Y)
			{
				Result.Add(FIntPoint(X, Y));
			}
		}
	}
	return Result;
}
