/* adapted for xtreme games march 2000 */
/* called four in a row now */
// now with
// toolbar
// menu accelerators
// tutorial
// connection to internet-page
// randomize
// more levels
// load and save game

// 99/1/19 fixed bug in load and save game
// 99/1/19 added support for drag&drop

//-------------------------------------------------------
// august 2005
// adapted 4 in a row to play with the bitboard engine
// changed a few thins in the interface too which makes it nicer (i hope)
// -> window is resized better than it used to be, pieces remain more circular
// -> new toolbar style (flat)
// -> new toolbar buttons (default windows buttons, new help and homepage button, 2player button)
// -> new much weaker beginner level
// -> new randomize option

#include "c4menu.h"
#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <commctrl.h>
#include <stdlib.h>
#include <shellapi.h>
#include <time.h>
#include <mmsystem.h>
#include "4inarow.h"

#define DELTA 90
#define NUMPARTS 1
#define NUMBUTTONS 11
/* function prototypes */

LRESULT CALLBACK WindowFunc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK DialogFunc(HWND, UINT, WPARAM, LPARAM);

void newgame(HWND, HDC, HDC, HWND);
void printboard(HWND, HDC, HDC, int);
int putmove(HWND, HDC, HDC, int, int);
void setlevel(int);
int getcomputermove(HWND, HDC, HWND, int, int *play);
void InitStatus(HWND hwnd);
void InitToolbar();
void takeback(int move);
void forward(int move, int color);
HWND CreateAToolBar(HWND hwndParent);

DWORD ThreadFunc(LPVOID param);

enum levels
{
	BEGINNER,
	INTERMEDIATE,
	EXPERT,
	MASTER,
	INFLEVEL,
	GAMETHEORETIC,
	SEARCHWIN
} level;
int leveltime[7] = {0, 8, 30, 300, 8600000, 86000000, 86000000};
/***********************/

/* global variables */
int error;
long maxX, maxY, X, Y;
HDC memdc, bmpdc;
HBRUSH hbrush;
HBITMAP hbit;
HMENU hmenu;
LPSIZE windowsize;
char szWinName[] = "MyWin"; /* name of window class */
int computer = -1;
int player = 1;
int b;
HWND hMainWnd, hStatusWnd;
TBBUTTON tbButtons[NUMBUTTONS];
HWND tbwnd;
HINSTANCE hInst;
int parts[NUMPARTS];
int flag = 0;
int xClientView, yClientView;
int game[42];
int count = 0;
int maxcount = 0;
int final = 0;

// settings passed to engine
int usethebook = 1;
int randomize = 0;
int searchwin = 0;
int gametheoretic = 0;
int evaltype = EVAL_SMART;
//----------------------------

int twoplayer = 0;
int ywindowoffset = 30;
DWORD ThreadId;
HANDLE hThread;
int calculating;
// extern int board[7][6];
int playnow = 0;
int toolbar = 1;
char out[255];
int playnowcopy = 0;
/********************/

int exitroutine2(int move)
{
	int dummy;
	RECT r;
	extern int originalfilling[7];

	EnableMenuItem(hmenu, IDM_NEWGAME, MF_ENABLED);

	if (gametheoretic)
	{
		calculating = 0;
		return 0;
	}
	dummy = putmove(hMainWnd, memdc, bmpdc, move, computer);
	game[count] = move;
	count++;
	maxcount = count;
	if (dummy == 1)
	{
		MessageBox(hMainWnd, "I win", "game over", MB_OK);
		newgame(hMainWnd, memdc, bmpdc, hStatusWnd);
		count = 0;
		computer = -1;
		player = 1;
		maxcount = 0;
	}
	if (dummy == 2)
	{
		MessageBox(hMainWnd, "draw", "game over", MB_OK);
		newgame(hMainWnd, memdc, bmpdc, hStatusWnd);
		computer = -1;
		player = 1;
		count = 0;
		maxcount = 0;
	}
	printboard(hMainWnd, memdc, bmpdc, 0);
	GetClientRect(hMainWnd, &r);
	r.left = r.right / 7 * move;
	r.right = r.right / 7 * (move + 1);
	r.top = r.bottom / 6 * (5 - originalfilling[move]);
	r.bottom = r.bottom / 6 * (7 - originalfilling[move]);
	// InvalidateRect(hMainWnd,&r,0);
	InvalidateRect(hMainWnd, NULL, 0);
	calculating = 0;
	return 0;
}

DWORD ThreadFunc(LPVOID param)
{
	int move;
	EnableMenuItem(hmenu, IDM_NEWGAME, MF_GRAYED);
	// this is ridiculous - the routine getcomputermove doesn't need
	// to know about window handles (or shouldnt...)
	move = getcomputermove(hMainWnd, memdc, hStatusWnd, computer, &playnowcopy);
	if (gametheoretic)
	{
		computer = -computer;
		player = -player;
	}
	exitroutine2(move);
	return 0;
}

BOOL CALLBACK DialogFunc(HWND hdwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		return 1;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_OK:
			EndDialog(hdwnd, 0);
			return 1;
		}
		break;
	}
	return 0;
}

int WINAPI WinMain(HINSTANCE hThisInst, HINSTANCE hPrevInst, LPSTR lpszArgs, int nWinMode)
{

	MSG msg;
	WNDCLASSEX wcl;
	HACCEL hAccel;
	// struct tagLPINITCOMMONCONTROLSEX cctrl;
	/* get screen coordinates */

	maxX = GetSystemMetrics(SM_CXSCREEN);
	maxY = GetSystemMetrics(SM_CYSCREEN);

	/* Define a window class. */

	wcl.hInstance = hThisInst;	   /* handle to this instance */
	wcl.lpszClassName = szWinName; /* window class name */
	wcl.lpfnWndProc = WindowFunc;  /* window function */
	wcl.style = 0;				   /* default style */
	wcl.cbSize = sizeof(WNDCLASSEX);
	wcl.hIcon = LoadIcon(hThisInst, "ICON_2");	/* icon style */
	wcl.hIconSm = LoadIcon(hThisInst, "ICON3"); /*small icon*/
	wcl.hCursor = LoadCursor(NULL, IDC_ARROW);	/* cursor style */
	wcl.lpszMenuName = "Connect4";				/* menu is connect4*/
	wcl.cbClsExtra = 0;							/* no extra */
	wcl.cbWndExtra = 0;							/* information needed */

	// make the window background the system color
	wcl.hbrBackground = (HBRUSH)GetSysColorBrush(GetSysColor(COLOR_MENU));

	/* register the window class */
	if (!RegisterClassEx(&wcl))
		return 0;
	/* Now that a window class has been registered, a window can be created */

	hMainWnd = CreateWindow(
		szWinName,				  /* name of window class */
		"4 in a row for Windows", /* title */
		WS_OVERLAPPEDWINDOW,	  /* window style */
		(maxX - 352) / 2,		  /* x coordinate - let windows decide */
		(maxY - 362) / 2,		  /* y coordinate - let windows decide */
		352,					  /* width  */
		362 + ywindowoffset,	  /* height: 362 for small fonts, 377 for large fonts */
		HWND_DESKTOP,			  /* no parent window */
		NULL,					  /* no menu */
		hThisInst,				  /* handle of this instance of the program */
		NULL					  /* no additional arguments */
	);

	/* save the current instance for later purposes */
	hInst = hThisInst;
	/* load the accelerator table */
	hAccel = LoadAccelerators(hThisInst, "CONNECT4");

	/* initialize common controls */
	InitCommonControls();
	/* Initialize the status bar */
	InitStatus(hMainWnd);
	/* Initialize the Toolbar */
	tbwnd = CreateAToolBar(hMainWnd);
	/* display the window */

	ShowWindow(hMainWnd, SW_SHOW);
	UpdateWindow(hMainWnd);

	/* create a timer */
	SetTimer(hMainWnd, 1, 100, NULL);

	/* create the message loop */
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(hMainWnd, hAccel, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	KillTimer(hMainWnd, 1);
	return msg.wParam;
}

/* this function is called by windows and is passed
messages from the message queue. */

char str[80] = ""; /* holds the output string */

LRESULT CALLBACK WindowFunc(HWND hwnd, UINT message,
							WPARAM wParam, LPARAM lParam)
{
	HDC hdc;
	PAINTSTRUCT paintstruct;
	int mousex, mousey, move, dummy;
	int i;
	static char outstring[255];
	LPTOOLTIPTEXT TTtext;
	FILE *fp;
	LPRECT lprec;
	OPENFILENAME of;
	char filename[255] = "";

	switch (message)
	{
	case WM_CREATE:
		srand(1);
		hdc = GetDC(hwnd);
		// sprintf(out,"sizeof struct composition %i", sizeof(struct composition));
		/* make  compatible memory image device */

		memdc = CreateCompatibleDC(hdc);
		bmpdc = CreateCompatibleDC(hdc);
		hbit = CreateCompatibleBitmap(hdc, maxX, maxY);
		SelectObject(memdc, hbit);
		hbrush = GetStockObject(DKGRAY_BRUSH);
		SelectObject(memdc, hbrush);
		PatBlt(memdc, 0, 0, maxX, maxY, PATCOPY);
		ReleaseDC(hwnd, hdc);

		/*****************************************/
		DragAcceptFiles(hwnd, 1);

		hmenu = GetMenu(hwnd);
		// newgame(hwnd,memdc,hStatusWnd);
		PostMessage(hwnd, WM_COMMAND, IDM_NEWGAME, 0);

		// DEFAULT SETTINGS
		usethebook = 1;
		level = INTERMEDIATE;

		/* read settings */

		fp = fopen("c4.ini", "r");

		if (fp != NULL)
		{
			fscanf(fp, "%i %i %i", &level, &usethebook, &evaltype);
		}

		if (usethebook)
			CheckMenuItem(hmenu, IDM_BOOK, MF_CHECKED);
		else
			CheckMenuItem(hmenu, IDM_BOOK, MF_UNCHECKED);
		CheckMenuItem(hmenu, IDM_BEGINNER, MF_UNCHECKED);
		CheckMenuItem(hmenu, IDM_INTERMEDIATE, MF_UNCHECKED);
		CheckMenuItem(hmenu, IDM_EXPERT, MF_UNCHECKED);
		CheckMenuItem(hmenu, IDM_MASTER, MF_UNCHECKED);
		CheckMenuItem(hmenu, IDM_INFINITE, MF_UNCHECKED);
		switch (level)
		{
		case BEGINNER:
			PostMessage(hwnd, WM_COMMAND, IDM_BEGINNER, 0);
			CheckMenuItem(hmenu, IDM_BEGINNER, MF_CHECKED);
			break;
		case INTERMEDIATE:
			PostMessage(hwnd, WM_COMMAND, IDM_INTERMEDIATE, 0);
			CheckMenuItem(hmenu, IDM_INTERMEDIATE, MF_CHECKED);
			break;
		case EXPERT:
			PostMessage(hwnd, WM_COMMAND, IDM_EXPERT, 0);
			CheckMenuItem(hmenu, IDM_EXPERT, MF_CHECKED);
			break;
		case MASTER:
			PostMessage(hwnd, WM_COMMAND, IDM_MASTER, 0);
			CheckMenuItem(hmenu, IDM_MASTER, MF_CHECKED);
			break;
		case INFLEVEL:
			PostMessage(hwnd, WM_COMMAND, IDM_INFINITE, 0);
			CheckMenuItem(hmenu, IDM_INFINITE, MF_CHECKED);
			break;
		case SEARCHWIN:
			PostMessage(hwnd, WM_COMMAND, IDM_SEARCHWIN, 0);
			CheckMenuItem(hmenu, IDM_SEARCHWIN, MF_CHECKED);
			break;
		case GAMETHEORETIC:
			PostMessage(hwnd, WM_COMMAND, IDM_GAMETHEORETIC, 0);
			CheckMenuItem(hmenu, IDM_GAMETHEORETIC, MF_CHECKED);
			break;
		}

		CheckMenuItem(hmenu, IDM_SMART, MF_UNCHECKED);
		CheckMenuItem(hmenu, IDM_AVERAGE, MF_UNCHECKED);
		CheckMenuItem(hmenu, IDM_DUMB, MF_UNCHECKED);
		switch (evaltype)
		{
		case EVAL_SMART:
			PostMessage(hwnd, WM_COMMAND, IDM_SMART, 0);
			CheckMenuItem(hmenu, IDM_SMART, MF_CHECKED);
			break;
		case EVAL_AVERAGE:
			PostMessage(hwnd, WM_COMMAND, IDM_AVERAGE, 0);
			CheckMenuItem(hmenu, IDM_AVERAGE, MF_CHECKED);
			break;
		case EVAL_DUMB:
			PostMessage(hwnd, WM_COMMAND, IDM_DUMB, 0);
			CheckMenuItem(hmenu, IDM_DUMB, MF_CHECKED);
			break;
		}

		PostMessage(hwnd, WM_COMMAND, ID_RANDOMIZE_0, 0);

		// load bitmaps from the "bmp" directory
		initbmp(hwnd, "bmp");

		break;

	case WM_DROPFILES:
		DragQueryFile((HDROP)wParam, 0, filename, sizeof(filename));
		DragFinish((HDROP)wParam);
		newgame(hwnd, memdc, bmpdc, hStatusWnd);
		fp = fopen(filename, "r");
		for (i = 0; i < 42; i++)
		{
			fscanf(fp, "%i", &game[i]);
		}
		maxcount = 41;
		for (i = 41; i >= 0; i--)
		{
			if (game[i] == -1)
				maxcount = i;
		}
		fclose(fp);
		break;

	case WM_TIMER:
		if (strcmp(out, outstring))
		{
			sprintf(outstring, "%s", out);
			SendMessage(hStatusWnd, SB_SETTEXT, (WPARAM)0, (LPARAM)out);
		}
		break;

	case WM_NOTIFY: /*respond to tooltip request */
		TTtext = (LPTOOLTIPTEXT)lParam;
		if (TTtext->hdr.code == TTN_NEEDTEXT)
			switch (TTtext->hdr.idFrom)
			{
			case IDM_NEWGAME:
				TTtext->lpszText = "New Game";
				break;
			case IDM_BACK:
				TTtext->lpszText = "Take Back";
				break;
			case IDM_FORWARD:
				TTtext->lpszText = "Forward";
				break;
			case IDM_PLAY:
				TTtext->lpszText = "Play";
				break;
			case IDM_HELP:
				TTtext->lpszText = "Help";
				break;
			case IDM_LOADGAME:
				TTtext->lpszText = "Load Game";
				break;
			case IDM_SAVEGAME:
				TTtext->lpszText = "Save Game";
				break;
			case IDM_HELP_HOMEPAGE:
				TTtext->lpszText = "4 in a row homepage";
				break;
			case IDM_TWOPLAYER:
				TTtext->lpszText = "Toggle single/two player mode";
			}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDM_NEWGAME:
			newgame(hwnd, memdc, bmpdc, hStatusWnd);
			computer = -1;
			player = 1;
			count = 0;
			maxcount = 0;
			for (i = 0; i < 42; i++)
				game[i] = -1;
			break;

		case IDM_QUIT:
			PostQuitMessage(0);
			break;

		case IDM_SAVEGAME:
			of.lStructSize = sizeof(OPENFILENAME);
			of.hwndOwner = NULL;
			of.hInstance = hInst;
			of.lpstrFilter = "four in a row games *.c4\0 *.c4\0 all files *.*\0 *.*\0";
			of.lpstrCustomFilter = NULL;
			of.nMaxCustFilter = 0;
			of.nFilterIndex = 0;
			of.lpstrFile = filename;
			of.nMaxFile = MAX_PATH;
			of.lpstrFileTitle = NULL;
			of.nMaxFileTitle = 0;
			of.lpstrInitialDir = NULL;
			of.lpstrTitle = NULL;
			of.Flags = OFN_HIDEREADONLY;
			of.nFileOffset = 0;
			of.nFileExtension = 0;
			of.lpstrDefExt = NULL;
			of.lCustData = 0;
			of.lpfnHook = NULL;
			of.lpTemplateName = NULL;
			if (GetSaveFileName(&of))
			{
				fp = fopen(filename, "w");
				for (i = 0; i < 42; i++)
				{
					fprintf(fp, "%i ", game[i]);
				}
				fclose(fp);
				// display info in task bar:
				sprintf(out, "Game saved!");
			}
			break;

		case IDM_LOADGAME:
			newgame(hwnd, memdc, bmpdc, hStatusWnd);
			of.lStructSize = sizeof(OPENFILENAME);
			of.hwndOwner = NULL;
			of.hInstance = hInst;
			of.lpstrFilter = "four in a row games *.c4\0 *.c4\000";
			of.lpstrCustomFilter = NULL;
			of.nMaxCustFilter = 0;
			of.nFilterIndex = 0;
			of.lpstrFile = filename;
			of.nMaxFile = MAX_PATH;
			of.lpstrFileTitle = NULL;
			of.nMaxFileTitle = 0;
			of.lpstrInitialDir = NULL;
			of.lpstrTitle = NULL;
			of.Flags = OFN_HIDEREADONLY;
			of.nFileOffset = 0;
			of.nFileExtension = 0;
			of.lpstrDefExt = NULL;
			of.lCustData = 0;
			of.lpfnHook = NULL;
			of.lpTemplateName = NULL;
			if (GetOpenFileName(&of))
			{
				fp = fopen(filename, "r");
				for (i = 0; i < 42; i++)
				{
					fscanf(fp, "%i", &game[i]);
				}
				maxcount = 41;
				for (i = 41; i >= 0; i--)
				{
					if (game[i] == -1)
						maxcount = i;
				}
				fclose(fp);
				// display info in task bar:
				sprintf(out, "Game loaded, use the arrow keys to move through it.");
			}
			break;

		case IDM_BACK:
			if (count > 0)
			{
				computer = -computer;
				player = -player;
				count--;
				takeback(game[count]);
				printboard(hwnd, memdc, bmpdc, 0);
				InvalidateRect(hwnd, NULL, 0);
			}
			break;

		case IDM_FORWARD:
			if (count < maxcount)
			{
				forward(game[count], player);
				computer = -computer;
				player = -player;
				count++;
				printboard(hwnd, memdc, bmpdc, 0);
				InvalidateRect(hwnd, NULL, 0);
			}
			break;

		case IDM_TWOPLAYER:
			if (!twoplayer)
				twoplayer = 1;
			else
				twoplayer = 0;
			if (twoplayer)
			{
				CheckMenuItem(hmenu, IDM_TWOPLAYER, MF_CHECKED);
				SendMessage(tbwnd, TB_CHANGEBITMAP, (WPARAM)IDM_TWOPLAYER, MAKELPARAM(4, 0));
			}
			else
			{
				CheckMenuItem(hmenu, IDM_TWOPLAYER, MF_UNCHECKED);
				SendMessage(tbwnd, TB_CHANGEBITMAP, (WPARAM)IDM_TWOPLAYER, MAKELPARAM(3, 0));
			}
			InvalidateRect(hwnd, NULL, 0);
			break;

		case IDM_RANDOMIZE:
			if (!randomize)
				randomize = 1;
			else
				randomize = 0;
			if (randomize)
			{
				CheckMenuItem(hmenu, IDM_RANDOMIZE, MF_CHECKED);
			}
			else
			{
				CheckMenuItem(hmenu, IDM_RANDOMIZE, MF_UNCHECKED);
			}

		case IDM_BOOK:
			if (usethebook)
				usethebook = 0;
			else
				usethebook = 1;
			if (usethebook)
				CheckMenuItem(hmenu, IDM_BOOK, MF_CHECKED);
			else
				CheckMenuItem(hmenu, IDM_BOOK, MF_UNCHECKED);
			break;

		case IDM_BEGINNER:
			level = BEGINNER;
			setlevel(leveltime[BEGINNER]);
			searchwin = 0;
			gametheoretic = 0;
			CheckMenuItem(hmenu, IDM_BEGINNER, MF_CHECKED);
			CheckMenuItem(hmenu, IDM_INTERMEDIATE, MF_UNCHECKED);
			CheckMenuItem(hmenu, IDM_EXPERT, MF_UNCHECKED);
			CheckMenuItem(hmenu, IDM_MASTER, MF_UNCHECKED);
			CheckMenuItem(hmenu, IDM_INFINITE, MF_UNCHECKED);
			CheckMenuItem(hmenu, IDM_SEARCHWIN, MF_UNCHECKED);
			CheckMenuItem(hmenu, IDM_GAMETHEORETIC, MF_UNCHECKED);
			break;

		case IDM_INTERMEDIATE:
			level = INTERMEDIATE;
			setlevel(leveltime[INTERMEDIATE]);
			gametheoretic = 0;
			searchwin = 0;
			CheckMenuItem(hmenu, IDM_BEGINNER, MF_UNCHECKED);
			CheckMenuItem(hmenu, IDM_INTERMEDIATE, MF_CHECKED);
			CheckMenuItem(hmenu, IDM_EXPERT, MF_UNCHECKED);
			CheckMenuItem(hmenu, IDM_MASTER, MF_UNCHECKED);
			CheckMenuItem(hmenu, IDM_INFINITE, MF_UNCHECKED);
			CheckMenuItem(hmenu, IDM_SEARCHWIN, MF_UNCHECKED);
			CheckMenuItem(hmenu, IDM_GAMETHEORETIC, MF_UNCHECKED);
			break;

		case IDM_EXPERT:
			level = EXPERT;
			gametheoretic = 0;
			searchwin = 0;
			setlevel(leveltime[EXPERT]);
			CheckMenuItem(hmenu, IDM_BEGINNER, MF_UNCHECKED);
			CheckMenuItem(hmenu, IDM_INTERMEDIATE, MF_UNCHECKED);
			CheckMenuItem(hmenu, IDM_EXPERT, MF_CHECKED);
			CheckMenuItem(hmenu, IDM_MASTER, MF_UNCHECKED);
			CheckMenuItem(hmenu, IDM_INFINITE, MF_UNCHECKED);
			CheckMenuItem(hmenu, IDM_SEARCHWIN, MF_UNCHECKED);
			CheckMenuItem(hmenu, IDM_GAMETHEORETIC, MF_UNCHECKED);
			break;

		case IDM_MASTER:
			level = MASTER;
			gametheoretic = 0;
			searchwin = 0;
			setlevel(leveltime[MASTER]);
			CheckMenuItem(hmenu, IDM_BEGINNER, MF_UNCHECKED);
			CheckMenuItem(hmenu, IDM_INTERMEDIATE, MF_UNCHECKED);
			CheckMenuItem(hmenu, IDM_EXPERT, MF_UNCHECKED);
			CheckMenuItem(hmenu, IDM_MASTER, MF_CHECKED);
			CheckMenuItem(hmenu, IDM_INFINITE, MF_UNCHECKED);
			CheckMenuItem(hmenu, IDM_SEARCHWIN, MF_UNCHECKED);
			CheckMenuItem(hmenu, IDM_GAMETHEORETIC, MF_UNCHECKED);
			break;

		case IDM_INFINITE:
			level = INFLEVEL;
			gametheoretic = 0;
			searchwin = 0;
			setlevel(leveltime[INFLEVEL]);
			CheckMenuItem(hmenu, IDM_BEGINNER, MF_UNCHECKED);
			CheckMenuItem(hmenu, IDM_INTERMEDIATE, MF_UNCHECKED);
			CheckMenuItem(hmenu, IDM_EXPERT, MF_UNCHECKED);
			CheckMenuItem(hmenu, IDM_MASTER, MF_UNCHECKED);
			CheckMenuItem(hmenu, IDM_INFINITE, MF_CHECKED);
			CheckMenuItem(hmenu, IDM_SEARCHWIN, MF_UNCHECKED);
			CheckMenuItem(hmenu, IDM_GAMETHEORETIC, MF_UNCHECKED);
			break;

		case IDM_SEARCHWIN:
			level = SEARCHWIN;
			setlevel(leveltime[SEARCHWIN]);
			searchwin = 1;
			gametheoretic = 0;
			CheckMenuItem(hmenu, IDM_BEGINNER, MF_UNCHECKED);
			CheckMenuItem(hmenu, IDM_INTERMEDIATE, MF_UNCHECKED);
			CheckMenuItem(hmenu, IDM_EXPERT, MF_UNCHECKED);
			CheckMenuItem(hmenu, IDM_MASTER, MF_UNCHECKED);
			CheckMenuItem(hmenu, IDM_INFINITE, MF_UNCHECKED);
			CheckMenuItem(hmenu, IDM_SEARCHWIN, MF_CHECKED);
			CheckMenuItem(hmenu, IDM_GAMETHEORETIC, MF_UNCHECKED);
			break;

		case IDM_GAMETHEORETIC:
			level = GAMETHEORETIC;
			gametheoretic = 1;
			searchwin = 0;
			setlevel(leveltime[GAMETHEORETIC]);
			CheckMenuItem(hmenu, IDM_BEGINNER, MF_UNCHECKED);
			CheckMenuItem(hmenu, IDM_INTERMEDIATE, MF_UNCHECKED);
			CheckMenuItem(hmenu, IDM_EXPERT, MF_UNCHECKED);
			CheckMenuItem(hmenu, IDM_MASTER, MF_UNCHECKED);
			CheckMenuItem(hmenu, IDM_INFINITE, MF_UNCHECKED);
			CheckMenuItem(hmenu, IDM_SEARCHWIN, MF_UNCHECKED);
			CheckMenuItem(hmenu, IDM_GAMETHEORETIC, MF_CHECKED);
			break;

		case IDM_SMART:
			evaltype = EVAL_SMART;
			CheckMenuItem(hmenu, IDM_SMART, MF_CHECKED);
			CheckMenuItem(hmenu, IDM_AVERAGE, MF_UNCHECKED);
			CheckMenuItem(hmenu, IDM_DUMB, MF_UNCHECKED);
			break;

		case IDM_AVERAGE:
			evaltype = EVAL_AVERAGE;
			CheckMenuItem(hmenu, IDM_SMART, MF_UNCHECKED);
			CheckMenuItem(hmenu, IDM_AVERAGE, MF_CHECKED);
			CheckMenuItem(hmenu, IDM_DUMB, MF_UNCHECKED);
			break;

		case IDM_DUMB:
			evaltype = EVAL_DUMB;
			CheckMenuItem(hmenu, IDM_SMART, MF_UNCHECKED);
			CheckMenuItem(hmenu, IDM_AVERAGE, MF_UNCHECKED);
			CheckMenuItem(hmenu, IDM_DUMB, MF_CHECKED);
			break;

		case IDM_PLAY:
			if (!calculating)
			{
				computer = -computer;
				player = -player;
				calculating = 1;
				playnow = 0;
				playnowcopy = 0;
				hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ThreadFunc, (LPVOID)0, 0, &ThreadId);
				SetThreadPriority(hThread, THREAD_PRIORITY_BELOW_NORMAL); /* next lower ist '_LOWEST', higher '_NORMAL' */
			}
			else
			{
				playnowcopy = 1;
				// playnow=1;
			}
			break;

		case IDM_HELP:
			error = (int)ShellExecute(NULL, "open", "help.htm", NULL, NULL, SW_SHOW);
			sprintf(str, "4 in a row could not open\nthe file help.htm\nError code %i", error);
			if (error < 32)
			{
				if (error == ERROR_FILE_NOT_FOUND)
					strcat(str, ": File not found");
				if (error == SE_ERR_NOASSOC)
					strcat(str, ": no .htm viewer configured");
				MessageBox(hwnd, str, "Error !", MB_OK);
			}
			break;

		case IDM_HELP_HOMEPAGE:
			error = (int)ShellExecute(NULL, "open", "http://www.fierz.ch/4inarow.htm", NULL, NULL, SW_SHOW);
			sprintf(str, "4 in a row could not open the website \nhttp://www.fierz.ch/4inarow.htm\nError code %i", error);
			if (error < 32)
			{
				if (error == ERROR_FILE_NOT_FOUND)
					strcat(str, ": File not found");
				if (error == SE_ERR_NOASSOC)
					strcat(str, ": no .htm viewer configured");
				MessageBox(hwnd, str, "Error !", MB_OK);
			}
			break;

		case IDM_HELP_TUTORIAL:
			error = (int)ShellExecute(NULL, "open", "4tutorial.htm", NULL, NULL, SW_SHOW);
			sprintf(str, "4 in a row could not open\nthe file 4tutorial.htm\nError code %i", error);
			if (error < 32)
			{
				if (error == ERROR_FILE_NOT_FOUND)
					strcat(str, ": File not found");
				if (error == SE_ERR_NOASSOC)
					strcat(str, ": no .htm viewer configured");
				MessageBox(hwnd, str, "Error !", MB_OK);
			}
			break;

		case IDM_ABOUT:
			// MessageBox(hwnd, "4 in a row for Windows - v1.2, \n2001 by Martin Fierz\n\nThanks to Andri Schaufelb�hl and Bernhard Seybold.","About", MB_OK);
			if (DialogBox(hInst, "C4ABOUT", hwnd, DialogFunc) == -1)
			{
				sprintf(str, "error code %i", GetLastError());
				MessageBox(hwnd, str, "Error", MB_OK);
			}
			break;
		}

	case WM_LBUTTONDOWN:
		mousey = HIWORD(lParam);
		if (mousey > 0 && !calculating)
		{
			/* get the player's move  */
			if (!flag)
			{
				mousex = LOWORD(lParam);
				move = (mousex * 7) / xClientView;
				if ((move >= 0) && (move <= 6))
				{
					dummy = putmove(hwnd, memdc, bmpdc, move, player);
					game[count] = move;
					count++;
					maxcount = count;
					if (dummy == 1)
					{
						if (twoplayer)
							MessageBox(hwnd, "game over", "game over!", MB_OK);
						else
							MessageBox(hwnd, "I lose", "game over", MB_OK);
						newgame(hwnd, memdc, bmpdc, hStatusWnd);
						computer = -1;
						player = 1;
						count = 0;
						maxcount = 0;
						PostMessage(hwnd, IDM_NEWGAME, 0, 0);
						break;
					}
					if (dummy == 2)
					{
						MessageBox(hwnd, "draw", "game over", MB_OK);
						newgame(hwnd, memdc, bmpdc, hStatusWnd);
						computer = -1;
						player = 1;
						count = 0;
						maxcount = 0;
						PostMessage(hwnd, IDM_NEWGAME, 0, 0);
						break;
					}
					if (dummy != -1)
					{
						computer = -computer;
						player = -player;
						if (!twoplayer)
							PostMessage(hMainWnd, WM_COMMAND, IDM_PLAY, 0);
					}
				}
			}
		}
		InvalidateRect(hwnd, NULL, 1);
		break;

	case WM_SIZE:
		xClientView = LOWORD(lParam);
		yClientView = HIWORD(lParam) - ywindowoffset;
		for (i = 1; i <= NUMPARTS; i++)
			parts[i - 1] = xClientView / NUMPARTS * i;
		SendMessage(hStatusWnd, SB_SETPARTS, (WPARAM)NUMPARTS, (LPARAM)parts);
		SendMessage(hStatusWnd, WM_SIZE, wParam, lParam);
		SendMessage(tbwnd, WM_SIZE, wParam, lParam);

		printboard(hwnd, memdc, bmpdc, 1);
		InvalidateRect(hwnd, NULL, 0);
		break;

	case WM_SIZING: /* keep window quadratic */
#define DELTA 90
		lprec = (LPRECT)lParam;
		(*lprec).right = ((*lprec).bottom - (*lprec).top - DELTA) * 7 / 6 + (*lprec).left;
		break;

	case WM_PAINT:
		hdc = BeginPaint(hwnd, &paintstruct);
		BitBlt(hdc, 0, 0, maxX, maxY, memdc, 0, 0, SRCCOPY);
		EndPaint(hwnd, &paintstruct);
		break;

	case WM_DESTROY: /* terminate the program */
		DeleteDC(memdc);
		sndPlaySound(NULL, 0);
		/* write settings to disk */
		fp = fopen("c4.ini", "w");
		fprintf(fp, "%i %i %i", level, usethebook, evaltype);
		fclose(fp);
		PostQuitMessage(0);
		break;

	default:
		/* Let Windows 95 process any messages not specified
		in the preceding switch statement */
		return DefWindowProc(hwnd, message, wParam, lParam);
	}
	return 0;
}

// CreateAToolBar creates a toolbar and adds a set of buttons to it.
// The function returns the handle to the toolbar if successful,
// or NULL otherwise.
// hwndParent is the handle to the toolbar's parent window.
HWND CreateAToolBar(HWND hwndParent)
{
	HWND hwndTB;
	TBADDBITMAP tbab;
	INITCOMMONCONTROLSEX icex;
	int i;
	int id[NUMBUTTONS] = {NUMBUTTONS + STD_FILENEW, NUMBUTTONS + STD_FILESAVE, NUMBUTTONS + STD_FILEOPEN,
						  0,
						  NUMBUTTONS + STD_UNDO, NUMBUTTONS + STD_REDOW, 2, 3,
						  0,
						  6, 1};
	int command[NUMBUTTONS] = {IDM_NEWGAME, IDM_SAVEGAME, IDM_LOADGAME,
							   0,
							   IDM_BACK, IDM_FORWARD, IDM_PLAY, IDM_TWOPLAYER,
							   0,
							   IDM_HELP, IDM_HELP_HOMEPAGE};

	int style[NUMBUTTONS] = {TBSTYLE_BUTTON, TBSTYLE_BUTTON, TBSTYLE_BUTTON,
							 TBSTYLE_SEP,
							 TBSTYLE_BUTTON, TBSTYLE_BUTTON, TBSTYLE_BUTTON, TBSTYLE_BUTTON,
							 TBSTYLE_SEP,
							 TBSTYLE_BUTTON, TBSTYLE_BUTTON};

	// Ensure that the common control DLL is loaded.
	icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icex.dwICC = ICC_BAR_CLASSES;
	InitCommonControlsEx(&icex);

	// Create a toolbar.
	hwndTB = CreateWindowEx(0, TOOLBARCLASSNAME, (LPSTR)NULL,
							WS_CHILD | WS_BORDER | WS_VISIBLE | TBSTYLE_TOOLTIPS | CCS_ADJUSTABLE | TBSTYLE_FLAT,
							0, 0, 0, 0, hwndParent,
							(HMENU)ID_TOOLBAR, hInst, NULL);

	// Send the TB_BUTTONSTRUCTSIZE message, which is required for
	// backward compatibility.
	SendMessage(hwndTB, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);

	// Fill the TBBUTTON array with button information, and add the
	// buttons to the toolbar. The buttons on this toolbar have text
	// but do not have bitmap images.
	for (i = 0; i < NUMBUTTONS; i++)
	{
		tbButtons[i].dwData = 0L;
		tbButtons[i].fsState = TBSTATE_ENABLED;
		tbButtons[i].fsStyle = style[i];
		tbButtons[i].iBitmap = id[i];
		tbButtons[i].idCommand = command[i];
		tbButtons[i].iString = 0; //"text";
	}

	// here's how toolbars work:
	// first, add bitmaps to the toolbar. the toolbar keeps a list of bitmaps.
	// then, add buttons to the toolbar, specifying the index of the bitmap
	// you want to use.
	// in this case, i first add custom bitmaps, NUMBUTTONS of them,
	// then i add the standard windows bitmaps.
	// so instead of using the index STD_FIND for the bitmap to find things,
	// i need to add NUMBUTTONS to that so that it works out!

	// add custom bitmaps
	tbab.hInst = hInst;
	tbab.nID = 102;
	SendMessage(hwndTB, TB_ADDBITMAP, NUMBUTTONS, (LPARAM)&tbab);

	// add default bitmaps
	tbab.hInst = HINST_COMMCTRL;
	tbab.nID = IDB_STD_SMALL_COLOR;
	// tbab.nID = IDB_VIEW_LARGE_COLOR;
	SendMessage(hwndTB, TB_ADDBITMAP, 0, (LPARAM)&tbab);

	// add buttons
	SendMessage(hwndTB, TB_ADDBUTTONS, (WPARAM)NUMBUTTONS, (LPARAM)(LPTBBUTTON)&tbButtons);

	// finally, resize
	SendMessage(hwndTB, TB_AUTOSIZE, 0, 0);

	ShowWindow(hwndTB, SW_SHOW);
	return hwndTB;
}

/* create a status bar */
void InitStatus(HWND hwnd)
{
	RECT WinDim;
	int i;

	GetClientRect(hwnd, &WinDim);

	for (i = 1; i <= NUMPARTS; i++)
		parts[i - 1] = WinDim.right / NUMPARTS * i;

	hStatusWnd = CreateWindow(STATUSCLASSNAME, "", WS_CHILD | WS_VISIBLE,
							  0, 0, 0, 0, hwnd, NULL, hInst, NULL);

	SendMessage(hStatusWnd, SB_SETPARTS, (WPARAM)NUMPARTS, (LPARAM)parts);
}
