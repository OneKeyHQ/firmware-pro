#include "common.h"
#include "fp_sensor_wrapper.h"
#include "fingerprint.h"
#include "irq.h"

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
    ensure_ex(
        fpsensor_adc_init(FPSENSOR_OFFSET, FPSENSOR_GAIN, FPSENSOR_PIXEL, 3), FPSENSOR_OK,
        "fpsensor_adc_init failed"
    );
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

#include "mipi_lcd.h"
#include "ff.h"
#include "mini_printf.h"

double calculate_mean(uint8_t* arr, int size)
{
    int sum = 0;
    for ( int i = 0; i < size; i++ )
    {
        sum += arr[i];
    }
    return (double)(sum / size);
}

double calculate_variance(uint8_t* arr, int size)
{
    double mean = calculate_mean(arr, size);
    double variance = 0;

    for ( int i = 0; i < size; i++ )
    {
        variance += pow(arr[i] - mean, 2);
    }
    variance /= size;

    return variance;
}

FIL frecord;

static void save_image_data(char* step, uint8_t shift, uint8_t gain, uint8_t pixel, uint8_t* data)
{
    FIL fsrc;
    char name[32] = {0};
    double mean = calculate_mean(data, 88 * 112);
    double variance = calculate_variance(data, 88 * 112);
    snprintf(name, 32, "fp_%s_%d_%d_%d_%d_%d.pgm", step, shift, gain, pixel, (int)mean, (int)variance);
    // f_printf(&frecord, "%s\n", name);

    f_open(&fsrc, name, FA_WRITE | FA_CREATE_ALWAYS);
    f_printf(&fsrc, "P5\n88 112\n255\n");
    f_write(&fsrc, data, 88 * 112, NULL);
    f_close(&fsrc);
}

int fingerprint_enroll(uint8_t counter)
{
    uint8_t ret = 0;
    uint8_t image_data[88 * 112 + 2];

    if ( FpsDetectFinger() != 1 )
    {
        return -1;
    }
    ret = FpsGetImageData(image_data);
    if ( ret != 0 )
    {
        return -1;
    }

    display_fp(300, 600, 88, 112, image_data);
    save_image_data("enroll", FPSENSOR_OFFSET, FPSENSOR_GAIN, FPSENSOR_PIXEL, image_data);

    ret = FpsGetImage();
    if ( ret != 0 )
    {
        return -1;
    }

    ret = FpaExtractfeature(counter);
    if ( ret != 0 )
    {
        return ret;
    }
    if ( FpaMergeFeatureToTemplate(counter) != 0 )
    {
        return -1;
    }

    return 0;
}

int fingerprint_save(uint8_t id)
{

    if ( id > MAX_FINGERPRINT_COUNT - 1 )
    {
        return -1;
    }

    if ( FpaEnrollTemplatesave(id) != 0 )
    {
        return -1;
    }
    // fpsensor_data_save();
    return 0;
}

int fingerprint_match(uint8_t* match_id)
{
    volatile int ret = 0;
    uint8_t image_data[88 * 112 + 2];
    uint32_t irq = disable_irq();
    for ( int i = 0; i < 1; i++ )
    {
        if ( FpsDetectFinger() == 1 )
        {
            ret = FpsGetImageData(image_data);
            if ( ret != 0 )
            {
                return -1;
            }

            display_fp(300, 600, 88, 112, image_data);
            save_image_data("match", FPSENSOR_OFFSET, FPSENSOR_GAIN, FPSENSOR_PIXEL, image_data);
            if ( FpsGetImage() == 0 )
            {
                display_fp(300, 600, 88, 112, image_data);
                FpsGetImage();
                if ( FpaExtractfeature(0) == 0 )
                {
                    ret = FpaIdentify(match_id);
                    if ( ret == 0 )
                    {
                        enable_irq(irq);
                        return 0;
                    }
                    else
                    {
                        fpsensor_delay_ms(10);
                    }
                }
            }
        }
    }
    enable_irq(irq);
    return -1;
}

int fingerprint_delete(uint8_t id)
{
    if ( id > MAX_FINGERPRINT_COUNT - 1 )
    {
        return -1;
    }
    if ( FpaDeleteTemplateId(id) != 0 )
    {

        return -1;
    }
    fpsensor_data_save();
    return 0;
}

int fingerprint_delete_all(void)
{
    if ( FpaClearTemplate() != 0 )
    {
        return -1;
    }
    fpsensor_data_save();
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

void fp_test1(void)
{
    int ret = 0;
    uint8_t image_data[88 * 112 + 2];
    uint8_t pixel[4] = {0, 4, 16, 20};
    uint8_t counter = 0;

    f_open(&frecord, "fp_record.txt", FA_WRITE | FA_CREATE_ALWAYS);
    for ( int i = 1; i < 2; i++ )
    {
        for ( int n = 12; n < 24; n++ )
        {
            for ( int m = 10; m < 16; m++ )
            {

                fpsensor_adc_init(n, m, pixel[i], 3);
                display_printf("Finger Detecting...\n");
                while ( fingerprint_detect() != 1 )
                {
                    hal_delay(100);
                    counter++;
                    if ( counter > 5 )
                    {
                        counter = 0;
                        break;
                    }
                }
                ret = FpsGetImageData(image_data);
                if ( ret != 0 )
                {
                    memset(image_data, 0, 88 * 112 + 2);
                }

                display_fp(300, 300, 88, 112, image_data);
                // save_image_data(n,m,pixel[i],image_data);
                hal_delay(100);
            }
        }
    }

    f_close(&frecord);
    display_printf("fp test done\n");
    display_printf("======================\n\n");
}

void fp_test(void)
{
    display_printf("TouchPro Demo Mode\n");
    display_printf("======================\n\n");

    uint8_t count = 0;
    uint8_t fp_list[32];
    bool failed = false;

    // register
    for ( int m = 0; m < 1; m++ )
    {
        display_printf("Finger registre %d...\n", m);
        for ( int i = 0; i < 5; i++ )
        {
            if ( failed )
            {
                failed = false;
                i--;
            }
            display_printf("Finger Detecting...\n");
            while ( fingerprint_detect() != 1 )
                ;
            display_printf("Finger enroll...\n");
            if ( fingerprint_enroll(i) != 0 )
            {
                display_printf("++++fp enroll Fail\n");
                failed = true;
                while ( fingerprint_detect() == 1 )
                    ;
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

// void fp_test(void)
// {
//     display_clear();
//     display_printf("TouchPro Demo Mode\n");
//     display_printf("======================\n\n");

//     uint8_t count = 0;
//     uint8_t fp_list[32];
//     fingerprint_get_count(&count);
//     if ( count != 0 )
//     {
//         fingerprint_delete_all();
//         count = 0;
//     }
//     // register
//     for ( int m = 0; m < 3; m++ )
//     {
//         display_printf("Finger register %d...\n", m);
//         for ( int i = 0; i < 6; i++ )
//         {
//             display_printf("Finger Detecting...\n");
//             while ( fingerprint_detect() != 1 );
//             display_printf("Finger enroll...\n");
//             while (1)
//             {
//                 FP_RESULT ret = fingerprint_enroll(i);
//                 switch (ret)
//                 {
//                 case FP_OK:
//                     display_printf("Finger enroll %d OK\n", i);
//                     break;
//                 case FP_NO_FP:
//                     display_printf("Finger enroll %d Fail, no finger\n", i);
//                     break;
//                 case FP_GET_IMAGE_FAIL:
//                     display_printf("Finger enroll %d Fail, get image fail\n", i);
//                     break;
//                 case FP_EXTRACT_FEATURE_FAIL:
//                     display_printf("Finger enroll %d Fail, extract feature fail\n", i);
//                     break;
//                 case FP_DUPLICATE:
//                     display_printf("Finger enroll %d Fail, duplicate\n", i);
//                     break;
//                 case FP_ERROR_OTHER:
//                     display_printf("Finger enroll %d Fail, other error\n", i);
//                     break;
//                 default:
//                     break;
//                 }
//                 if ( ret == FP_OK )
//                 {
//                     break;
//                 } else if ( ret == FP_NO_FP )
//                 {
//                     while ( fingerprint_detect() == 0 );
//                 }
//             }
//             while ( fingerprint_detect() != 0 );
//         }
//         display_printf("Save finger...\n");
//         while ( fingerprint_detect() == 1 );
//         if ( fingerprint_save(m) != 0 )
//         {
//             display_printf("fp save Fail\n");
//             while ( 1 );
//         }
//     }

//     fingerprint_get_count(&count);
//     display_printf("fp count: %d \n", count);
//     fingerprint_get_list(fp_list, 32);
//     display_printf("fp list: ");
//     for ( int i = 0; i < 10; i++ )
//     {
//         display_printf("%x ", fp_list[i]);
//     }
//     display_printf("\n");

//     // match
//     uint16_t counter = 500;
//     while ( counter-- )
//     {
//         uint8_t match_id;
//         display_printf("Finger Detecting...\n");
//         while ( fingerprint_detect() != 1 );

//         if ( fingerprint_match(&match_id) != 0 )
//         {
//             display_printf("Finger match Fail\n");
//             continue;
//         }
//         display_printf("Finger matched %d \n", match_id);
//         while ( fingerprint_detect() != 0 )
//             ;
//     }
// }

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
