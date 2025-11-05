/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
  /* USER CODE END Header */
  /* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdlib.h>
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef struct {
	uint8_t row;
	uint8_t col;
} Position;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define NROW 12
#define NCOL 16
#define SQ_SIZE 240/NROW
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

// --- A3: Wejscie z debounce + auto-repeat (polling) ---
typedef enum { DIR_NONE=0, DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT } Dir;

#define BTN_DEBOUNCE_MS       20U    // filtr drgaĹ„ stykĂłw
#define BTN_REPEAT_DELAY_MS  160U    // po tyle ms pierwszy powtĂłrzony krok
#define BTN_REPEAT_MS         80U    // odstÄ™p kolejnych krokĂłw przy trzymaniu

static Dir     g_btn_last = DIR_NONE;        // ostatni stabilny kierunek
static uint32_t g_btn_last_change_ms = 0;    // kiedy zmieniĹ‚ siÄ™ stan
static uint32_t g_btn_last_repeat_ms = 0;    // kiedy byĹ‚ ostatni â€žrepeatâ€ť

// --- A1: FPS / timing (bez rysowania) ---
#define FRAME_MS_TARGET   33U            // ~30 Hz
static volatile uint32_t g_frame_ms = 0; // ostatni czas klatki [ms]
static volatile uint32_t g_fps10    = 0; // FPS*10 (np. 298 => 29.8 FPS)
static uint32_t pinky_timer_ms      = 0; // akumulator do ruchu Pinky

// Obsługa przycisku SEL (pauza/restart) z debounce i long-press:
static uint8_t  sel_pressed = 0;
static uint32_t sel_change_ms = 0;
static uint32_t sel_down_start_ms = 0;

// D1: liczba „jadalnych” pól (bez ścian):
uint16_t totalDots = 0;

/* D1: stała mapa 12x16: 1 = ściana, 0 = wolne */
static const uint8_t walls[NROW][NCOL] = {
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,1,1,0,0,0,0,1,0,0,0,0,1,1,0,0},
  {0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0},
  {0,0,0,1,1,1,1,1,0,1,1,1,1,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0},
  {0,1,1,0,0,0,0,1,0,0,0,0,1,1,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
};



__ALIGN_END uint8_t pinkyLeft[776] = {
0x42,0x4d,0x08,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x36,0x00,0x00,0x00,0x28,0x00,
0x00,0x00,0x13,0x00,0x00,0x00,0x13,0x00,0x00,0x00,0x01,0x00,0x10,0x00,0x00,0x00,
0x00,0x00,0xd0,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,
0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0xd9,0xfc,0xd9,0xfc,
0xd9,0xfc,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0xd9,0xfc,0xd9,0xfc,
0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,
0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0x45,0xa1,
0x45,0xa1,0x45,0xa1,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,
0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,
0xd9,0xfc,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,
0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,
0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,
0x45,0xa1,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,
0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0x45,0xa1,0x45,0xa1,0x45,0xa1,
0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,
0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0x45,0xa1,
0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,
0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,
0xd9,0xfc,0xd9,0xfc,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,
0x45,0xa1,0x45,0xa1,0x45,0xa1,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,
0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,
0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0xd9,0xfc,0xd9,0xfc,
0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0x45,0xa1,
0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,
0x45,0xa1,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,
0xd9,0xfc,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,
0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,
0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,
0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,
0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0x45,0xa1,
0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,
0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,
0xd9,0xfc,0xd9,0xfc,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,
0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0xd9,0xfc,
0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0x45,0xa1,0x45,0xa1,0x45,0xa1,
0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,
0x45,0xa1,0x45,0xa1,0x45,0xa1,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,
0xd9,0xfc,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,
0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0xd9,0xfc,0xd9,0xfc,
0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,
0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,
0x45,0xa1,0x45,0xa1,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0x45,0xa1,
0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,
0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0xd9,0xfc,0xd9,0xfc,
0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,
0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,
0x45,0xa1,0x45,0xa1,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0x45,0xa1,0x45,0xa1,
0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1
};
__ALIGN_END uint8_t pinkyRight[776] = {
0x42,0x4d,0x08,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x36,0x00,0x00,0x00,0x28,0x00,
0x00,0x00,0x13,0x00,0x00,0x00,0x13,0x00,0x00,0x00,0x01,0x00,0x10,0x00,0x00,0x00,
0x00,0x00,0xd0,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,
0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,
0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,
0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,
0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,
0xd9,0xfc,0x45,0xa1,0x45,0xa1,0x45,0xa1,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,
0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,
0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,
0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,
0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0x45,0xa1,0x45,0xa1,
0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,
0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,
0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,
0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,
0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,
0x45,0xa1,0x45,0xa1,0x45,0xa1,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,
0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0x45,0xa1,0x45,0xa1,0x45,0xa1,
0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0xd9,0xfc,
0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,
0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,
0x45,0xa1,0x45,0xa1,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,
0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,
0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0xd9,0xfc,0xd9,0xfc,
0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0x45,0xa1,0x45,0xa1,
0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,
0x45,0xa1,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,
0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,
0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,
0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,
0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0xd9,0xfc,
0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0x45,0xa1,0x45,0xa1,0x45,0xa1,
0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,
0x45,0xa1,0x45,0xa1,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,
0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,
0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,
0xd9,0xfc,0xd9,0xfc,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,
0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0xd9,0xfc,
0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0x45,0xa1,0x45,0xa1,0x45,0xa1,
0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,
0x45,0xa1,0x45,0xa1,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0x45,0xa1,
0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,
0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,
0xd9,0xfc,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,
0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0xd9,0xfc,
0xd9,0xfc,0xd9,0xfc,0xd9,0xfc,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1,
0x45,0xa1,0x45,0xa1,0x45,0xa1,0x45,0xa1
};
// gameBoard represents game arena:
// 0: field is empty;
// 1: Pac-Man;
// 2: Pinky;
uint8_t gameBoard[NROW][NCOL] = { {0} };
// visitedFields keeps track of fields visited by Pac-Man and facilitates counting points:
// 0: field not visited yet;
// 1: field visited already;
uint8_t visitedFields[NROW][NCOL] = { {0} };
Position pacmanPos = { 0, 0 };
Position pinkyPos = { NROW - 1, NCOL - 1 };
JOYState_TypeDef JoyState = JOY_NONE;
uint16_t pointsCounter = 0;
// gameStatus represents status of current game:
// 0: Pinky wins;
// 1: game running;
// 2: Pac-Man wins;
uint8_t gameStatus = 1;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static inline void DoMove(Dir d);
static void HandleInput(uint32_t now_ms);


/* USER CODE BEGIN PFP */
void myRedLedInit(void);
void myLowLevelRedLedInit(void);
int8_t myAdc1Init(void);
uint32_t getSeedValue(void);
void gameSetup(void);
void moveDown(void);
void moveUp(void);
void moveLeft(void);
void moveRight(void);
void movePinky(void);
void myDrawPixel(uint16_t, uint16_t, uint16_t);
void myDrawFullRectangle(uint16_t, uint16_t, uint16_t, uint16_t, uint16_t);
void myDrawFullCircle(uint16_t, uint16_t, uint16_t, uint16_t);
void gameOver(void);
void DrawHeart(uint16_t x, uint16_t y, uint16_t color);
void DrawHUD (void);
void quickRestart(void);
void showPauseBannerInHUD(void);
void clearPauseBannerInHUD(void);
void loseLifeAndRespawn(void); 
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void) {
	/* USER CODE BEGIN 1 */
	uint32_t seed = 0;
	/* USER CODE END 1 */

	/* MCU Configuration--------------------------------------------------------*/

	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	/* USER CODE BEGIN Init */
	/* USER CODE END Init */

	/* Configure the system clock */
	SystemClock_Config();

	/* USER CODE BEGIN SysInit */
	/* USER CODE END SysInit */

	/* Initialize all configured peripherals */
	/* USER CODE BEGIN 2 */
	/* Initialize the LEDs */
	// BSP_LED_Init(LED_GREEN);
	// BSP_LED_Init(LED_ORANGE);
	// BSP_LED_Init(LED_RED);
	// myRedLedInit();
	myLowLevelRedLedInit();
	// BSP_LED_Init(LED_BLUE);

	/* Configure the Key push-button in GPIO Mode */
	// BSP_PB_Init(BUTTON_KEY, BUTTON_MODE_GPIO);

	/*## Initialize the LCD #################################################*/
	if (BSP_LCD_Init() != LCD_OK) {
		BSP_LED_On(LED_RED);
		Error_Handler();
	}

	// Configure the LCD (for device setup error messages only):
	BSP_LCD_Clear(LCD_COLOR_DARKGRAY);
	BSP_LCD_SetFont(&Font12);
	BSP_LCD_SetTextColor(LCD_COLOR_RED);

	// Configure joystick in polling mode:
	/*
	if (BSP_JOY_Init(JOY_MODE_GPIO) != IO_OK) {
		BSP_LCD_DisplayStringAt(0, 145, (uint8_t *)"ERROR", CENTER_MODE);
		BSP_LCD_DisplayStringAt(0, 160, (uint8_t *)"Joystick cannot be initialized", CENTER_MODE);
		Error_Handler();
	}
	*/
	// Configure joystick in polling mode (prościej dla debounce/repeat):
	if (BSP_JOY_Init(JOY_MODE_GPIO) != IO_OK) {
		BSP_LCD_DisplayStringAt(0, 145, (uint8_t *)"ERROR", CENTER_MODE);
		BSP_LCD_DisplayStringAt(0, 160, (uint8_t *)"Joystick cannot be initialized", CENTER_MODE);
		Error_Handler();
	}

	// Configure ADC1:
	if (myAdc1Init() != HAL_OK) {
		BSP_LCD_DisplayStringAt(0, 145, (uint8_t*)"ERROR", CENTER_MODE);
		BSP_LCD_DisplayStringAt(0, 160, (uint8_t*)"ADC1 cannot be initialized", CENTER_MODE);
		Error_Handler();
	}

	// Use ADC1 to get a seed value:
	seed = getSeedValue();
	// Initialize the random number generator:
	srand(seed);
	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	/* USER CODE BEGIN WHILE */
	gameSetup();
	DrawHUD();

	uint8_t firstFrame = 1;

	while (1) {
		// --- start klatki ---
		uint32_t frame_start = HAL_GetTick();

		// 1) Wejście (polling + debounce + auto-repeat)
		HandleInput(frame_start);

		// 2) Domknięcie do ~33 ms
		uint32_t now_ms = HAL_GetTick();
		uint32_t elapsed = now_ms - frame_start;
		if (elapsed < FRAME_MS_TARGET) {
			HAL_Delay(FRAME_MS_TARGET - elapsed);
		}

	  // 5) Warunki konca gry
	  if (pointsCounter >= totalDots) { gameStatus = 2; gameOver(); }
	  if ((pinkyPos.row == pacmanPos.row) && (pinkyPos.col == pacmanPos.col)) {
	    loseLifeAndRespawn();
	  }

		/// 4) Harmonogram ducha: co ~500 ms (tylko gdy nie pauzujemy)
		if (!paused && !gameOverState) {
			if (firstFrame) { firstFrame = 0; pinky_timer_ms = 500U; }
			pinky_timer_ms += g_frame_ms;
			while (pinky_timer_ms >= 500U) {
				movePinky();
				pinky_timer_ms -= 500U;
			}

			// 5) Warunki końca gry
			if (pointsCounter >= totalDots) { gameStatus = 2; gameOver(); }
			if ((pinkyPos.row == pacmanPos.row) && (pinkyPos.col == pacmanPos.col)) {
				loseLifeAndRespawn();
			}
		}

		// 6) HUD – odświeżaj co klatkę (również w pauzie/po GAME OVER)
		DrawHUD();

		/* USER CODE END WHILE */

		/* USER CODE BEGIN 3 */
	}
	/* USER CODE END 3 */
}
/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void) {
	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };
	RCC_PeriphCLKInitTypeDef PeriphClkInit = { 0 };

	/** Initializes the RCC Oscillators according to the specified parameters
	* in the RCC_OscInitTypeDef structure.
	*/
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE | RCC_OSCILLATORTYPE_LSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV5;
	RCC_OscInitStruct.LSEState = RCC_LSE_ON;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.Prediv1Source = RCC_PREDIV1_SOURCE_PLL2;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
	RCC_OscInitStruct.PLL2.PLL2State = RCC_PLL2_ON;
	RCC_OscInitStruct.PLL2.PLL2MUL = RCC_PLL2_MUL8;
	RCC_OscInitStruct.PLL2.HSEPrediv2Value = RCC_HSE_PREDIV2_DIV5;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		Error_Handler();
	}
	/** Initializes the CPU, AHB and APB buses clocks
	*/
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
		| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
		Error_Handler();
	}
	PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC | RCC_PERIPHCLK_ADC
		| RCC_PERIPHCLK_USB;
	PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
	PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
	PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_PLL_DIV3;
	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
		Error_Handler();
	}
	// HAL_RCC_MCOConfig(RCC_MCO, RCC_MCO1SOURCE_HSE, RCC_MCODIV_1);
	// __HAL_RCC_PLLI2S_ENABLE();
}

/* USER CODE BEGIN 4 */
// Function configuring RED LED using HAL:
void myRedLedInit(void) {
	GPIO_InitTypeDef gpioInit = {0};

	/* Enable the GPIO_LED clock */
	__HAL_RCC_GPIOD_CLK_ENABLE();

	/* Configure the GPIO_LED pin */
	gpioInit.Pin    = GPIO_PIN_3;
	gpioInit.Mode   = GPIO_MODE_OUTPUT_PP;
	gpioInit.Pull   = GPIO_NOPULL;
	gpioInit.Speed  = GPIO_SPEED_FREQ_LOW;

	HAL_GPIO_Init(GPIOD, &gpioInit);
	// HAL_GPIO_WritePin(GPIOD, GPIO_PIN_3, 0);
}


// Function configuring RED LED using registers:
void myLowLevelRedLedInit(void) {
	/* Enable the GPIO_LED clock */
	// Set bit 5 to '1':
	// RCC->APB2ENR |= 0b00000000000000000000000000100000;
	RCC->APB2ENR |= 1 << 5;

	// Configure PD3 as output 2 MHz:
	// Set bit 13 and 12 to '10':
	// GPIOD->CRL = GPIOD->CRL | 0b00000000000000000010000000000000;
	// GPIOD->CRL |= 0b00000000000000000010000000000000;
	GPIOD->CRL |= 1 << 13;
	// GPIOD->CRL |= 0x00002000;
	// GPIOD->CRL &= 0b11111111111111111110111111111111;
	// GPIOD->CRL &=~0b00000000000000000001000000000000;
	GPIOD->CRL &= ~(1 << 12);
	// Configure PD3 as output push/pull:
	// Set bits 15 and 14 to '00':
	GPIOD->CRL &=~0b00000000000000001100000000000000;
}


// Function initializing ADC1:
int8_t myAdc1Init(void) {
	uint8_t ret = HAL_OK;
	ADC_HandleTypeDef hadc1 = {0};
	ADC_ChannelConfTypeDef sConfig = {0};

	/** Common configuration */
	hadc1.Instance = ADC1;
	hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
	hadc1.Init.ContinuousConvMode = DISABLE;
	hadc1.Init.DiscontinuousConvMode = DISABLE;
	hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
	hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
	hadc1.Init.NbrOfConversion = 1;
	ret = HAL_ADC_Init(&hadc1);
	if (ret != HAL_OK) {
		return ret;
	}

	/** Configure regular channel group */
	sConfig.Channel = ADC_CHANNEL_7;
	sConfig.Rank = ADC_REGULAR_RANK_1;
	sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
	ret = HAL_ADC_ConfigChannel(&hadc1, &sConfig);

	return ret;
}


// Function used to force ADC1 to sample a channel (channel 7 in our case).
// Returns seed necessary to initialize random number generator:
uint32_t getSeedValue(void) {
	uint32_t ret = 0;

	ADC1->CR2 |= 0x00000001;
	// Wait for the ADC to stabilize:
	HAL_Delay(500);
	// Start conversion:
	ADC1->CR2 |= 0x00000001;
	// Wait for end of the conversion:
	while ((ADC1->SR & 0x00000002) == 0);
	ret = (uint32_t)ADC1->DR;
	// Clear the STRT bit:
	ADC1->SR &=~0x00000010;

	return ret;
}


// Function responsible for game setup:
void gameSetup(void) {
	// 1) Ekran
	BSP_LCD_Clear(LCD_COLOR_BLACK);
	BSP_LCD_SetTextColor(LCD_COLOR_ORANGE);

	// 2) Krata
	for (uint16_t i = 0; i < 240; i += SQ_SIZE) BSP_LCD_DrawHLine(0, i, 320);
	for (uint16_t i = 0; i < 320; i += SQ_SIZE) BSP_LCD_DrawVLine(i, 0, 240);

	// 3) Wyzeruj stan i policz kropki
	totalDots = 0;
	for (uint8_t r = 0; r < NROW; ++r) {
		for (uint8_t c = 0; c < NCOL; ++c) {
			gameBoard[r][c] = 0;
			visitedFields[r][c] = 0;

			if (!walls[r][c]) {
				// kropka w środku kafla
				myDrawPixel(c * SQ_SIZE + SQ_SIZE / 2, r * SQ_SIZE + SQ_SIZE / 2, LCD_COLOR_WHITE);
				totalDots++;
			}
		}
	}

	// 4) Ściany
	drawWalls();

	// 5) Losowy start Pac-Mana i Pinky – tylko wolne pola i różne pozycje
	do { pacmanPos.row = rand() % NROW; pacmanPos.col = rand() % NCOL; } while (walls[pacmanPos.row][pacmanPos.col]);
	gameBoard[pacmanPos.row][pacmanPos.col] = 1;
	visitedFields[pacmanPos.row][pacmanPos.col] = 1; // zjedzona kropka startowa
	pointsCounter = 1;
	myDrawFullCircle(SQ_SIZE * pacmanPos.col + SQ_SIZE / 2, SQ_SIZE * pacmanPos.row + SQ_SIZE / 2, SQ_SIZE / 2 - 1, LCD_COLOR_YELLOW);

	do { pinkyPos.row = rand() % NROW; pinkyPos.col = rand() % NCOL; } while (walls[pinkyPos.row][pinkyPos.col] || (pinkyPos.row == pacmanPos.row && pinkyPos.col == pacmanPos.col));
	gameBoard[pinkyPos.row][pinkyPos.col] = 2;
	BSP_LCD_DrawBitmap(pinkyPos.col * SQ_SIZE + 1, pinkyPos.row * SQ_SIZE + 1, pinkyLeft);

	// 6) HUD
	DrawHUD();
}



void moveUp(void) {
	uint8_t r2 = (pacmanPos.row == 0) ? (NROW - 1) : (pacmanPos.row - 1);
	uint8_t c2 = pacmanPos.col;
	if (walls[r2][c2]) return; // ściana → brak ruchu

	// wymazanie starego pola
	myDrawFullRectangle(pacmanPos.col * SQ_SIZE + 1, pacmanPos.row * SQ_SIZE + 1, SQ_SIZE - 1, SQ_SIZE - 1, LCD_COLOR_BLACK);
	gameBoard[pacmanPos.row][pacmanPos.col] = 0;

	pacmanPos.row = r2;
	gameBoard[pacmanPos.row][pacmanPos.col] = 1;

	if (visitedFields[pacmanPos.row][pacmanPos.col] == 0) {
		pointsCounter++;
		visitedFields[pacmanPos.row][pacmanPos.col] = 1;
	}

	myDrawFullCircle(SQ_SIZE * pacmanPos.col + SQ_SIZE / 2, SQ_SIZE * pacmanPos.row + SQ_SIZE / 2, SQ_SIZE / 2 - 1, LCD_COLOR_YELLOW);
}

void moveDown(void) {
	uint8_t r2 = (pacmanPos.row == NROW - 1) ? 0 : (pacmanPos.row + 1);
	uint8_t c2 = pacmanPos.col;
	if (walls[r2][c2]) return;

	myDrawFullRectangle(pacmanPos.col * SQ_SIZE + 1, pacmanPos.row * SQ_SIZE + 1, SQ_SIZE - 1, SQ_SIZE - 1, LCD_COLOR_BLACK);
	gameBoard[pacmanPos.row][pacmanPos.col] = 0;

	pacmanPos.row = r2;
	gameBoard[pacmanPos.row][pacmanPos.col] = 1;

	if (visitedFields[pacmanPos.row][pacmanPos.col] == 0) {
		pointsCounter++;
		visitedFields[pacmanPos.row][pacmanPos.col] = 1;
	}

	myDrawFullCircle(SQ_SIZE * pacmanPos.col + SQ_SIZE / 2, SQ_SIZE * pacmanPos.row + SQ_SIZE / 2, SQ_SIZE / 2 - 1, LCD_COLOR_YELLOW);
}

void moveLeft(void) {
	uint8_t r2 = pacmanPos.row;
	uint8_t c2 = (pacmanPos.col == 0) ? (NCOL - 1) : (pacmanPos.col - 1);
	if (walls[r2][c2]) return;

	myDrawFullRectangle(pacmanPos.col * SQ_SIZE + 1, pacmanPos.row * SQ_SIZE + 1, SQ_SIZE - 1, SQ_SIZE - 1, LCD_COLOR_BLACK);
	gameBoard[pacmanPos.row][pacmanPos.col] = 0;

	pacmanPos.col = c2;
	gameBoard[pacmanPos.row][pacmanPos.col] = 1;

	if (visitedFields[pacmanPos.row][pacmanPos.col] == 0) {
		pointsCounter++;
		visitedFields[pacmanPos.row][pacmanPos.col] = 1;
	}

	myDrawFullCircle(SQ_SIZE * pacmanPos.col + SQ_SIZE / 2, SQ_SIZE * pacmanPos.row + SQ_SIZE / 2, SQ_SIZE / 2 - 1, LCD_COLOR_YELLOW);
}

void moveRight(void) {
	uint8_t r2 = pacmanPos.row;
	uint8_t c2 = (pacmanPos.col == NCOL - 1) ? 0 : (pacmanPos.col + 1);
	if (walls[r2][c2]) return;

	myDrawFullRectangle(pacmanPos.col * SQ_SIZE + 1, pacmanPos.row * SQ_SIZE + 1, SQ_SIZE - 1, SQ_SIZE - 1, LCD_COLOR_BLACK);
	gameBoard[pacmanPos.row][pacmanPos.col] = 0;

	pacmanPos.col = c2;
	gameBoard[pacmanPos.row][pacmanPos.col] = 1;

	if (visitedFields[pacmanPos.row][pacmanPos.col] == 0) {
		pointsCounter++;
		visitedFields[pacmanPos.row][pacmanPos.col] = 1;
	}

	myDrawFullCircle(SQ_SIZE * pacmanPos.col + SQ_SIZE / 2, SQ_SIZE * pacmanPos.row + SQ_SIZE / 2, SQ_SIZE / 2 - 1, LCD_COLOR_YELLOW);
}


void movePinky(void) {
	int16_t dr = (int16_t)pacmanPos.row - (int16_t)pinkyPos.row;
	int16_t dc = (int16_t)pacmanPos.col - (int16_t)pinkyPos.col;

	uint8_t r1 = pinkyPos.row, c1 = pinkyPos.col;

	// kandydat ruchu po WIERSZU
	uint8_t cand_row_r = r1, cand_row_c = c1;
	if (dr < 0)      cand_row_r = (r1 == 0) ? (NROW - 1) : (r1 - 1);
	else if (dr > 0) cand_row_r = (r1 == NROW - 1) ? 0 : (r1 + 1);

	// kandydat ruchu po KOLUMNIE
	uint8_t cand_col_r = r1, cand_col_c = c1;
	if (dc < 0)      cand_col_c = (c1 == 0) ? (NCOL - 1) : (c1 - 1);
	else if (dc > 0) cand_col_c = (c1 == NCOL - 1) ? 0 : (c1 + 1);

	// który kierunek próbujemy najpierw?
	uint8_t tryRowFirst = (abs((int)dr) >= abs((int)dc));

	// wybór pierwszego kandydata
	uint8_t nr = tryRowFirst ? cand_row_r : cand_col_r;
	uint8_t nc = tryRowFirst ? cand_row_c : cand_col_c;

	// jeśli pierwszy kandydat to ściana – spróbuj drugiego
	if (walls[nr][nc]) {
		nr = tryRowFirst ? cand_col_r : cand_row_r;
		nc = tryRowFirst ? cand_col_c : cand_row_c;
	}

	// jeśli też ściana albo brak zmiany — nie ruszaj się
	if (walls[nr][nc] || (nr == r1 && nc == c1)) return;

	// wyczyść stare miejsce Pinky
	myDrawFullRectangle(pinkyPos.col * SQ_SIZE + 1, pinkyPos.row * SQ_SIZE + 1, SQ_SIZE - 1, SQ_SIZE - 1, LCD_COLOR_BLACK);
	gameBoard[pinkyPos.row][pinkyPos.col] = 0;

	// odtwórz kropkę, jeśli była i nie ma ściany
	if (!walls[pinkyPos.row][pinkyPos.col] && visitedFields[pinkyPos.row][pinkyPos.col] == 0) {
		myDrawPixel(SQ_SIZE * pinkyPos.col + SQ_SIZE / 2, SQ_SIZE * pinkyPos.row + SQ_SIZE / 2, LCD_COLOR_WHITE);
	}

	// przesuń Pinky
	pinkyPos.row = nr;
	pinkyPos.col = nc;
	gameBoard[pinkyPos.row][pinkyPos.col] = 2;

	// sprite lewo/prawo
	if (nc > c1 || (c1 == NCOL - 1 && nc == 0)) {
		BSP_LCD_DrawBitmap(pinkyPos.col * SQ_SIZE + 1, pinkyPos.row * SQ_SIZE + 1, pinkyRight);
	}
	else if (nc < c1 || (c1 == 0 && nc == NCOL - 1)) {
		BSP_LCD_DrawBitmap(pinkyPos.col * SQ_SIZE + 1, pinkyPos.row * SQ_SIZE + 1, pinkyLeft);
	}
	else {
		BSP_LCD_DrawBitmap(pinkyPos.col * SQ_SIZE + 1, pinkyPos.row * SQ_SIZE + 1, pinkyLeft);
	}
}



/**
  * @brief  Draws a pixel on LCD.
  * @param  posX: X position
  * @param  posY: Y position
  * @param  color: pixel color in RGB mode (5-6-5)
  */
void myDrawPixel(uint16_t posX, uint16_t posY, uint16_t color) {
	uint16_t backup_color = BSP_LCD_GetTextColor();
	BSP_LCD_SetTextColor(color);

	BSP_LCD_DrawHLine(posX, posY, 1);

	BSP_LCD_SetTextColor(backup_color);
}


// Function drawing a full rectangle:
void myDrawFullRectangle(uint16_t Xpos, uint16_t Ypos, uint16_t Width, uint16_t Height, uint16_t color) {
	uint16_t backup_color = BSP_LCD_GetTextColor();
	BSP_LCD_SetTextColor(color);

	while(Height--) {
		BSP_LCD_DrawHLine(Xpos, Ypos++, Width);
	}

	BSP_LCD_SetTextColor(backup_color);
}

/* B1: rysowanie serduszka i hud*/

//male serduszko 16x16 tworzone z 2 kolek

void DrawHeart(uint16_t x, uint16_t y, uint16_t color){
//dwa poswiaty 
myDrawFullCircle(x+5, y+5, 5, color);
myDrawFullCircle(x+11, y+5, 5, color);
myDrawFullRectangle(x+2, y+8, 12, 6, color);
myDrawFullRectangle(x+5, y+12, 6, 5, color);
myDrawFullRectangle(x+7, y+16, 2, 3, color);
}

// Pasek HUD w gornym rzedzie (wysokosc = SQ_SIZE-1)
void DrawHUD(void) {
  char buf[32];

  // Czarny pasek na gorze (nie boj sie: rysujemy go co klatke, jest maly)
  myDrawFullRectangle(0, 0, 320, SQ_SIZE-1, LCD_COLOR_BLACK);

  // Serduszka po lewej (3 szt.)
  for (uint8_t i = 0; i < 3; ++i) {
    if (i < livesLeft) {
      DrawHeart(2 + i*20, 2, LCD_COLOR_RED);        // Zywe serce
    } else {
      DrawHeart(2 + i*20, 2, LCD_COLOR_DARKGRAY);   // Puste serce
    }
  }

// Wynik
  BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
  snprintf(buf, sizeof(buf), "SCORE: %u", (unsigned)pointsCounter);
  BSP_LCD_DisplayStringAt(80, 4, (uint8_t*)buf, LEFT_MODE);
}

void drawWalls(void) {
  for (uint8_t r = 0; r < NROW; ++r) {
    for (uint8_t c = 0; c < NCOL; ++c) {
      if (walls[r][c]) {
        // Zamaluj kafelek na niebieski
    	  myDrawFullRectangle(c*SQ_SIZE+1, r*SQ_SIZE+1, SQ_SIZE-1, SQ_SIZE-1, LCD_COLOR_BLUE);
      }
    }
  }
}

// Function drawing a full circle:
void myDrawFullCircle(uint16_t centerX, uint16_t centerY, uint16_t radius, uint16_t color) {
	int16_t i, j;
	uint16_t backup_color = BSP_LCD_GetTextColor();
	BSP_LCD_SetTextColor(color);

	for (i = (-1)*radius;i <= radius;i++) {
		for (j = (-1)*radius;j <= radius;j++) {
			if ((i*i + j*j) <= radius*radius) {
				BSP_LCD_DrawVLine(centerX+i, centerY+j, (-2)*j);
				break;
			}
		}
	}

	BSP_LCD_SetTextColor(backup_color);
}

/* === B1: Rysowanie serduszka i HUD === */

// Małe serduszko ~16x16 tworzone z 2 kółek + prostokątów (proste i szybkie)
void DrawHeart(uint16_t x, uint16_t y, uint16_t color) {
	// Dwa „płatki”
	myDrawFullCircle(x + 5, y + 5, 5, color);
	myDrawFullCircle(x + 11, y + 5, 5, color);
	// „Trójkąt” do dołu złożony z prostokątów
	myDrawFullRectangle(x + 2, y + 8, 12, 6, color);
	myDrawFullRectangle(x + 5, y + 12, 6, 5, color);
	myDrawFullRectangle(x + 7, y + 16, 2, 3, color);
}

// Pasek HUD w górnym rzędzie (wysokość = SQ_SIZE-1)
void DrawHUD(void) {
	char buf[32];

	// Czarny pasek na górze (nie bój się: rysujemy go co klatkę, jest mały)
	myDrawFullRectangle(0, 0, 320, SQ_SIZE - 1, LCD_COLOR_BLACK);

	// Serduszka po lewej (3 szt.)
	for (uint8_t i = 0; i < 3; ++i) {
		if (i < livesLeft) {
			DrawHeart(2 + i * 20, 2, LCD_COLOR_RED);        // żywe serce
		}
		else {
			DrawHeart(2 + i * 20, 2, LCD_COLOR_DARKGRAY);   // „puste” serce
		}
	}

	// Wynik
	BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
	snprintf(buf, sizeof(buf), "SCORE: %u", (unsigned)pointsCounter);
	BSP_LCD_DisplayStringAt(80, 4, (uint8_t*)buf, LEFT_MODE);
}

void drawWalls(void) {
	for (uint8_t r = 0; r < NROW; ++r) {
		for (uint8_t c = 0; c < NCOL; ++c) {
			if (walls[r][c]) {
				// Zamaluj kafelek na „mur”
				myDrawFullRectangle(c * SQ_SIZE + 1, r * SQ_SIZE + 1, SQ_SIZE - 1, SQ_SIZE - 1, LCD_COLOR_BLUE);
			}
		}
	}
}

if (visitedFields[pacmanPos.row][pacmanPos.col]==0){
	visitedFields[pacmanPos.row][pacmanPos.col]=1;
}

gameBoard[pacmanPos.row][pacmanPos.col]=1;
myDrawFullCircle(SQ_SIZE*pacmanPos.col+SQ_SIZE/2,SQ_SIZE*pacmanPos.row+SQ_SIZE/2-1, LCD_COLOR_YELLOW);
if (livesLeft==0) {gameStatus = 0; gameOver();}

void quickRestart(void) {
	// Zresetuj stan gry (bez inicjalizacji HAL/ekranów/joysticka)
	pointsCounter = 0;
	livesLeft = 3;
	gameStatus = 1;
	paused = 0;
	gameOverState = 0;

	// Wyczyszczenie danych planszy
	for (uint8_t r = 0; r < NROW; ++r) {
		for (uint8_t c = 0; c < NCOL; ++c) {
			gameBoard[r][c] = 0;
			visitedFields[r][c] = 0;
		}
	}

static inline void DoMove(Dir d) {
  switch (d) {
    case DIR_UP:    moveUp();    break;
    case DIR_DOWN:  moveDown();  break;
    case DIR_LEFT:  moveLeft();  break;
    case DIR_RIGHT: moveRight(); break;
    default: break;
  }
}

static Dir MapJoyToDir(JOYState_TypeDef js) {
  switch (js) {
    case JOY_UP:    return DIR_UP;
    case JOY_DOWN:  return DIR_DOWN;
    case JOY_LEFT:  return DIR_LEFT;
    case JOY_RIGHT: return DIR_RIGHT;
    default:        return DIR_NONE;
  }
}

// Polling z debounce i auto-repeat
static void HandleInput(uint32_t now_ms) {
	JOYState_TypeDef js = BSP_JOY_GetState();
	Dir d = MapJoyToDir(js);

	// Zmiana stanu debounce
	if (d != g_btn_last) {
		if ((now_ms - g_btn_last_change_ms) >= BTN_DEBOUNCE_MS) {
			g_btn_last = d;
			g_btn_last_change_ms = now_ms;
			g_btn_last_repeat_ms = 0;     // reset powtarzania
			if (d != DIR_NONE) {
				DoMove(d);                  // natychmiast pierwszy krok po stabilnej zmianie
			}
		}
		return; // jeszcze nic wiÄ™cej â€“ czekamy aĹĽ siÄ™ ustabilizuje/odmierzamy delay
	}

	// Trzymanie auto-repeat
	if (d != DIR_NONE) {
		if (g_btn_last_repeat_ms == 0) {
			// pierwszy repeat po opĂłĹşnieniu
			if ((now_ms - g_btn_last_change_ms) >= BTN_REPEAT_DELAY_MS) {
				DoMove(d);
				g_btn_last_repeat_ms = now_ms;
			}
		}
		else {
			// kolejne repeaty co BTN_REPEAT_MS
			if ((now_ms - g_btn_last_repeat_ms) >= BTN_REPEAT_MS) {
				DoMove(d);
				g_btn_last_repeat_ms = now_ms;
			}
		}
	}
}


void gameOver(void) {
	if (gameStatus == 0) {
		BSP_LCD_DisplayStringAt(0, 80, (uint8_t *)"Game over. You lost.", CENTER_MODE);
		BSP_LCD_DisplayStringAt(0, 100, (uint8_t *)"Press \'Reset\' to play again.", CENTER_MODE);
		while(1);
	}
	if (gameStatus == 2) {
		BSP_LCD_DisplayStringAt(0, 80, (uint8_t *)"Congratulations. You won.", CENTER_MODE);
		BSP_LCD_DisplayStringAt(0, 100, (uint8_t *)"Press \'Reset\' to play again.", CENTER_MODE);
		while(1);
	}
}

#if 0
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_PIN) {
  if (GPIO_PIN == IOE_IT_PIN) {
    JoyState = BSP_JOY_GetState();   // DODAJ TO
    switch (JoyState) {
      case JOY_DOWN:  moveDown();  break;
      case JOY_UP:    moveUp();    break;
      case JOY_LEFT:  moveLeft();  break;
      case JOY_RIGHT: moveRight(); break;
      default: break;
    }
  }
}
#endif

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void) {
	/* USER CODE BEGIN Error_Handler_Debug */
	// BSP_LED_On(LED_RED);
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1) {
	}
	/* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{
	/* USER CODE BEGIN 6 */
	/* User can add his own implementation to report the file name and line number,
	   ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
	   /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
