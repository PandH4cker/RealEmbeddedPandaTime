/**
  ******************************************************************************
  * @file    main.c
  * @author  MCD Application Team
  * @version V1.0.1
  * @date    26-February-2014
  * @brief   This file provides main program functions
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2014 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */
/* Mise a jour du 31/12/2018 : P. Foubet
 *
 */
/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "interfaces.h"
#include "display.h"
#include "semphr.h"

#define MODE 2 // 0 = graphes, 1 = console, 2 = Console+Trace
#define STACK_SIZE 128 // A ajuster !!

xSemaphoreHandle writer;
int needToWrite = 0;
char * message = NULL;


/* sleep en millisecondes */
void msleep (uint32_t ms)
{
TickType_t t = ms / portTICK_PERIOD_MS;
  // Idem t = pdMS_TO_TICKS(ms);
  vTaskDelay(t ? t : 1);
}

/* Threads functions ---------------------------------------------------------*/

#define NLMIN 5  // No ligne de depart
#define NLMAX 35 // No de ligne maxi. de l'ecran
#define HLPIX 9  // Nb de pixels hauteur font
void affTexte(char *m)
{
static int l=NLMIN;
	//GUI_DispStringInRectWrap(m, &Rect, GUI_TA_LEFT, GUI_WRAPMODE_CHAR);
	GUI_DispStringAtCEOL(" ",0,l*HLPIX);
	GUI_DispStringAtCEOL(m,0,l*HLPIX);
	l++;
	if (l==NLMAX) l=NLMIN;
}

void Graphe_Thread(void *p)
{
int i=0;
char buf[100];
	 /* Init the STemWin GUI Library */
	 GUI_Init();
	 GUI_Initialized = 1;

	 /* Initialize RTC and Backup */
	 RTC_Init();

	 /* Activate the use of memory device feature */
	 WM_SetCreateFlags(WM_CF_MEMDEV);

	 /* Do the calibration if needed */
	 CALIBRATION_Check();

	 /* Start graph */
	 //initCons();
	 if (MODE) display2();
	 while(1) {
	   if (MODE == 0) display();
	   else { // MODE = 1 | 2
		 sprintf(buf,"Ligne du message no %d\n",i++);
		 affTexte(buf);
		 msleep(50);
	   }
	 }


}

void Print_Thread(void * args)
{
	while(1)
	{
		if (writer != NULL)
			if (xSemaphoreTake(writer, (TickType_t) 10) == pdTRUE)
			{
				if (MODE && needToWrite)
				{
					affTexte(message);
					msleep(50);
					needToWrite = 0;
				}
				xSemaphoreGive(writer);
			}
	}
}

void printToConsole(char * m)
{
	message = m;
	needToWrite = 1;
}

void Cursor_Thread(void *p)
{
	GUI_Init();
	GUI_Initialized = 1;
	RTC_Init();
	WM_SetCreateFlags(WM_CF_MEMDEV);
	//CALIBRATION_Check();
	display2();

	while(1) {
	  if (MODE) msleep(100); // Toutes les secondes
	  else msleep(50);
      /* Capture input event and update cursor */
      if (GUI_Initialized == 1)  {
    	  BSP_Pointer_Update();
    	  TS_StateTypeDef  ts;
    	  BSP_TS_GetState(&ts);
    	  if (MODE==2)
    	  {
    		  if (ts.TouchDetected)
    		  {
				  if (writer != NULL)
					  if (xSemaphoreTake(writer, (TickType_t) 10) == pdTRUE)
					  {
						  printToConsole("Ecran appuyé\n");
						  xSemaphoreGive(writer);
					  }
    		  }
    	  }
      }
	}
}

/**
  * @brief  Main program.
  * @param  None
  * @retval None
  */
int main(void)
{
  interfaces_init();

  writer = xSemaphoreCreateMutex();

  /* Creation des threads */
  if (!(pdPASS == xTaskCreate( Print_Thread, "Printer" ,STACK_SIZE*4,(void *) NULL,1,NULL ))) goto err;
  if (!(pdPASS == xTaskCreate( Cursor_Thread, "Curseur",STACK_SIZE*4,NULL,2,NULL ))) goto err;

  /* on lance le systeme ! */
  vTaskStartScheduler();

err:              // En principe jamais atteint !
  while(1);
  return 0;
}



