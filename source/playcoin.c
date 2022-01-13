#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <3ds.h>

#include "archive.h"

typedef int (*menuent_funcptr)(void);

int menu_300coins();
int menu_10coins();
int menu_0coins();
int menu_gamecoindat2sd();
int menu_sd2gamecoindat();
int setcoins(u8 highByte, u8 lowByte);

int mainmenu_totalentries = 5;
char *mainmenu_entries[5] = {
"Set Play Coins to 300",
"Set Play Coins to 10",
"Set Play Coins to 0",
"Copy gamecoin.dat from extdata to sd",
"Copy gamecoin.dat from sd to extdata"};
menuent_funcptr mainmenu_entryhandlers[5] = {menu_300coins, menu_10coins, menu_0coins, menu_gamecoindat2sd, menu_sd2gamecoindat};

u8 *filebuffer;
u32 filebuffer_maxsize = 0x400000;

int draw_menu(char **menu_entries, int total_menuentries, int x, int y)
{
	int i;
	int cursor = 0;
	int update_menu = 1;
	int entermenu = 0;

	y += 1;
	
	while(aptMainLoop())
	{
		gspWaitForVBlank();
		hidScanInput();

		u32 kDown = hidKeysDown();

		if(kDown & KEY_A)
		{
			entermenu = 1;
			break;
		}
		if(kDown & KEY_B)return -1;

		if(kDown & KEY_UP)
		{
			update_menu = 1;
			cursor--;
			if(cursor<0)cursor = total_menuentries-1;
		}

		if(kDown & KEY_DOWN)
		{
			update_menu = 1;
			cursor++;
			if(cursor>=total_menuentries)cursor = 0;
		}

		if(update_menu)
		{
			for(i=0; i<total_menuentries; i++)
			{
				if(cursor!=i)printf("\x1b[%d;%dH   %s", y+i, x, menu_entries[i]);
				if(cursor==i)printf("\x1b[%d;%dH-> %s", y+i, x, menu_entries[i]);
			}

			gfxFlushBuffers();
			gfxSwapBuffers();
		}
	}

	if(!entermenu)return -2;
	return cursor;
}

int menu_300coins()
{
	return setcoins(0x01, 0x2C);
}

int menu_10coins()
{
	return setcoins(0x00, 0x0A);
}

int menu_0coins()
{
	return setcoins(0x00, 0x00);
}

int menu_gamecoindat2sd()
{
	Result ret=0;
	char filepath[256];

	memset(filebuffer, 0, filebuffer_maxsize);

	memset(filepath, 0, 256);
	strncpy(filepath, "gamecoin.dat", 255);

	ret = archive_copyfile(GameCoin_Extdata, SDArchive, "/gamecoin.dat", filepath, filebuffer, 0x14, filebuffer_maxsize, "gamecoin.dat");

	if(ret==0)
	{
		printf("Successfully finished.\n");
		gfxFlushBuffers();
		gfxSwapBuffers();
	}

	return 0;
}

int menu_sd2gamecoindat()
{
	Result ret=0;
	char filepath[256];

	memset(filebuffer, 0, filebuffer_maxsize);

	memset(filepath, 0, 256);
	strncpy(filepath, "gamecoin.dat", 255);

	ret = archive_copyfile(SDArchive, GameCoin_Extdata, filepath, "/gamecoin.dat", filebuffer, 0x14, filebuffer_maxsize, "gamecoin.dat");

	if(ret==0)
	{
		printf("Successfully finished.\n");
		gfxFlushBuffers();
		gfxSwapBuffers();
	}

	return 0;
}

int setcoins(u8 highByte, u8 lowByte)
{	
	Result ret=0;

	printf("Reading gamecoin.dat...\n");
	gfxFlushBuffers();
	gfxSwapBuffers();

	ret = archive_readfile(GameCoin_Extdata, "/gamecoin.dat", filebuffer, 0x14);
	if(ret!=0)
	{
		printf("Failed to read file: 0x%08x\n", (unsigned int)ret);
		gfxFlushBuffers();
		gfxSwapBuffers();
		return 0;
	}

	filebuffer[0x4]=lowByte;
	filebuffer[0x5]=highByte;

	printf("Writing updated gamecoin.dat...\n");
	gfxFlushBuffers();
	gfxSwapBuffers();

	ret = archive_writefile(GameCoin_Extdata, "/gamecoin.dat", filebuffer, 0x14);
	if(ret!=0)
	{
		printf("Failed to write file: 0x%08x\n", (unsigned int)ret);
		gfxFlushBuffers();
		gfxSwapBuffers();
	}

	return 0;
}

int handle_menus()
{
	int ret;

	gfxFlushBuffers();
	gfxSwapBuffers();

	while(aptMainLoop())
	{
		consoleClear();

		ret = draw_menu(mainmenu_entries, mainmenu_totalentries, 0, 0);
		consoleClear();

		if(ret<0)return ret;

		ret = mainmenu_entryhandlers[ret]();
		if(ret==-2)return ret;

		svcSleepThread(5000000000LL);
	}

	return -2;
}

int main()
{
	Result ret = 0;

	// Initialize services
	gfxInitDefault();

	consoleInit(GFX_BOTTOM, NULL);

	printf("Play Coin Setter 3DSx\n");
	gfxFlushBuffers();
	gfxSwapBuffers();

	filebuffer = (u8*)malloc(0x400000);
	if(filebuffer==NULL)
	{
		printf("Failed to allocate memory.\n");
		gfxFlushBuffers();
		gfxSwapBuffers();
		ret = -1;
	}
	else
	{
		memset(filebuffer, 0, filebuffer_maxsize);
	}

	if(ret>=0)
	{
		printf("Opening extdata archives...\n");
		gfxFlushBuffers();
		gfxSwapBuffers();
		ret = open_extdata();
		if(ret==0)
		{
			printf("Finished opening extdata.\n");
			gfxFlushBuffers();
			gfxSwapBuffers();

			consoleClear();
			handle_menus();
		}
	}

	if(ret<0)
	{
		printf("Press the START button to exit.\n");
		// Main loop
		while (aptMainLoop())
		{
			gspWaitForVBlank();
			hidScanInput();

			u32 kDown = hidKeysDown();
			if (kDown & KEY_START)
				break; // break in order to return to hbmenu

			// Flush and swap framebuffers
			gfxFlushBuffers();
			gfxSwapBuffers();
		}
	}

	free(filebuffer);
	close_extdata();

	// Exit services
	gfxExit();
	return 0;
}

