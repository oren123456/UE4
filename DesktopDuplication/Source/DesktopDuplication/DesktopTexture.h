// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
//#include <d3d11.h>
//#include <dxgi1_2.h>

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "DesktopTexture.generated.h"

class UMaterialInstanceDynamic;

//
// FRAME_DATA holds information about an acquired frame
//
//typedef struct _FRAME_DATA
//{
//	ID3D11Texture2D* Frame;
//	DXGI_OUTDUPL_FRAME_INFO FrameInfo;
//	_Field_size_bytes_((MoveCount * sizeof(DXGI_OUTDUPL_MOVE_RECT)) + (DirtyCount * sizeof(RECT))) BYTE* MetaData;
//	UINT DirtyCount;
//	UINT MoveCount;
//} FRAME_DATA;
/**
 * 
 */
UCLASS()
class DESKTOPDUPLICATION_API UDesktopTexture : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	///
	UDesktopTexture();
	///
	~UDesktopTexture();
	/** Loads an image file from disk into a texture on a worker thread. This will not block the calling thread.
	@return An image loader object with an OnLoadCompleted event that users can bind to, to get notified when loading is done. */
	UFUNCTION(BlueprintCallable, Category = "oren" , meta = (HidePin = "Outer", DefaultToSelf = "Outer"))
		static UTexture2D* UpdateDesktopTexture(UObject* Outer , UTexture2D* texture);

	UFUNCTION(BlueprintCallable, Category = "oren", meta = (HidePin = "Outer", DefaultToSelf = "Outer"))
		static UTexture2D* CreateDesktopTexture(UObject* Outer);

private:
	//HRESULT DoneWithFrame();
	//HRESULT UDesktopTexture::GetFrame(_Out_ FRAME_DATA* Data);
	//IDXGIOutputDuplication* m_DeskDupl;
	//ID3D12Resource* m_AcquiredDesktopImage;
	/*_Field_size_bytes_(m_MetaDataSize) BYTE* m_MetaDataBuffer;
	UINT m_MetaDataSize;
	UINT m_OutputNumber;
	DXGI_OUTPUT_DESC m_OutputDesc;  */

	static HDC m_hdcScreen;
	static HDC m_hdcMemDC;
	static HBITMAP m_hbmScreen;
	static RECT m_rcClient;
	static BITMAP m_bmpScreen;
	static HANDLE m_hDIB;
	static char* m_lpbitmap;
};
