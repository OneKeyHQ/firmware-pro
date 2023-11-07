#include "fpsensor_common.h"
#include "fpsensor_driver.h"
// #include <math.h>

#define R_FULL 200.0 // Glass Cover Modules

#if 1
    /************/
  #define R_MID       150.0
  #define R_MID_TH_L  (R_MID - 25) // 110
  #define R_MID_TH_H  (R_MID + 25) // 140

  #define R_GAIN_TH_L 0.8
  #define R_GAIN_TH_H 1.0
#else
    /*Docment Parameter*/
  #define R_MID       140.0
  #define R_MID_TH_L  (R_MID - 25) // 100
  #define R_MID_TH_H  (R_MID + 25) // 150

  #define R_GAIN_TH_L 0.8
  #define R_GAIN_TH_H 1.0
#endif

#ifndef FPSENSOR_TYPE_FPC1021
  #define R_SG 5.62f //
static uint8_t adj_gen1[] = {15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 0, 0};
static float coverage_table[] = {0.102, 0.108, 0.116, 0.124, 0.134, 0.146, 0.159, 0.176, 0.196,
                                 0.221, 0.254, 0.299, 0.362, 0.457, 0.616, 0.776, 0.873, 1.0};
#else
  #define R_SG 7.20f // fpc
static uint8_t adj_gen1[] = {15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
static float coverage_table[] = {0.176, 0.183, 0.193, 0.205, 0.220, 0.238, 0.256, 0.283,
                                 0.306, 0.340, 0.383, 0.437, 0.507, 0.607, 0.755, 1.0};
#endif

#define MIN_AVERAGE  50
#define MAX_AVERAGE  200
#define MIN_VARIANCE 40
#define MIN_RANGE    180

#define CROP_HEIGHT  10
#define CROP_WIDTH   80
#define ADJUST_TIMES 30

#ifdef CAC_TEST_MULTI_IMAGE
uint8_t flag = TRUE;
extern fpsensor_adc_t adc_fixed;
#endif

#ifdef CAC_TEST_UP_PARAMETER
fpsensor_adc_t adc_paras[ADJUST_TIMES];
fpsensor_adc_t* ptr_temp;
fpsensor_adc_paralog_t adc_paralog;
#endif

uint32_t calculate_average(uint8_t* image, int32_t height, int32_t width)
{
    uint32_t sum = 0;
    uint32_t avg = 0;
    if ( image != NULL )
    {
        for ( int i = 0; i < height; i++ )
            for ( int j = 0; j < width; j++ )
                sum += image[i * width + j];
        avg = sum / ((uint32_t)height * (uint32_t)width);
    }

    return avg;
}

uint8_t get_max(uint8_t* image, int32_t height, int32_t width)
{
    uint8_t max = 0;
    if ( image != NULL )
    {
        for ( int i = 0; i < height; i++ )
            for ( int j = 0; j < width; j++ )
            {
                if ( image[i * width + j] > max )
                {
                    max = image[i * width + j];
                    if ( max == 255 )
                        return max;
                }
            }
    }
    return max;
}

uint8_t get_min(uint8_t* image, int32_t height, int32_t width)
{
    uint8_t min = 255;
    if ( image != NULL )
    {
        for ( int i = 0; i < height; i++ )
            for ( int j = 0; j < width; j++ )
            {
                if ( image[i * width + j] < min )
                {
                    min = image[i * width + j];
                    if ( min == 0 )
                        return min;
                }
            }
    }
    return min;
}

uint8_t compare_adc(fpsensor_adc_t* pre_adc, fpsensor_adc_t* next_adc)
{
    if ( next_adc->shift == pre_adc->shift && next_adc->gain == pre_adc->gain &&
         next_adc->pixel == pre_adc->pixel && next_adc->et1 == pre_adc->et1 )
        return TRUE;
    return FALSE;
}

uint8_t shift_up_pixel(fpsensor_adc_t* adc)
{
    uint8_t result = FALSE;
    switch ( adc->pixel )
    {
    case 0:
        adc->pixel = 2;
        result = TRUE;
        break;
    case 2:
        adc->pixel = 8;
        result = TRUE;
        break;
    case 8:
        adc->pixel = 10;
        result = TRUE;
        break;
    case 10:
        break;
    default:
        break;
    }
    return result;
}
uint8_t shift_down_pixel(fpsensor_adc_t* adc)
{
    uint8_t result = FALSE;
    switch ( adc->pixel )
    {
    case 0:
        break;
    case 2:
        adc->pixel = 0;
        result = TRUE;
        break;
    case 8:
        adc->pixel = 2;
        result = TRUE;
        break;
    case 10:
        adc->pixel = 8;
        result = TRUE;
        break;
    default:
        break;
    }
    return result;
}

void fpsensor_cac_init(fpsensor_adc_t* adc)
{
#ifdef CAC_TEST_MULTI_IMAGE
    adc->shift = adc_fixed.shift;
    adc->gain = adc_fixed.gain;
    adc->pixel = adc_fixed.pixel;
    adc->et1 = adc_fixed.et1;
#endif

#ifdef CAC_TEST_UP_PARAMETER
    adc_paralog.count = 0;
    memset(adc_paras, 0, sizeof(fpsensor_adc_t) * sizeof(adc_paras));
    ptr_temp = adc_paras;
    adc_paralog.paras = &ptr_temp;
#endif
}

void cac_shift_gain(uint8_t r_mid, uint8_t r_gain, fpsensor_adc_t* adc)
{
    int shift_new;
    float coverage;
    int coverage_index_pre = 15 - adc->gain;

    float abs_gain = r_gain * coverage_table[coverage_index_pre];
    for ( int i = 0; i < sizeof(coverage_table); i++ )
    {
        if ( coverage_table[i] > abs_gain )
        {
            adc->gain = adj_gen1[i];
            coverage = coverage_table[i];
            break;
        }
    }
    float r_mid_change = r_mid * (coverage_table[coverage_index_pre] / coverage);

    int shift_change = (int)((r_mid_change - R_MID) / R_SG * coverage / (1));

    shift_new = adc->shift + shift_change;
    if ( shift_new >= 31 )
    {
        adc->shift = 31;
    }
    else if ( shift_new < 0 )
    {
        adc->shift = 0;
    }
    else
    {
        adc->shift = (uint8_t)shift_new;
    }
}
uint8_t fpsensor_cac2(uint8_t* image, int32_t height, int32_t width, fpsensor_adc_t* adc)
{
    uint8_t r_max = get_max(image, height, width);
    uint8_t r_min = get_min(image, height, width);
    uint8_t r_mid /* = 255 - (r_max + r_min)/2*/;

    r_mid = 255 - calculate_average(image, height, width);

    fpsensor_adc_t pre_adc;
    pre_adc.shift = adc->shift;
    pre_adc.gain = adc->gain;
    pre_adc.pixel = adc->pixel;
    pre_adc.et1 = adc->et1;

    float r_gain = (r_max - r_min) / R_FULL;

    if ( r_mid <= R_MID_TH_L )
    {
        if ( adc->shift > 0 )
            adc->shift--;
        else
            shift_down_pixel(adc);

        return FALSE;
    }
    else if ( r_mid >= R_MID_TH_H )
    {
        if ( adc->shift < 31 )
            adc->shift++;
        else
            shift_up_pixel(adc);
        return FALSE;
    }
    else
    {
        if ( r_gain >= R_GAIN_TH_L && r_gain <= R_GAIN_TH_H )
        {
            return TRUE;
        }
        else
        {
            cac_shift_gain(r_mid, r_gain, adc);
        }
    }
    return compare_adc(&pre_adc, adc) ? TRUE : FALSE;
}
#ifndef NEW_CAC
uint8_t fpsensor_run_cac(uint8_t* buffer, uint32_t length, fpsensor_adc_t* adc)
{
    uint8_t count = 0;
    uint8_t status = 0;
    uint8_t soft_irq = 0;
    uint8_t result = FALSE;

    uint32_t rows_start = (FPSENSOR_IMAGE_HEIGHT - CROP_HEIGHT) / 2;
    uint32_t cols_start = (FPSENSOR_IMAGE_WIDTH - CROP_WIDTH) / 2;
    if ( buffer == NULL || length < CROP_HEIGHT * CROP_WIDTH + 2 )
    {
        return FPSENSOR_BUFFER_ERROR;
    }
  #ifndef CAC_TEST_MULTI_IMAGE
    fpsensor_cac_init(adc);
  #endif
    while ( !result )
    {
        status |= fpsensor_set_capture_crop(buffer, length, rows_start, CROP_HEIGHT, cols_start, CROP_WIDTH);
        status |= fpsensor_set_adc(adc);
        status |= fpsensor_capture_image();
        while ( !soft_irq )
        {
            fpsensor_delay_ms(1);
            status |= fpsensor_read_irq_with_clear(buffer, length);
            soft_irq = buffer[1] & FPSENSOR_IRQ_REG_BIT_FIFO_NEW_DATA;

            if ( FPSENSOR_OK != status )
            {
                LOG("Fpsensor read image soft irq failed, err: 0x%02x\n", status);
            }
        }
        status |= fpsensor_get_img_data(buffer, CROP_HEIGHT * CROP_WIDTH + 2);
  #if defined(CAC_TEST_UP_PARAMETER)
        adc_paras[count].shift = adc->shift;
        adc_paras[count].gain = adc->gain;
        adc_paras[count].pixel = adc->pixel;
        adc_paras[count].et1 = adc->et1;
        adc_paralog.count = count + 1;
  #endif
  #if defined(CAC_TEST_MULTI_IMAGE)
        flag = fpsensor_cac2(buffer + 2, CROP_HEIGHT, CROP_WIDTH, adc);
        LOG("Fpsensor run cac, shift = %d gain = %d pixel = %d \n", adc->shift, adc->gain, adc->pixel);
        result = TRUE;
  #else
        result = fpsensor_cac2(buffer + 2, CROP_HEIGHT, CROP_WIDTH, adc);
  #endif

        if ( ++count > ADJUST_TIMES )
        {
            adc->shift = adc_fixed.shift;
            adc->gain = adc_fixed.gain;
            adc->pixel = adc_fixed.pixel;
            break;
        }
    }

    return status;
}
#endif
/*****************************************************************
CAC 未调用函数
******************************************************************/
#if 0
fpsensor_adc_t *adc_pointer = NULL;

uint32_t calculate_histgram(uint8_t *image, uint8_t *mask, int32_t height, int32_t width, uint32_t hist[256])
{

	uint32_t cnt_masked = 0;
	int32_t  hei_i, wid_j;
	if(image == NULL ||	hist == NULL)
	{
		return FPSENSOR_BUFFER_ERROR;
	}

	memset(hist, 0, 256*sizeof(uint32_t));
	
	for(hei_i = 0; hei_i < height; hei_i++)
	{
		for(wid_j = 0; wid_j < width; wid_j++)
		{
			if(mask != NULL && mask[wid_j+hei_i*width] == 0)
			{
				cnt_masked++;
			}
			else
			{
				hist[image[wid_j+hei_i*width]]++;
			}
		}
	}
	return cnt_masked;
}



uint32_t calculate_average_trim_ends(uint8_t *image, int32_t height, int32_t width)
{
	uint32_t sum = 0;
	uint32_t avg = 0;
	uint32_t cnt = 0;
	if(image != NULL)
	{
		for(int i=0; i<height; i++)
			for(int j=0; j<width; j++)
		{
			if(image[i*width + j] != 255 && image[i*width + j] != 0)
			{
				sum += image[i*width + j];
				cnt++;
			}
		}				
		avg = sum/cnt;
	}
	return avg;
}

uint32_t calculate_variance(uint8_t *image,  int32_t height, int32_t width, uint32_t* average_in)
{
	uint32_t sum = 0;
	uint32_t var = 0;
	uint32_t average;
	if(image == NULL)
	{
		if(average_in == NULL)
			average = calculate_average(image, height, width);
		else average = *average_in;
		
		for(int i=0; i<height; i++)
			for(int j=0; j<width; j++)
			{
				sum += (image[i*width+j]-average)*(image[i*width+j]-average);
			}
		
		var = sum/((uint32_t)height*(uint32_t)width);
	}
	return var;
} 

uint8_t get_max_trim_ends(uint8_t *image, int32_t height, int32_t width)
{
	uint8_t max = 0;
	if(image != NULL)
	{
		for(int i=0; i<height; i++)
			for(int j=0; j<width; j++)
			{
				if(image[i*width+j] != 255 && image[i*width+j] > max)
				{
					max = image[i*width+j];
				}
			}
	}
	return max;
}

uint8_t get_min_trim_ends(uint8_t *image, int32_t height, int32_t width)
{
	uint8_t min = 255;
	if(image == NULL)
	{
		for(int i=0; i<height; i++)
			for(int j=0; j<width; j++)
			{
				if(image[i*width+j] != 0 && image[i*width+j] < min)
				{
					min = image[i*width+j];
				}
			}
	}
	return min;
}


uint32_t calculate_dot_product(uint32_t *oper1, uint32_t *oper2, uint32_t len)
{
	uint32_t result = 0;
	for (int i = 0; i<len; i++)
	{
		result += oper1[i] * oper2[i];
	}
	return result;
}
uint8_t find_hist_peak(uint32_t hist[256], uint8_t *peak)
{
	uint32_t conv_result[256];

	uint32_t conv_operator[] = { 1, 2, 3, 2, 1 };

	uint32_t length = sizeof(conv_operator) / sizeof(uint32_t);

	int32_t half_len = length / 2;

	uint32_t conv_temp[sizeof(conv_operator) / sizeof(uint32_t)-1];

	for (int i = 0; i<half_len; i++)
	{
		int k = 0;
		for (int j = half_len - i; j<length; j++)
		{
			conv_temp[k] = conv_operator[j];
			if (conv_operator[j] < conv_operator[half_len - i])
				conv_temp[k] *= 2;
			k++;
		}
		conv_result[i] = calculate_dot_product(conv_temp, hist, k);

		k = 0;
		for (int j = 0; j<=half_len + i; j++)
		{
			conv_temp[k] = conv_operator[j];
			if (conv_operator[j] < conv_operator[half_len + i])
				conv_temp[k] *= 2;
			k++;
		}
		conv_result[255 - i] = calculate_dot_product(conv_temp, &hist[256 - k], k);

	}

	for (int i = half_len; i<256 - half_len; i++)
	{
		conv_result[i] = calculate_dot_product(conv_operator, &hist[i - half_len], length);
	}

	uint32_t max = 0;
	for(int i=1; i<sizeof(conv_result); i++)
	{
		if(conv_result[i] > max)
		{
			max = conv_result[i];
			*peak = i;
		}
	}

	return 0;
}


uint8_t find_pixel_max(uint32_t hist[256])
{
	for(int32_t i=255; i >= 0; i--)
	{
		if(hist[i] != 0)
		{
			return i;
		}
	}
	return 0;
}

uint8_t find_pixel_min(uint32_t hist[256])
{
	for(int32_t i=0; i < 256; i++)
	{
		if(hist[i] != 0)
		{
			return i;
		}
	}
	return 255;
}

fpsensor_finger_type_t judge_gray_average(uint32_t avg)
{
  #define SHIFT_STEP 1

	if(avg > MAX_AVERAGE + MIN_AVERAGE/2)
	{
		if(adc_pointer->shift <= SHIFT_STEP*2)
		{
			shift_down_pixel(adc_pointer) ? (adc_pointer->shift += SHIFT_STEP/2) : (adc_pointer->shift = 0);
		}
		else
		{
			adc_pointer->shift -= (SHIFT_STEP*2);
		}
		return VERY_DRY_FINGER;
	}
	else if(avg > MAX_AVERAGE)
	{
		if(adc_pointer->shift <= SHIFT_STEP)
		{
			shift_down_pixel(adc_pointer) ? (adc_pointer->shift += SHIFT_STEP/2) : (adc_pointer->shift = 0);
		}
		else
		{
			adc_pointer->shift -= SHIFT_STEP;
		}
		return DRY_FINGER;
	}
	else if(avg < MIN_AVERAGE/2)
	{
		if(adc_pointer->shift > 31 - SHIFT_STEP*2) //max shift = 31
		{
			shift_up_pixel(adc_pointer) ? (adc_pointer->shift -= SHIFT_STEP/2) : (adc_pointer->shift = 31);
		}
		else
		{
			adc_pointer->shift += SHIFT_STEP*2;
		}
		return VERY_WET_FINGER;
	}
	else if(avg < MIN_AVERAGE)
	{
		if(adc_pointer->shift > 31 - SHIFT_STEP) //
		{
			shift_up_pixel(adc_pointer) ? (adc_pointer->shift -= SHIFT_STEP/2) : (adc_pointer->shift = 31);
		}
		else
		{
			adc_pointer->shift += SHIFT_STEP;
		}
		return WET_FINGER;
	}
	else
	{
		return NORMAL_FINGER;
	}
}

uint8_t judge_gray_variance(uint32_t var, uint32_t avg)
{
  #define GAIN_STEP  1
	uint32_t square_var = MIN_VARIANCE * MIN_VARIANCE;
	
	if(var < square_var)
	{

		if(adc_pointer->gain >= 14)
		{
			adc_pointer->gain = 15; //max gain value = 15
		}
		else
		{
			adc_pointer->gain += GAIN_STEP;
		}
		if(avg < MAX_AVERAGE && avg > MAX_AVERAGE - MIN_AVERAGE/2)
		{
			adc_pointer->shift--;
		}
		if(avg > MIN_AVERAGE && avg < MIN_AVERAGE*3/2)
		{
			adc_pointer->shift++;
		}
		return FALSE;
	}

	return TRUE;
}

uint8_t judge_gray_range(uint8_t range)
{
	if(range > MIN_RANGE)
	{
		return TRUE;
	}
	
	return FALSE;
}

uint8_t fpsensor_cac(uint8_t * image, int32_t height, int32_t width ,fpsensor_adc_t *adc)
{
	if(image == NULL || adc == NULL) return FPSENSOR_BUFFER_ERROR;

	fpsensor_adc_t pre_adc;
	pre_adc.shift = adc->shift;
	pre_adc.gain  = adc->gain;
	pre_adc.pixel = adc->pixel;
	pre_adc.et1   = adc->et1;
	
	adc_pointer = adc;

  #if 1
	uint32_t avg = calculate_average(image, height, width);
	uint32_t var = calculate_variance(image, height, width, &avg);
	
	fpsensor_finger_type_t finger_type = judge_gray_average(avg);
	uint8_t var_result = judge_gray_variance(var, avg);

	if((finger_type == NORMAL_FINGER && var_result == TRUE)
		|| compare_adc(&pre_adc, adc))
	{
		return TRUE;
	}
	else
		return FALSE;
  #else
	uint32_t avg = calculate_average(image, height, width);
	fpsensor_finger_type_t finger_type = judge_gray_average(avg);
	return finger_type == NORMAL_FINGER ? TRUE : FALSE;
  #endif
}

#endif
