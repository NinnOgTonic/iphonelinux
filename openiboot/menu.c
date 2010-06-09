#ifndef SMALL
#ifndef NO_STBIMAGE

#include "openiboot.h"
#include "lcd.h"
#include "util.h"
#include "framebuffer.h"
#include "buttons.h"
#include "timer.h"
#include "images/ConsolePNG.h"
#include "images/iPhoneOSPNG.h"
#include "images/AndroidOSPNG.h"
#include "images/AndroidOS2PNG.h"
#include "images/ConsoleSelectedPNG.h"
#include "images/iPhoneOSSelectedPNG.h"
#include "images/AndroidOSSelectedPNG.h"
#include "images/AndroidOS2SelectedPNG.h"
#include "images/HeaderPNG.h"
#include "images.h"
#include "actions.h"
#include "stb_image.h"
#include "pmu.h"
#include "nand.h"
#include "radio.h"
#include "hfs/fs.h"
#include "ftl.h"
#include "scripting.h"
int globalFtlHasBeenRestored = 0; /* global variable to tell wether a ftl_restore has been done*/

static uint32_t FBWidth;
static uint32_t FBHeight;



// iPhone OS
static uint32_t* imgiPhoneOS;
static int imgiPhoneOSWidth;
static int imgiPhoneOSHeight;
static int imgiPhoneOSX;
static int imgiPhoneOSY;

static uint32_t* imgiPhoneOSSelected;
static uint32_t* imgConsoleSelected;
static uint32_t* imgAndroidOSSelected;
// static uint32_t* imgAndroidOSSelected_unblended;
static uint32_t* imgAndroidOS2Selected;
// static uint32_t* imgAndroidOS2Selected_unblended;

// Console
static uint32_t* imgConsole;
static int imgConsoleWidth;
static int imgConsoleHeight;
static int imgConsoleX;
static int imgConsoleY;

// Android 1.6
static uint32_t* imgAndroidOS;
// static uint32_t* imgAndroidOS_unblended;
static int imgAndroidOSWidth;
static int imgAndroidOSHeight;
static int imgAndroidOSX;
static int imgAndroidOSY;

// Android 2.x
static uint32_t* imgAndroidOS2;
// static uint32_t* imgAndroidOS2_unblended;
static int imgAndroidOS2Width;
static int imgAndroidOS2Height;
static int imgAndroidOS2X;
static int imgAndroidOS2Y;


static uint32_t* imgHeader;
static int imgHeaderWidth;
static int imgHeaderHeight;
static int imgHeaderX;
static int imgHeaderY;

typedef enum MenuSelection {
	MenuSelectioniPhoneOS,
	MenuSelectionConsole,
	MenuSelectionAndroidOS,
        MenuSelectionAndroidOS2
} MenuSelection;


static MenuSelection Selection;

volatile uint32_t* OtherFramebuffer;


static void drawSelectionBox() {
	volatile uint32_t* oldFB = CurFramebuffer;

	CurFramebuffer = OtherFramebuffer;
	currentWindow->framebuffer.buffer = CurFramebuffer;
	OtherFramebuffer = oldFB;

	if(Selection == MenuSelectioniPhoneOS) {
		framebuffer_draw_image(imgConsole, imgConsoleX, imgConsoleY, imgConsoleWidth, imgConsoleHeight);
		framebuffer_draw_image(imgAndroidOS, imgAndroidOSX, imgAndroidOSY, imgAndroidOSWidth, imgAndroidOSHeight);
		framebuffer_draw_image(imgAndroidOS2, imgAndroidOS2X, imgAndroidOS2Y, imgAndroidOS2Width, imgAndroidOS2Height);
		framebuffer_draw_image(imgiPhoneOSSelected, imgiPhoneOSX, imgiPhoneOSY, imgiPhoneOSWidth, imgiPhoneOSHeight);
	}

	if(Selection == MenuSelectionConsole) {
		framebuffer_draw_image(imgiPhoneOS, imgiPhoneOSX, imgiPhoneOSY, imgiPhoneOSWidth, imgiPhoneOSHeight);
		framebuffer_draw_image(imgAndroidOS, imgAndroidOSX, imgAndroidOSY, imgAndroidOSWidth, imgAndroidOSHeight);
		framebuffer_draw_image(imgAndroidOS2, imgAndroidOS2X, imgAndroidOS2Y, imgAndroidOS2Width, imgAndroidOS2Height);
		framebuffer_draw_image(imgConsoleSelected, imgConsoleX, imgConsoleY, imgConsoleWidth, imgConsoleHeight);
	}

	if(Selection == MenuSelectionAndroidOS) {
		framebuffer_draw_image(imgConsole, imgConsoleX, imgConsoleY, imgConsoleWidth, imgConsoleHeight);
		framebuffer_draw_image(imgAndroidOS2, imgAndroidOS2X, imgAndroidOS2Y, imgAndroidOS2Width, imgAndroidOS2Height);
		framebuffer_draw_image(imgiPhoneOS, imgiPhoneOSX, imgiPhoneOSY, imgiPhoneOSWidth, imgiPhoneOSHeight);
		framebuffer_draw_image(imgAndroidOSSelected, imgAndroidOSX, imgAndroidOSY, imgAndroidOSWidth, imgAndroidOSHeight);
	}

        if(Selection == MenuSelectionAndroidOS2) {
		framebuffer_draw_image(imgiPhoneOS, imgiPhoneOSX, imgiPhoneOSY, imgiPhoneOSWidth, imgiPhoneOSHeight);
		framebuffer_draw_image(imgConsole, imgConsoleX, imgConsoleY, imgConsoleWidth, imgConsoleHeight);
		framebuffer_draw_image(imgAndroidOS, imgAndroidOSX, imgAndroidOSY, imgAndroidOSWidth, imgAndroidOSHeight);
		framebuffer_draw_image(imgAndroidOS2Selected, imgAndroidOS2X, imgAndroidOS2Y, imgAndroidOS2Width, imgAndroidOS2Height);
	}


	lcd_window_address(2, (uint32_t) CurFramebuffer);
}


static void toggle(int forward) {
	if(forward)
	{
		if(Selection == MenuSelectioniPhoneOS)
			Selection = MenuSelectionConsole;
		else if(Selection == MenuSelectionConsole)
			Selection = MenuSelectionAndroidOS;
		else if(Selection == MenuSelectionAndroidOS)
			Selection = MenuSelectionAndroidOS2;
		else if(Selection == MenuSelectionAndroidOS2)
			Selection = MenuSelectioniPhoneOS;
	} else
	{
		if(Selection == MenuSelectioniPhoneOS)
			Selection = MenuSelectionAndroidOS2;
		else if(Selection == MenuSelectionAndroidOS2)
			Selection = MenuSelectionAndroidOS;
		else if(Selection == MenuSelectionAndroidOS)
			Selection = MenuSelectionConsole;
		else if(Selection == MenuSelectionConsole)
			Selection = MenuSelectioniPhoneOS;
	}

	drawSelectionBox();
}

int menu_setup(int timeout) {
	FBWidth = currentWindow->framebuffer.width;
	FBHeight = currentWindow->framebuffer.height;	

	imgiPhoneOS = framebuffer_load_image(dataiPhoneOSPNG, dataiPhoneOSPNG_size, &imgiPhoneOSWidth, &imgiPhoneOSHeight, TRUE);
	imgiPhoneOSSelected = framebuffer_load_image(dataiPhoneOSSelectedPNG, dataiPhoneOSSelectedPNG_size, &imgiPhoneOSWidth, &imgiPhoneOSHeight, TRUE);
	imgConsole = framebuffer_load_image(dataConsolePNG, dataConsolePNG_size, &imgConsoleWidth, &imgConsoleHeight, TRUE);
	imgConsoleSelected = framebuffer_load_image(dataConsoleSelectedPNG, dataConsoleSelectedPNG_size, &imgConsoleWidth, &imgConsoleHeight, TRUE);
	imgAndroidOS = framebuffer_load_image(dataAndroidOSPNG, dataAndroidOSPNG_size, &imgAndroidOSWidth, &imgAndroidOSHeight, TRUE);
	imgAndroidOSSelected = framebuffer_load_image(dataAndroidOSSelectedPNG, dataAndroidOSSelectedPNG_size, &imgAndroidOSWidth, &imgAndroidOSHeight, TRUE);
	imgAndroidOS2 = framebuffer_load_image(dataAndroidOS2PNG, dataAndroidOS2PNG_size, &imgAndroidOS2Width, &imgAndroidOS2Height, TRUE);
	imgAndroidOS2Selected = framebuffer_load_image(dataAndroidOS2SelectedPNG, dataAndroidOS2SelectedPNG_size, &imgAndroidOS2Width, &imgAndroidOS2Height, TRUE);
	imgHeader = framebuffer_load_image(dataHeaderPNG, dataHeaderPNG_size, &imgHeaderWidth, &imgHeaderHeight, TRUE);

	bufferPrintf("menu: images loaded\r\n");

	imgAndroidOSX = 1;
	imgAndroidOSY = 270;

	imgAndroidOS2X = 80;
	imgAndroidOS2Y = 270;

	imgiPhoneOSX = 159;
	imgiPhoneOSY = 270;

	imgConsoleX = 234;
	imgConsoleY = 270;

	imgHeaderX = 0;
	imgHeaderY = 0;

	framebuffer_draw_image(imgHeader, imgHeaderX, imgHeaderY, imgHeaderWidth, imgHeaderHeight);

	framebuffer_setloc(0, 47);
	framebuffer_setcolors(COLOR_WHITE, COLOR_BLACK);
	framebuffer_print_force("\r\n");
	framebuffer_print_force("Tap volume up/down at the side to select option.\r\n");
	framebuffer_print_force("Tap the home button to boot selected option.\r\n");
	framebuffer_print_force("Tap the power button to turn off your device.\r\n");
	framebuffer_print_force(OPENIBOOT_VERSION_STR);
	framebuffer_setcolors(COLOR_WHITE, COLOR_BLACK);
	framebuffer_setloc(0, 0);

	Selection = MenuSelectioniPhoneOS;

	OtherFramebuffer = CurFramebuffer;
	CurFramebuffer = (volatile uint32_t*) NextFramebuffer;

	drawSelectionBox();

	pmu_set_iboot_stage(0);

	memcpy((void*)NextFramebuffer, (void*) CurFramebuffer, NextFramebuffer - (uint32_t)CurFramebuffer);

	uint64_t startTime = timer_get_system_microtime();
	while(TRUE) {
#ifdef CONFIG_IPOD
		if(buttons_is_pushed(BUTTONS_HOLD)) {
			toggle(FALSE);
			startTime = timer_get_system_microtime();
			udelay(200000);
		}
#endif
#ifndef CONFIG_IPOD
		if(buttons_is_pushed(BUTTONS_HOLD)) {
			pmu_poweroff();
		}
		if(!buttons_is_pushed(BUTTONS_VOLUP)) {
			toggle(TRUE);
			startTime = timer_get_system_microtime();
			udelay(200000);
		}
		if(!buttons_is_pushed(BUTTONS_VOLDOWN)) {
			toggle(FALSE);
			startTime = timer_get_system_microtime();
			udelay(200000);
		}
#endif
		if(buttons_is_pushed(BUTTONS_HOME)) {
			break;
		}
		if(timeout > 0 && has_elapsed(startTime, (uint64_t)timeout * 1000)) {
			bufferPrintf("menu: timed out, selecting current item\r\n");
			break;
		}
		udelay(10000);
	}


	if(Selection == MenuSelectioniPhoneOS) {
		Image* image = images_get(fourcc("ibox"));
		if(image == NULL)
			image = images_get(fourcc("ibot"));
		void* imageData;
		images_read(image, &imageData);
		chainload((uint32_t)imageData);
	}

	if(Selection == MenuSelectionConsole) {
		// Reset framebuffer back to original if necessary
		if((uint32_t) CurFramebuffer == NextFramebuffer)
		{
			CurFramebuffer = OtherFramebuffer;
			currentWindow->framebuffer.buffer = CurFramebuffer;
			lcd_window_address(2, (uint32_t) CurFramebuffer);
		}

		framebuffer_setdisplaytext(TRUE);
		framebuffer_clear();
	}

	if(Selection == MenuSelectionAndroidOS) {
		// Reset framebuffer back to original if necessary
		if((uint32_t) CurFramebuffer == NextFramebuffer)
		{
			CurFramebuffer = OtherFramebuffer;
			currentWindow->framebuffer.buffer = CurFramebuffer;
			lcd_window_address(2, (uint32_t) CurFramebuffer);
		}

		framebuffer_setdisplaytext(TRUE);
		framebuffer_clear();

#ifndef NO_HFS
#ifndef CONFIG_IPOD
		radio_setup();
#endif
		nand_setup();
		fs_setup();
		if(globalFtlHasBeenRestored) /* if ftl has been restored, sync it, so kernel doesn't have to do a ftl_restore again */
		{
			if(ftl_sync())
			{
				bufferPrintf("ftl synced successfully");
			}
			else
			{
				bufferPrintf("error syncing ftl");
			}
		}

		pmu_set_iboot_stage(0);
		startScripting("linux"); //start script mode if there is a script file
		boot_linux_from_files();
#endif
	}

  
	if(Selection == MenuSelectionAndroidOS2) {
		// Reset framebuffer back to original if necessary
		if((uint32_t) CurFramebuffer == NextFramebuffer)
		{
			CurFramebuffer = OtherFramebuffer;
			currentWindow->framebuffer.buffer = CurFramebuffer;
			lcd_window_address(2, (uint32_t) CurFramebuffer);
		}

		framebuffer_setdisplaytext(TRUE);
		framebuffer_clear();

#ifndef NO_HFS
#ifndef CONFIG_IPOD
		radio_setup();
#endif
		nand_setup();
		fs_setup();
		if(globalFtlHasBeenRestored) // if ftl has been restored, sync it, so kernel doesn't have to do a ftl_restore again
		{
			if(ftl_sync())
			{
				bufferPrintf("ftl synced successfully");
			}
			else
			{
				bufferPrintf("error syncing ftl");
			}
		}

		pmu_set_iboot_stage(0);
		startScripting("linux"); //start script mode if there is a script file
		boot_linux_from_files2();
#endif
	} 



	return 0;
}

#endif
#endif
