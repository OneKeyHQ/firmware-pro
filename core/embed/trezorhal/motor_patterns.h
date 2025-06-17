#pragma once

#include "motor.h"

// simple patterns
// Whisper
static MOTOR_ACTION MAL_Whisper[] = {
    {.state = MOTOR_FORWARD, .duration_us = 1500},  //
    {.state = MOTOR_REVERSE, .duration_us = 1500},  //
    {.state = MOTOR_BRAKE, .duration_us = 10},      //
};
// Light
static MOTOR_ACTION MAL_Light[] = {
    //
    {.state = MOTOR_FORWARD, .duration_us = 1500},  //
    {.state = MOTOR_REVERSE, .duration_us = 800},   //
    {.state = MOTOR_FORWARD, .duration_us = 1500},  //
    {.state = MOTOR_REVERSE, .duration_us = 800},   //
    {.state = MOTOR_COAST, .duration_us = 10},      //
};
// Medium
static MOTOR_ACTION MAL_Medium[] = {
    {.state = MOTOR_FORWARD, .duration_us = 2080},  //
    {.state = MOTOR_REVERSE, .duration_us = 2080},  //
    {.state = MOTOR_FORWARD, .duration_us = 2080},  //
    {.state = MOTOR_REVERSE, .duration_us = 2080},  //
    {.state = MOTOR_BRAKE, .duration_us = 10},      //
};
// Heavy
static MOTOR_ACTION MAL_Heavy[] = {
    {.state = MOTOR_FORWARD, .duration_us = 2080},  //
    {.state = MOTOR_REVERSE, .duration_us = 2080},  //
    {.state = MOTOR_FORWARD, .duration_us = 2080},  //
    {.state = MOTOR_REVERSE, .duration_us = 2080},  //
    {.state = MOTOR_FORWARD, .duration_us = 2080},  //
    {.state = MOTOR_REVERSE, .duration_us = 2080},  //
    {.state = MOTOR_BRAKE, .duration_us = 10},      //
};
// Relax (Coast)
static MOTOR_ACTION MAL_relax[] = {
    {.state = MOTOR_COAST, .duration_us = 50},  //
};

// sequence patterns
static void seq_Success(MOTOR_ACTION* act_list, size_t* act_list_len) {
  *act_list_len = 0;
  MOTOR_ACTION* idx_p = act_list;

  // Medium
  memcpy(idx_p, MAL_Medium, sizeof(MAL_Medium));
  idx_p += sizeof(MAL_Medium) / sizeof(MOTOR_ACTION);

  // interval: 50ms
  *idx_p = (MOTOR_ACTION){.state = MOTOR_COAST, .duration_us = 50000};
  idx_p++;
  // interval: 50ms
  *idx_p = (MOTOR_ACTION){.state = MOTOR_COAST, .duration_us = 50000};
  idx_p++;

  // Heavy
  memcpy(idx_p, MAL_Heavy, sizeof(MAL_Heavy));
  idx_p += sizeof(MAL_Heavy) / sizeof(MOTOR_ACTION);

  *idx_p = (MOTOR_ACTION){.state = MOTOR_COAST, .duration_us = 65535};
  idx_p++;

  *act_list_len = idx_p - act_list;
}

static void seq_Warning(MOTOR_ACTION* act_list, size_t* act_list_len) {
  *act_list_len = 0;
  MOTOR_ACTION* idx_p = act_list;

  // Heavy
  memcpy(idx_p, MAL_Heavy, sizeof(MAL_Heavy));
  idx_p += sizeof(MAL_Heavy) / sizeof(MOTOR_ACTION);

  // interval: 50ms
  *idx_p = (MOTOR_ACTION){.state = MOTOR_COAST, .duration_us = 50000};
  idx_p++;
  // interval: 50ms
  *idx_p = (MOTOR_ACTION){.state = MOTOR_COAST, .duration_us = 50000};
  idx_p++;

  // Medium
  memcpy(idx_p, MAL_Medium, sizeof(MAL_Medium));
  idx_p += sizeof(MAL_Medium) / sizeof(MOTOR_ACTION);

  *idx_p = (MOTOR_ACTION){.state = MOTOR_COAST, .duration_us = 65535};
  idx_p++;

  *act_list_len = idx_p - act_list;
}

static void seq_Error(MOTOR_ACTION* act_list, size_t* act_list_len) {
  *act_list_len = 0;
  MOTOR_ACTION* idx_p = act_list;

  // Medium
  memcpy(idx_p, MAL_Medium, sizeof(MAL_Medium));
  idx_p += sizeof(MAL_Medium) / sizeof(MOTOR_ACTION);

  // interval: 50ms
  *idx_p = (MOTOR_ACTION){.state = MOTOR_COAST, .duration_us = 65535};
  idx_p++;

  // Medium
  memcpy(idx_p, MAL_Medium, sizeof(MAL_Medium));
  idx_p += sizeof(MAL_Medium) / sizeof(MOTOR_ACTION);

  // interval: 50ms
  *idx_p = (MOTOR_ACTION){.state = MOTOR_COAST, .duration_us = 65535};
  idx_p++;

  // Heavy
  memcpy(idx_p, MAL_Heavy, sizeof(MAL_Heavy));
  idx_p += sizeof(MAL_Heavy) / sizeof(MOTOR_ACTION);

  // interval: 50ms
  *idx_p = (MOTOR_ACTION){.state = MOTOR_COAST, .duration_us = 65535};
  idx_p++;

  // Medium
  memcpy(idx_p, MAL_Medium, sizeof(MAL_Medium));
  idx_p += sizeof(MAL_Medium) / sizeof(MOTOR_ACTION);

  *idx_p = (MOTOR_ACTION){.state = MOTOR_COAST, .duration_us = 65535};
  idx_p++;

  *act_list_len = idx_p - act_list;
}

static void seq_Slide(MOTOR_ACTION* act_list, size_t* act_list_len) {
  *act_list_len = 0;
  MOTOR_ACTION* idx_p = act_list;

  // Whisper
  memcpy(idx_p, MAL_Whisper, sizeof(MAL_Whisper));
  idx_p += sizeof(MAL_Whisper) / sizeof(MOTOR_ACTION);

  *idx_p = (MOTOR_ACTION){.state = MOTOR_COAST, .duration_us = 50000};
  idx_p++;
  // Light
  memcpy(idx_p, MAL_Light, sizeof(MAL_Light));
  idx_p += sizeof(MAL_Light) / sizeof(MOTOR_ACTION);

  *idx_p = (MOTOR_ACTION){.state = MOTOR_COAST, .duration_us = 50000};
  idx_p++;
  // Medium
  memcpy(idx_p, MAL_Medium, sizeof(MAL_Medium));
  idx_p += sizeof(MAL_Medium) / sizeof(MOTOR_ACTION);

  *idx_p = (MOTOR_ACTION){.state = MOTOR_COAST, .duration_us = 50000};
  idx_p++;
  // Heavy
  memcpy(idx_p, MAL_Heavy, sizeof(MAL_Heavy));
  idx_p += sizeof(MAL_Heavy) / sizeof(MOTOR_ACTION);

  *idx_p = (MOTOR_ACTION){.state = MOTOR_COAST, .duration_us = 50000};
  idx_p++;

  *act_list_len = idx_p - act_list;
}
