/*
 * main.c
 *
 *  Created on: 2014 aug. 5
 *      Author: dm13dv1b
 */




/*
    ChibiOS/RT - Copyright (C) 2006,2007,2008,2009,2010,
                 2011 Giovanni Di Sirio.

    This file is part of ChibiOS/RT.

    ChibiOS/RT is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    ChibiOS/RT is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "ch.h"
#include "hal.h"
#include "gfx.h"

static GConsoleObject			gc;
static GListener				gl;
static GHandle					ghc;
static GSourceHandle			gs;
static GEventMouse				*pem;
static bool_t					calibrated;
static GEventType				deviceType;
	coord_t width, height;
	coord_t	bWidth, bHeight;
	coord_t	swidth, sheight;
/*
 * This is a periodic thread that does absolutely nothing except flashing
 * a LED.
 */


static font_t					font;

static WORKING_AREA(waThread1, 256);
static msg_t Thread1(void *arg) {

  (void)arg;
  chRegSetThreadName("blinker");
  while (TRUE) {
    palSetPad(GPIOD, GPIOD_LED3);       /* Orange.  */
    chThdSleepMilliseconds(500);
    palClearPad(GPIOD, GPIOD_LED3);     /* Orange.  */
    chThdSleepMilliseconds(500);
  }
}

/*
 * Application entry point.
 */
int main(void) {

  /*
   * System initializations.
   * - HAL initialization, this also initializes the configured device drivers
   *   and performs the board-specific initializations.
   * - Kernel initialization, the main() function becomes a thread and the
   *   RTOS is active.
   */
  halInit();
  chSysInit();

  //chThdSleepMilliseconds(1000);

  /* initialize and clear the display */
  gfxInit();
  // Get the screen size

  width = gdispGetWidth();
  height = gdispGetHeight();

  swidth = gdispGetWidth();
  sheight = gdispGetHeight();

  calibrated = FALSE;

	// Create our title
	font = gdispOpenFont("UI2");
	gwinSetDefaultFont(font);
	bWidth = gdispGetStringWidth("Next", font);
	bHeight = gdispGetStringWidth("Prev", font);
	if (bHeight > bWidth) bWidth = bHeight;
	bWidth += 4;
	bHeight = gdispGetFontMetric(font, fontHeight)+2;
	gdispFillStringBox(0, 0, swidth, bHeight, "Touch Calibration", font, Red, White, justifyLeft);

	gwinSetDefaultBgColor(Black);
	gwinSetDefaultColor(White);
	// Create our main display window
	{
		GWindowInit				wi;

		gwinClearInit(&wi);
		wi.show = TRUE;
		wi.x = 0;
		wi.y = bHeight;
		wi.width = swidth;
		wi.height = height-bHeight;
		ghc = gwinConsoleCreate(&gc, &wi);
	}
	gwinClear(ghc);

	// Initialize the mouse in our special no calibration mode.
	geventListenerInit(&gl);
	gs = ginputGetMouse(9999);
	geventAttachSource(&gl, gs, GLISTEN_MOUSEDOWNMOVES|GLISTEN_MOUSEMETA);

	StepDeviceType:
		gwinClear(ghc);
		gwinSetColor(ghc, Yellow);
		gwinPrintf(ghc, "\n1. DEVICE TYPE\n\n");

		pem = (GEventMouse *)&gl.event;
		ginputGetMouseStatus(0, pem);
		deviceType = pem->type;

		gwinSetColor(ghc, White);
		gwinPrintf(ghc, "This is detected as a %s device\n\n",
			deviceType == GEVENT_MOUSE ? "MOUSE" : (pem->type == GEVENT_TOUCH ? "TOUCH" : "UNKNOWN"));

		if (calibrated)
			gwinPrintf(ghc, "Press Next or Back to continue.\n");
		else if (deviceType == GEVENT_MOUSE)
			gwinPrintf(ghc, "Click the mouse button to move on to the next test.\n");
		else
			gwinPrintf(ghc, "Press and release your finger to move on to the next test.\n");

		while(1) {
			pem = (GEventMouse *)geventEventWait(&gl, TIME_INFINITE);
			if (calibrated) {
				if (pem->y < bHeight && pem->x >= swidth-2*bWidth) {
					if ((pem->meta & GMETA_MOUSE_UP)) {
						if (pem->x >= swidth-bWidth)
							break;
						goto StepClickJitter;
					}
				}
			} else if ((pem->meta & GMETA_MOUSE_UP))
				break;
		}

		/*
		 * Test: Mouse raw reading jitter
		 */

	StepRawJitter:
		gwinClear(ghc);
		gwinSetColor(ghc, Yellow);
		gwinPrintf(ghc, "\n2. GINPUT_MOUSE_READ_CYCLES\n\n");

		gwinSetColor(ghc, White);
		if (deviceType == GEVENT_MOUSE)
			gwinPrintf(ghc, "Press and hold the mouse button.\n\n");
		else
			gwinPrintf(ghc, "Press and hold on the surface.\n\n");
		gwinPrintf(ghc, "Numbers will display in this window.\n"
				"Ensure that values don't jump around very much when your finger is stationary.\n\n"
				"Increasing GINPUT_MOUSE_READ_CYCLES helps reduce jitter but increases CPU usage.\n\n");

		if (calibrated)
			gwinPrintf(ghc, "Press Next or Back to continue.\n");
		else if (deviceType == GEVENT_MOUSE)
			gwinPrintf(ghc, "Release the mouse button to move on to the next test.\n");
		else
			gwinPrintf(ghc, "Release your finger to move on to the next test.\n");

		// For this test turn on ALL mouse movement events
		geventAttachSource(&gl, gs, GLISTEN_MOUSEDOWNMOVES|GLISTEN_MOUSEUPMOVES|GLISTEN_MOUSEMETA|GLISTEN_MOUSENOFILTER);

		while(1) {
			// Always sleep a bit first to enable other events. We actually don't
			// mind missing events for this test.
			gfxSleepMilliseconds(100);
			pem = (GEventMouse *)geventEventWait(&gl, TIME_INFINITE);
			if (calibrated) {
				if (pem->y < bHeight && pem->x >= swidth-2*bWidth) {
					if ((pem->meta & GMETA_MOUSE_UP)) {
						if (pem->x >= swidth-bWidth)
							break;
						goto StepDeviceType;
					}
				}
			} else if ((pem->meta & GMETA_MOUSE_UP))
				break;
			gwinPrintf(ghc, "%u:%u z=%u b=0x%04x m=%04x\n", pem->x, pem->y, pem->z, pem->current_buttons, pem->meta);
		}

		// Reset to just changed movements.
		geventAttachSource(&gl, gs, GLISTEN_MOUSEDOWNMOVES|GLISTEN_MOUSEMETA);

		/*
		 * Test: Calibration
		 */

	StepCalibrate:
		gwinClear(ghc);
		gwinSetColor(ghc, Yellow);
		gwinPrintf(ghc, "\n3. GINPUT_MOUSE_CALIBRATION_ERROR\n\n");
		gwinSetColor(ghc, Black);
		gwinPrintf(ghc, "Ensure GINPUT_MOUSE_NEED_CALIBRATION = TRUE and GINPUT_MOUSE_CALIBRATION_ERROR is >= 0\n\n");
		gwinSetColor(ghc, White);
		gwinPrintf(ghc, "You will be presented with a number of points to touch.\nPress them in turn.\n\n"
				"If the calibration repeatedly fails, increase GINPUT_MOUSE_CALIBRATION_ERROR and try again.\n\n");

		if (calibrated)
			gwinPrintf(ghc, "Press Next to start the calibration.\n");
		else if (deviceType == GEVENT_MOUSE)
			gwinPrintf(ghc, "Click the mouse button to start the calibration.\n");
		else
			gwinPrintf(ghc, "Press and release your finger to start the calibration.\n");

		while(1) {
			pem = (GEventMouse *)geventEventWait(&gl, TIME_INFINITE);
			if (calibrated) {
				if (pem->y < bHeight && pem->x >= swidth-2*bWidth) {
					if ((pem->meta & GMETA_MOUSE_UP)) {
						if (pem->x >= swidth-bWidth)
							break;
						goto StepRawJitter;
					}
				}
			} else if ((pem->meta & GMETA_MOUSE_UP))
				break;
		}

		// Calibrate
		ginputCalibrateMouse(0);
		calibrated = TRUE;

		// Calibration used the whole screen - re-establish our title and Next and Previous Buttons
		gdispFillStringBox(0, 0, swidth, bHeight, "Touch Calibration", font, White, White, justifyLeft);
		gdispFillStringBox(swidth-2*bWidth, 0, bWidth-1, bHeight, "Prev", font, Black, Gray, justifyCenter);
		gdispFillStringBox(swidth-1*bWidth, 0, bWidth  , bHeight, "Next", font, Black, Gray, justifyCenter);

		/*
		 * Test: Mouse coords
		 */

	StepMouseCoords:
		gwinClear(ghc);
		gwinSetColor(ghc, Yellow);
		gwinPrintf(ghc, "\n4. Show Mouse Coordinates\n\n");

		gwinSetColor(ghc, White);
		if (deviceType == GEVENT_MOUSE)
			gwinPrintf(ghc, "Press and hold the mouse button.\n\n");
		else
			gwinPrintf(ghc, "Press and hold on the surface.\n\n");
		gwinPrintf(ghc, "Numbers will display in this window.\n"
				"Check the coordinates against where it should be on the screen.\n\n");

		gwinPrintf(ghc, "Press Next or Back to continue.\n");

		// For this test normal mouse movement events
		geventAttachSource(&gl, gs, GLISTEN_MOUSEDOWNMOVES|GLISTEN_MOUSEMETA);

		while(1) {
			// Always sleep a bit first to enable other events. We actually don't
			// mind missing events for this test.
			gfxSleepMilliseconds(100);
			pem = (GEventMouse *)geventEventWait(&gl, TIME_INFINITE);
			if (pem->y < bHeight && pem->x >= swidth-2*bWidth) {
				if ((pem->meta & GMETA_MOUSE_UP)) {
					if (pem->x >= swidth-bWidth)
						break;
					goto StepCalibrate;
				}
			}
			if ((pem->current_buttons & GINPUT_MOUSE_BTN_LEFT))
				gwinPrintf(ghc, "%u:%u z=%u\n", pem->x, pem->y, pem->z);
		}

		// Reset to just changed movements.
		geventAttachSource(&gl, gs, GLISTEN_MOUSEDOWNMOVES|GLISTEN_MOUSEMETA);

		/*
		 * Test: Mouse movement jitter
		 */

	StepJitter:
		gwinClear(ghc);
		gwinSetColor(ghc, Yellow);
		gwinPrintf(ghc, "\n4. GINPUT_MOUSE_MOVE_JITTER\n\n");

		gwinSetColor(ghc, White);
		if (deviceType == GEVENT_MOUSE)
			gwinPrintf(ghc, "Press and hold the mouse button and move around as if to draw.\n\n");
		else
			gwinPrintf(ghc, "Press firmly on the surface and move around as if to draw.\n\n");

		gwinPrintf(ghc, "Dots will display in this window. Ensure that when you stop moving your finger that "
				"new dots stop displaying.\nNew dots should only display when your finger is moving.\n\n"
				"Adjust GINPUT_MOUSE_MOVE_JITTER to the smallest value that this reliably works for.\n\n");
		gwinPrintf(ghc, "Press Next or Back to continue.\n\n");

		while(1) {
			pem = (GEventMouse *)geventEventWait(&gl, TIME_INFINITE);
			if (pem->y < bHeight && pem->x >= swidth-2*bWidth) {
				if ((pem->meta & GMETA_MOUSE_UP)) {
					if (pem->x >= swidth-bWidth)
						break;
					goto StepMouseCoords;
				}
			}
			if ((pem->current_buttons & GINPUT_MOUSE_BTN_LEFT))
				gwinPrintf(ghc, ".");
		}

		/*
		 * Test: Polling frequency
		 */

	StepPolling:
		gwinClear(ghc);
		gwinSetColor(ghc, Yellow);
		gwinPrintf(ghc, "\n5. GINPUT_MOUSE_POLL_PERIOD\n\n");

		gwinSetColor(ghc, White);
		gwinPrintf(ghc, "Press firmly on the surface (or press and hold the mouse button) and move around as if to draw.\n\n");
		gwinPrintf(ghc, "A White line will follow your finger.\n"
				"Adjust GINPUT_MOUSE_POLL_PERIOD to the highest value that provides a line without "
				"gaps that are too big.\nDecreasing the value increases CPU usage.\n"
				"About 25 (millisecs) normally produces good results."
				"This test can be ignored for interrupt driven drivers.\n\n");
		gwinPrintf(ghc, "Press Next or Back to continue.\n\n");

		while(1) {
			pem = (GEventMouse *)geventEventWait(&gl, TIME_INFINITE);
			if (pem->y < bHeight && pem->x >= swidth-2*bWidth) {
				if ((pem->meta & GMETA_MOUSE_UP)) {
					if (pem->x >= swidth-bWidth)
						break;
					goto StepJitter;
				}
			}
			if ((pem->current_buttons & GINPUT_MOUSE_BTN_LEFT))
				gdispDrawPixel(pem->x, pem->y, White);
		}

		/*
		 * Test: Click Jitter
		 */

	StepClickJitter:
		gwinClear(ghc);
		gwinSetColor(ghc, Yellow);
		gwinPrintf(ghc, "\n6. GINPUT_MOUSE_MAX_CLICK_JITTER\n\n");

		gwinSetColor(ghc, White);
		gwinPrintf(ghc, "Press and release the touch surface to \"click\".\nTry both short and long presses.\n");
		gwinPrintf(ghc, "For a mouse click with the left and right buttons.\n\n");
		gwinPrintf(ghc, "Dots will display in this window. A White dash is a left (or short) click. "
				"A red x is a right (or long) click.\n\n"
				"Adjust GINPUT_MOUSE_CLICK_JITTER to the smallest value that this reliably works for.\n"
				"Adjust GINPUT_MOUSE_CLICK_TIME to adjust distinguishing short vs long presses.\n"
				"TIME_INFINITE means there are no long presses (although a right mouse button will still work).\n\n"
				"Note: moving your finger (mouse) during a click cancels it.\n\n");
		gwinPrintf(ghc, "This is the last test but you can press Next or Back to continue.\n\n");

		while(1) {
			pem = (GEventMouse *)geventEventWait(&gl, TIME_INFINITE);
			if (pem->y < bHeight && pem->x >= swidth-2*bWidth) {
				if ((pem->meta & GMETA_MOUSE_UP)) {
					if (pem->x >= swidth-bWidth)
						break;
					goto StepPolling;
				}
			}
			if ((pem->meta & GMETA_MOUSE_CLICK)) {
				gwinSetColor(ghc, White);
				gwinPrintf(ghc, "-");
			}
			if ((pem->meta & GMETA_MOUSE_CXTCLICK)) {
				gwinSetColor(ghc, Red);
				gwinPrintf(ghc, "x");
			}
		}

		// Can't let this really exit
goto StepDeviceType;

  chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO, Thread1, NULL);
  /*
   * Normal main() thread activity, in this demo it does nothing except
   * sleeping in a loop
   */


  while (TRUE) {
    chThdSleepMilliseconds(500);
  }
}

