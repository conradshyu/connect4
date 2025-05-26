#define BMPBACKGROUND 0
#define BMPPEG 1
#define BMPPEGMASK 2
#define BMPYELLOWSTONE 3
#define BMPREDSTONE 4
#define BMPSTONEMASK 5

#define BMPSIZE 100

int initbmp(HWND hwnd, char dir[256]);
HBITMAP getBitmap(int type);
