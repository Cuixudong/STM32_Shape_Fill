#include "sys.h"
#include "delay.h"
#include "usart.h" 
#include "led.h" 		 	 
#include "lcd.h"  
#include "key.h"      
#include "malloc.h"
#include "sdio_sdcard.h"  
#include "w25qxx.h"    
#include "ff.h"  
#include "exfuns.h"   
#include "text.h"  
#include "pyinput.h"
#include "touch.h"	
#include "string.h"    	
#include "usmart.h"     
#include "Display_3D.h"

#include "math.h"

uint8_t points = 25;

float pointsx[25] = {10,30,50,100,110,117,230,310,315,400,420,460,462,470,465,412,300,280,113,89,32,27,220,13,10};
float pointsy[25] = {230,117,110,100,50,30,10,30,50,100,110,117,230,310,315,400,420,460,462,470,290,280,300,233,230};
int maxx,maxy,minx,miny;
	
uint8_t block[8][12] = 
{
	//�������1��Z��������С����ô��䶥��0132������0451������0246���ɵ������ı���
	//��Ҫ�����ҵ���ת���������㼫ֵ�Ըõ�Ϊ���ĵ�����ƽ�����
	{0,1,3,2,0,4,5,1,0,2,6,4},//��1�������깹��3��ƽ������Ķ���λ��
	{1,3,2,0,1,5,7,3,1,0,4,5},
	{2,0,1,3,2,3,7,6,2,6,4,0},
	{3,7,6,2,3,1,5,7,3,2,0,1},
	{4,0,2,6,4,5,1,0,4,5,7,6},
	{5,7,3,1,5,7,6,4,5,1,0,4},
	{6,2,3,7,6,7,5,4,6,4,0,2},
	{7,3,1,5,7,6,2,3,7,6,4,5}
};

void calc_min_max(int nvert, float *vertx, float *verty)
{
	uint32_t res;
	int i, j;
	res=0;
	for(i=0;i<nvert;i++)
	{
		res += vertx[i];
	}
	maxx = minx = res/nvert;//����ƽ��Xֵ
	res=0;
	for(i=0;i<nvert;i++)
	{
		res += verty[i];
	}
	maxy = miny = res/nvert;//����ƽ��Yֵ
	for(i=0;i<nvert;i++)
	{
		if(vertx[i] >= maxx)
		{
			maxx = vertx[i];//��X���ֵ
		}
		if(vertx[i] < minx)
		{
			minx = vertx[i];//��X��Сֵ
		}
	}
	for(i=0;i<nvert;i++)
	{
		if(verty[i] >= maxy)
		{
			maxy = verty[i];//��X���ֵ
		}
		if(verty[i] < miny)
		{
			miny = verty[i];//��X��Сֵ
		}
	}
	LCD_DrawLine(minx,miny,maxx,miny);
	LCD_DrawLine(minx,miny,minx,maxy);
	LCD_DrawLine(maxx,miny,maxx,maxy);
	LCD_DrawLine(minx,maxy,maxx,maxy);
	printf("min:%5d,%5d -->> max:%5d,%5d\r\n",minx,miny,maxx,maxy);
}

uint16_t is_point_in(int nvert, float *vertx, float *verty, float testx, float testy)
{
	uint8_t res;
	int i, j, c = 0;
	res=0;
	if((testx > maxx)||(testy > maxy)||(testx < minx)||(testy < miny))
		return 1000;
	for (i = 0, j = nvert-1; i < nvert; j = i++) 
	{
		if ( ( (verty[i]>testy) != (verty[j]>testy) ) && (testx < (vertx[j]-vertx[i]) * (testy-verty[i]) / (verty[j]-verty[i]) + vertx[i]) )
		c = !c;
	}
	return c;
}

void draw_shape(int nvert, float *vertx, float *verty)
{
	int i,j;
	POINT_COLOR = BLUE;
	for(i=0;i<nvert-1;i++)
	{
		printf("%5.1f,%5.1f -->> %5.1f,%5.1f\r\n",vertx[i],verty[i],vertx[i+1],verty[i+1]);
		LCD_DrawLine((uint16_t)vertx[i],(uint16_t)verty[i],(uint16_t)vertx[i+1],(uint16_t)verty[i+1]);
	}
	LCD_DrawLine((uint16_t)vertx[nvert-1],(uint16_t)verty[nvert-1],(uint16_t)vertx[0],(uint16_t)verty[0]);
}

void create_five_star(float *vertx, float *verty,float x,float y,float r,float offset_angle)
{
	//�������������
	int i,j;
	for(i=0;i<10;i++)
	{
		if(i % 2)
		{
			*(vertx + i) = cos((i * 36 + offset_angle) * 3.1415926 / 180) * r + x;
			*(verty + i) = sin((i * 36 + offset_angle) * 3.1415926 / 180) * r + y;
		}
		else
		{
			*(vertx + i) = cos((i * 36 + offset_angle) * 3.1415926 / 180) * r * 0.3 + x;
			*(verty + i) = sin((i * 36 + offset_angle) * 3.1415926 / 180) * r * 0.3 + y;
		}
	}
	
//	int i,j;
//	for(i=0;i<10;i++)
//	{
//		if(i % 2)
//		{
//			*(vertx + i) = cos((i * 72 + offset_angle) * 3.1415926 / 180) * r + x;
//			*(verty + i) = sin((i * 72 + offset_angle) * 3.1415926 / 180) * r + y;
//		}
//		else
//		{
//			*(vertx + i) = cos((i * 72 + 36 + offset_angle) * 3.1415926 / 180) * r * sqrt(0.73) + x;
//			*(verty + i) = sin((i * 72 + 36 + offset_angle) * 3.1415926 / 180) * r * sqrt(0.73) + y;
//		}
//	}
}

void fill_shape(void)
{
	int i,j;
	int res;
	for(i=miny;i<maxy;i++)
	{
		for(j=minx;j<maxx;j++)
		{
			res = is_point_in(points,pointsx,pointsy,j,i);
			if(res == 1000)
			{
			}
			else if((res % 2) == 0)
			{
			}
			else if((res % 2) == 1)
			{
				LCD_Fast_DrawPoint(j,i,YELLOW);
			}
		}
	}
}

void tset_3D(float x,float y,float z)
{
	float gMAT[4][4];
	u8 i;
	_3Dzuobiao temp;
	_2Dzuobiao Cube_dis[8];

	structure_3D(gMAT);          //����Ϊ��λ����
	Translate3D(gMAT,-8,-8,-8); //ƽ�Ʊ任����,��Ϊ�߳���һ��ĵ���ʱ��ͼ�������Լ���ת����ԭ��������
	Scale_3D(gMAT,4,4,4);		 //�����任����
	Rotate_3D(gMAT,x,y,z);			//��ת�任����
	Translate3D(gMAT,8,8,8); //ƽ�Ʊ任����
//	for(i=0;i<8;i++)
//	{
//		printf("cube%3d: x:%4d y:%4d z:%4d \r\n",i,(int)Cube[i].x,(int)Cube[i].y,(int)Cube[i].z);
//	}
	for(i=0;i<8;i++)
	{
		temp=vector_matrix_MULTIPLY(Cube[i],gMAT);//ʸ����������
		printf("cube%3d: x:%4d y:%4d z:%4d \r\n",i,(int)temp.x,(int)temp.y,(int)temp.z);
		Cube_dis[i]=	PerProject(temp,240,400);//����ͶӰ
		LCD_Draw_Circle(Cube_dis[i].x,Cube_dis[i].y,i*2+1);
		LCD_ShowNum(Cube_dis[i].x,Cube_dis[i].y,i,1,16);
		//Cube_dis[i].x+=240;
		//Cube_dis[i].y+=432;//�������������Ļ����������⡣ȥ����˳ʱ��ת��������߽����Ҳ����������޻��ߣ�
											//������Ļ�Ŀ�Ⱦͽ���ˣ�����˵���飬����Դ�Ϊ240���߶�Ϊ432����Ȼ����Ϊ400��������Ҫע��ͼ��Ҫ�󵽳���������
	}
	LCD_DrawLine(Cube_dis[0].x,Cube_dis[0].y,Cube_dis[1].x,Cube_dis[1].y);
	LCD_DrawLine(Cube_dis[0].x,Cube_dis[0].y,Cube_dis[2].x,Cube_dis[2].y);
	LCD_DrawLine(Cube_dis[3].x,Cube_dis[3].y,Cube_dis[1].x,Cube_dis[1].y);
	LCD_DrawLine(Cube_dis[3].x,Cube_dis[3].y,Cube_dis[2].x,Cube_dis[2].y);


	LCD_DrawLine(Cube_dis[0+4].x,Cube_dis[0+4].y,Cube_dis[1+4].x,Cube_dis[1+4].y);
	LCD_DrawLine(Cube_dis[0+4].x,Cube_dis[0+4].y,Cube_dis[2+4].x,Cube_dis[2+4].y);
	LCD_DrawLine(Cube_dis[3+4].x,Cube_dis[3+4].y,Cube_dis[1+4].x,Cube_dis[1+4].y);
	LCD_DrawLine(Cube_dis[3+4].x,Cube_dis[3+4].y,Cube_dis[2+4].x,Cube_dis[2+4].y);


	LCD_DrawLine(Cube_dis[0].x,Cube_dis[0].y,Cube_dis[0+4].x,Cube_dis[0+4].y);
	LCD_DrawLine(Cube_dis[1].x,Cube_dis[1].y,Cube_dis[1+4].x,Cube_dis[1+4].y);
	LCD_DrawLine(Cube_dis[2].x,Cube_dis[2].y,Cube_dis[2+4].x,Cube_dis[2+4].y);
	LCD_DrawLine(Cube_dis[3].x,Cube_dis[3].y,Cube_dis[3+4].x,Cube_dis[3+4].y);
}

//���ֱ�
const u8* kbd_tbl[9]={"��","2","3","4","5","6","7","8","9",};
//�ַ���
const u8* kbs_tbl[9]={"DEL","abc","def","ghi","jkl","mno","pqrs","tuv","wxyz",};

u16 kbdxsize;	//������̰������
u16 kbdysize;	//������̰����߶�

//���ؼ��̽���
//x,y:������ʼ����
void py_load_ui(u16 x,u16 y)
{
	u16 i;
	POINT_COLOR=RED;
	LCD_DrawRectangle(x,y,x+kbdxsize*3,y+kbdysize*3);						   
	LCD_DrawRectangle(x+kbdxsize,y,x+kbdxsize*2,y+kbdysize*3);						   
	LCD_DrawRectangle(x,y+kbdysize,x+kbdxsize*3,y+kbdysize*2);
	POINT_COLOR=BLUE;
	for(i=0;i<9;i++)
	{
		Show_Str_Mid(x+(i%3)*kbdxsize,y+4+kbdysize*(i/3),(u8*)kbd_tbl[i],16,kbdxsize);		
		Show_Str_Mid(x+(i%3)*kbdxsize,y+kbdysize/2+kbdysize*(i/3),(u8*)kbs_tbl[i],16,kbdxsize);		
	}  		 					   
}
//����״̬����
//x,y:��������
//key:��ֵ��0~8��
//sta:״̬��0���ɿ���1�����£�
void py_key_staset(u16 x,u16 y,u8 keyx,u8 sta)
{		  
	u16 i=keyx/3,j=keyx%3;
	if(keyx>8)return;
	if(sta)LCD_Fill(x+j*kbdxsize+1,y+i*kbdysize+1,x+j*kbdxsize+kbdxsize-1,y+i*kbdysize+kbdysize-1,GREEN);
	else LCD_Fill(x+j*kbdxsize+1,y+i*kbdysize+1,x+j*kbdxsize+kbdxsize-1,y+i*kbdysize+kbdysize-1,WHITE); 
	Show_Str_Mid(x+j*kbdxsize,y+4+kbdysize*i,(u8*)kbd_tbl[keyx],16,kbdxsize);		
	Show_Str_Mid(x+j*kbdxsize,y+kbdysize/2+kbdysize*i,(u8*)kbs_tbl[keyx],16,kbdxsize);		 
}
//�õ�������������
//x,y:��������
//����ֵ��������ֵ��1~9��Ч��0,��Ч��
u8 py_get_keynum(u16 x,u16 y)
{
	u16 i,j;
	static u8 key_x=0;//0,û���κΰ������£�1~9��1~9�Ű�������
	u8 key=0;
	tp_dev.scan(0); 		 
	if(tp_dev.sta&TP_PRES_DOWN)			//������������
	{	
		for(i=0;i<3;i++)
		{
			for(j=0;j<3;j++)
			{
			 	if(tp_dev.x[0]<(x+j*kbdxsize+kbdxsize)&&tp_dev.x[0]>(x+j*kbdxsize)&&tp_dev.y[0]<(y+i*kbdysize+kbdysize)&&tp_dev.y[0]>(y+i*kbdysize))
				{	
					key=i*3+j+1;	 
					break;	 		   
				}
			}
			if(key)
			{	   
				if(key_x==key)key=0;
				else 
				{
					py_key_staset(x,y,key_x-1,0);
					key_x=key;
					py_key_staset(x,y,key_x-1,1);
				}
				break;
			}
		}  
	}else if(key_x) 
	{
		py_key_staset(x,y,key_x-1,0);
		key_x=0;
	} 
	return key; 
}
							   
//��ʾ���.
//index:0,��ʾû��һ��ƥ��Ľ��.���֮ǰ����ʾ
//   ����,������	
void py_show_result(u8 index)
{
 	LCD_ShowNum(30+144,125,index,1,16); 		//��ʾ��ǰ������
	LCD_Fill(30+40,125,30+40+48,130+16,WHITE);	//���֮ǰ����ʾ
 	LCD_Fill(30+40,145,lcddev.width,145+48,WHITE);//���֮ǰ����ʾ    
	if(index)
	{
 		Show_Str(30+40,125,200,16,t9.pymb[index-1]->py,16,0); 	//��ʾƴ��
		Show_Str(30+40,145,lcddev.width-70,48,t9.pymb[index-1]->pymb,16,0);//��ʾ��Ӧ�ĺ���
		printf("\r\nƴ��:%s\r\n",t9.pymb[index-1]->py);	//�������ƴ��
		printf("���:%s\r\n",t9.pymb[index-1]->pymb);	//����������
	}
}


 int main(void)
 {	 
 	u8 i=0;	    	  	    
	u8 result_num;
	u8 cur_index;
	u8 key;
	u8 inputstr[7];		//�������6���ַ�+������
	u8 inputlen;		//���볤��	 
	 
	delay_init();	    	 //��ʱ������ʼ��	  
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//�����ж����ȼ�����Ϊ��2��2λ��ռ���ȼ���2λ��Ӧ���ȼ�
	uart_init(115200);	 	//���ڳ�ʼ��Ϊ115200
	usmart_dev.init(72);		//��ʼ��USMART
 	LED_Init();		  			//��ʼ����LED���ӵ�Ӳ���ӿ�
	KEY_Init();					//��ʼ������
	LCD_Init();			   		//��ʼ��LCD     
	W25QXX_Init();				//��ʼ��W25Q128
	tp_dev.init();				//��ʼ��������
 	my_mem_init(SRAMIN);		//��ʼ���ڲ��ڴ��
	exfuns_init();				//Ϊfatfs��ر��������ڴ�  
 	f_mount(fs[0],"0:",1); 		//����SD�� 
 	f_mount(fs[1],"1:",1); 		//����FLASH.
 	POINT_COLOR=RED;       
 	while(font_init()) 			//����ֿ�
	{	    
		LCD_ShowString(30,50,200,16,16,"Font Error!");
		delay_ms(200);				  
		LCD_Fill(30,50,240,66,WHITE);//�����ʾ	     
	}
	
	//calc_min_max(points,pointsx,pointsy);
	
	//draw_shape(points,pointsx,pointsy);
	
	create_five_star(pointsx,pointsy,240,400,200,0);
	calc_min_max(10,pointsx,pointsy);
	draw_shape(10,pointsx,pointsy);
	
	while(1)
	{
		i++;
		delay_ms(10);
		key=KEY_Scan(0);
		if(key==WKUP_PRES)//KEY_UP����,���ǵ�����
		{
			fill_shape();
		}
		else if(key == KEY0_PRES)
		{
			tset_3D(30,30,0);
		}
		if(i==30)
		{
			i=0;
			LED0=!LED0;
		}		   
	}
	
 RESTART:
 	POINT_COLOR=RED;       
	Show_Str(30,5,200,16,"��ӢSTM32������",16,0);				    	 
	Show_Str(30,25,200,16,"ƴ�����뷨ʵ��",16,0);				    	 
	Show_Str(30,45,200,16,"����ԭ��@ALIENTEK",16,0);				    	 
 	Show_Str(30,65,200,16,"KEY_UP:У׼  KEY0:���",16,0);	 
 	Show_Str(30,85,200,16,"  KEY1:�·�",16,0);	 
	Show_Str(30,105,200,16,"����:        ƥ��:  ",16,0);
 	Show_Str(30,125,200,16,"ƴ��:        ��ǰ:  ",16,0);
 	Show_Str(30,145,210,32,"���:",16,0);
	if(lcddev.id==0X5310){kbdxsize=86;kbdysize=43;}//����LCD�ֱ������ð�����С
	else if(lcddev.id==0X5510||lcddev.id==0X1963){kbdxsize=140;kbdysize=70;}
	else {kbdxsize=60;kbdysize=40;}
	py_load_ui(30,195);
	memset(inputstr,0,7);	//ȫ������
	inputlen=0;				//���볤��Ϊ0
	result_num=0;			//��ƥ��������
	cur_index=0;			
	while(1)
	{
		i++;
		delay_ms(10);
		key=py_get_keynum(30,195);
		if(key)
		{
			if(key==1)//ɾ��
			{
				if(inputlen)inputlen--;
				inputstr[inputlen]='\0';//��ӽ�����
			}else 
			{
				inputstr[inputlen]=key+'0';//�����ַ�
				if(inputlen<7)inputlen++;
			}
			if(inputstr[0]!=NULL)
			{
				key=t9.getpymb(inputstr);	//�õ�ƥ��Ľ����
				if(key)//�в���ƥ��/��ȫƥ��Ľ��
				{
					result_num=key&0X7F;	//��ƥ����
					cur_index=1;			//��ǰΪ��һ������ 
					if(key&0X80)		   	//�ǲ���ƥ��
					{
						inputlen=key&0X7F;	//��Чƥ��λ��
						inputstr[inputlen]='\0';//��ƥ���λ��ȥ��
						if(inputlen>1)result_num=t9.getpymb(inputstr);//���»�ȡ��ȫƥ���ַ���
					}  
				}else 						//û���κ�ƥ��
				{				   	
					inputlen--;
					inputstr[inputlen]='\0';
				}
			}else
			{
				cur_index=0;
				result_num=0;
			}
			LCD_Fill(30+40,105,30+40+48,110+16,WHITE);	//���֮ǰ����ʾ
			LCD_ShowNum(30+144,105,result_num,1,16); 	//��ʾƥ��Ľ����
			Show_Str(30+40,105,200,16,inputstr,16,0);	//��ʾ��Ч�����ִ�		 
	 		py_show_result(cur_index);					//��ʾ��cur_index��ƥ����
		}	 
		key=KEY_Scan(0);
		if(key==WKUP_PRES&&tp_dev.touchtype==0)//KEY_UP����,���ǵ�����
		{ 
			tp_dev.adjust();
			LCD_Clear(WHITE);
			goto RESTART;
		}
		if(result_num)	//����ƥ��Ľ��	
		{	
			switch(key)
			{ 
 				case KEY1_PRES://�·�
	   				if(cur_index>1)cur_index--;
					else cur_index=result_num;
					py_show_result(cur_index);	//��ʾ��cur_index��ƥ����
					break;
				case KEY0_PRES://�������
 					LCD_Fill(30+40,145,lcddev.width,145+48,WHITE);	//���֮ǰ����ʾ    
					goto RESTART;			 		 	   
			}   	 
		}
		if(i==30)
		{
			i=0;
			LED0=!LED0;
		}		   
	}         										    			    
}
