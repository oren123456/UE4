// Fill out your copyright notice in the Description page of Project Settings.

#include "DesktopTexture.h"
#include "Engine/Texture2D.h"
#include "RenderUtils.h"
#include <iostream>

#undef UpdateResource

HDC UDesktopTexture::m_hdcScreen = NULL;
HDC UDesktopTexture::m_hdcMemDC = NULL;
HBITMAP UDesktopTexture::m_hbmScreen = NULL;
RECT UDesktopTexture::m_rcClient;
BITMAP UDesktopTexture::m_bmpScreen ;
char* UDesktopTexture::m_lpbitmap = NULL;
HANDLE UDesktopTexture::m_hDIB = NULL;

UDesktopTexture::UDesktopTexture()
{
	// Retrieve the handle to a display device context for the client  area of the window. 
	m_hdcScreen = GetDC(NULL);
	// Create a compatible DC which is used in a BitBlt from the window DC
	m_hdcMemDC = CreateCompatibleDC(m_hdcScreen);
	if (!m_hdcMemDC)
		std::cout << "CreateCompatibleDC has failed" << std::endl;
	// Get the client area for size calculation
	GetClientRect(WindowFromDC(m_hdcScreen), &m_rcClient);

	// Create a compatible bitmap from the Window DC
	m_hbmScreen = CreateCompatibleBitmap(m_hdcScreen, m_rcClient.right - m_rcClient.left, m_rcClient.bottom - m_rcClient.top);
	if (!m_hbmScreen)
		std::cout << L"CreateCompatibleBitmap Failed" << std::endl;
	// Select the compatible bitmap into the compatible memory DC.
	SelectObject(m_hdcMemDC, m_hbmScreen);

	// Get the BITMAP from the HBITMAP
	GetObject(m_hbmScreen, sizeof(BITMAP), &m_bmpScreen);

	DWORD dwBmpSize = ((m_bmpScreen.bmWidth * 32/*bi.biBitCount*/ + 31) / 32) * 4 * m_bmpScreen.bmHeight;
	HANDLE hDIB = GlobalAlloc(GHND, dwBmpSize);
	m_lpbitmap = (char *)GlobalLock(hDIB);
}

UDesktopTexture::~UDesktopTexture()
 {
	 GlobalUnlock(m_hDIB);
	 GlobalFree(m_hDIB);
 }
 
 UTexture2D* UDesktopTexture::CreateDesktopTexture(UObject* Outer)
 {
	 UTexture2D * NewTexture = UTexture2D::CreateTransient(m_rcClient.right - m_rcClient.left, m_rcClient.bottom - m_rcClient.top, EPixelFormat::PF_B8G8R8A8); 
	 NewTexture->UpdateResource();
	// Save the texture renaming as it will be useful to remap the texture streaming data.
	 NewTexture->AddressX = TA_Wrap;
	 NewTexture->AddressY = TA_Wrap;
	 NewTexture->Filter = TF_Default; 
	 NewTexture->RefreshSamplerStates();
	 return NewTexture;
 }

 
 UTexture2D* UDesktopTexture::UpdateDesktopTexture(UObject* Outer , UTexture2D * texture)
{
	// Bit block transfer into our compatible memory DC. 
	if (!BitBlt(m_hdcMemDC, 0, 0, m_bmpScreen.bmWidth, m_bmpScreen.bmHeight, m_hdcScreen, 0, 0, SRCCOPY))
		std::cout << L"BitBlt has failed" << std::endl;  

	CURSORINFO cursor = { sizeof(cursor) }; 
	::GetCursorInfo(&cursor);
	if (cursor.flags == CURSOR_SHOWING) {
		//RECT rcWnd;
		//::GetWindowRect(hwnd, &rcWnd);
		ICONINFOEXW info = { sizeof(info) };
		::GetIconInfoExW(cursor.hCursor, &info);
		const int x = cursor.ptScreenPos.x - m_rcClient.left /*- rc.left*/ - info.xHotspot;
		const int y = cursor.ptScreenPos.y - m_rcClient.top /*- rc.top*/ - info.yHotspot;
		BITMAP bmpCursor = { 0 };
		::GetObject(info.hbmColor, sizeof(bmpCursor), &bmpCursor);
		::DrawIconEx(m_hdcMemDC, x, y, cursor.hCursor, bmpCursor.bmWidth, bmpCursor.bmHeight,
			0, NULL, DI_NORMAL);
	}


	// Starting with 32-bit Windows, GlobalAlloc and LocalAlloc are implemented as wrapper functions that  call HeapAlloc using a handle to the process's default heap. Therefore, GlobalAlloc and LocalAlloc 
	// have greater overhead than HeapAlloc.
	BITMAPINFOHEADER   bi;
	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = m_bmpScreen.bmWidth;
	bi.biHeight = -m_bmpScreen.bmHeight;
	bi.biPlanes = 1;
	bi.biBitCount = 32;
	bi.biCompression = BI_RGB;
	bi.biSizeImage = 0;
	bi.biXPelsPerMeter = 0;
	bi.biYPelsPerMeter = 0;
	bi.biClrUsed = 0;
	bi.biClrImportant = 0;

	// Gets the "bits" from the bitmap and copies them into a buffer  which is pointed to by lpbitmap.
	GetDIBits(m_hdcScreen, m_hbmScreen, 0, (UINT)m_bmpScreen.bmHeight, m_lpbitmap, (BITMAPINFO *)&bi, DIB_RGB_COLORS); 

	uint8 * v = (uint8 *)m_lpbitmap;
	ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(	UpdateDynamicTextureCode,	UTexture2D*, pTexture, texture, const uint8 *, v, v,
		{
				FUpdateTextureRegion2D region;
				region.SrcX = 0;
				region.SrcY = 0;
				region.DestX = 0;
				region.DestY = 0;
				region.Width = m_rcClient.right - m_rcClient.left; 
				region.Height = m_rcClient.bottom - m_rcClient.top;
				FTexture2DResource* resource = (FTexture2DResource*)pTexture->Resource;
				RHIUpdateTexture2D(resource->GetTexture2DRHI(), 0, region, region.Width * 4, v);
				//delete[] v;
		}
	);
	return texture;
}
 
 
// 
// 
// //
//// Get next frame and write it into Data
////
// HRESULT UDesktopTexture::GetFrame(_Out_ FRAME_DATA* Data)
// {
//	 HRESULT hr = S_OK;
//
//	 IDXGIResource* DesktopResource = NULL;
//	 DXGI_OUTDUPL_FRAME_INFO FrameInfo;
//
//	 //Get new frame
//	 hr = DeskDupl->AcquireNextFrame(500, &FrameInfo, &DesktopResource);
//	 if (FAILED(hr))
//	 {
//		 if ((hr != DXGI_ERROR_ACCESS_LOST) && (hr != DXGI_ERROR_WAIT_TIMEOUT))
//		 {
//			 std::cout << "Failed to acquire next frame in DUPLICATIONMANAGER" << std::endl;
//		 }
//		 return hr;
//	 }
//
//	 // If still holding old frame, destroy it
//	 if (AcquiredDesktopImage)
//	 {
//		 AcquiredDesktopImage->Release();
//		 AcquiredDesktopImage = NULL;
//	 }
//
//	 // QI for IDXGIResource
//	 hr = DesktopResource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void **>(&AcquiredDesktopImage));
//	 DesktopResource->Release();
//	 DesktopResource = NULL;
//	 if (FAILED(hr))
//	 {
//		 std::cout << "Failed to QI for ID3D11Texture2D from acquired IDXGIResource in DUPLICATIONMANAGER" << std::endl;
//		 return hr;
//	 }
//
//	 // Get metadata
//	 if (FrameInfo.TotalMetadataBufferSize)
//	 {
//		 // Old buffer too small
//		 if (FrameInfo.TotalMetadataBufferSize > MetaDataSize)
//		 {
//			 if (MetaDataBuffer)
//			 {
//				 delete[] MetaDataBuffer;
//				 MetaDataBuffer = NULL;
//			 }
//			 MetaDataBuffer = new (std::nothrow) BYTE[FrameInfo.TotalMetadataBufferSize];
//			 if (!MetaDataBuffer)
//			 {
//				 std::cout << "Failed to allocate memory for metadata in DUPLICATIONMANAGER", L"Error" << std::endl;
//				 MetaDataSize = 0;
//				 Data->MoveCount = 0;
//				 Data->DirtyCount = 0;
//				 return E_OUTOFMEMORY;
//			 }
//			 MetaDataSize = FrameInfo.TotalMetadataBufferSize;
//		 }
//
//		 UINT BufSize = FrameInfo.TotalMetadataBufferSize;
//
//		 // Get move rectangles
//		 hr = DeskDupl->GetFrameMoveRects(BufSize, reinterpret_cast<DXGI_OUTDUPL_MOVE_RECT*>(MetaDataBuffer), &BufSize);
//		 if (FAILED(hr))
//		 {
//			 if (hr != DXGI_ERROR_ACCESS_LOST)
//			 {
//				 std::cout << "Failed to get frame move rects in DUPLICATIONMANAGER" <<
//			 }
//			 Data->MoveCount = 0;
//			 Data->DirtyCount = 0;
//			 return hr;
//		 }
//		 Data->MoveCount = BufSize / sizeof(DXGI_OUTDUPL_MOVE_RECT);
//
//		 BYTE* DirtyRects = MetaDataBuffer + BufSize;
//		 BufSize = FrameInfo.TotalMetadataBufferSize - BufSize;
//
//		 // Get dirty rectangles
//		 hr = DeskDupl->GetFrameDirtyRects(BufSize, reinterpret_cast<RECT*>(DirtyRects), &BufSize);
//		 if (FAILED(hr))
//		 {
//			 if (hr != DXGI_ERROR_ACCESS_LOST)
//				 std::cout << "Failed to get frame dirty rects in DUPLICATIONMANAGER" << std::endl;
//			 Data->MoveCount = 0;
//			 Data->DirtyCount = 0;
//			 return hr;
//		 }
//		 Data->DirtyCount = BufSize / sizeof(RECT);
//
//		 Data->MetaData = MetaDataBuffer;
//	 }
//
//	 Data->Frame = AcquiredDesktopImage;
//	 Data->FrameInfo = FrameInfo;
//
//	 return hr;
// }
//
// //
// // Release frame
// //
// HRESULT UDesktopTexture::DoneWithFrame()
// {
//	 HRESULT hr = S_OK;
//
//	 hr = DeskDupl->ReleaseFrame();
//	 if (FAILED(hr))
//	 {
//		 std::cout << "Failed to release frame in DUPLICATIONMANAGER" << std::endl;
//		 return hr;
//	 }
//
//	 if (AcquiredDesktopImage)
//	 {
//		 AcquiredDesktopImage->Release();
//		 AcquiredDesktopImage = NULL;
//	 }
//
//	 return hr;
// }
// 
 
 
 
 
 
 
 
 
 
 
 //m_NewTexture = (UTexture2D*)texture;  









 /* int32 NumBlocksX = m_bmpScreen.bmWidth / GPixelFormats[EPixelFormat::PF_B8G8R8A8].BlockSizeX;
	 int32 NumBlocksY = m_bmpScreen.bmHeight / GPixelFormats[EPixelFormat::PF_B8G8R8A8].BlockSizeY;
	 void* TextureData = Mip->BulkData.Realloc(NumBlocksX * NumBlocksY * GPixelFormats[EPixelFormat::PF_B8G8R8A8].BlockBytes);*/
 

 //m_NewTexture = UTexture2D::CreateTransient(m_rcClient.right - m_rcClient.left, m_rcClient.bottom - m_rcClient.top, EPixelFormat::PF_B8G8R8A8); 
	//m_NewTexture->UpdateResource();
	//	// Save the texture renaming as it will be useful to remap the texture streaming data.
	//m_NewTexture->AddressX = TA_Wrap;
	//m_NewTexture->AddressY = TA_Wrap;
	//m_NewTexture->Filter = TF_Default;
	//m_NewTexture->RefreshSamplerStates(); 

 /*FTexture2DMipMap* Mip = &m_NewTexture->PlatformData->Mips[0];
	void* TextureData = Mip->BulkData.Lock(LOCK_READ_WRITE);
	FMemory::Memcpy(TextureData, m_lpbitmap, dwBmpSize);
	Mip->BulkData.Unlock();
	m_NewTexture->UpdateResource(); */

 //UTexture* RenamedTexture = NULL;
	//UTexture* texture = NULL;
	//FMaterialParameterInfo ParameterInfo(ParameterName);
	//mat->GetTextureParameterValue(ParameterInfo, texture);
	//UTexture2D* NewTexture = (UTexture2D*)texture;
 //NewTexture->PlatformData->Mips[0].SizeX = m_bmpScreen.bmWidth;
	//NewTexture->PlatformData->Mips[0].SizeY = m_bmpScreen.bmHeight;

	//NewTexture->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
	//void* TextureData = NewTexture->PlatformData->Mips[0].BulkData.Realloc(NumBlocksX * NumBlocksY * GPixelFormats[EPixelFormat::PF_B8G8R8A8].BlockBytes);
	//NewTexture->PlatformData->Mips[0].BulkData.Unlock();
 //if (!m_NewTexture->PlatformData)	{
	//	m_NewTexture->PlatformData = new FTexturePlatformData();
	//	m_NewTexture->PlatformData->SizeX = m_bmpScreen.bmWidth;
	//	m_NewTexture->PlatformData->SizeY = m_bmpScreen.bmHeight;
	//	m_NewTexture->PlatformData->PixelFormat = EPixelFormat::PF_B8G8R8A8;
	//	// Allocate first mipmap and upload the pixel data 
	//	int32 NumBlocksX = m_bmpScreen.bmWidth / GPixelFormats[EPixelFormat::PF_B8G8R8A8].BlockSizeX;
	//	int32 NumBlocksY = m_bmpScreen.bmHeight / GPixelFormats[EPixelFormat::PF_B8G8R8A8].BlockSizeY;
	//	FTexture2DMipMap* Mip = new(m_NewTexture->PlatformData->Mips) FTexture2DMipMap();
	//	Mip->SizeX = m_bmpScreen.bmWidth;
	//	Mip->SizeY = m_bmpScreen.bmHeight;
	//	Mip->BulkData.Lock(LOCK_READ_WRITE);
	//	Mip->BulkData.Realloc(NumBlocksX * NumBlocksY * GPixelFormats[EPixelFormat::PF_B8G8R8A8].BlockBytes);
	//	Mip->BulkData.Unlock();
	//}