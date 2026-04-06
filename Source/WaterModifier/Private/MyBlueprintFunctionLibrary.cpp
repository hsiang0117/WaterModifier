// Fill out your copyright notice in the Description page of Project Settings.

#include "MyBlueprintFunctionLibrary.h"

#include "IImageWrapperModule.h"
#include "IImageWrapper.h"
#include "XmlFile.h"
#include "Modules/ModuleManager.h"
#include "Engine/Texture2D.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformFilemanager.h"

UTexture2D* UMyBlueprintFunctionLibrary::MyImportFileAsTexture2D(const FString& ImagePath)
{
    if (!FPlatformFileManager::Get().GetPlatformFile().FileExists(*ImagePath))
    {
        UE_LOG(LogTemp, Warning, TEXT("File not found: %s"), *ImagePath);
        return nullptr;
    }

    // 读取文件内容到数组中
    TArray<uint8> RawFileData;
    if (!FFileHelper::LoadFileToArray(RawFileData, *ImagePath))
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to load file: %s"), *ImagePath);
        return nullptr;
    }

    // 创建图像包装器模块
    IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));

    // 检测图像格式（支持JPEG、PNG等）
    EImageFormat ImageFormat = ImageWrapperModule.DetectImageFormat(RawFileData.GetData(), RawFileData.Num());
    if (ImageFormat == EImageFormat::Invalid)
    {
        UE_LOG(LogTemp, Warning, TEXT("Invalid image format for file: %s"), *ImagePath);
        return nullptr;
    }

    // 创建图像包装器
    TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(ImageFormat);
    if (!ImageWrapper.IsValid() || !ImageWrapper->SetCompressed(RawFileData.GetData(), RawFileData.Num()))
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to initialize image wrapper for file: %s"), *ImagePath);
        return nullptr;
    }

    // 解压缩图像数据
    TArray<uint8> UncompressedBGRA;
    if (!ImageWrapper->GetRaw(ERGBFormat::BGRA, 8, UncompressedBGRA))
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to get raw image data for file: %s"), *ImagePath);
        return nullptr;
    }
    UTexture2D* ImportedDynamicTexture = UTexture2D::CreateTransient(ImageWrapper->GetWidth(), ImageWrapper->GetHeight(), PF_B8G8R8A8);
    ImportedDynamicTexture->bNotOfflineProcessed = true;
    ImportedDynamicTexture->AddressX = TA_Clamp;
    ImportedDynamicTexture->AddressY = TA_Clamp;
    // 将数据拷贝到UTexture2D
    void* TextureData = ImportedDynamicTexture->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
    FMemory::Memcpy(TextureData, UncompressedBGRA.GetData(), UncompressedBGRA.Num());
    ImportedDynamicTexture->GetPlatformData()->Mips[0].BulkData.Unlock();

    // 更新纹理
    ImportedDynamicTexture->UpdateResource();

    return ImportedDynamicTexture;
}

UTexture2D* UMyBlueprintFunctionLibrary::CreateTextureFromWatermaskArray(const FString& FilePath, const int size)
{
    UTexture2D* GeneratedDynamicTexture = UTexture2D::CreateTransient(size,size,PF_B8G8R8A8);
    GeneratedDynamicTexture->bNotOfflineProcessed = true;
    GeneratedDynamicTexture->AddressX = TA_Clamp;
    GeneratedDynamicTexture->AddressY = TA_Clamp;

    if (GeneratedDynamicTexture)
    {
        TArray<uint8> Array=ReadWatermask(FilePath);
        const int32 TotalPixels = size * size;
        void* TextureData = GeneratedDynamicTexture->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
        uint8* DestPtr = static_cast<uint8*>(TextureData);
        if (Array.Num()==TotalPixels)
        {
            for (int32 i = 0; i < TotalPixels; ++i)
            {
                uint8 ColorValue = Array[i] ? 255 : 0;
                uint8 alpha = Array[i] ? 64 : 0;

                // 注意：纹理数据按 BGRA 排列
                DestPtr[i * 4 + 0] = ColorValue; // Blue
                DestPtr[i * 4 + 1] = 0; // Green
                DestPtr[i * 4 + 2] = 0; // Red
                DestPtr[i * 4 + 3] = alpha;        // Alpha，设为不透明
            }
            GeneratedDynamicTexture->GetPlatformData()->Mips[0].BulkData.Unlock();
            GeneratedDynamicTexture->UpdateResource();
            return GeneratedDynamicTexture;
        }
        for (int32 i = 0; i < TotalPixels; ++i)
        {
            // 注意：纹理数据按 BGRA 排列
            DestPtr[i * 4 + 0] = 0; // Blue
            DestPtr[i * 4 + 1] = 0; // Green
            DestPtr[i * 4 + 2] = 0; // Red
            DestPtr[i * 4 + 3] = 0; // Alpha，设为透明
        }
        GeneratedDynamicTexture->GetPlatformData()->Mips[0].BulkData.Unlock();
        GeneratedDynamicTexture->UpdateResource();
        return GeneratedDynamicTexture;
    }
    return nullptr;
}

int UMyBlueprintFunctionLibrary::GetWatermaskPos(const FString& FilePath)
{
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    TUniquePtr<IFileHandle> FileHandle(PlatformFile.OpenRead(*FilePath));
    if (!FileHandle)
    {
        return -1;  // 如果文件无法打开，返回 -1
    }

    int64 FileSize = FileHandle->Size();
    FileHandle->Seek(88);  // 定位到文件的第88个字节

    // 读取 vertex_count
    uint32 VertexCount = 0;
    FileHandle->Read(reinterpret_cast<uint8*>(&VertexCount), sizeof(uint32));

    // 跳过2 * vertex_count * 3 字节
    FileHandle->Seek(FileHandle->Tell() + 2 * VertexCount * 3);

    // 读取 triangle_count
    uint32 TriangleCount = 0;
    FileHandle->Read(reinterpret_cast<uint8*>(&TriangleCount), sizeof(uint32));
    uint32 IndicesSize = (TriangleCount < 65536) ? 2 : 4;

    // 跳过 indices_size * triangle_count * 3 字节
    FileHandle->Seek(FileHandle->Tell() + IndicesSize * TriangleCount * 3);

    // 读取四个方向的 vertex_count，并跳过相应的字节
    uint32 WestVertexCount = 0;
    FileHandle->Read(reinterpret_cast<uint8*>(&WestVertexCount), sizeof(uint32));
    FileHandle->Seek(FileHandle->Tell() + 2 * WestVertexCount);

    uint32 SouthVertexCount = 0;
    FileHandle->Read(reinterpret_cast<uint8*>(&SouthVertexCount), sizeof(uint32));
    FileHandle->Seek(FileHandle->Tell() + 2 * SouthVertexCount);

    uint32 EastVertexCount = 0;
    FileHandle->Read(reinterpret_cast<uint8*>(&EastVertexCount), sizeof(uint32));
    FileHandle->Seek(FileHandle->Tell() + 2 * EastVertexCount);

    uint32 NorthVertexCount = 0;
    FileHandle->Read(reinterpret_cast<uint8*>(&NorthVertexCount), sizeof(uint32));
    FileHandle->Seek(FileHandle->Tell() + 2 * NorthVertexCount);

    // 开始遍历扩展部分
    while (FileHandle->Tell() < FileSize)
    {
        uint8 ExtensionType = 0;
        FileHandle->Read(&ExtensionType, sizeof(uint8));

        if (ExtensionType == 2)
        {
            return FileHandle->Tell();  // 返回当前文件位置
        }
        else
        {
            uint32 ExtensionLength = 0;
            FileHandle->Read(reinterpret_cast<uint8*>(&ExtensionLength), sizeof(uint32));
            FileHandle->Seek(FileHandle->Tell() + ExtensionLength);  // 跳过扩展长度
        }
    }

    return -1;
}

TArray<uint8> UMyBlueprintFunctionLibrary::ReadWatermask(const FString& FilePath)
{
    TArray<uint8> WatermaskData;
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    TUniquePtr<IFileHandle> FileHandle(PlatformFile.OpenRead(*FilePath));
    if (!FileHandle)
    {
        return WatermaskData; // 文件打开失败
    }

    // 获取文件大小
    int64 FileSize = FileHandle->Size();
    int Position = GetWatermaskPos(FilePath);
    // 检查位置是否合法
    if (Position < 0 || Position >= FileSize)
    {
        return WatermaskData; // 无效的位置
    }

    // 定位到指定位置
    FileHandle->Seek(Position);

    // 读取水印长度（4字节）
    uint32 WatermaskLength = 0;
    FileHandle->Read(reinterpret_cast<uint8*>(&WatermaskLength), sizeof(uint32));

    // 检查水印长度是否有效
    if (WatermaskLength == 0 || WatermaskLength > FileSize - FileHandle->Tell())
    {
        return WatermaskData; // 无效的水印长度
    }

    // 读取水印数据
    WatermaskData.SetNumUninitialized(WatermaskLength);
    FileHandle->Read(WatermaskData.GetData(), WatermaskLength);

    return WatermaskData;
}

bool UMyBlueprintFunctionLibrary::CheckMapRootPath(const FString& MapRootPath)
{
    const FString FilePath = MapRootPath + "tilemapresource.xml";
    if (FPlatformFileManager::Get().GetPlatformFile().FileExists(*FilePath))
    {
        return true;
    }
    return false;
}

bool UMyBlueprintFunctionLibrary::CheckTerrainRootPath(const FString& TerrainRootPath)
{
    const FString FilePath = TerrainRootPath + "layer.json";
    if (FPlatformFileManager::Get().GetPlatformFile().FileExists(*FilePath))
    {
        return true;
    }
    return false;
}

//读取地图的最大lod等级
int UMyBlueprintFunctionLibrary::ReadMapMaxLod(const FString& MapRootPath)
{
    int MaxLod = -1;
    const FString jsonPath = MapRootPath + TEXT("\\meta.json");
	
    if(FPlatformFileManager::Get().GetPlatformFile().FileExists(*jsonPath))
    {
        FString jsonString;
        FFileHelper::LoadFileToString(jsonString,*jsonPath);
        TSharedPtr<FJsonObject> jsonObject;
        TSharedRef<TJsonReader<TCHAR>> reader = TJsonReaderFactory<TCHAR>::Create(jsonString);
        if (FJsonSerializer::Deserialize(reader, jsonObject) && jsonObject.IsValid())
        {
            MaxLod = jsonObject->GetIntegerField(TEXT("maxzoom"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to get maxzoom"));
    }
    return MaxLod;
}

FIntVector2d UMyBlueprintFunctionLibrary::GetTileStartNumber(const FString& MapRootPath, int CurrentLod)
{
    FIntVector2d TileStart = {0,0};

    IFileManager& MyFileManager = IFileManager::Get();
    if(MyFileManager.DirectoryExists(*MapRootPath))
    {
        FString FolderPath = MapRootPath / FString::FromInt(CurrentLod) / TEXT("*");
        TArray<FString> FolderNames;

        MyFileManager.FindFiles(FolderNames,*FolderPath,false,true);
        if(FolderNames.Num()!=0)
        {
            TileStart.X = FCString::Atoi(*FolderNames[0]);
            FString MapPath = MapRootPath / FString::FromInt(CurrentLod) / FolderNames[0] / TEXT("*");
            TArray<FString> MapNames;
            MyFileManager.FindFiles(MapNames,*MapPath,true,false);
            if(MapNames.Num()!=0)
            {
                TileStart.Y = FCString::Atoi(*MapNames[0]);
            }
        }
    }
	
    return TileStart;
}

FIntVector2d UMyBlueprintFunctionLibrary::GetTileNumbersByLod(const FString& MapRootPath, int CurrentLod)
{
    FIntVector2d XYNums={0,0};
	
    IFileManager& MyFileManager = IFileManager::Get();
    if(MyFileManager.DirectoryExists(*MapRootPath))
    {
        FString FolderPath = MapRootPath / FString::FromInt(CurrentLod) / TEXT("*");
        TArray<FString> FolderNames;

        MyFileManager.FindFiles(FolderNames,*FolderPath,false,true);
        if(FolderNames.Num()!=0)
        {
            XYNums.X = FolderNames.Num();
            FString MapPath = MapRootPath / FString::FromInt(CurrentLod) / FolderNames[0] / TEXT("*");
            TArray<FString> MapNames;
            MyFileManager.FindFiles(MapNames,*MapPath,true,false);
            XYNums.Y = MapNames.Num();
        }
    }
	
    return XYNums;
}

TArray<double> UMyBlueprintFunctionLibrary::GetUnitsList(const FString& MapRootPath)
{
    TArray<double> List;
    IFileManager& MyFileManager = IFileManager::Get();
    
    if(MyFileManager.DirectoryExists(*MapRootPath))
    {
        FString XmlPath = MapRootPath / TEXT("\\tilemapresource.xml");
        FString XmlContent;
        FFileHelper::LoadFileToString(XmlContent,*XmlPath);

        //ue5的xml解析器要求属性名，等号，属性值之间不能有空格，用正则表达式去除空格
        FRegexPattern Pattern(TEXT(R"(([^\s<>]+)\s*=\s*(".*?"))"));
        FRegexMatcher Matcher(Pattern, XmlContent);
        FString NewXmlContent;
        int32 LastIndex = 0;
        while (Matcher.FindNext())
        {
            int32 MatchBegin = Matcher.GetMatchBeginning();
            int32 MatchEnd = Matcher.GetMatchEnding();
            // 将上次结束到本次匹配前的内容复制过去
            NewXmlContent += XmlContent.Mid(LastIndex, MatchBegin - LastIndex);
            // 获取捕获组：属性名和属性值
            FString AttrName = Matcher.GetCaptureGroup(1);
            FString AttrValue = Matcher.GetCaptureGroup(2);
            // 组合成无空格的形式
            NewXmlContent += AttrName + TEXT("=") + AttrValue;
            LastIndex = MatchEnd;
        }
        // 追加最后剩余的内容
        NewXmlContent += XmlContent.Mid(LastIndex);
        
        FXmlFile XmlFile(NewXmlContent, EConstructMethod::ConstructFromBuffer);
        if(!XmlFile.IsValid())
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to load xmlfile"));
        }
        if(FXmlNode* RootNode = XmlFile.GetRootNode())
        {
            if(const FXmlNode* TileSetsNode = RootNode->FindChildNode(TEXT("TileSets")))
            {
                const TArray<FXmlNode*>& TileSets = TileSetsNode->GetChildrenNodes();
                for(const FXmlNode* TileSetNode : TileSets)
                {
                    FString UnitsPerPixel = TileSetNode->GetAttribute(TEXT("units-per-pixel"));
                    List.Add(FCString::Atod(*UnitsPerPixel));
                }
            }else
            {
                UE_LOG(LogTemp, Error, TEXT("Failed to get TileSets node"));
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to get rootnode"));
        }
    }
    return List;
}

FVector2D UMyBlueprintFunctionLibrary::GetCurrentCameraCoordinates(FVector CameraLocation,
                                                                   FIntVector2d StartXY, const int TileSize, int CurrentLod, TArray<double> UnitsList)
{
    FVector2D Coordinates;
    
    if(UnitsList.Num()!=0)
    {
        if (CurrentLod>=0&&CurrentLod<UnitsList.Num())
        {
            Coordinates.X = ((StartXY.X * TileSize) + CameraLocation.X) * UnitsList[CurrentLod] - 180;
            Coordinates.Y = ((StartXY.Y * TileSize) + CameraLocation.Z) * UnitsList[CurrentLod] - 90;
        }
    }
    
    return Coordinates;
}

static int LastLod = -1;
bool UMyBlueprintFunctionLibrary::IsLodChanged(const int CurrentLod)
{
    if(LastLod != CurrentLod)
    {
        LastLod = CurrentLod;
        return true;
    }
    return false;
}

FVector2D UMyBlueprintFunctionLibrary::GetNewPositionAfterLodChange(const int NewLod, const int TileSize,
                                                                    FVector2D CameraCoordinates, TArray<double> UnitsList, FIntVector2d StartXY)
{
    FVector2D NewPosition;
    if (UnitsList.Num()!=0)
    {
        if (NewLod>=0&&NewLod<UnitsList.Num())
        {
            NewPosition.X = (CameraCoordinates.X + 180)/UnitsList[NewLod] - StartXY.X * TileSize;
            NewPosition.Y = (CameraCoordinates.Y + 90)/UnitsList[NewLod] - StartXY.Y * TileSize;
        }
    }
    return NewPosition;
}

FString UMyBlueprintFunctionLibrary::GetBottomLeft(UCameraComponent* CameraComponent, const int TileSize, FIntVector2d TileStartNumber)
{
    FString BottomLeft;

    FVector CameraLocation = CameraComponent->GetComponentLocation();
    int OrthalWidth = CameraComponent->OrthoWidth;

    if (TileSize>0)
    {
        int BottomLeftX = floor((CameraLocation.X - OrthalWidth/2)/TileSize) + TileStartNumber.X;
        int BottomLeftY = floor((CameraLocation.Z - OrthalWidth/2)/TileSize) + TileStartNumber.Y;
        BottomLeft.Append(FString::FromInt(BottomLeftX));
        BottomLeft.Append(" ");
        BottomLeft.Append(FString::FromInt(BottomLeftY));
    }
    else
    {
        BottomLeft.Append("0 0");
    }
	
    return BottomLeft;
}

FString UMyBlueprintFunctionLibrary::GetOffsetFromTopLeft(UCameraComponent* CameraComponent,const int TileSize)
{
    FString Offset;
    FVector CameraLocation = CameraComponent->GetComponentLocation();

    if (TileSize>0)
    {
        int OffsetX = static_cast<int>(floor(CameraLocation.X)) % TileSize;
        int OffsetY = TileSize - static_cast<int>(floor(CameraLocation.Z)) % TileSize == TileSize ? 0 : TileSize - static_cast<int>(floor(CameraLocation.Z)) % TileSize;
        Offset.Append(FString::FromInt(OffsetX));
        Offset.Append(" ");
        Offset.Append(FString::FromInt(OffsetY));
    }else
    {
        Offset.Append("0 0");
    }
    
    return Offset;
}

bool UMyBlueprintFunctionLibrary::ReadFile(FString FilePath, FString& OutContent)
{
    return FFileHelper::LoadFileToString(OutContent, *FilePath);
}
