/*******************************************************************************
*
* 文件名称: AS5600.c
* 版 本 号: V1.0 
* 作    者: linbo.liu
* 生成日期: 2024年x月x日
* 功能描述: 
*
*******************************************************************************/
/*******************************************************************************
 *----------------------------------头文件--------------------------------------
 ******************************************************************************/
#include "Srv.h"

/*******************************************************************************
 *----------------------------------宏定义---------------------------------------
 ******************************************************************************/


/*******************************************************************************
 *--------------------------------数据类型定义-----------------------------------
 ******************************************************************************/


/*******************************************************************************
 *----------------------------------函数声明-------------------------------------
 ******************************************************************************/


/*******************************************************************************
 *-----------------------------------常量定义------------------------------------
 ******************************************************************************/


/*******************************************************************************
 *-----------------------——--------全局变量定义----------------------------------
 ******************************************************************************/


/*******************************************************************************
 *---------------------------—----—--函数实现------------------------------------
 ******************************************************************************/
void IIC_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOE_CLK_ENABLE();

    /*Configure GPIO pins : PDPin PDPin */
    GPIO_InitStruct.Pin = SCL_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = SDA_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
    HAL_GPIO_WritePin(GPIOE, SCL_Pin | SDA_Pin, GPIO_PIN_SET);
}


void delay_us(void)
{
    uint16_t i = 20;
    while ((i--))
    {
        
    }
}
void IIC_Start(void)
{
    SDA_OUT();
    IIC_SDA(GPIO_PIN_SET);
    IIC_SCL(GPIO_PIN_SET);
    delay_us();
    IIC_SDA(GPIO_PIN_RESET);
    delay_us();
    IIC_SCL(GPIO_PIN_RESET);
}

void IIC_Stop(void)
{
    SDA_OUT();
    IIC_SCL(GPIO_PIN_RESET);
    IIC_SDA(GPIO_PIN_RESET);
    delay_us();
    IIC_SCL(GPIO_PIN_SET);
    IIC_SDA(GPIO_PIN_SET);
    delay_us();
}

uint8_t IIC_Wait_Ack(void)
{
    uint8_t ucErrTime = 0;
    SDA_IN();
    IIC_SDA(GPIO_PIN_SET);
    delay_us();
    IIC_SCL(GPIO_PIN_SET);
    delay_us();
    while (READ_SDA())
    {
        ucErrTime++;
        if (ucErrTime > 250)
        {
            IIC_Stop();
            return 1;
        }
    }
    IIC_SCL(GPIO_PIN_RESET);
    return 0;
}

void IIC_Ack(void)
{
    IIC_SCL(GPIO_PIN_RESET);
    SDA_OUT();
    IIC_SDA(GPIO_PIN_RESET);
    delay_us();
    IIC_SCL(GPIO_PIN_SET);
    delay_us();
    IIC_SCL(GPIO_PIN_RESET);
}

void IIC_NAck(void)
{
    IIC_SCL(GPIO_PIN_RESET);
    SDA_OUT();
    IIC_SDA(GPIO_PIN_SET);
    delay_us();
    IIC_SCL(GPIO_PIN_SET);
    delay_us();
    IIC_SCL(GPIO_PIN_RESET);
}

void IIC_Send_Byte(uint8_t txd)
{
    uint8_t t;
    SDA_OUT();
    IIC_SCL(GPIO_PIN_RESET);
    delay_us();
    for (t = 0; t < 8; t++)
    {
        if ((txd & 0x80) >> 7)
            IIC_SDA(GPIO_PIN_SET);
        else
            IIC_SDA(GPIO_PIN_RESET);
        txd <<= 1;
        delay_us();
        IIC_SCL(GPIO_PIN_SET);
        delay_us();
        IIC_SCL(GPIO_PIN_RESET);
        delay_us();
    }
}

uint8_t IIC_Read_Byte(unsigned char ack)
{
    unsigned char i, receive = 0;
    SDA_IN();
    for (i = 0; i < 8; i++)
    {
        IIC_SCL(GPIO_PIN_RESET);
        delay_us();
        IIC_SCL(GPIO_PIN_SET);
        receive <<= 1;
        if (READ_SDA())
            receive++;
        delay_us();
    }
    if (!ack)
        IIC_NAck();
    else
        IIC_Ack();
    return receive;
}

uint8_t AS5600_ReadOneByte(uint16_t ReadAddr)
{
    uint8_t temp = -1;
    IIC_Start();
    IIC_Send_Byte((0X36 << 1) | 0x00);
    IIC_Wait_Ack();
    IIC_Send_Byte(ReadAddr);
    IIC_Wait_Ack();
    IIC_Start();
    IIC_Send_Byte((0X36 << 1) | 0x01);
    IIC_Wait_Ack();
    temp = IIC_Read_Byte(0);
    IIC_Stop();
    return temp;
}

void AS5600_WriteOneByte(uint16_t WriteAddr, uint8_t WriteData)
{
    IIC_Start();
    IIC_Send_Byte((0X36 << 1) | 0x00);
    IIC_Wait_Ack();
    IIC_Send_Byte(WriteAddr);
    IIC_Wait_Ack();
    IIC_Start();
    IIC_Send_Byte(WriteData);
    IIC_Wait_Ack();
    IIC_Stop();
}

uint16_t AS5600_ReadTwoByte(uint16_t ReadAddr_hi, uint16_t ReadAddr_lo)
{
    uint16_t TwoByte_Data = -1;
    uint8_t hi_Data = 0, lo_Data = 0;
    hi_Data = AS5600_ReadOneByte(ReadAddr_hi);
    lo_Data = AS5600_ReadOneByte(ReadAddr_lo);
    TwoByte_Data = (hi_Data << 8) | lo_Data;
    return TwoByte_Data;
}


/*******************************************************************************
 *---------------------------—----—---结束--------------------------------------
 ******************************************************************************/