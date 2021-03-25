#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "lcd.h"
#include "key.h"
#include "math.h"

uint8_t points = 25;

float pointsx[25] = {10, 30, 50, 100, 110, 117, 230, 310, 315, 400, 420, 460, 462, 470, 465, 412, 300, 280, 113, 89, 32, 27, 220, 13, 10};
float pointsy[25] = {230, 117, 110, 100, 50, 30, 10, 30, 50, 100, 110, 117, 230, 310, 315, 400, 420, 460, 462, 470, 290, 280, 300, 233, 230};
int maxx, maxy, minx, miny;

void calc_min_max(int nvert, float *vertx, float *verty)
{
    uint32_t res;
    int i, j;
    res = 0;

    for (i = 0; i < nvert; i++) {
        res += vertx[i];
    }

    maxx = minx = res / nvert; //计算平均X值
    res = 0;

    for (i = 0; i < nvert; i++) {
        res += verty[i];
    }

    maxy = miny = res / nvert; //计算平均Y值

    for (i = 0; i < nvert; i++) {
        if (vertx[i] >= maxx) {
            maxx = vertx[i];//找X最大值
        }

        if (vertx[i] < minx) {
            minx = vertx[i];//找X最小值
        }
    }

    for (i = 0; i < nvert; i++) {
        if (verty[i] >= maxy) {
            maxy = verty[i];//找X最大值
        }

        if (verty[i] < miny) {
            miny = verty[i];//找X最小值
        }
    }

    LCD_DrawLine(minx, miny, maxx, miny);
    LCD_DrawLine(minx, miny, minx, maxy);
    LCD_DrawLine(maxx, miny, maxx, maxy);
    LCD_DrawLine(minx, maxy, maxx, maxy);
    printf("min:%5d,%5d -->> max:%5d,%5d\r\n", minx, miny, maxx, maxy);
}

uint16_t is_point_in(int nvert, float *vertx, float *verty, float testx, float testy)
{
    uint8_t res;
    int i, j, c = 0;
    res = 0;

    if ((testx > maxx) || (testy > maxy) || (testx < minx) || (testy < miny))
        return 1000;

    for (i = 0, j = nvert - 1; i < nvert; j = i++) {
        if ( ( (verty[i] > testy) != (verty[j] > testy) ) && (testx < (vertx[j] - vertx[i]) * (testy - verty[i]) / (verty[j] - verty[i]) + vertx[i]) )
            c = !c;
    }

    return c;
}

void draw_shape(int nvert, float *vertx, float *verty)
{
    int i, j;
    POINT_COLOR = BLUE;

    for (i = 0; i < nvert - 1; i++) {
        printf("%5.1f,%5.1f -->> %5.1f,%5.1f\r\n", vertx[i], verty[i], vertx[i + 1], verty[i + 1]);
        LCD_DrawLine((uint16_t)vertx[i], (uint16_t)verty[i], (uint16_t)vertx[i + 1], (uint16_t)verty[i + 1]);
    }

    LCD_DrawLine((uint16_t)vertx[nvert - 1], (uint16_t)verty[nvert - 1], (uint16_t)vertx[0], (uint16_t)verty[0]);
}

void create_five_star(float *vertx, float *verty, float x, float y, float r, float offset_angle)
{
    //生成五角星坐标
    int i, j;

    for (i = 0; i < 10; i++) {
        if (i % 2) {
            *(vertx + i) = cos((i * 36 + offset_angle) * 3.1415926 / 180) * r + x;
            *(verty + i) = sin((i * 36 + offset_angle) * 3.1415926 / 180) * r + y;
        } else {
            *(vertx + i) = cos((i * 36 + offset_angle) * 3.1415926 / 180) * r * 0.3 + x;
            *(verty + i) = sin((i * 36 + offset_angle) * 3.1415926 / 180) * r * 0.3 + y;
        }
    }
}

void fill_shape(uint16_t color)
{
    int i, j;
    int res;

    for (i = miny; i < maxy; i++) {
        for (j = minx; j < maxx; j++) {
            res = is_point_in(points, pointsx, pointsy, j, i);

            if (res == 1000) {
            } else if ((res % 2) == 0) {
            } else if ((res % 2) == 1) {
                LCD_Fast_DrawPoint(j, i, color);
            }
        }
    }
}

int main(void)
{
    u8 i = 0;

    delay_init();       //延时函数初始化
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//设置中断优先级分组为组2：2位抢占优先级，2位响应优先级
    uart_init(115200);   //串口初始化为115200
    LED_Init();       //初始化与LED连接的硬件接口
    KEY_Init();     //初始化按键
    LCD_Init();        //初始化LCD
    
    calc_min_max(points, pointsx, pointsy);
    draw_shape(points, pointsx, pointsy);
    fill_shape(BLUE);
    delay_ms(1000);
    LCD_Clear(WHITE);
    create_five_star(pointsx,pointsy,240,400,200,-36);
    calc_min_max(10,pointsx,pointsy);
    draw_shape(10,pointsx,pointsy);
    fill_shape(BLUE);

    while (1) {
        i++;
        delay_ms(10);
        if (i == 30) {
            i = 0;
            LED0 = !LED0;
        }
    }

}
