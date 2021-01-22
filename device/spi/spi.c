#include "spi.h"
#include "stm32f4xx_hal.h"

//SPI_HandleTypeDef SPI1_Handler;  //SPI句柄


typedef struct StSpiBus {
	SPI_HandleTypeDef hdl;
	s32 is_inited; //是否已经初始化
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

	pSpiBus->hdl.Init.Mode				= SPI_MODE_MASTER;             //设置SPI工作模式，设置为主模式
	pSpiBus->hdl.Init.Direction			= SPI_DIRECTION_2LINES;   //设置SPI单向或者双向的数据模式:SPI设置为双线模式
	pSpiBus->hdl.Init.DataSize			= SPI_DATASIZE_8BIT;       //设置SPI的数据大小:SPI发送接收8位帧结构
	pSpiBus->hdl.Init.CLKPolarity		= SPI_POLARITY_HIGH;    //串行同步时钟的空闲状态为高电平
	pSpiBus->hdl.Init.CLKPhase			= SPI_PHASE_2EDGE;         //串行同步时钟的第二个跳变沿（上升或下降）数据被采样
	pSpiBus->hdl.Init.NSS				= SPI_NSS_SOFT;                 //NSS信号由硬件（NSS管脚）还是软件（使用SSI位）管理:内部NSS信号有SSI位控制
	pSpiBus->hdl.Init.BaudRatePrescaler	= SPI_BAUDRATEPRESCALER_256;//定义波特率预分频的值:波特率预分频值为256
	pSpiBus->hdl.Init.FirstBit			= SPI_FIRSTBIT_MSB;        //指定数据传输从MSB位还是LSB位开始:数据传输从MSB位开始
	pSpiBus->hdl.Init.TIMode			= SPI_TIMODE_DISABLE;        //关闭TI模式
	pSpiBus->hdl.Init.CRCCalculation	= SPI_CRCCALCULATION_DISABLE;//关闭硬件CRC校验
	pSpiBus->hdl.Init.CRCPolynomial		= 7;                  //CRC值计算的多项式
    HAL_SPI_Init(&pSpiBus->hdl);//初始化

//    pSpiBus->hdl.Instance->CR1	&= 0XFFC7;          //位3-5清零，用来设置波特率
//    pSpiBus->hdl.Instance->CR1	|= BaudRatePrescaler;//设置SPI速度

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
	    GPIO_Initure.Mode		=GPIO_MODE_AF_PP;              //复用推挽输出
	    GPIO_Initure.Pull		=GPIO_PULLUP;                  //上拉
	    GPIO_Initure.Speed		=GPIO_SPEED_FAST;             //快速
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








