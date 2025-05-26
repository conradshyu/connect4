#include <windows.h>
#include <commctrl.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "bmp.h"

/* function prototypes */
void printboard(HWND, HDC, HDC, int);
void takeback(int);
int checkfour(int[7][6]);
int findmove(int board[7][6], int color, double maxtime, char out[255], int *play, int sw, int rl, int usebook, int gt, int dumb);
int exitroutine2(int bestmove);
/* global variables */

static int originalboard[7][6];
static char str[80];
int originalfilling[7];
static int player = 1;
static int computer = -1;
static int level = 10; // level set to intermediate as default.

static long info[20];
extern int final;
extern char out[255];
int bestmove;

void setlevel(int l)
{
	level = l;
}

void setselect(int s)
{
	info[19] = s;
}

void exitroutine(int bestmove)
{
	// extern int playnow;
	extern int calculating;

	calculating = 0;
	// playnow=0;
	exitroutine2(bestmove);
	ExitThread(0);
}

int getcomputermove(HWND hwnd, HDC hdc, HWND hStatusWnd, int color, int *play)
{
	int bestmove;
	extern int randomize, usethebook, gametheoretic, searchwin, evaltype;

	double maxtime;
	maxtime = ((double)level) / 10.0;

	/* call findmove in pro4_2.c */
	sprintf(out, "calling findmove");
	bestmove = findmove(originalboard, color, maxtime, out, play, searchwin, randomize, usethebook, gametheoretic, evaltype);
	return (bestmove);
}

void takeback(int move)
{
	originalboard[move][originalfilling[move] - 1] = 0;
	originalfilling[move]--;
}

void forward(int move, int color)
{
	originalboard[move][originalfilling[move]] = color;
	originalfilling[move]++;
}
/* putmove returns 1 if a move was winning and 2 if the game is a draw */

int putmove(HWND hwnd, HDC memdc, HDC bmpdc, int move, int color)
{
	int win;

	if (originalfilling[move] < 6)
	{
		/*write move into board */
		originalboard[move][originalfilling[move]] = color;
		originalfilling[move]++;
		/************************/

		/* check if there are four in a row */
		win = checkfour(originalboard);
		/************************************/

		/*print board and request a repaint of window */
		printboard(hwnd, memdc, bmpdc, 0);
		// GetClientRect(hwnd,&r);
		// r.left=r.right/7*move;
		// r.right=r.right/7*(move+1);
		// r.top=r.bottom/6*(5-originalfilling[move]);
		// r.bottom=r.bottom/6*(7-originalfilling[move]);
		// InvalidateRect(hwnd,&r,FALSE);
		InvalidateRect(hwnd, NULL, 1);
		/**********************************************/

		return (win);
	}
	else
		return (-1);
}
/* checkfour returns 1 if one of the players has won and 2 if it is a draw */
int checkfour(int board[7][6])
{
	int count;
	int i, j;

	/* von links nach rechts */
	for (i = 0; i <= 3; i++)
	{
		for (j = 0; j <= 5; j++)
		{
			count = 0;
			count = board[i][j] + board[i + 1][j] + board[i + 2][j] + board[i + 3][j];
			if (abs(count) == 4)
				return (1);
		}
	}
	/* von oben nach unten */
	for (i = 0; i <= 6; i++)
	{
		for (j = 0; j <= 2; j++)
		{
			count = 0;
			count = board[i][j] + board[i][j + 1] + board[i][j + 2] + board[i][j + 3];
			if (abs(count) == 4)
				return (1);
		}
	}
	/* von links unten nach rechts oben*/
	for (i = 0; i <= 3; i++)
	{
		for (j = 0; j <= 2; j++)
		{
			count = 0;
			count = board[i][j] + board[i + 1][j + 1] + board[i + 2][j + 2] + board[i + 3][j + 3];
			if (abs(count) == 4)
				return (1);
		}
	}
	/* von links oben nach rechts unten*/
	for (i = 0; i <= 3; i++)
	{
		for (j = 3; j <= 5; j++)
		{
			count = board[i][j] + board[i + 1][j - 1] + board[i + 2][j - 2] + board[i + 3][j - 3];
			if (abs(count) == 4)
				return (1);
		}
	}
	count = 0;
	for (i = 0; i <= 6; i++)
	{
		for (j = 0; j <= 5; j++)
			if (board[i][j])
				count++;
	}
	if (count == 42)
		return (2);
	else
		return (0);
}

void newgame(HWND hwnd, HDC memdc, HDC bmpdc, HWND hStatusWnd)
{

	int i, j;
	extern int calculating;
	// extern int playnow;
	extern int count;
	count = 0;
	// playnow=0;
	calculating = 0;
	for (i = 0; i <= 6; i++)
	{
		for (j = 0; j <= 5; j++)
		{
			originalboard[i][j] = 0;
		}
	}
	for (i = 0; i <= 6; i++)
		originalfilling[i] = 0;
	computer = -1;
	printboard(hwnd, memdc, bmpdc, 1);
	InvalidateRect(hwnd, NULL, FALSE);

	/* set the statusbar */
	sprintf(str, "");
	SendMessage(hStatusWnd, SB_SETTEXT, (WPARAM)0, (LPARAM)str);
	/*SendMessage(hStatusWnd,SB_SETTEXT,(WPARAM) 1,(LPARAM) str);
	SendMessage(hStatusWnd,SB_SETTEXT,(WPARAM) 2,(LPARAM) str);
	SendMessage(hStatusWnd,SB_SETTEXT,(WPARAM) 3,(LPARAM) str);
	SendMessage(hStatusWnd,SB_SETTEXT,(WPARAM) 4,(LPARAM) str);*/
}

void printboard(HWND hwnd, HDC memdc, HDC bmpdc, int resize)
{

	int i, j, xmetric, ymetric, maxX, maxY, statussize;
	extern int xClientView, yClientView;
	HBRUSH /*hbrush,*/ yellowbrush, redbrush, blackbrush;
	extern int ywindowoffset;
	HBITMAP bmp_background, bmp_yellowstone, bmp_redstone, bmp_stonemask, oldbitmap;

	bmp_background = getBitmap(BMPBACKGROUND);
	bmp_yellowstone = getBitmap(BMPYELLOWSTONE);
	bmp_redstone = getBitmap(BMPREDSTONE);
	bmp_stonemask = getBitmap(BMPSTONEMASK);

	SetStretchBltMode(memdc, HALFTONE);
	SetStretchBltMode(bmpdc, HALFTONE);

	oldbitmap = SelectObject(bmpdc, bmp_background);

	/*if(hOldBitmap == NULL)
		{
		CBlog("dark square bitmap is null");
		}
	if(bmp_dark == NULL)
		CBlog("bmp dark is null");*/

	redbrush = CreateSolidBrush(RGB(255, 0, 0));
	yellowbrush = CreateSolidBrush(RGB(255, 255, 0));
	blackbrush = CreateSolidBrush(RGB(0, 0, 0));

	statussize = GetSystemMetrics(SM_CYSIZE);
	xmetric = xClientView / 7;
	ymetric = (yClientView - statussize) / 6;

	maxX = GetSystemMetrics(SM_CXSCREEN);
	maxY = GetSystemMetrics(SM_CYSCREEN);

	StretchBlt(memdc, 0, ywindowoffset, xClientView, yClientView, bmpdc, 0, 0, 346, 346, SRCCOPY);

	// background
	/*if(resize)
		{
		hbrush = GetStockObject(DKGRAY_BRUSH);
		SelectObject(memdc, hbrush);
		PatBlt(memdc, 0,0,maxX,maxY,PATCOPY);
		}*/

	for (i = 0; i <= 6; i++)
	{
		for (j = 0; j <= 5; j++)
		{

			if (originalboard[i][j] == 0)
				continue;
			if (originalboard[i][j] == player)
			{
				oldbitmap = SelectObject(bmpdc, bmp_stonemask);
				StretchBlt(memdc, xmetric * i, ymetric * (5 - j) + ywindowoffset, xmetric, xmetric, bmpdc, 0, 0, BMPSIZE, BMPSIZE, SRCAND);
				SelectObject(bmpdc, oldbitmap);

				oldbitmap = SelectObject(bmpdc, bmp_yellowstone);
				StretchBlt(memdc, xmetric * i, ymetric * (5 - j) + ywindowoffset, xmetric, xmetric, bmpdc, 0, 0, BMPSIZE, BMPSIZE, SRCPAINT);
				SelectObject(bmpdc, oldbitmap);
				// SelectObject(memdc,yellowbrush);
				// Ellipse(memdc,xmetric*i,ymetric*(6-j)+ywindowoffset,xmetric*(i+1)-1,ymetric*(6-j-1)+1+ywindowoffset);
			}

			if (originalboard[i][j] == computer)
			{
				oldbitmap = SelectObject(bmpdc, bmp_stonemask);
				StretchBlt(memdc, xmetric * i, ymetric * (5 - j) + ywindowoffset, xmetric, xmetric, bmpdc, 0, 0, BMPSIZE, BMPSIZE, SRCAND);
				SelectObject(bmpdc, oldbitmap);

				oldbitmap = SelectObject(bmpdc, bmp_redstone);
				StretchBlt(memdc, xmetric * i, ymetric * (5 - j) + ywindowoffset, xmetric, xmetric, bmpdc, 0, 0, BMPSIZE, BMPSIZE, SRCPAINT);
				SelectObject(bmpdc, oldbitmap);
				// SelectObject(memdc,redbrush);
				// Ellipse(memdc,xmetric*i,ymetric*(6-j)+ywindowoffset,xmetric*(i+1)-1,ymetric*(6-j-1)+1+ywindowoffset);
			}

			// SelectObject(memdc,blackbrush);
			// Ellipse(memdc,xmetric*i,ymetric*(6-j)+ywindowoffset,xmetric*(i+1),ymetric*(6-j-1)+ywindowoffset);
			// Ellipse(memdc,xmetric*i+1,ymetric*(6-j)-1+ywindowoffset,xmetric*(i+1)-1,ymetric*(6-j-1)+1+ywindowoffset);
		}
	}
	DeleteObject(redbrush);
	DeleteObject(yellowbrush);
	DeleteObject(blackbrush);
	InvalidateRect(hwnd, NULL, FALSE);

	// if(resize)
	//	DeleteObject(hbrush);
}
