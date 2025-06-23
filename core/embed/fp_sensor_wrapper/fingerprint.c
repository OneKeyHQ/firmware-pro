#include "common.h"
#ifndef EMULATOR
  #include "fp_sensor_wrapper.h"
#endif
#include "fingerprint.h"
#include "irq.h"
#ifdef SYSTEM_VIEW
  #include "systemview.h"
  #include "mipi_lcd.h"
#endif
extern uint8_t MAX_USER_COUNT;

static bool fingerprint_module_status = false;

bool fingerprint_module_status_get(void)
{
    return fingerprint_module_status;
}

#ifdef EMULATOR
void fingerprint_get_version(char* version)
{
    strcpy(version, "1.0.0");
}
int fingerprint_detect(void)
{
    return 1;
}
int fingerprint_enroll(uint8_t counter)
{
    (void)counter;
    return 0;
}
int fingerprint_save(uint8_t index)
{
    (void)index;
    return 0;
}
int fingerprint_match(uint8_t* match_id)
{
    (void)match_id;
    return 0;
}
int fingerprint_delete(uint8_t id)
{
    (void)id;
    return 0;
}
int fingerprint_delete_all(void)
{
    return 0;
}
int fingerprint_get_count(uint8_t* count)
{
    (void)count;
    return 0;
}
int fingerprint_get_list(uint8_t* list, uint8_t len)
{
    (void)list;
    (void)len;
    return 0;
}
#else
void fingerprint_get_version(char* version)
{
    FpLibVersion(version);
}
void fingerprint_init(void)
{
    ensure_ex(fpsensor_gpio_init(), FPSENSOR_OK, "fpsensor_gpio_init failed");
    ensure_ex(fpsensor_spi_init(), FPSENSOR_OK, "fpsensor_spi_init failed");
    ensure_ex(fpsensor_hard_reset(), FPSENSOR_OK, "fpsensor_hard_reset failed");
    ensure_ex(fpsensor_init(), FPSENSOR_OK, "fpsensor_init failed");
    ensure_ex(fpsensor_adc_init(18, 13, 4, 3), FPSENSOR_OK, "fpsensor_adc_init failed");
    ensure_ex(fpsensor_set_config_param(0x3c, 10), FPSENSOR_OK, "fpsensor_set_config_param failed");
    if ( FpAlgorithmInit(TEMPLATE_ADDR_START) == FPSENSOR_OK )
    {
        fingerprint_module_status = true;
    }
    else
    {
        return;
    }
    MAX_USER_COUNT = MAX_FINGERPRINT_COUNT;
    fingerprint_enter_sleep();
}

int fingerprint_enter_sleep(void)
{
    if ( !fingerprint_module_status )
    {
        return -1;
    }
    if ( FpsSleep(256) != 0 )
    {
        return -1;
    }
    fpsensor_irq_enable();
    return 0;
}

int fingerprint_detect(void)
{
    if ( !fingerprint_module_status )
    {
        return 0;
    }
    return FpsDetectFinger();
}

FP_RESULT fingerprint_enroll(uint8_t counter)
{

    if ( FpsDetectFinger() != 1 )
    {
        return FP_NO_FP;
    }
  #if SYSTEM_VIEW
    uint8_t image_data[88 * 112 + 2];
    if ( FpsGetImageData(image_data) != 0 )
    {
        return -1;
    }

    display_fp(300, 600, 88, 112, image_data);
  #endif
    if ( FpsGetImage() != 0 )
    {
        return FP_GET_IMAGE_FAIL;
    }
    uint8_t res = FpaExtractfeature(counter);
    if ( res == 2 )
    {
        return FP_DUPLICATE;
    }
    if ( res != 0 )
    {
        return FP_EXTRACT_FEATURE_FAIL;
    }
    if ( FpaMergeFeatureToTemplate(counter) != 0 )
    {
        return FP_ERROR_OTHER;
    }
    return FP_OK;
}

int fingerprint_register_template(uint8_t id)
{

    if ( id > fpsensor_get_max_template_count() - 1 )
    {
        return -1;
    }
    if ( FpaEnrollTemplatesave(id) != 0 )
    {
        return -1;
    }
    return 0;
}

int fingerprint_save(uint8_t index)
{
    fpsensor_data_save(index);
    return 0;
}

void fingerprint_get_group(uint8_t group[8])
{
    fpsensor_data_get_group(group);
}

FP_RESULT fingerprint_match(uint8_t* match_id)
{
    volatile int ret = 0;
    uint32_t irq = disable_irq();
    if ( FpsDetectFinger() != 1 )
    {
        enable_irq(irq);
        return FP_NO_FP;
    }
  #if SYSTEM_VIEW
    uint8_t image_data[88 * 112 + 2];
    if ( FpsGetImageData(image_data) != 0 )
    {
        enable_irq(irq);
        return -1;
    }
    else
    {
        display_fp(300, 600, 88, 112, image_data);
    }
  #endif
    if ( FpsGetImage() != 0 )
    {
        enable_irq(irq);
        return FP_GET_IMAGE_FAIL;
    }
    if ( FpaExtractfeature(0) != 0 )
    {
        enable_irq(irq);
        return FP_EXTRACT_FEATURE_FAIL;
    }
    ret = FpaIdentify(match_id);
    enable_irq(irq);
    if ( ret != 0 )
    {
        return FP_NOT_MATCH;
    }
    return FP_OK;
}

int fingerprint_delete(uint8_t id)
{
    if ( id > fpsensor_get_max_template_count() - 1 )
    {
        return -1;
    }
    if ( FpaDeleteTemplateId(id) != 0 )
    {

        return -1;
    }
    fpsensor_data_delete(false, id);
    return 0;
}

int fingerprint_delete_group(uint8_t group_id[4])
{
    fpsensor_data_delete_group(group_id);
    return 0;
}

int fingerprint_delete_all(void)
{
    if ( FpaClearTemplate() != 0 )
    {
        return -1;
    }
    fpsensor_data_delete(true, 0);
    return 0;
}

int fingerprint_get_count(uint8_t* count)
{
    return FpaGetTemplateNum(count);
}

int fingerprint_get_list(uint8_t* list, uint8_t len)
{
    uint8_t fp_list[32];
    if ( FpaGetTemplateIDlist(fp_list) != 0 )
    {
        return -1;
    }

    len = len > 32 ? 32 : len;
    memcpy(list, fp_list, len);
    return 0;
}

void fp_test(void)
{
    display_printf("Function Test\n");
    display_printf("%s\n", __func__);
    display_printf("======================\n\n");

    uint8_t count = 0;
    uint8_t fp_list[32];

    // register
    for ( int m = 0; m < 3; m++ )
    {
        display_printf("Finger registre %d...\n", m);
        for ( int i = 0; i < 5; i++ )
        {
            display_printf("Finger Detecting...\n");
            while ( fingerprint_detect() != 1 )
                ;
            display_printf("Finger enroll...\n");
            if ( fingerprint_enroll(i) != 0 )
            {
                display_printf("fp enroll Fail\n");
                continue;
            }

            display_printf("Remove finger...\n");
            while ( fingerprint_detect() == 1 )
                ;
        }
        if ( fingerprint_save(m) != 0 )
        {
            display_printf("fp save Fail\n");
            while ( 1 )
                ;
        }
    }

    fingerprint_get_count(&count);
    display_printf("fp count: %d\n", count);

    fingerprint_get_list(fp_list, 32);
    display_printf("fp list: ");
    for ( int i = 0; i < 10; i++ )
    {
        display_printf("%x ", fp_list[i]);
    }

    // match
    while ( 1 )
    {
        uint8_t match_id;
        display_printf("Finger Detecting...\n");
        while ( fingerprint_detect() != 1 )
            ;

        if ( fingerprint_match(&match_id) != 0 )
        {
            display_printf("Finger match Fail\n");
            continue;
        }
        display_printf("Finger matched %d \n", match_id);
        while ( fingerprint_detect() != 0 )
            ;
    }
}

void fingerprint_test(void)
{
    display_printf("Function Test\n");
    display_printf("%s\n", __func__);
    display_printf("======================\n\n");
    char fpver[32];
    FpLibVersion(fpver);
    display_printf("FP Lib - %s\n", fpver);
    display_printf("FP Init...");

    uint8_t finger_index = 0;

    // register
    for ( int i = 0; i < 5; i++ )
    {
        display_printf("Finger Detecting...\n");
        while ( FpsDetectFinger() != 1 )
            ;
        display_printf("Finger Getting Image...\n");
        if ( FpsGetImage() != 0 )
        {
            display_printf("FpsGetImage Fail\n");
            continue;
        }

        if ( FpaExtractfeature(i) != 0 )
        {
            display_printf("FpaExtractfeature Fail\n");
            continue;
        }

        if ( FpaMergeFeatureToTemplate(i) != 0 )
        {
            display_printf("FpaMergeFeatureToTemplate Fail\n");
            continue;
        }
        display_printf("Remove finger...\n");
        while ( FpsDetectFinger() == 1 )
            ;
    }
    if ( FpaEnrollTemplatesave(finger_index) != 0 )
    {
        display_printf("FpaEnrollTemplatesave Fail\n");
        while ( 1 )
            ;
    }

    // match
    while ( 1 )
    {
        uint8_t match_id;
        display_printf("Finger Detecting...\n");
        while ( FpsDetectFinger() != 1 )
            ;

        if ( FpsGetImage() != 0 )
        {
            display_printf("FpsGetImage Fail\n");
            continue;
        }

        if ( FpaExtractfeature(0) != 0 )
        {
            display_printf("FpaExtractfeature Fail\n");
            continue;
        }

        display_printf("remove finger...\n");

        if ( FpaIdentify(&match_id) != 0 )
        {
            display_printf("FpaIdentify Fail\n");
            continue;
        }
        display_printf("FpaIdentify matched\n");
    }
}
#endif
