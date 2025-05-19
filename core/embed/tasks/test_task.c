#include "test_task.h"
#include "cmsis_os2.h"
#include "display.h"
#include "stdio.h"
#include "usart.h"

static void TestTask(void *pvParameter);

osThreadId_t g_testTaskId;

void CreateTestTask(void) {
  const osThreadAttr_t testTaskAttributes = {
      .name = "TestTask",
      .priority = osPriorityNormal,
      .stack_size = 1024,
  };
  g_testTaskId = osThreadNew(TestTask, NULL, &testTaskAttributes);
  // printf("g_testTaskId=0x%lX\n", (uint32_t)g_testTaskId);
  if (g_testTaskId == NULL) {
    printf("Failed to create TestTask\n");
  } else {
    printf("TestTask created successfully\n");
  }
}

static void TestTask(void *pvParameter) {
  // UNUSED(pvParameter);
  printf("Enter TestTask\n");
  //int light = 200;
  uint32_t count = 0;
  while (1) {
    //if (light != 0) {
    //  light = 0;
    //} else {
    //  light = 200;
    //}
    //display_backlight(light);
    printf("Test task running,count=%lu\n", count);
    count++;
    osDelay(1000);
  }
}
