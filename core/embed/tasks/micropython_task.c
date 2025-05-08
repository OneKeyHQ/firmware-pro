#include "micropython_task.h"
#include "cmsis_os2.h"
#include "display.h"
#include "stdio.h"
#include "usart.h"

static void MicroPythonTask(void *pvParameter);

osThreadId_t g_microPythonTasknId;

void CreateMicroPythonTask(void) {
  const osThreadAttr_t micropythonTaskAttributes = {
      .name = "MicroPythonTask",
      .priority = osPriorityNormal,
      .stack_size = 1024,
  };
  g_microPythonTasknId = osThreadNew(MicroPythonTask, NULL, &micropythonTaskAttributes);
  if (g_microPythonTasknId == NULL) {
    printf("Failed to create MicroPythonTask\n");
  }
}

static void MicroPythonTask(void *pvParameter) {
  uint32_t count = 0;
  while (1) {
    printf("MicroPythonTask running,count=%lu\n", count);
    count++;
    osDelay(100);
  }
}
