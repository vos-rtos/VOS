#include "spi.h"
#include "stm32f4xx_hal.h"

//SPI_HandleTypeDef SPI1_Handler;  //SPI���


typedef struct StSpiBus {
	SPI_HandleTypeDef hdl;
	s32 is_inited; //�Ƿ��Ѿ���ʼ��
}StSpiBus;


struct StSpiBus gSpiBus[5];

s32 spi_open(s32 port, s32 BaudRatePrescaler)
{
	struct StSpiBus *pSpiBus = &gSpiBus[port];
	switch (port) {
		case 0: pSpiBus->hdl.Instance = SPI1; break;
		case 1: pSpiBus->hdl.Instance = SPI2; break;
		case 2: pSpiBus->hdl.Instance = SPI3; break;
		default: return -1;
	}

	pSpiBus->hdl.Init.Mode				= SPI_MODE_MASTER;             //����SPI����ģʽ������Ϊ��ģʽ
	pSpiBus->hdl.Init.Direction			= SPI_DIRECTION_2LINES;   //����SPI�������˫�������ģʽ:SPI����Ϊ˫��ģʽ
	pSpiBus->hdl.Init.DataSize			= SPI_DATASIZE_8BIT;       //����SPI�����ݴ�С:SPI���ͽ���8λ֡�ṹ
	pSpiBus->hdl.Init.CLKPolarity		= SPI_POLARITY_HIGH;    //����ͬ��ʱ�ӵĿ���״̬Ϊ�ߵ�ƽ
	pSpiBus->hdl.Init.CLKPhase			= SPI_PHASE_2EDGE;         //����ͬ��ʱ�ӵĵڶ��������أ��������½������ݱ�����
	pSpiBus->hdl.Init.NSS				= SPI_NSS_SOFT;                 //NSS�ź���Ӳ����NSS�ܽţ����������ʹ��SSIλ������:�ڲ�NSS�ź���SSIλ����
	pSpiBus->hdl.Init.BaudRatePrescaler	= SPI_BAUDRATEPRESCALER_256;//���岨����Ԥ��Ƶ��ֵ:������Ԥ��ƵֵΪ256
	pSpiBus->hdl.Init.FirstBit			= SPI_FIRSTBIT_MSB;        //ָ�����ݴ����MSBλ����LSBλ��ʼ:���ݴ����MSBλ��ʼ
	pSpiBus->hdl.Init.TIMode			= SPI_TIMODE_DISABLE;        //�ر�TIģʽ
	pSpiBus->hdl.Init.CRCCalculation	= SPI_CRCCALCULATION_DISABLE;//�ر�Ӳ��CRCУ��
	pSpiBus->hdl.Init.CRCPolynomial		= 7;                  //CRCֵ����Ķ���ʽ
    HAL_SPI_Init(&pSpiBus->hdl);//��ʼ��

//    pSpiBus->hdl.Instance->CR1	&= 0XFFC7;          //λ3-5���㣬�������ò�����
//    pSpiBus->hdl.Instance->CR1	|= BaudRatePrescaler;//����SPI�ٶ�

    __HAL_SPI_ENABLE(&pSpiBus->hdl);

    spi_read_write(port, 0Xff, 1000);
}

s32 spi_ctrl(s32 port, s32 tag, void *value, s32 len)
{
	struct StSpiBus *pSpiBus = &gSpiBus[port];
	__HAL_SPI_DISABLE(&pSpiBus->hdl);
	switch (tag) {
	case PARA_SPI_DIRECTION:

		break;
	case PARA_SPI_DATASIZE:
		break;
	case PARA_SPI_CLK_POLARITY:
		pSpiBus->hdl.Init.CLKPolarity = *(u32*)value;
		HAL_SPI_Init(&pSpiBus->hdl);
		break;
	case PARA_SPI_CLK_PHASE:
		pSpiBus->hdl.Init.CLKPhase = *(u32*)value;
		HAL_SPI_Init(&pSpiBus->hdl);
		break;
	case PARA_SPI_BAUDRATE_PRESCALER:
	    pSpiBus->hdl.Instance->CR1 &= 0XFFC7;
	    pSpiBus->hdl.Instance->CR1 |= *(u32*)value;
		break;
	default:
		break;
	};
	__HAL_SPI_ENABLE(&pSpiBus->hdl);
}

s32 spi_read_write(s32 port, u8 ch, u32 timeout)
{
	struct StSpiBus *pSpiBus = &gSpiBus[port];
    u8 data = 0;
    HAL_SPI_TransmitReceive(&pSpiBus->hdl, &ch, &data, 1, timeout);
 	return data;
}

s32 spi_close(s32 port)
{
	struct StSpiBus *pSpiBus = &gSpiBus[port];
	HAL_SPI_DeInit(&pSpiBus->hdl);
	return 0;
}

void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi)
{
	GPIO_InitTypeDef GPIO_Initure;
	if (hspi->Instance == SPI1) {
	    __HAL_RCC_GPIOB_CLK_ENABLE();
	    __HAL_RCC_SPI1_CLK_ENABLE();
	    GPIO_Initure.Pin		=GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5;
	    GPIO_Initure.Mode		=GPIO_MODE_AF_PP;              //�����������
	    GPIO_Initure.Pull		=GPIO_PULLUP;                  //����
	    GPIO_Initure.Speed		=GPIO_SPEED_FAST;             //����
	    GPIO_Initure.Alternate	=GPIO_AF5_SPI1;
	    HAL_GPIO_Init(GPIOB, &GPIO_Initure);
	}
	if (hspi->Instance == SPI2) {
	    __HAL_RCC_GPIOC_CLK_ENABLE();
	    __HAL_RCC_GPIOB_CLK_ENABLE();
	    __HAL_RCC_SPI2_CLK_ENABLE();

	    GPIO_Initure.Pin	= GPIO_PIN_3;
	    GPIO_Initure.Mode	= GPIO_MODE_AF_PP;
	    GPIO_Initure.Pull	= GPIO_PULLUP;
	    GPIO_Initure.Speed	= GPIO_SPEED_FAST;
	    GPIO_Initure.Alternate = GPIO_AF5_SPI2;
	    HAL_GPIO_Init(GPIOC, &GPIO_Initure);

	    GPIO_Initure.Pin		= GPIO_PIN_13|GPIO_PIN_14;
	    GPIO_Initure.Alternate	= GPIO_AF5_SPI2;
	    HAL_GPIO_Init(GPIOB, &GPIO_Initure);
	}
}

void HAL_SPI_MspDeInit(SPI_HandleTypeDef *hspi)
{
	if (hspi->Instance == SPI1) {
		__HAL_RCC_SPI1_FORCE_RESET();
		__HAL_RCC_SPI1_RELEASE_RESET();
		HAL_GPIO_DeInit(GPIOB, GPIO_PIN_3);
		HAL_GPIO_DeInit(GPIOB, GPIO_PIN_4);
		HAL_GPIO_DeInit(GPIOB, GPIO_PIN_5);
	}
	if (hspi->Instance == SPI2) {
		__HAL_RCC_SPI2_FORCE_RESET();
		__HAL_RCC_SPI1_RELEASE_RESET();
		HAL_GPIO_DeInit(GPIOC, GPIO_PIN_5);
		HAL_GPIO_DeInit(GPIOB, GPIO_PIN_13);
		HAL_GPIO_DeInit(GPIOB, GPIO_PIN_14);
	}
}








