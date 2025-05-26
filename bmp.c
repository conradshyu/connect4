// bmp.c
//
//
// this file loads bitmaps for pieces and board
// can read .bmp files and returns HBITMAPs - handles to bitmaps.
//
// when using this from another file, you basically need two functions:
// int initbmp(HWND hwnd, char dir[256]); which is called to start with, and loads bitmaps
// from a directory and
// HBITMAP getCBbitmap(int type); which returns a handle to a bitmap.
// in the other file, you use HBITMAP bitmap;
// and bitmap = getCBbitmap(BMPBLACKMAN);
/*SetStretchBltMode(hdc,HALFTONE);

	// dark squares with BMPs
	hOldBitmap=SelectObject(bmpdc, bmp_dark);

	if(hOldBitmap == NULL)
		{
		CBlog("dark square bitmap is null");
		}
	if(bmp_dark == NULL)
		CBlog("bmp dark is null");

	StretchBlt(hdc,x0*xmetric, ymetric*y0+upperoffset, xmetric, xmetric, bmpdc, 0,0,BMPSIZE,BMPSIZE,SRCCOPY);
*/

// include headers
#include <windows.h>
#include <stdio.h>
#include "bmp.h"

// function prototypes
BITMAPFILEHEADER *DibLoadImage(PTSTR pstrFileName);
HBITMAP CreateBitmapObjectFromDibFile(HDC hdc, PTSTR szFileName);

// global bitmaps for checkers and background.
static HBITMAP bmp_background, bmp_peg, bmp_pegmask, bmp_yellowstone, bmp_redstone, bmp_stonemask;

HBITMAP getBitmap(int type)
{

	switch (type)
	{
	case BMPBACKGROUND:
		return bmp_background;
		break;
	case BMPPEG:
		return bmp_peg;
		break;
	case BMPPEGMASK:
		return bmp_pegmask;
		break;
	case BMPYELLOWSTONE:
		return bmp_yellowstone;
		break;
	case BMPREDSTONE:
		return bmp_redstone;
		break;
	case BMPSTONEMASK:
		return bmp_stonemask;
		break;

	default:
		return NULL;
	}
}

int initbmp(HWND hwnd, char dir[256])
{
	static HDC hdc;
	char filename[256];

	// get device context
	hdc = GetDC(hwnd);

	GetCurrentDirectory(255, filename);
	// load bitmaps.
	sprintf(filename, "%s\\background.bmp", dir);
	bmp_background = CreateBitmapObjectFromDibFile(hdc, filename);

	sprintf(filename, "%s\\peg.bmp", dir);
	bmp_peg = CreateBitmapObjectFromDibFile(hdc, filename);

	sprintf(filename, "%s\\pegmask.bmp", dir);
	bmp_pegmask = CreateBitmapObjectFromDibFile(hdc, filename);

	sprintf(filename, "%s\\yellowstone.bmp", dir);
	bmp_yellowstone = CreateBitmapObjectFromDibFile(hdc, filename);

	sprintf(filename, "%s\\redstone.bmp", dir);
	bmp_redstone = CreateBitmapObjectFromDibFile(hdc, filename);

	sprintf(filename, "%s\\stonemask.bmp", dir);
	bmp_stonemask = CreateBitmapObjectFromDibFile(hdc, filename);

	ReleaseDC(hwnd, hdc);

	return 0;
}

BITMAPFILEHEADER *DibLoadImage(PTSTR pstrFileName)
{
	BOOL bSuccess;
	DWORD dwFileSize, dwHighSize, dwBytesRead;
	HANDLE hFile;
	BITMAPFILEHEADER *pbmfh;
	hFile = CreateFile(pstrFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return NULL;
	dwFileSize = GetFileSize(hFile, &dwHighSize);
	if (dwHighSize)
	{
		CloseHandle(hFile);
		return NULL;
	}
	pbmfh = malloc(dwFileSize);
	if (!pbmfh)
	{
		CloseHandle(hFile);
		return NULL;
	}
	bSuccess = ReadFile(hFile, pbmfh, dwFileSize, &dwBytesRead, NULL);
	CloseHandle(hFile);
	if (!bSuccess || (dwBytesRead != dwFileSize) || (pbmfh->bfType != *(WORD *)"BM") || (pbmfh->bfSize != dwFileSize))
	{
		free(pbmfh);
		return NULL;
	}
	return pbmfh;
}

HBITMAP CreateBitmapObjectFromDibFile(HDC hdc, PTSTR szFileName)
{
	BITMAPFILEHEADER *pbmfh;
	BOOL bSuccess;
	DWORD dwFileSize, dwHighSize, dwBytesRead;
	HANDLE hFile;
	HBITMAP hBitmap;

	hFile = CreateFile(szFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return NULL;
	dwFileSize = GetFileSize(hFile, &dwHighSize);
	if (dwHighSize)
	{
		CloseHandle(hFile);
		return NULL;
	}
	pbmfh = malloc(dwFileSize);

	if (!pbmfh)
	{
		CloseHandle(hFile);
		return NULL;
	}

	bSuccess = ReadFile(hFile, pbmfh, dwFileSize, &dwBytesRead, NULL);
	CloseHandle(hFile);
	if (!bSuccess || (dwBytesRead != dwFileSize) || (pbmfh->bfType != *(WORD *)"BM") || (pbmfh->bfSize != dwFileSize))
	{
		free(pbmfh);
		return NULL;
	}
	hBitmap = CreateDIBitmap(hdc, (BITMAPINFOHEADER *)(pbmfh + 1), CBM_INIT, (BYTE *)pbmfh + pbmfh->bfOffBits, (BITMAPINFO *)(pbmfh + 1), DIB_RGB_COLORS);
	free(pbmfh);
	return hBitmap;
}
