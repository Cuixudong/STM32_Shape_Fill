#include "lcd.h"
#include "stdlib.h"
#include "font.h" 
#include "usart.h"	 
#include "delay.h"	   

#define FSMC_LCD_CMD                   ((uint32_t)0x6C0007FE)	    //FSMC_Bank1_NORSRAM1用于LCD命令操作的地址
#define FSMC_LCD_DATA                  ((uint32_t)0x6C000800)      //FSMC_Bank1_NORSRAM1用于LCD数据操作的地址      
#define LCD_WRITE_CMD(x)               *(__IO uint16_t *)FSMC_LCD_CMD  = x 
#define LCD_WRITE_DATA(x)              *(__IO uint16_t *)FSMC_LCD_DATA = x
#define LCD_READ_DATA()                *(__IO uint16_t *)FSMC_LCD_DATA

//LCD的画笔颜色和背景色	   
u16 POINT_COLOR=0x0000;	//画笔颜色
u16 BACK_COLOR=0xFFFF;  //背景色 
  
//管理LCD重要参数
//默认为竖屏
_lcd_dev lcddev;
	 
//写寄存器函数
//regval:寄存器值
void LCD_WR_REG(u16 regval)
{   
	LCD->LCD_REG=regval;//写入要写的寄存器序号	 
	//printf("WREG:0x%08X\r\n",(int)(regval));
}
//写LCD数据
//data:要写入的值
void LCD_WR_DATA(u16 data)
{
	LCD->LCD_RAM=data;	
	//printf("WDAT:0x%08X\r\n",(int)(data));
}
//读LCD数据
//返回值:读到的值
u16 LCD_RD_DATA(void)
{
	vu16 ram;			//防止被优化
	ram=LCD->LCD_RAM;	
	return ram;	 
}					   
//写寄存器
//LCD_Reg:寄存器地址
//LCD_RegValue:要写入的数据
void LCD_WriteReg(u16 LCD_Reg,u16 LCD_RegValue)
{	
	LCD->LCD_REG = LCD_Reg;		//写入要写的寄存器序号	 
	LCD->LCD_RAM = LCD_RegValue;//写入数据	    
	//printf("WREG:0x%08X\r\n",(int)(LCD_Reg));
	//printf("WDAT:0x%08X\r\n",(int)(LCD_RegValue));
}	   
//读寄存器
//LCD_Reg:寄存器地址
//返回值:读到的数据
u16 LCD_ReadReg(u16 LCD_Reg)
{										   
	LCD_WR_REG(LCD_Reg);		//写入要读的寄存器序号
	//printf("WREG:0x%08X\r\n",(int)(LCD_Reg));
	delay_us(5);		  
	return LCD_RD_DATA();		//返回读到的值
}   
//开始写GRAM
void LCD_WriteRAM_Prepare(void)
{
 	LCD->LCD_REG=lcddev.wramcmd;	
	//printf("WREG:0x%08X\r\n",(int)(lcddev.wramcmd));
}	 
//LCD写GRAM
//RGB_Code:颜色值
void LCD_WriteRAM(u16 RGB_Code)
{							    
	LCD->LCD_RAM = RGB_Code;//写十六位GRAM
	//printf("WDAT:0x%08X\r\n",(int)(RGB_Code));
}
//从ILI93xx读出的数据为GBR格式，而我们写入的时候为RGB格式。
//通过该函数转换
//c:GBR格式的颜色值
//返回值：RGB格式的颜色值
u16 LCD_BGR2RGB(u16 c)
{
	u16  r,g,b,rgb;   
	b=(c>>0)&0x1f;
	g=(c>>5)&0x3f;
	r=(c>>11)&0x1f;	 
	rgb=(b<<11)+(g<<5)+(r<<0);		 
	return(rgb);
} 
//当mdk -O1时间优化时需要设置
//延时i
void opt_delay(u8 i)
{
	while(i--);
}
//读取个某点的颜色值	 
//x,y:坐标
//返回值:此点的颜色
u16 LCD_ReadPoint(u16 x,u16 y)
{
 	u16 r=0,g=0,b=0;
	if(x>=lcddev.width||y>=lcddev.height)return 0;	//超过了范围,直接返回		   
	LCD_SetCursor(x,y);	    
	LCD_WR_REG(0X2E00);	//5510 发送读GRAM指令    
 	r=LCD_RD_DATA();								//dummy Read	   
	opt_delay(2);	  
 	r=LCD_RD_DATA();  		  						//实际坐标颜色
	opt_delay(2);	  
	b=LCD_RD_DATA(); 
	g=r&0XFF;		//对于9341/5310/5510,第一次读取的是RG的值,R在前,G在后,各占8位
	g<<=8;
	return (((r>>11)<<11)|((g>>10)<<5)|(b>>11));//ILI9341/NT35310/NT35510需要公式转换一下
}			 
//LCD开启显示
void LCD_DisplayOn(void)
{					   
	LCD_WR_REG(0X2900);	//开启显示
}	 
//LCD关闭显示
void LCD_DisplayOff(void)
{	   
	LCD_WR_REG(0X2800);	//关闭显示
}   
//设置光标位置
//Xpos:横坐标
//Ypos:纵坐标
void LCD_SetCursor(u16 Xpos, u16 Ypos)
{
	LCD_WR_REG(lcddev.setxcmd);LCD_WR_DATA(Xpos>>8); 		
	LCD_WR_REG(lcddev.setxcmd+1);LCD_WR_DATA(Xpos&0XFF);			 
	LCD_WR_REG(lcddev.setycmd);LCD_WR_DATA(Ypos>>8);  		
	LCD_WR_REG(lcddev.setycmd+1);LCD_WR_DATA(Ypos&0XFF);			
} 		 
//设置LCD的自动扫描方向
//注意:其他函数可能会受到此函数设置的影响(尤其是9341/6804这两个奇葩),
//所以,一般设置为L2R_U2D即可,如果设置为其他扫描方式,可能导致显示不正常.
//dir:0~7,代表8个方向(具体定义见lcd.h)
//9320/9325/9328/4531/4535/1505/b505/5408/9341/5310/5510/1963等IC已经实际测试	   	   
void LCD_Scan_Dir(u8 dir)
{
	u16 regval=0;
	u16 dirreg=0;
	u16 temp;
	switch(dir)
	{
		case L2R_U2D://从左到右,从上到下
			regval|=(0<<7)|(0<<6)|(0<<5); 
			break;
		case L2R_D2U://从左到右,从下到上
			regval|=(1<<7)|(0<<6)|(0<<5); 
			break;
		case R2L_U2D://从右到左,从上到下
			regval|=(0<<7)|(1<<6)|(0<<5); 
			break;
		case R2L_D2U://从右到左,从下到上
			regval|=(1<<7)|(1<<6)|(0<<5); 
			break;	 
		case U2D_L2R://从上到下,从左到右
			regval|=(0<<7)|(0<<6)|(1<<5); 
			break;
		case U2D_R2L://从上到下,从右到左
			regval|=(0<<7)|(1<<6)|(1<<5); 
			break;
		case D2U_L2R://从下到上,从左到右
			regval|=(1<<7)|(0<<6)|(1<<5); 
			break;
		case D2U_R2L://从下到上,从右到左
			regval|=(1<<7)|(1<<6)|(1<<5); 
			break;	 
	}
	if(lcddev.id==0X5510)dirreg=0X3600;   
	LCD_WriteReg(dirreg,regval);
	if(lcddev.id==0X5510)
	{
		LCD_WR_REG(lcddev.setxcmd);LCD_WR_DATA(0); 
		LCD_WR_REG(lcddev.setxcmd+1);LCD_WR_DATA(0); 
		LCD_WR_REG(lcddev.setxcmd+2);LCD_WR_DATA((lcddev.width-1)>>8); 
		LCD_WR_REG(lcddev.setxcmd+3);LCD_WR_DATA((lcddev.width-1)&0XFF); 
		LCD_WR_REG(lcddev.setycmd);LCD_WR_DATA(0); 
		LCD_WR_REG(lcddev.setycmd+1);LCD_WR_DATA(0); 
		LCD_WR_REG(lcddev.setycmd+2);LCD_WR_DATA((lcddev.height-1)>>8); 
		LCD_WR_REG(lcddev.setycmd+3);LCD_WR_DATA((lcddev.height-1)&0XFF);
	}
}     
//画点
//x,y:坐标
//POINT_COLOR:此点的颜色
void LCD_DrawPoint(u16 x,u16 y)
{
	LCD_SetCursor(x,y);		//设置光标位置 
	LCD_WriteRAM_Prepare();	//开始写入GRAM
	LCD->LCD_RAM=POINT_COLOR; 
}
//快速画点
//x,y:坐标
//color:颜色
void LCD_Fast_DrawPoint(u16 x,u16 y,u16 color)
{
	LCD_WR_REG(lcddev.setxcmd);LCD_WR_DATA(x>>8);  
	LCD_WR_REG(lcddev.setxcmd+1);LCD_WR_DATA(x&0XFF);	  
	LCD_WR_REG(lcddev.setycmd);LCD_WR_DATA(y>>8);  
	LCD_WR_REG(lcddev.setycmd+1);LCD_WR_DATA(y&0XFF); 
	LCD->LCD_REG=lcddev.wramcmd; 
	LCD->LCD_RAM=color; 
}	 
//SSD1963 背光设置
//pwm:背光等级,0~100.越大越亮.
void LCD_SSD_BackLightSet(u8 pwm)
{	
	LCD_WR_REG(0xBE);	//配置PWM输出
	LCD_WR_DATA(0x05);	//1设置PWM频率
	LCD_WR_DATA(pwm*2.55);//2设置PWM占空比
	LCD_WR_DATA(0x01);	//3设置C
	LCD_WR_DATA(0xFF);	//4设置D
	LCD_WR_DATA(0x00);	//5设置E
	LCD_WR_DATA(0x00);	//6设置F
}

//设置LCD显示方向
//dir:0,竖屏；1,横屏
void LCD_Display_Dir(u8 dir)
{
	if(dir==0)			//竖屏
	{
		lcddev.dir=0;	//竖屏
		lcddev.wramcmd=0X2C00;
		lcddev.setxcmd=0X2A00;
		lcddev.setycmd=0X2B00; 
		lcddev.width=480;
		lcddev.height=800;
	}else 				//横屏
	{	  				
		lcddev.dir=1;	//横屏
		lcddev.width=320;
		lcddev.height=240;

		lcddev.wramcmd=0X2C00;
		lcddev.setxcmd=0X2A00;
		lcddev.setycmd=0X2B00; 
		lcddev.width=800;
		lcddev.height=480;
	} 
	LCD_Scan_Dir(DFT_SCAN_DIR);	//默认扫描方向
}	 
//设置窗口,并自动设置画点坐标到窗口左上角(sx,sy).
//sx,sy:窗口起始坐标(左上角)
//width,height:窗口宽度和高度,必须大于0!!
//窗体大小:width*height. 
void LCD_Set_Window(u16 sx,u16 sy,u16 width,u16 height)
{    
	u8 hsareg,heareg,vsareg,veareg;
	u16 hsaval,heaval,vsaval,veaval; 
	u16 twidth,theight;
	twidth=sx+width-1;
	theight=sy+height-1;
	LCD_WR_REG(lcddev.setxcmd);LCD_WR_DATA(sx>>8);  
	LCD_WR_REG(lcddev.setxcmd+1);LCD_WR_DATA(sx&0XFF);	  
	LCD_WR_REG(lcddev.setxcmd+2);LCD_WR_DATA(twidth>>8);   
	LCD_WR_REG(lcddev.setxcmd+3);LCD_WR_DATA(twidth&0XFF);   
	LCD_WR_REG(lcddev.setycmd);LCD_WR_DATA(sy>>8);   
	LCD_WR_REG(lcddev.setycmd+1);LCD_WR_DATA(sy&0XFF);  
	LCD_WR_REG(lcddev.setycmd+2);LCD_WR_DATA(theight>>8);   
	LCD_WR_REG(lcddev.setycmd+3);LCD_WR_DATA(theight&0XFF);  
}
//初始化lcd
//该初始化函数可以初始化各种ILI93XX液晶,但是其他函数是基于ILI9320的!!!
//在其他型号的驱动芯片上没有测试! 
void LCD_Init(void)
{ 					
 	GPIO_InitTypeDef GPIO_InitStructure;
	FSMC_NORSRAMInitTypeDef  FSMC_NORSRAMInitStructure;
  FSMC_NORSRAMTimingInitTypeDef  readWriteTiming; 
	FSMC_NORSRAMTimingInitTypeDef  writeTiming;
	
	printf("LCD REG:0x%08X\r\n",(int)(&(LCD->LCD_REG)));
	printf("LCD RAM:0x%08X\r\n",(int)(&(LCD->LCD_RAM)));
	
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_FSMC,ENABLE);	//使能FSMC时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB|RCC_APB2Periph_GPIOD|RCC_APB2Periph_GPIOE|RCC_APB2Periph_GPIOG,ENABLE);//使能PORTB,D,E,G以及AFIO复用功能时钟

 
 	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;				 //PB0 推挽输出 背光
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 //推挽输出
 	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
 	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
 	//PORTD复用推挽输出  
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_4|GPIO_Pin_5|GPIO_Pin_8|GPIO_Pin_9|GPIO_Pin_10|GPIO_Pin_14|GPIO_Pin_15;				 //	//PORTD复用推挽输出  
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; 		 //复用推挽输出   
 	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
 	GPIO_Init(GPIOD, &GPIO_InitStructure); 
  	 
	//PORTE复用推挽输出  
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7|GPIO_Pin_8|GPIO_Pin_9|GPIO_Pin_10|GPIO_Pin_11|GPIO_Pin_12|GPIO_Pin_13|GPIO_Pin_14|GPIO_Pin_15;				 //	//PORTD复用推挽输出  
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; 		 //复用推挽输出   
 	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
 	GPIO_Init(GPIOE, &GPIO_InitStructure);    	    	 											 

   	//	//PORTG12复用推挽输出 A0	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_12;	 //	//PORTD复用推挽输出  
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; 		 //复用推挽输出   
 	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
 	GPIO_Init(GPIOG, &GPIO_InitStructure); 

	readWriteTiming.FSMC_AddressSetupTime = 0x01;	 //地址建立时间（ADDSET）为2个HCLK 1/36M=27ns
  readWriteTiming.FSMC_AddressHoldTime = 0x00;	 //地址保持时间（ADDHLD）模式A未用到	
  readWriteTiming.FSMC_DataSetupTime = 0x0f;		 // 数据保存时间为16个HCLK,因为液晶驱动IC的读数据的时候，速度不能太快，尤其对1289这个IC。
  readWriteTiming.FSMC_BusTurnAroundDuration = 0x00;
  readWriteTiming.FSMC_CLKDivision = 0x00;
  readWriteTiming.FSMC_DataLatency = 0x00;
  readWriteTiming.FSMC_AccessMode = FSMC_AccessMode_A;	 //模式A 
    

	writeTiming.FSMC_AddressSetupTime = 0x00;	 //地址建立时间（ADDSET）为1个HCLK  
  writeTiming.FSMC_AddressHoldTime = 0x00;	 //地址保持时间（A		
  writeTiming.FSMC_DataSetupTime = 0x03;		 ////数据保存时间为4个HCLK	
  writeTiming.FSMC_BusTurnAroundDuration = 0x00;
  writeTiming.FSMC_CLKDivision = 0x00;
  writeTiming.FSMC_DataLatency = 0x00;
  writeTiming.FSMC_AccessMode = FSMC_AccessMode_A;	 //模式A 

 
  FSMC_NORSRAMInitStructure.FSMC_Bank = FSMC_Bank1_NORSRAM4;//  这里我们使用NE4 ，也就对应BTCR[6],[7]。
  FSMC_NORSRAMInitStructure.FSMC_DataAddressMux = FSMC_DataAddressMux_Disable; // 不复用数据地址
  FSMC_NORSRAMInitStructure.FSMC_MemoryType =FSMC_MemoryType_SRAM;// FSMC_MemoryType_SRAM;  //SRAM   
  FSMC_NORSRAMInitStructure.FSMC_MemoryDataWidth = FSMC_MemoryDataWidth_16b;//存储器数据宽度为16bit   
  FSMC_NORSRAMInitStructure.FSMC_BurstAccessMode =FSMC_BurstAccessMode_Disable;// FSMC_BurstAccessMode_Disable; 
  FSMC_NORSRAMInitStructure.FSMC_WaitSignalPolarity = FSMC_WaitSignalPolarity_Low;
	FSMC_NORSRAMInitStructure.FSMC_AsynchronousWait=FSMC_AsynchronousWait_Disable; 
  FSMC_NORSRAMInitStructure.FSMC_WrapMode = FSMC_WrapMode_Disable;   
  FSMC_NORSRAMInitStructure.FSMC_WaitSignalActive = FSMC_WaitSignalActive_BeforeWaitState;  
  FSMC_NORSRAMInitStructure.FSMC_WriteOperation = FSMC_WriteOperation_Enable;	//  存储器写使能
  FSMC_NORSRAMInitStructure.FSMC_WaitSignal = FSMC_WaitSignal_Disable;   
  FSMC_NORSRAMInitStructure.FSMC_ExtendedMode = FSMC_ExtendedMode_Enable; // 读写使用不同的时序
  FSMC_NORSRAMInitStructure.FSMC_WriteBurst = FSMC_WriteBurst_Disable; 
  FSMC_NORSRAMInitStructure.FSMC_ReadWriteTimingStruct = &readWriteTiming; //读写时序
  FSMC_NORSRAMInitStructure.FSMC_WriteTimingStruct = &writeTiming;  //写时序

  FSMC_NORSRAMInit(&FSMC_NORSRAMInitStructure);  //初始化FSMC配置

 	FSMC_NORSRAMCmd(FSMC_Bank1_NORSRAM4, ENABLE);  // 使能BANK1 

	delay_ms(50); 					// delay 50 ms 
	
	
	LCD_WR_REG(0XDA00);	
	lcddev.id=LCD_RD_DATA();		//读回0X00	 
	LCD_WR_REG(0XDB00);	
	lcddev.id=LCD_RD_DATA();		//读回0X80
	lcddev.id<<=8;	
	LCD_WR_REG(0XDC00);	
	lcddev.id|=LCD_RD_DATA();		//读回0X00		
	if(lcddev.id==0x8000)lcddev.id=0x5510;//NT35510读回的ID是8000H,为方便区分,我们强制设置为5510
 	printf(" LCD ID:%x\r\n",lcddev.id); //打印LCD ID   
	if(lcddev.id==0x5510)
	{
		LCD_WriteReg(0xF000,0x55);
		LCD_WriteReg(0xF001,0xAA);
		LCD_WriteReg(0xF002,0x52);
		LCD_WriteReg(0xF003,0x08);
		LCD_WriteReg(0xF004,0x01);
		//AVDD Set AVDD 5.2V
		LCD_WriteReg(0xB000,0x0D);
		LCD_WriteReg(0xB001,0x0D);
		LCD_WriteReg(0xB002,0x0D);
		//AVDD ratio
		LCD_WriteReg(0xB600,0x34);
		LCD_WriteReg(0xB601,0x34);
		LCD_WriteReg(0xB602,0x34);
		//AVEE -5.2V
		LCD_WriteReg(0xB100,0x0D);
		LCD_WriteReg(0xB101,0x0D);
		LCD_WriteReg(0xB102,0x0D);
		//AVEE ratio
		LCD_WriteReg(0xB700,0x34);
		LCD_WriteReg(0xB701,0x34);
		LCD_WriteReg(0xB702,0x34);
		//VCL -2.5V
		LCD_WriteReg(0xB200,0x00);
		LCD_WriteReg(0xB201,0x00);
		LCD_WriteReg(0xB202,0x00);
		//VCL ratio
		LCD_WriteReg(0xB800,0x24);
		LCD_WriteReg(0xB801,0x24);
		LCD_WriteReg(0xB802,0x24);
		//VGH 15V (Free pump)
		LCD_WriteReg(0xBF00,0x01);
		LCD_WriteReg(0xB300,0x0F);
		LCD_WriteReg(0xB301,0x0F);
		LCD_WriteReg(0xB302,0x0F);
		//VGH ratio
		LCD_WriteReg(0xB900,0x34);
		LCD_WriteReg(0xB901,0x34);
		LCD_WriteReg(0xB902,0x34);
		//VGL_REG -10V
		LCD_WriteReg(0xB500,0x08);
		LCD_WriteReg(0xB501,0x08);
		LCD_WriteReg(0xB502,0x08);
		LCD_WriteReg(0xC200,0x03);
		//VGLX ratio
		LCD_WriteReg(0xBA00,0x24);
		LCD_WriteReg(0xBA01,0x24);
		LCD_WriteReg(0xBA02,0x24);
		//VGMP/VGSP 4.5V/0V
		LCD_WriteReg(0xBC00,0x00);
		LCD_WriteReg(0xBC01,0x78);
		LCD_WriteReg(0xBC02,0x00);
		//VGMN/VGSN -4.5V/0V
		LCD_WriteReg(0xBD00,0x00);
		LCD_WriteReg(0xBD01,0x78);
		LCD_WriteReg(0xBD02,0x00);
		//VCOM
		LCD_WriteReg(0xBE00,0x00);
		LCD_WriteReg(0xBE01,0x64);
		//Gamma Setting
		LCD_WriteReg(0xD100,0x00);
		LCD_WriteReg(0xD101,0x33);
		LCD_WriteReg(0xD102,0x00);
		LCD_WriteReg(0xD103,0x34);
		LCD_WriteReg(0xD104,0x00);
		LCD_WriteReg(0xD105,0x3A);
		LCD_WriteReg(0xD106,0x00);
		LCD_WriteReg(0xD107,0x4A);
		LCD_WriteReg(0xD108,0x00);
		LCD_WriteReg(0xD109,0x5C);
		LCD_WriteReg(0xD10A,0x00);
		LCD_WriteReg(0xD10B,0x81);
		LCD_WriteReg(0xD10C,0x00);
		LCD_WriteReg(0xD10D,0xA6);
		LCD_WriteReg(0xD10E,0x00);
		LCD_WriteReg(0xD10F,0xE5);
		LCD_WriteReg(0xD110,0x01);
		LCD_WriteReg(0xD111,0x13);
		LCD_WriteReg(0xD112,0x01);
		LCD_WriteReg(0xD113,0x54);
		LCD_WriteReg(0xD114,0x01);
		LCD_WriteReg(0xD115,0x82);
		LCD_WriteReg(0xD116,0x01);
		LCD_WriteReg(0xD117,0xCA);
		LCD_WriteReg(0xD118,0x02);
		LCD_WriteReg(0xD119,0x00);
		LCD_WriteReg(0xD11A,0x02);
		LCD_WriteReg(0xD11B,0x01);
		LCD_WriteReg(0xD11C,0x02);
		LCD_WriteReg(0xD11D,0x34);
		LCD_WriteReg(0xD11E,0x02);
		LCD_WriteReg(0xD11F,0x67);
		LCD_WriteReg(0xD120,0x02);
		LCD_WriteReg(0xD121,0x84);
		LCD_WriteReg(0xD122,0x02);
		LCD_WriteReg(0xD123,0xA4);
		LCD_WriteReg(0xD124,0x02);
		LCD_WriteReg(0xD125,0xB7);
		LCD_WriteReg(0xD126,0x02);
		LCD_WriteReg(0xD127,0xCF);
		LCD_WriteReg(0xD128,0x02);
		LCD_WriteReg(0xD129,0xDE);
		LCD_WriteReg(0xD12A,0x02);
		LCD_WriteReg(0xD12B,0xF2);
		LCD_WriteReg(0xD12C,0x02);
		LCD_WriteReg(0xD12D,0xFE);
		LCD_WriteReg(0xD12E,0x03);
		LCD_WriteReg(0xD12F,0x10);
		LCD_WriteReg(0xD130,0x03);
		LCD_WriteReg(0xD131,0x33);
		LCD_WriteReg(0xD132,0x03);
		LCD_WriteReg(0xD133,0x6D);
		LCD_WriteReg(0xD200,0x00);
		LCD_WriteReg(0xD201,0x33);
		LCD_WriteReg(0xD202,0x00);
		LCD_WriteReg(0xD203,0x34);
		LCD_WriteReg(0xD204,0x00);
		LCD_WriteReg(0xD205,0x3A);
		LCD_WriteReg(0xD206,0x00);
		LCD_WriteReg(0xD207,0x4A);
		LCD_WriteReg(0xD208,0x00);
		LCD_WriteReg(0xD209,0x5C);
		LCD_WriteReg(0xD20A,0x00);

		LCD_WriteReg(0xD20B,0x81);
		LCD_WriteReg(0xD20C,0x00);
		LCD_WriteReg(0xD20D,0xA6);
		LCD_WriteReg(0xD20E,0x00);
		LCD_WriteReg(0xD20F,0xE5);
		LCD_WriteReg(0xD210,0x01);
		LCD_WriteReg(0xD211,0x13);
		LCD_WriteReg(0xD212,0x01);
		LCD_WriteReg(0xD213,0x54);
		LCD_WriteReg(0xD214,0x01);
		LCD_WriteReg(0xD215,0x82);
		LCD_WriteReg(0xD216,0x01);
		LCD_WriteReg(0xD217,0xCA);
		LCD_WriteReg(0xD218,0x02);
		LCD_WriteReg(0xD219,0x00);
		LCD_WriteReg(0xD21A,0x02);
		LCD_WriteReg(0xD21B,0x01);
		LCD_WriteReg(0xD21C,0x02);
		LCD_WriteReg(0xD21D,0x34);
		LCD_WriteReg(0xD21E,0x02);
		LCD_WriteReg(0xD21F,0x67);
		LCD_WriteReg(0xD220,0x02);
		LCD_WriteReg(0xD221,0x84);
		LCD_WriteReg(0xD222,0x02);
		LCD_WriteReg(0xD223,0xA4);
		LCD_WriteReg(0xD224,0x02);
		LCD_WriteReg(0xD225,0xB7);
		LCD_WriteReg(0xD226,0x02);
		LCD_WriteReg(0xD227,0xCF);
		LCD_WriteReg(0xD228,0x02);
		LCD_WriteReg(0xD229,0xDE);
		LCD_WriteReg(0xD22A,0x02);
		LCD_WriteReg(0xD22B,0xF2);
		LCD_WriteReg(0xD22C,0x02);
		LCD_WriteReg(0xD22D,0xFE);
		LCD_WriteReg(0xD22E,0x03);
		LCD_WriteReg(0xD22F,0x10);
		LCD_WriteReg(0xD230,0x03);
		LCD_WriteReg(0xD231,0x33);
		LCD_WriteReg(0xD232,0x03);
		LCD_WriteReg(0xD233,0x6D);
		LCD_WriteReg(0xD300,0x00);
		LCD_WriteReg(0xD301,0x33);
		LCD_WriteReg(0xD302,0x00);
		LCD_WriteReg(0xD303,0x34);
		LCD_WriteReg(0xD304,0x00);
		LCD_WriteReg(0xD305,0x3A);
		LCD_WriteReg(0xD306,0x00);
		LCD_WriteReg(0xD307,0x4A);
		LCD_WriteReg(0xD308,0x00);
		LCD_WriteReg(0xD309,0x5C);
		LCD_WriteReg(0xD30A,0x00);

		LCD_WriteReg(0xD30B,0x81);
		LCD_WriteReg(0xD30C,0x00);
		LCD_WriteReg(0xD30D,0xA6);
		LCD_WriteReg(0xD30E,0x00);
		LCD_WriteReg(0xD30F,0xE5);
		LCD_WriteReg(0xD310,0x01);
		LCD_WriteReg(0xD311,0x13);
		LCD_WriteReg(0xD312,0x01);
		LCD_WriteReg(0xD313,0x54);
		LCD_WriteReg(0xD314,0x01);
		LCD_WriteReg(0xD315,0x82);
		LCD_WriteReg(0xD316,0x01);
		LCD_WriteReg(0xD317,0xCA);
		LCD_WriteReg(0xD318,0x02);
		LCD_WriteReg(0xD319,0x00);
		LCD_WriteReg(0xD31A,0x02);
		LCD_WriteReg(0xD31B,0x01);
		LCD_WriteReg(0xD31C,0x02);
		LCD_WriteReg(0xD31D,0x34);
		LCD_WriteReg(0xD31E,0x02);
		LCD_WriteReg(0xD31F,0x67);
		LCD_WriteReg(0xD320,0x02);
		LCD_WriteReg(0xD321,0x84);
		LCD_WriteReg(0xD322,0x02);
		LCD_WriteReg(0xD323,0xA4);
		LCD_WriteReg(0xD324,0x02);
		LCD_WriteReg(0xD325,0xB7);
		LCD_WriteReg(0xD326,0x02);
		LCD_WriteReg(0xD327,0xCF);
		LCD_WriteReg(0xD328,0x02);
		LCD_WriteReg(0xD329,0xDE);
		LCD_WriteReg(0xD32A,0x02);
		LCD_WriteReg(0xD32B,0xF2);
		LCD_WriteReg(0xD32C,0x02);
		LCD_WriteReg(0xD32D,0xFE);
		LCD_WriteReg(0xD32E,0x03);
		LCD_WriteReg(0xD32F,0x10);
		LCD_WriteReg(0xD330,0x03);
		LCD_WriteReg(0xD331,0x33);
		LCD_WriteReg(0xD332,0x03);
		LCD_WriteReg(0xD333,0x6D);
		LCD_WriteReg(0xD400,0x00);
		LCD_WriteReg(0xD401,0x33);
		LCD_WriteReg(0xD402,0x00);
		LCD_WriteReg(0xD403,0x34);
		LCD_WriteReg(0xD404,0x00);
		LCD_WriteReg(0xD405,0x3A);
		LCD_WriteReg(0xD406,0x00);
		LCD_WriteReg(0xD407,0x4A);
		LCD_WriteReg(0xD408,0x00);
		LCD_WriteReg(0xD409,0x5C);
		LCD_WriteReg(0xD40A,0x00);
		LCD_WriteReg(0xD40B,0x81);

		LCD_WriteReg(0xD40C,0x00);
		LCD_WriteReg(0xD40D,0xA6);
		LCD_WriteReg(0xD40E,0x00);
		LCD_WriteReg(0xD40F,0xE5);
		LCD_WriteReg(0xD410,0x01);
		LCD_WriteReg(0xD411,0x13);
		LCD_WriteReg(0xD412,0x01);
		LCD_WriteReg(0xD413,0x54);
		LCD_WriteReg(0xD414,0x01);
		LCD_WriteReg(0xD415,0x82);
		LCD_WriteReg(0xD416,0x01);
		LCD_WriteReg(0xD417,0xCA);
		LCD_WriteReg(0xD418,0x02);
		LCD_WriteReg(0xD419,0x00);
		LCD_WriteReg(0xD41A,0x02);
		LCD_WriteReg(0xD41B,0x01);
		LCD_WriteReg(0xD41C,0x02);
		LCD_WriteReg(0xD41D,0x34);
		LCD_WriteReg(0xD41E,0x02);
		LCD_WriteReg(0xD41F,0x67);
		LCD_WriteReg(0xD420,0x02);
		LCD_WriteReg(0xD421,0x84);
		LCD_WriteReg(0xD422,0x02);
		LCD_WriteReg(0xD423,0xA4);
		LCD_WriteReg(0xD424,0x02);
		LCD_WriteReg(0xD425,0xB7);
		LCD_WriteReg(0xD426,0x02);
		LCD_WriteReg(0xD427,0xCF);
		LCD_WriteReg(0xD428,0x02);
		LCD_WriteReg(0xD429,0xDE);
		LCD_WriteReg(0xD42A,0x02);
		LCD_WriteReg(0xD42B,0xF2);
		LCD_WriteReg(0xD42C,0x02);
		LCD_WriteReg(0xD42D,0xFE);
		LCD_WriteReg(0xD42E,0x03);
		LCD_WriteReg(0xD42F,0x10);
		LCD_WriteReg(0xD430,0x03);
		LCD_WriteReg(0xD431,0x33);
		LCD_WriteReg(0xD432,0x03);
		LCD_WriteReg(0xD433,0x6D);
		LCD_WriteReg(0xD500,0x00);
		LCD_WriteReg(0xD501,0x33);
		LCD_WriteReg(0xD502,0x00);
		LCD_WriteReg(0xD503,0x34);
		LCD_WriteReg(0xD504,0x00);
		LCD_WriteReg(0xD505,0x3A);
		LCD_WriteReg(0xD506,0x00);
		LCD_WriteReg(0xD507,0x4A);
		LCD_WriteReg(0xD508,0x00);
		LCD_WriteReg(0xD509,0x5C);
		LCD_WriteReg(0xD50A,0x00);
		LCD_WriteReg(0xD50B,0x81);

		LCD_WriteReg(0xD50C,0x00);
		LCD_WriteReg(0xD50D,0xA6);
		LCD_WriteReg(0xD50E,0x00);
		LCD_WriteReg(0xD50F,0xE5);
		LCD_WriteReg(0xD510,0x01);
		LCD_WriteReg(0xD511,0x13);
		LCD_WriteReg(0xD512,0x01);
		LCD_WriteReg(0xD513,0x54);
		LCD_WriteReg(0xD514,0x01);
		LCD_WriteReg(0xD515,0x82);
		LCD_WriteReg(0xD516,0x01);
		LCD_WriteReg(0xD517,0xCA);
		LCD_WriteReg(0xD518,0x02);
		LCD_WriteReg(0xD519,0x00);
		LCD_WriteReg(0xD51A,0x02);
		LCD_WriteReg(0xD51B,0x01);
		LCD_WriteReg(0xD51C,0x02);
		LCD_WriteReg(0xD51D,0x34);
		LCD_WriteReg(0xD51E,0x02);
		LCD_WriteReg(0xD51F,0x67);
		LCD_WriteReg(0xD520,0x02);
		LCD_WriteReg(0xD521,0x84);
		LCD_WriteReg(0xD522,0x02);
		LCD_WriteReg(0xD523,0xA4);
		LCD_WriteReg(0xD524,0x02);
		LCD_WriteReg(0xD525,0xB7);
		LCD_WriteReg(0xD526,0x02);
		LCD_WriteReg(0xD527,0xCF);
		LCD_WriteReg(0xD528,0x02);
		LCD_WriteReg(0xD529,0xDE);
		LCD_WriteReg(0xD52A,0x02);
		LCD_WriteReg(0xD52B,0xF2);
		LCD_WriteReg(0xD52C,0x02);
		LCD_WriteReg(0xD52D,0xFE);
		LCD_WriteReg(0xD52E,0x03);
		LCD_WriteReg(0xD52F,0x10);
		LCD_WriteReg(0xD530,0x03);
		LCD_WriteReg(0xD531,0x33);
		LCD_WriteReg(0xD532,0x03);
		LCD_WriteReg(0xD533,0x6D);
		LCD_WriteReg(0xD600,0x00);
		LCD_WriteReg(0xD601,0x33);
		LCD_WriteReg(0xD602,0x00);
		LCD_WriteReg(0xD603,0x34);
		LCD_WriteReg(0xD604,0x00);
		LCD_WriteReg(0xD605,0x3A);
		LCD_WriteReg(0xD606,0x00);
		LCD_WriteReg(0xD607,0x4A);
		LCD_WriteReg(0xD608,0x00);
		LCD_WriteReg(0xD609,0x5C);
		LCD_WriteReg(0xD60A,0x00);
		LCD_WriteReg(0xD60B,0x81);

		LCD_WriteReg(0xD60C,0x00);
		LCD_WriteReg(0xD60D,0xA6);
		LCD_WriteReg(0xD60E,0x00);
		LCD_WriteReg(0xD60F,0xE5);
		LCD_WriteReg(0xD610,0x01);
		LCD_WriteReg(0xD611,0x13);
		LCD_WriteReg(0xD612,0x01);
		LCD_WriteReg(0xD613,0x54);
		LCD_WriteReg(0xD614,0x01);
		LCD_WriteReg(0xD615,0x82);
		LCD_WriteReg(0xD616,0x01);
		LCD_WriteReg(0xD617,0xCA);
		LCD_WriteReg(0xD618,0x02);
		LCD_WriteReg(0xD619,0x00);
		LCD_WriteReg(0xD61A,0x02);
		LCD_WriteReg(0xD61B,0x01);
		LCD_WriteReg(0xD61C,0x02);
		LCD_WriteReg(0xD61D,0x34);
		LCD_WriteReg(0xD61E,0x02);
		LCD_WriteReg(0xD61F,0x67);
		LCD_WriteReg(0xD620,0x02);
		LCD_WriteReg(0xD621,0x84);
		LCD_WriteReg(0xD622,0x02);
		LCD_WriteReg(0xD623,0xA4);
		LCD_WriteReg(0xD624,0x02);
		LCD_WriteReg(0xD625,0xB7);
		LCD_WriteReg(0xD626,0x02);
		LCD_WriteReg(0xD627,0xCF);
		LCD_WriteReg(0xD628,0x02);
		LCD_WriteReg(0xD629,0xDE);
		LCD_WriteReg(0xD62A,0x02);
		LCD_WriteReg(0xD62B,0xF2);
		LCD_WriteReg(0xD62C,0x02);
		LCD_WriteReg(0xD62D,0xFE);
		LCD_WriteReg(0xD62E,0x03);
		LCD_WriteReg(0xD62F,0x10);
		LCD_WriteReg(0xD630,0x03);
		LCD_WriteReg(0xD631,0x33);
		LCD_WriteReg(0xD632,0x03);
		LCD_WriteReg(0xD633,0x6D);
		//LV2 Page 0 enable
		LCD_WriteReg(0xF000,0x55);
		LCD_WriteReg(0xF001,0xAA);
		LCD_WriteReg(0xF002,0x52);
		LCD_WriteReg(0xF003,0x08);
		LCD_WriteReg(0xF004,0x00);
		//Display control
		LCD_WriteReg(0xB100, 0xCC);
		LCD_WriteReg(0xB101, 0x00);
		//Source hold time
		LCD_WriteReg(0xB600,0x05);
		//Gate EQ control
		LCD_WriteReg(0xB700,0x70);
		LCD_WriteReg(0xB701,0x70);
		//Source EQ control (Mode 2)
		LCD_WriteReg(0xB800,0x01);
		LCD_WriteReg(0xB801,0x03);
		LCD_WriteReg(0xB802,0x03);
		LCD_WriteReg(0xB803,0x03);
		//Inversion mode (2-dot)
		LCD_WriteReg(0xBC00,0x02);
		LCD_WriteReg(0xBC01,0x00);
		LCD_WriteReg(0xBC02,0x00);
		//Timing control 4H w/ 4-delay
		LCD_WriteReg(0xC900,0xD0);
		LCD_WriteReg(0xC901,0x02);
		LCD_WriteReg(0xC902,0x50);
		LCD_WriteReg(0xC903,0x50);
		LCD_WriteReg(0xC904,0x50);
		LCD_WriteReg(0x3500,0x00);
		LCD_WriteReg(0x3A00,0x55);  //16-bit/pixel
		LCD_WR_REG(0x1100);
		delay_us(120);
		LCD_WR_REG(0x2900);
	}
	LCD_Display_Dir(0);		//默认为竖屏
	LCD_LED=1;				//点亮背光
	LCD_Clear(WHITE);
}  
//清屏函数
//color:要清屏的填充色
void LCD_Clear(u16 color)
{
	u32 index=0;      
	u32 totalpoint=lcddev.width;
	totalpoint*=lcddev.height; 			//得到总点数
	if((lcddev.id==0X6804)&&(lcddev.dir==1))//6804横屏的时候特殊处理  
	{						    
 		lcddev.dir=0;	 
 		lcddev.setxcmd=0X2A;
		lcddev.setycmd=0X2B;  	 			
		LCD_SetCursor(0x00,0x0000);		//设置光标位置  
 		lcddev.dir=1;	 
  		lcddev.setxcmd=0X2B;
		lcddev.setycmd=0X2A;  	 
 	}else LCD_SetCursor(0x00,0x0000);	//设置光标位置 
	LCD_WriteRAM_Prepare();     		//开始写入GRAM	 	  
	for(index=0;index<totalpoint;index++)
	{
		LCD->LCD_RAM=color;	
	}
}  
//在指定区域内填充单个颜色
//(sx,sy),(ex,ey):填充矩形对角坐标,区域大小为:(ex-sx+1)*(ey-sy+1)   
//color:要填充的颜色
void LCD_Fill(u16 sx,u16 sy,u16 ex,u16 ey,u16 color)
{          
	u16 i,j;
	u16 xlen=0;
	u16 temp;
	if((lcddev.id==0X6804)&&(lcddev.dir==1))	//6804横屏的时候特殊处理  
	{
		temp=sx;
		sx=sy;
		sy=lcddev.width-ex-1;	  
		ex=ey;
		ey=lcddev.width-temp-1;
 		lcddev.dir=0;	 
 		lcddev.setxcmd=0X2A;
		lcddev.setycmd=0X2B;  	 			
		LCD_Fill(sx,sy,ex,ey,color);  
 		lcddev.dir=1;	 
  		lcddev.setxcmd=0X2B;
		lcddev.setycmd=0X2A;  	 
 	}else
	{
		xlen=ex-sx+1;	 
		for(i=sy;i<=ey;i++)
		{
		 	LCD_SetCursor(sx,i);      				//设置光标位置 
			LCD_WriteRAM_Prepare();     			//开始写入GRAM	  
			for(j=0;j<xlen;j++)LCD->LCD_RAM=color;	//显示颜色 	    
		}
	}	 
}  
//在指定区域内填充指定颜色块			 
//(sx,sy),(ex,ey):填充矩形对角坐标,区域大小为:(ex-sx+1)*(ey-sy+1)   
//color:要填充的颜色
void LCD_Color_Fill(u16 sx,u16 sy,u16 ex,u16 ey,u16 *color)
{  
	u16 height,width;
	u16 i,j;
	width=ex-sx+1; 			//得到填充的宽度
	height=ey-sy+1;			//高度
 	for(i=0;i<height;i++)
	{
 		LCD_SetCursor(sx,sy+i);   	//设置光标位置 
		LCD_WriteRAM_Prepare();     //开始写入GRAM
		for(j=0;j<width;j++)LCD->LCD_RAM=color[i*width+j];//写入数据 
	}		  
}  
//画线
//x1,y1:起点坐标
//x2,y2:终点坐标  
void LCD_DrawLine(u16 x1, u16 y1, u16 x2, u16 y2)
{
	u16 t; 
	int xerr=0,yerr=0,delta_x,delta_y,distance; 
	int incx,incy,uRow,uCol; 
	delta_x=x2-x1; //计算坐标增量 
	delta_y=y2-y1; 
	uRow=x1; 
	uCol=y1; 
	if(delta_x>0)incx=1; //设置单步方向 
	else if(delta_x==0)incx=0;//垂直线 
	else {incx=-1;delta_x=-delta_x;} 
	if(delta_y>0)incy=1; 
	else if(delta_y==0)incy=0;//水平线 
	else{incy=-1;delta_y=-delta_y;} 
	if( delta_x>delta_y)distance=delta_x; //选取基本增量坐标轴 
	else distance=delta_y; 
	for(t=0;t<=distance+1;t++ )//画线输出 
	{  
		LCD_DrawPoint(uRow,uCol);//画点 
		xerr+=delta_x ; 
		yerr+=delta_y ; 
		if(xerr>distance) 
		{ 
			xerr-=distance; 
			uRow+=incx; 
		} 
		if(yerr>distance) 
		{ 
			yerr-=distance; 
			uCol+=incy; 
		} 
	}  
}    
//画矩形	  
//(x1,y1),(x2,y2):矩形的对角坐标
void LCD_DrawRectangle(u16 x1, u16 y1, u16 x2, u16 y2)
{
	LCD_DrawLine(x1,y1,x2,y1);
	LCD_DrawLine(x1,y1,x1,y2);
	LCD_DrawLine(x1,y2,x2,y2);
	LCD_DrawLine(x2,y1,x2,y2);
}
//在指定位置画一个指定大小的圆
//(x,y):中心点
//r    :半径
void LCD_Draw_Circle(u16 x0,u16 y0,u8 r)
{
	int a,b;
	int di;
	a=0;b=r;	  
	di=3-(r<<1);             //判断下个点位置的标志
	while(a<=b)
	{
		LCD_DrawPoint(x0+a,y0-b);             //5
 		LCD_DrawPoint(x0+b,y0-a);             //0           
		LCD_DrawPoint(x0+b,y0+a);             //4               
		LCD_DrawPoint(x0+a,y0+b);             //6 
		LCD_DrawPoint(x0-a,y0+b);             //1       
 		LCD_DrawPoint(x0-b,y0+a);             
		LCD_DrawPoint(x0-a,y0-b);             //2             
  		LCD_DrawPoint(x0-b,y0-a);             //7     	         
		a++;
		//使用Bresenham算法画圆     
		if(di<0)di +=4*a+6;	  
		else
		{
			di+=10+4*(a-b);   
			b--;
		} 						    
	}
} 									  
//在指定位置显示一个字符
//x,y:起始坐标
//num:要显示的字符:" "--->"~"
//size:字体大小 12/16/24
//mode:叠加方式(1)还是非叠加方式(0)
void LCD_ShowChar(u16 x,u16 y,u8 num,u8 size,u8 mode)
{  							  
    u8 temp,t1,t;
	u16 y0=y;
	u8 csize=(size/8+((size%8)?1:0))*(size/2);		//得到字体一个字符对应点阵集所占的字节数	
 	num=num-' ';//得到偏移后的值（ASCII字库是从空格开始取模，所以-' '就是对应字符的字库）
	for(t=0;t<csize;t++)
	{   
		if(size==12)temp=asc2_1206[num][t]; 	 	//调用1206字体
		else if(size==16)temp=asc2_1608[num][t];	//调用1608字体
		else if(size==24)temp=asc2_2412[num][t];	//调用2412字体
		else return;								//没有的字库
		for(t1=0;t1<8;t1++)
		{			    
			if(temp&0x80)LCD_Fast_DrawPoint(x,y,POINT_COLOR);
			else if(mode==0)LCD_Fast_DrawPoint(x,y,BACK_COLOR);
			temp<<=1;
			y++;
			if(y>=lcddev.height)return;		//超区域了
			if((y-y0)==size)
			{
				y=y0;
				x++;
				if(x>=lcddev.width)return;	//超区域了
				break;
			}
		}  	 
	}  	    	   	 	  
}   
//m^n函数
//返回值:m^n次方.
u32 LCD_Pow(u8 m,u8 n)
{
	u32 result=1;	 
	while(n--)result*=m;    
	return result;
}			 
//显示数字,高位为0,则不显示
//x,y :起点坐标	 
//len :数字的位数
//size:字体大小
//color:颜色 
//num:数值(0~4294967295);	 
void LCD_ShowNum(u16 x,u16 y,u32 num,u8 len,u8 size)
{         	
	u8 t,temp;
	u8 enshow=0;						   
	for(t=0;t<len;t++)
	{
		temp=(num/LCD_Pow(10,len-t-1))%10;
		if(enshow==0&&t<(len-1))
		{
			if(temp==0)
			{
				LCD_ShowChar(x+(size/2)*t,y,' ',size,0);
				continue;
			}else enshow=1; 
		 	 
		}
	 	LCD_ShowChar(x+(size/2)*t,y,temp+'0',size,0); 
	}
} 
//显示数字,高位为0,还是显示
//x,y:起点坐标
//num:数值(0~999999999);	 
//len:长度(即要显示的位数)
//size:字体大小
//mode:
//[7]:0,不填充;1,填充0.
//[6:1]:保留
//[0]:0,非叠加显示;1,叠加显示.
void LCD_ShowxNum(u16 x,u16 y,u32 num,u8 len,u8 size,u8 mode)
{  
	u8 t,temp;
	u8 enshow=0;						   
	for(t=0;t<len;t++)
	{
		temp=(num/LCD_Pow(10,len-t-1))%10;
		if(enshow==0&&t<(len-1))
		{
			if(temp==0)
			{
				if(mode&0X80)LCD_ShowChar(x+(size/2)*t,y,'0',size,mode&0X01);  
				else LCD_ShowChar(x+(size/2)*t,y,' ',size,mode&0X01);  
 				continue;
			}else enshow=1; 
		 	 
		}
	 	LCD_ShowChar(x+(size/2)*t,y,temp+'0',size,mode&0X01); 
	}
} 
//显示字符串
//x,y:起点坐标
//width,height:区域大小  
//size:字体大小
//*p:字符串起始地址		  
void LCD_ShowString(u16 x,u16 y,u16 width,u16 height,u8 size,u8 *p)
{         
	u8 x0=x;
	width+=x;
	height+=y;
    while((*p<='~')&&(*p>=' '))//判断是不是非法字符!
    {       
        if(x>=width){x=x0;y+=size;}
        if(y>=height)break;//退出
        LCD_ShowChar(x,y,*p,size,0);
        x+=size/2;
        p++;
    }  
}

/*
LCD REG:0x6C0007FE
LCD RAM:0x6C000800
LCD ID:5510

WREG:0x0000DA00
WREG:0x0000DB00
WREG:0x0000DC00

WREG:0x0000F000
WDAT:0x00000055
WREG:0x0000F001
WDAT:0x000000AA
WREG:0x0000F002
WDAT:0x00000052
WREG:0x0000F003
WDAT:0x00000008
WREG:0x0000F004
WDAT:0x00000001
WREG:0x0000B000
WDAT:0x0000000D
WREG:0x0000B001
WDAT:0x0000000D
WREG:0x0000B002
WDAT:0x0000000D
WREG:0x0000B600
WDAT:0x00000034
WREG:0x0000B601
WDAT:0x00000034
WREG:0x0000B602
WDAT:0x00000034
WREG:0x0000B100
WDAT:0x0000000D
WREG:0x0000B101
WDAT:0x0000000D
WREG:0x0000B102
WDAT:0x0000000D
WREG:0x0000B700
WDAT:0x00000034
WREG:0x0000B701
WDAT:0x00000034
WREG:0x0000B702
WDAT:0x00000034
WREG:0x0000B200
WDAT:0x00000000
WREG:0x0000B201
WDAT:0x00000000
WREG:0x0000B202
WDAT:0x00000000
WREG:0x0000B800
WDAT:0x00000024
WREG:0x0000B801
WDAT:0x00000024
WREG:0x0000B802
WDAT:0x00000024
WREG:0x0000BF00
WDAT:0x00000001
WREG:0x0000B300
WDAT:0x0000000F
WREG:0x0000B301
WDAT:0x0000000F
WREG:0x0000B302
WDAT:0x0000000F
WREG:0x0000B900
WDAT:0x00000034
WREG:0x0000B901
WDAT:0x00000034
WREG:0x0000B902
WDAT:0x00000034
WREG:0x0000B500
WDAT:0x00000008
WREG:0x0000B501
WDAT:0x00000008
WREG:0x0000B502
WDAT:0x00000008
WREG:0x0000C200
WDAT:0x00000003
WREG:0x0000BA00
WDAT:0x00000024
WREG:0x0000BA01
WDAT:0x00000024
WREG:0x0000BA02
WDAT:0x00000024
WREG:0x0000BC00
WDAT:0x00000000
WREG:0x0000BC01
WDAT:0x00000078
WREG:0x0000BC02
WDAT:0x00000000
WREG:0x0000BD00
WDAT:0x00000000
WREG:0x0000BD01
WDAT:0x00000078
WREG:0x0000BD02
WDAT:0x00000000
WREG:0x0000BE00
WDAT:0x00000000
WREG:0x0000BE01
WDAT:0x00000064
WREG:0x0000D100
WDAT:0x00000000
WREG:0x0000D101
WDAT:0x00000033
WREG:0x0000D102
WDAT:0x00000000
WREG:0x0000D103
WDAT:0x00000034
WREG:0x0000D104
WDAT:0x00000000
WREG:0x0000D105
WDAT:0x0000003A
WREG:0x0000D106
WDAT:0x00000000
WREG:0x0000D107
WDAT:0x0000004A
WREG:0x0000D108
WDAT:0x00000000
WREG:0x0000D109
WDAT:0x0000005C
WREG:0x0000D10A
WDAT:0x00000000
WREG:0x0000D10B
WDAT:0x00000081
WREG:0x0000D10C
WDAT:0x00000000
WREG:0x0000D10D
WDAT:0x000000A6
WREG:0x0000D10E
WDAT:0x00000000
WREG:0x0000D10F
WDAT:0x000000E5
WREG:0x0000D110
WDAT:0x00000001
WREG:0x0000D111
WDAT:0x00000013
WREG:0x0000D112
WDAT:0x00000001
WREG:0x0000D113
WDAT:0x00000054
WREG:0x0000D114
WDAT:0x00000001
WREG:0x0000D115
WDAT:0x00000082
WREG:0x0000D116
WDAT:0x00000001
WREG:0x0000D117
WDAT:0x000000CA
WREG:0x0000D118
WDAT:0x00000002
WREG:0x0000D119
WDAT:0x00000000
WREG:0x0000D11A
WDAT:0x00000002
WREG:0x0000D11B
WDAT:0x00000001
WREG:0x0000D11C
WDAT:0x00000002
WREG:0x0000D11D
WDAT:0x00000034
WREG:0x0000D11E
WDAT:0x00000002
WREG:0x0000D11F
WDAT:0x00000067
WREG:0x0000D120
WDAT:0x00000002
WREG:0x0000D121
WDAT:0x00000084
WREG:0x0000D122
WDAT:0x00000002
WREG:0x0000D123
WDAT:0x000000A4
WREG:0x0000D124
WDAT:0x00000002
WREG:0x0000D125
WDAT:0x000000B7
WREG:0x0000D126
WDAT:0x00000002
WREG:0x0000D127
WDAT:0x000000CF
WREG:0x0000D128
WDAT:0x00000002
WREG:0x0000D129
WDAT:0x000000DE
WREG:0x0000D12A
WDAT:0x00000002
WREG:0x0000D12B
WDAT:0x000000F2
WREG:0x0000D12C
WDAT:0x00000002
WREG:0x0000D12D
WDAT:0x000000FE
WREG:0x0000D12E
WDAT:0x00000003
WREG:0x0000D12F
WDAT:0x00000010
WREG:0x0000D130
WDAT:0x00000003
WREG:0x0000D131
WDAT:0x00000033
WREG:0x0000D132
WDAT:0x00000003
WREG:0x0000D133
WDAT:0x0000006D
WREG:0x0000D200
WDAT:0x00000000
WREG:0x0000D201
WDAT:0x00000033
WREG:0x0000D202
WDAT:0x00000000
WREG:0x0000D203
WDAT:0x00000034
WREG:0x0000D204
WDAT:0x00000000
WREG:0x0000D205
WDAT:0x0000003A
WREG:0x0000D206
WDAT:0x00000000
WREG:0x0000D207
WDAT:0x0000004A
WREG:0x0000D208
WDAT:0x00000000
WREG:0x0000D209
WDAT:0x0000005C
WREG:0x0000D20A
WDAT:0x00000000
WREG:0x0000D20B
WDAT:0x00000081
WREG:0x0000D20C
WDAT:0x00000000
WREG:0x0000D20D
WDAT:0x000000A6
WREG:0x0000D20E
WDAT:0x00000000
WREG:0x0000D20F
WDAT:0x000000E5
WREG:0x0000D210
WDAT:0x00000001
WREG:0x0000D211
WDAT:0x00000013
WREG:0x0000D212
WDAT:0x00000001
WREG:0x0000D213
WDAT:0x00000054
WREG:0x0000D214
WDAT:0x00000001
WREG:0x0000D215
WDAT:0x00000082
WREG:0x0000D216
WDAT:0x00000001
WREG:0x0000D217
WDAT:0x000000CA
WREG:0x0000D218
WDAT:0x00000002
WREG:0x0000D219
WDAT:0x00000000
WREG:0x0000D21A
WDAT:0x00000002
WREG:0x0000D21B
WDAT:0x00000001
WREG:0x0000D21C
WDAT:0x00000002
WREG:0x0000D21D
WDAT:0x00000034
WREG:0x0000D21E
WDAT:0x00000002
WREG:0x0000D21F
WDAT:0x00000067
WREG:0x0000D220
WDAT:0x00000002
WREG:0x0000D221
WDAT:0x00000084
WREG:0x0000D222
WDAT:0x00000002
WREG:0x0000D223
WDAT:0x000000A4
WREG:0x0000D224
WDAT:0x00000002
WREG:0x0000D225
WDAT:0x000000B7
WREG:0x0000D226
WDAT:0x00000002
WREG:0x0000D227
WDAT:0x000000CF
WREG:0x0000D228
WDAT:0x00000002
WREG:0x0000D229
WDAT:0x000000DE
WREG:0x0000D22A
WDAT:0x00000002
WREG:0x0000D22B
WDAT:0x000000F2
WREG:0x0000D22C
WDAT:0x00000002
WREG:0x0000D22D
WDAT:0x000000FE
WREG:0x0000D22E
WDAT:0x00000003
WREG:0x0000D22F
WDAT:0x00000010
WREG:0x0000D230
WDAT:0x00000003
WREG:0x0000D231
WDAT:0x00000033
WREG:0x0000D232
WDAT:0x00000003
WREG:0x0000D233
WDAT:0x0000006D
WREG:0x0000D300
WDAT:0x00000000
WREG:0x0000D301
WDAT:0x00000033
WREG:0x0000D302
WDAT:0x00000000
WREG:0x0000D303
WDAT:0x00000034
WREG:0x0000D304
WDAT:0x00000000
WREG:0x0000D305
WDAT:0x0000003A
WREG:0x0000D306
WDAT:0x00000000
WREG:0x0000D307
WDAT:0x0000004A
WREG:0x0000D308
WDAT:0x00000000
WREG:0x0000D309
WDAT:0x0000005C
WREG:0x0000D30A
WDAT:0x00000000
WREG:0x0000D30B
WDAT:0x00000081
WREG:0x0000D30C
WDAT:0x00000000
WREG:0x0000D30D
WDAT:0x000000A6
WREG:0x0000D30E
WDAT:0x00000000
WREG:0x0000D30F
WDAT:0x000000E5
WREG:0x0000D310
WDAT:0x00000001
WREG:0x0000D311
WDAT:0x00000013
WREG:0x0000D312
WDAT:0x00000001
WREG:0x0000D313
WDAT:0x00000054
WREG:0x0000D314
WDAT:0x00000001
WREG:0x0000D315
WDAT:0x00000082
WREG:0x0000D316
WDAT:0x00000001
WREG:0x0000D317
WDAT:0x000000CA
WREG:0x0000D318
WDAT:0x00000002
WREG:0x0000D319
WDAT:0x00000000
WREG:0x0000D31A
WDAT:0x00000002
WREG:0x0000D31B
WDAT:0x00000001
WREG:0x0000D31C
WDAT:0x00000002
WREG:0x0000D31D
WDAT:0x00000034
WREG:0x0000D31E
WDAT:0x00000002
WREG:0x0000D31F
WDAT:0x00000067
WREG:0x0000D320
WDAT:0x00000002
WREG:0x0000D321
WDAT:0x00000084
WREG:0x0000D322
WDAT:0x00000002
WREG:0x0000D323
WDAT:0x000000A4
WREG:0x0000D324
WDAT:0x00000002
WREG:0x0000D325
WDAT:0x000000B7
WREG:0x0000D326
WDAT:0x00000002
WREG:0x0000D327
WDAT:0x000000CF
WREG:0x0000D328
WDAT:0x00000002
WREG:0x0000D329
WDAT:0x000000DE
WREG:0x0000D32A
WDAT:0x00000002
WREG:0x0000D32B
WDAT:0x000000F2
WREG:0x0000D32C
WDAT:0x00000002
WREG:0x0000D32D
WDAT:0x000000FE
WREG:0x0000D32E
WDAT:0x00000003
WREG:0x0000D32F
WDAT:0x00000010
WREG:0x0000D330
WDAT:0x00000003
WREG:0x0000D331
WDAT:0x00000033
WREG:0x0000D332
WDAT:0x00000003
WREG:0x0000D333
WDAT:0x0000006D
WREG:0x0000D400
WDAT:0x00000000
WREG:0x0000D401
WDAT:0x00000033
WREG:0x0000D402
WDAT:0x00000000
WREG:0x0000D403
WDAT:0x00000034
WREG:0x0000D404
WDAT:0x00000000
WREG:0x0000D405
WDAT:0x0000003A
WREG:0x0000D406
WDAT:0x00000000
WREG:0x0000D407
WDAT:0x0000004A
WREG:0x0000D408
WDAT:0x00000000
WREG:0x0000D409
WDAT:0x0000005C
WREG:0x0000D40A
WDAT:0x00000000
WREG:0x0000D40B
WDAT:0x00000081
WREG:0x0000D40C
WDAT:0x00000000
WREG:0x0000D40D
WDAT:0x000000A6
WREG:0x0000D40E
WDAT:0x00000000
WREG:0x0000D40F
WDAT:0x000000E5
WREG:0x0000D410
WDAT:0x00000001
WREG:0x0000D411
WDAT:0x00000013
WREG:0x0000D412
WDAT:0x00000001
WREG:0x0000D413
WDAT:0x00000054
WREG:0x0000D414
WDAT:0x00000001
WREG:0x0000D415
WDAT:0x00000082
WREG:0x0000D416
WDAT:0x00000001
WREG:0x0000D417
WDAT:0x000000CA
WREG:0x0000D418
WDAT:0x00000002
WREG:0x0000D419
WDAT:0x00000000
WREG:0x0000D41A
WDAT:0x00000002
WREG:0x0000D41B
WDAT:0x00000001
WREG:0x0000D41C
WDAT:0x00000002
WREG:0x0000D41D
WDAT:0x00000034
WREG:0x0000D41E
WDAT:0x00000002
WREG:0x0000D41F
WDAT:0x00000067
WREG:0x0000D420
WDAT:0x00000002
WREG:0x0000D421
WDAT:0x00000084
WREG:0x0000D422
WDAT:0x00000002
WREG:0x0000D423
WDAT:0x000000A4
WREG:0x0000D424
WDAT:0x00000002
WREG:0x0000D425
WDAT:0x000000B7
WREG:0x0000D426
WDAT:0x00000002
WREG:0x0000D427
WDAT:0x000000CF
WREG:0x0000D428
WDAT:0x00000002
WREG:0x0000D429
WDAT:0x000000DE
WREG:0x0000D42A
WDAT:0x00000002
WREG:0x0000D42B
WDAT:0x000000F2
WREG:0x0000D42C
WDAT:0x00000002
WREG:0x0000D42D
WDAT:0x000000FE
WREG:0x0000D42E
WDAT:0x00000003
WREG:0x0000D42F
WDAT:0x00000010
WREG:0x0000D430
WDAT:0x00000003
WREG:0x0000D431
WDAT:0x00000033
WREG:0x0000D432
WDAT:0x00000003
WREG:0x0000D433
WDAT:0x0000006D
WREG:0x0000D500
WDAT:0x00000000
WREG:0x0000D501
WDAT:0x00000033
WREG:0x0000D502
WDAT:0x00000000
WREG:0x0000D503
WDAT:0x00000034
WREG:0x0000D504
WDAT:0x00000000
WREG:0x0000D505
WDAT:0x0000003A
WREG:0x0000D506
WDAT:0x00000000
WREG:0x0000D507
WDAT:0x0000004A
WREG:0x0000D508
WDAT:0x00000000
WREG:0x0000D509
WDAT:0x0000005C
WREG:0x0000D50A
WDAT:0x00000000
WREG:0x0000D50B
WDAT:0x00000081
WREG:0x0000D50C
WDAT:0x00000000
WREG:0x0000D50D
WDAT:0x000000A6
WREG:0x0000D50E
WDAT:0x00000000
WREG:0x0000D50F
WDAT:0x000000E5
WREG:0x0000D510
WDAT:0x00000001
WREG:0x0000D511
WDAT:0x00000013
WREG:0x0000D512
WDAT:0x00000001
WREG:0x0000D513
WDAT:0x00000054
WREG:0x0000D514
WDAT:0x00000001
WREG:0x0000D515
WDAT:0x00000082
WREG:0x0000D516
WDAT:0x00000001
WREG:0x0000D517
WDAT:0x000000CA
WREG:0x0000D518
WDAT:0x00000002
WREG:0x0000D519
WDAT:0x00000000
WREG:0x0000D51A
WDAT:0x00000002
WREG:0x0000D51B
WDAT:0x00000001
WREG:0x0000D51C
WDAT:0x00000002
WREG:0x0000D51D
WDAT:0x00000034
WREG:0x0000D51E
WDAT:0x00000002
WREG:0x0000D51F
WDAT:0x00000067
WREG:0x0000D520
WDAT:0x00000002
WREG:0x0000D521
WDAT:0x00000084
WREG:0x0000D522
WDAT:0x00000002
WREG:0x0000D523
WDAT:0x000000A4
WREG:0x0000D524
WDAT:0x00000002
WREG:0x0000D525
WDAT:0x000000B7
WREG:0x0000D526
WDAT:0x00000002
WREG:0x0000D527
WDAT:0x000000CF
WREG:0x0000D528
WDAT:0x00000002
WREG:0x0000D529
WDAT:0x000000DE
WREG:0x0000D52A
WDAT:0x00000002
WREG:0x0000D52B
WDAT:0x000000F2
WREG:0x0000D52C
WDAT:0x00000002
WREG:0x0000D52D
WDAT:0x000000FE
WREG:0x0000D52E
WDAT:0x00000003
WREG:0x0000D52F
WDAT:0x00000010
WREG:0x0000D530
WDAT:0x00000003
WREG:0x0000D531
WDAT:0x00000033
WREG:0x0000D532
WDAT:0x00000003
WREG:0x0000D533
WDAT:0x0000006D
WREG:0x0000D600
WDAT:0x00000000
WREG:0x0000D601
WDAT:0x00000033
WREG:0x0000D602
WDAT:0x00000000
WREG:0x0000D603
WDAT:0x00000034
WREG:0x0000D604
WDAT:0x00000000
WREG:0x0000D605
WDAT:0x0000003A
WREG:0x0000D606
WDAT:0x00000000
WREG:0x0000D607
WDAT:0x0000004A
WREG:0x0000D608
WDAT:0x00000000
WREG:0x0000D609
WDAT:0x0000005C
WREG:0x0000D60A
WDAT:0x00000000
WREG:0x0000D60B
WDAT:0x00000081
WREG:0x0000D60C
WDAT:0x00000000
WREG:0x0000D60D
WDAT:0x000000A6
WREG:0x0000D60E
WDAT:0x00000000
WREG:0x0000D60F
WDAT:0x000000E5
WREG:0x0000D610
WDAT:0x00000001
WREG:0x0000D611
WDAT:0x00000013
WREG:0x0000D612
WDAT:0x00000001
WREG:0x0000D613
WDAT:0x00000054
WREG:0x0000D614
WDAT:0x00000001
WREG:0x0000D615
WDAT:0x00000082
WREG:0x0000D616
WDAT:0x00000001
WREG:0x0000D617
WDAT:0x000000CA
WREG:0x0000D618
WDAT:0x00000002
WREG:0x0000D619
WDAT:0x00000000
WREG:0x0000D61A
WDAT:0x00000002
WREG:0x0000D61B
WDAT:0x00000001
WREG:0x0000D61C
WDAT:0x00000002
WREG:0x0000D61D
WDAT:0x00000034
WREG:0x0000D61E
WDAT:0x00000002
WREG:0x0000D61F
WDAT:0x00000067
WREG:0x0000D620
WDAT:0x00000002
WREG:0x0000D621
WDAT:0x00000084
WREG:0x0000D622
WDAT:0x00000002
WREG:0x0000D623
WDAT:0x000000A4
WREG:0x0000D624
WDAT:0x00000002
WREG:0x0000D625
WDAT:0x000000B7
WREG:0x0000D626
WDAT:0x00000002
WREG:0x0000D627
WDAT:0x000000CF
WREG:0x0000D628
WDAT:0x00000002
WREG:0x0000D629
WDAT:0x000000DE
WREG:0x0000D62A
WDAT:0x00000002
WREG:0x0000D62B
WDAT:0x000000F2
WREG:0x0000D62C
WDAT:0x00000002
WREG:0x0000D62D
WDAT:0x000000FE
WREG:0x0000D62E
WDAT:0x00000003
WREG:0x0000D62F
WDAT:0x00000010
WREG:0x0000D630
WDAT:0x00000003
WREG:0x0000D631
WDAT:0x00000033
WREG:0x0000D632
WDAT:0x00000003
WREG:0x0000D633
WDAT:0x0000006D
WREG:0x0000F000
WDAT:0x00000055
WREG:0x0000F001
WDAT:0x000000AA
WREG:0x0000F002
WDAT:0x00000052
WREG:0x0000F003
WDAT:0x00000008
WREG:0x0000F004
WDAT:0x00000000
WREG:0x0000B100
WDAT:0x000000CC
WREG:0x0000B101
WDAT:0x00000000
WREG:0x0000B600
WDAT:0x00000005
WREG:0x0000B700
WDAT:0x00000070
WREG:0x0000B701
WDAT:0x00000070
WREG:0x0000B800
WDAT:0x00000001
WREG:0x0000B801
WDAT:0x00000003
WREG:0x0000B802
WDAT:0x00000003
WREG:0x0000B803
WDAT:0x00000003
WREG:0x0000BC00
WDAT:0x00000002
WREG:0x0000BC01
WDAT:0x00000000
WREG:0x0000BC02
WDAT:0x00000000
WREG:0x0000C900
WDAT:0x000000D0
WREG:0x0000C901
WDAT:0x00000002
WREG:0x0000C902
WDAT:0x00000050
WREG:0x0000C903
WDAT:0x00000050
WREG:0x0000C904
WDAT:0x00000050
WREG:0x00003500
WDAT:0x00000000
WREG:0x00003A00
WDAT:0x00000055
WREG:0x00001100
WREG:0x00002900
//开启显示


WREG:0x00003600
WDAT:0x00000000
WREG:0x00002A00
WDAT:0x00000000
WREG:0x00002A01
WDAT:0x00000000
WREG:0x00002A02
WDAT:0x00000001
WREG:0x00002A03
WDAT:0x000000DF
WREG:0x00002B00
WDAT:0x00000000
WREG:0x00002B01
WDAT:0x00000000
WREG:0x00002B02
WDAT:0x00000003
WREG:0x00002B03
WDAT:0x0000001F
//设置好现实方向

WREG:0x00002A00
WDAT:0x00000000
WREG:0x00002A01
WDAT:0x00000000
WREG:0x00002B00
WDAT:0x00000000
WREG:0x00002B01
WDAT:0x00000000
WREG:0x00002C00
CTP ID:9147
*/

