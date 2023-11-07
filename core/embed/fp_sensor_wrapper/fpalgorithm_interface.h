#ifndef FPALGORITHM_INTERFACE_H
#define FPALGORITHM_INTERFACE_H

#include<stdint.h>

#define MAX_USER_COUNT 5

/*
* Function：     FpLibVersion 
* Description：  获取指纹算法库版本，ASCII字符。
* Input：        无。
* Output：       pu8Ver : 返回的字符串。
* Return:        无。
* Others:        指纹算法库版本信息长度最大为32个字节。
*/
void FpLibVersion(char *pcVer);

/*
* Function：     FpAlgorithmInit 
* Description：  算法库初始化。
* Input：        addr : 指纹模板存储的起始地址。
* Output：       无。
* Return:        0-成功，其他-失败。
* Others:        无。
*/
uint8_t FpAlgorithmInit(uint32_t addr);

/*
* Function：     FpsDetectFinger 
* Description：  检测传感器上有无按压手指。
* Input：        无。 
* Output：       无。
* Return:        0-无手指，1-有手指。
* Others:        无。
*/
uint8_t FpsDetectFinger(void);
/*
* Function：     FpsGetImageData 
* Description：  检测手指并获取指纹图像数据。
* Input：        无。 
* Output：       pu8ImageBuf ： 采集图像数据放到该缓冲区中，缓冲区应大于等于 112 *88 + 2。
								 图像数据保存在缓冲区前112 * 88个字节中，缓冲区最后两字节仅采图过程中使用，采图完成后为无效数据。
* Return:        0-成功，其他-传感器上无指纹(1)，或指纹面积太小(2)。
* Others:        无。
*/
uint8_t FpsGetImageData(uint8_t *pu8ImageBuf);
/*
* Function：     FpsGetImage 
* Description：  检测手指并获取指纹图像数据。
* Input：        无。 
* Output：       无
* Return:        0-成功，其他-传感器上无指纹(1)，或指纹面积太小(2)。
* Others:        无。
*/
uint8_t FpsGetImage(void);

/*
* Function：     FpsSleep 
* Description：  指纹传感器进入休眠模式，降低功耗。
* Input：        u32DectPeriod ：休眠唤醒扫描周期，扫描周期越长功耗越低，但相应的唤醒反应速度也越慢。
                 u8Threshold : 手指检测阈值。
* Output：       无。
* Return:        0-成功，其他-失败。
* Others:        指纹传感器进入休眠后会定期唤醒扫描是否有指纹按下，若有则唤醒并产生中断信号。
				 注意休眠前手指应离开传感器表面，否则传感器无法休眠成功。
*/
uint8_t FpsSleep(uint32_t u32DectPeriod, uint8_t u8Threshold);

/*
* Function：     FpaExtractfeature 
* Description：  提取指纹图像特征。
* Input：        u8Num : 最多存10幅图的特征，此次存储为第u8Num幅图的特征。
* Output：       无。
* Return:        0-成功，其他-失败。
* Others:        无。
*/
uint8_t FpaExtractfeature(uint8_t u8Num);

/*
* Function：     FpaMergeFeatureToTemplate  
* Description：  融合图像特征生成指纹模板。
* Input：        u8Num:几幅图的特征融合成模板。
* Output：       无。
* Return:        0-成功，其他-失败。
* Others:        无。
*/
uint8_t FpaMergeFeatureToTemplate(uint8_t u8Num);

/*
* Function：     FpaEnrollTemplatesave 
* Description：  存储模板数据
* Input：        tpl_num : 模板ID号。
* Output：       无。
* Return:        0-成功，其他-失败。
* Others:        无。
*/
uint8_t FpaEnrollTemplatesave(uint16_t tpl_num);

/*
* Function：     FpaIdentify 
* Description：  搜索指纹库中模板是否与当前指纹匹配。
* Input：        无。
* Output：       MatchId ： 若匹配则输出对应模板编号。
* Return:        0-成功，其他-失败。注：函数运行结果，非搜索结果。
* Others:        无。
*/
uint8_t FpaIdentify(uint8_t * MatchId);

/*
* Function：     FpaDeleteTemplateId 
* Description：  删除模板库中对应编号的模板数据。
* Input：        u8TemplateId ：所删除模板的编号。
* Output：       无。
* Return:        0-成功，其他-失败。
* Others:        无。
*/
uint8_t FpaDeleteTemplateId(uint8_t u8TemplateId);

/*
* Function：     FpaClearTemplate 
* Description：  清空模板库中所有模板数据。
* Input：        无。
* Output：       无。
* Return:        0-成功，其他-失败。
* Others:        无。
*/
uint8_t FpaClearTemplate(void);

/*
* Function：     FpaGetTemplateNum 
* Description：  获取指纹库中已注册模板个数。
* Input：        无。
* Output：       u8TemplateNum: 已注册模板个数。
* Return:        0-成功，其他-失败。
* Others:        无。
*/
uint8_t FpaGetTemplateNum(uint8_t *u8TemplateNum);
/*
* Function：     FpaGetTemplateIDlist 
* Description：  获取指纹库中已注册模板列表。
* Input：        无。
* Output：       bit_flag: 32位数组，共256bit（ID=0到ID=255）,哪bit置1，其ID存在。
* Return:        0-成功，其他-失败。
* Others:        无。
*/
uint8_t FpaGetTemplateIDlist(uint8_t *bit_flag);


#endif


