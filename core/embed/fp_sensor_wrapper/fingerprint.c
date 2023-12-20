#include "common.h"
#include "fp_sensor_wrapper.h"
#include "fingerprint.h"

extern uint8_t MAX_USER_COUNT;

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
    ensure_ex(fpsensor_adc_init(12, 12, 16, 3), FPSENSOR_OK, "fpsensor_adc_init failed");
    ensure_ex(fpsensor_set_config_param(0xC0, 8), FPSENSOR_OK, "fpsensor_set_config_param failed");
    ensure_ex(FpAlgorithmInit(TEMPLATE_ADDR_START), FPSENSOR_OK, "FpAlgorithmInit failed");
    MAX_USER_COUNT = MAX_FINGERPRINT_COUNT;
    fingerprint_enter_sleep();
}

void fingerprint_enter_sleep(void)
{
    FpsSleep(256);
    fpsensor_irq_enable();
}

int fingerprint_detect(void)
{
    return FpsDetectFinger();
}

int fingerprint_enroll(uint8_t counter)
{
    if ( FpsDetectFinger() != 1 )
    {
        return -1;
    }
    if ( FpsGetImage() != 0 )
    {
        return -1;
    }

    if ( FpaExtractfeature(counter) != 0 )
    {
        return -1;
    }
    if ( FpaMergeFeatureToTemplate(counter) != 0 )
    {
        return -1;
    }

    return 0;
}

int fingerprint_save(uint8_t id)
{
    if ( id > MAX_USER_COUNT - 1 )
    {
        return -1;
    }
    return FpaEnrollTemplatesave(id);
}

int fingerprint_match(uint8_t* match_id)
{
    if ( FpsDetectFinger() != 1 )
    {
        return -1;
    }
    if ( FpsGetImage() != 0 )
    {
        return -1;
    }

    if ( FpaExtractfeature(0) != 0 )
    {
        return -1;
    }

    if ( FpaIdentify(match_id) != 0 )
    {
        return -1;
    }
    return 0;
}

int fingerprint_delete(uint8_t id)
{
    if ( id > MAX_USER_COUNT - 1 )
    {
        return -1;
    }
    return FpaDeleteTemplateId(id);
}

int fingerprint_delete_all(void)
{
    return FpaClearTemplate();
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
    display_printf("TouchPro Demo Mode\n");
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
    display_printf("TouchPro Demo Mode\n");
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
