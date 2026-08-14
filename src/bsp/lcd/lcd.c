/**
 ****************************************************************************************************
 * @file        lcd.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2025-01-03
 * @brief       SPI LCD(MCU屏) 驱动代码
 *              支持驱动IC型号包括:ILI9341等
 *
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 * @attention
 *
 * 实验平台:正点原子 RP2350A 最小系统板
 * 在线视频:www.yuanzige.com
 * 技术论坛:www.openedv.com
 * 公司网址:www.alientek.com
 * 购买地址:openedv.taobao.com
 *
 ****************************************************************************************************
 */

#include "bsp/lcd/lcd.h"
#include "lcdfont.h"


uint16_t lcd_buf[LCD_PIXEL_MAX];
lcd_obj_t lcd_self;

static void lcd_spi_8bit(void)
{
    spi_set_format(SPI_PORT, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
}

static void lcd_spi_16bit(void)
{
    spi_set_format(SPI_PORT, 16, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
}

/* LCD需要初始化一组命令/参数值。它们存储在此结构中 */
typedef struct
{
    uint8_t cmd;
    uint8_t data[16];
    uint8_t databytes; /* 数据中没有数据；比特7＝设置后的延迟；0xFF=cmds结束 */
} lcd_init_cmd_t;

/**
 * @brief       发送命令到LCD，使用轮询方式阻塞等待传输完成(由于数据传输量很少，因此在轮询方式处理可提高速度。使用中断方式的开销要超过轮询方式)
 * @param       cmd 传输的8位命令数据
 * @retval      无
 */
void lcd_write_cmd(uint8_t cmd)
{
    LCD_WR(0);
    LCD_CS(0);
    spi_write_blocking(SPI_PORT, &cmd, 1);
    LCD_CS(1);
}

/**
 * @brief       发送数据到LCD，使用轮询方式阻塞等待传输完成(由于数据传输量很少，因此在轮询方式处理可提高速度。使用中断方式的开销要超过轮询方式)
 * @param       data 传输的8位数据
 * @retval      无
 */
void lcd_write_data(const uint8_t data[], int len)
{
    LCD_WR(1);
    LCD_CS(0);
    spi_write_blocking(SPI_PORT, data, len);
    LCD_CS(1);
}

/**
 * @brief       发送数据到LCD，使用轮询方式阻塞等待传输完成(由于数据传输量很少，因此在轮询方式处理可提高速度。使用中断方式的开销要超过轮询方式)
 * @param       data 传输的16位数据
 * @retval      无
 */
void lcd_write_data16(uint16_t data)
{
    uint8_t dataBuf[2] = {0,0};

    dataBuf[0] = data >> 8;
    dataBuf[1] = data & 0xFF;
    LCD_WR(1);
    LCD_CS(0);
    spi_write_blocking(SPI_PORT, dataBuf, 2);
    LCD_CS(1);
}

/**
 * @brief       设置窗口大小
 * @param       xstar：左上角x轴
 * @param       ystar：左上角y轴
 * @param       xend：右下角x轴
 * @param       yend：右下角y轴
 * @retval      无
 */
void lcd_set_window(uint16_t xstar, uint16_t ystar,uint16_t xend,uint16_t yend)
{
    uint8_t databuf[4] = {0,0,0,0};
    
    if (lcd_self.dir == 1)                  /* 横屏 */
    {
        databuf[0] = (xstar + 40) >> 8;
        databuf[1] = 0xFF & (xstar + 40);
        databuf[2] = (xend + 40) >> 8;
        databuf[3] = 0xFF & (xend + 40);
        lcd_write_cmd(lcd_self.setxcmd);
        lcd_write_data(databuf, 4);

        databuf[0] = (ystar + 52) >> 8;
        databuf[1] = 0xFF & (ystar + 52);
        databuf[2] = (yend + 52) >> 8;
        databuf[3] = 0xFF & (yend + 52);
        lcd_write_cmd(lcd_self.setycmd);
        lcd_write_data(databuf, 4);
    }
    else
    {
        databuf[0] = (xstar + 52) >> 8;
        databuf[1] = 0xFF & (xstar + 52);
        databuf[2] = (xend + 52) >> 8;
        databuf[3] = 0xFF & (xend + 52);
        lcd_write_cmd(lcd_self.setxcmd);
        lcd_write_data(databuf, 4);

        databuf[0] = (ystar + 40) >> 8;
        databuf[1] = 0xFF & (ystar + 40);
        databuf[2] = (yend + 40) >> 8;
        databuf[3] = 0xFF & (yend + 40);
        lcd_write_cmd(lcd_self.setycmd);
        lcd_write_data(databuf, 4);
    }

    lcd_write_cmd(lcd_self.wramcmd);    /* 开始写入GRAM */
}

/**
 * @brief       以一种颜色清空LCD屏
 * @param       color 清屏颜色
 * @retval      无
 */
void lcd_clear(uint16_t color)
{
    uint32_t n = (uint32_t)lcd_self.width * lcd_self.height;
    uint32_t i;

    for (i = 0; i < n; i++)
    {
        lcd_buf[i] = color;
    }
}

void lcd_flush_rect(uint16_t x, uint16_t y, uint16_t xend, uint16_t yend)
{
    uint16_t row;
    uint16_t fb_w;
    uint16_t rect_w;

    if (lcd_self.width == 0 || lcd_self.height == 0)
    {
        return;
    }
    if (x >= lcd_self.width || y >= lcd_self.height)
    {
        return;
    }
    if (xend >= lcd_self.width)
    {
        xend = lcd_self.width - 1;
    }
    if (yend >= lcd_self.height)
    {
        yend = lcd_self.height - 1;
    }
    if (x > xend || y > yend)
    {
        return;
    }

    fb_w = lcd_self.width;
    rect_w = xend - x + 1;

    lcd_set_window(x, y, xend, yend);
    LCD_WR(1);
    LCD_CS(0);
    lcd_spi_16bit();

    if (x == 0 && xend == fb_w - 1)
    {
        spi_write16_blocking(SPI_PORT, &lcd_buf[(uint32_t)y * fb_w],
                             (size_t)rect_w * (yend - y + 1));
    }
    else
    {
        for (row = y; row <= yend; row++)
        {
            spi_write16_blocking(SPI_PORT, &lcd_buf[(uint32_t)row * fb_w + x], rect_w);
        }
    }

    lcd_spi_8bit();
    LCD_CS(1);
}

void lcd_flush(void)
{
    if (lcd_self.width == 0 || lcd_self.height == 0)
    {
        return;
    }
    lcd_flush_rect(0, 0, lcd_self.width - 1, lcd_self.height - 1);
}

/**
 * @brief       在指定区域内填充单个颜色
 * @param       (sx,sy),(ex,ey):填充矩形对角坐标,区域大小为:(ex - sx + 1) * (ey - sy + 1)
 * @param       color:要填充的颜色(32位颜色,方便兼容LTDC)
 * @retval      无
 */
void lcd_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t color)
{
    uint16_t x;
    uint16_t y;

    if (sx > ex)
    {
        uint16_t t = sx;
        sx = ex;
        ex = t;
    }
    if (sy > ey)
    {
        uint16_t t = sy;
        sy = ey;
        ey = t;
    }
    if (sx >= lcd_self.width || sy >= lcd_self.height)
    {
        return;
    }
    if (ex >= lcd_self.width)
    {
        ex = lcd_self.width - 1;
    }
    if (ey >= lcd_self.height)
    {
        ey = lcd_self.height - 1;
    }

    for (y = sy; y <= ey; y++)
    {
        uint16_t *row = &lcd_buf[(uint32_t)y * lcd_self.width];
        for (x = sx; x <= ex; x++)
        {
            row[x] = color;
        }
    }
}

/**
 * @brief       设置光标的位置
 * @param       Xpos：左上角x轴
 * @param       Ypos：左上角y轴
 * @retval      无
 */
void lcd_set_cursor(uint16_t xpos, uint16_t ypos)
{
    lcd_set_window(xpos,ypos,xpos,ypos);
} 

/**
 * @brief       设置LCD的自动扫描方向(对RGB屏无效)
 * @param       dir:0~7,代表8个方向(具体定义见lcd.h)
 * @retval      无
 */
void lcd_scan_dir(uint8_t dir)
{
    uint8_t regval = 0;
    uint8_t dirreg = 0;
    uint16_t temp;

    /* 横屏时，对1963不改变扫描方向, 其他IC改变扫描方向！竖屏时1963改变方向, 其他IC不改变扫描方向 */
    if (lcd_self.dir == 1)
    {
        dir = 6;
    }

    switch (dir)
    {
        case L2R_U2D:                           /* 从左到右,从上到下 */
            regval |= (0 << 7) | (0 << 6) | (0 << 5);
            break;

        case L2R_D2U:                           /* 从左到右,从下到上 */
            regval |= (1 << 7) | (0 << 6) | (0 << 5);
            break;

        case R2L_U2D:                           /* 从右到左,从上到下 */
            regval |= (0 << 7) | (1 << 6) | (0 << 5);
            break;

        case R2L_D2U:                           /* 从右到左,从下到上 */
            regval |= (1 << 7) | (1 << 6) | (0 << 5);
            break;

        case U2D_L2R:                           /* 从上到下,从左到右 */
            regval |= (0 << 7) | (0 << 6) | (1 << 5);
            break;

        case U2D_R2L:                           /* 从上到下,从右到左 */
            regval |= (0 << 7) | (1 << 6) | (1 << 5);
            break;

        case D2U_L2R:                           /* 从下到上,从左到右 */
            regval |= (1 << 7) | (0 << 6) | (1 << 5);
            break;

        case D2U_R2L:                           /* 从下到上,从右到左 */
            regval |= (1 << 7) | (1 << 6) | (1 << 5);
            break;
    }

    dirreg = 0x36;                              /* 对绝大部分驱动IC, 由0X36寄存器控制 */
    
    uint8_t date_send[1] = {regval};
    
    lcd_write_cmd(dirreg);
    lcd_write_data(date_send,1);
    
    if (regval & 0x20)
    {
        if (lcd_self.width < lcd_self.height)   /* 交换X,Y */
        {
            temp = lcd_self.width;
            lcd_self.width = lcd_self.height;
            lcd_self.height = temp;
        }
    }
    else
    {
        if (lcd_self.width > lcd_self.height)   /* 交换X,Y */
        {
            temp = lcd_self.width;
            lcd_self.width = lcd_self.height;
            lcd_self.height = temp;
        }
    }
    
    lcd_set_window(0, 0, lcd_self.width,lcd_self.height);
}

/**
 * @brief       设置LCD显示方向
 * @param       dir:0,竖屏; 1,横屏
 * @retval      无
 */
void lcd_display_dir(uint8_t dir)
{
    lcd_self.dir = dir;
    
    if (lcd_self.dir == 0)                  /* 竖屏 */
    {
        lcd_self.width      = 135;
        lcd_self.height     = 240;
        lcd_self.wramcmd    = 0X2C;
        lcd_self.setxcmd    = 0X2A;
        lcd_self.setycmd    = 0X2B;
    }
    else                                    /* 横屏 */
    {
        lcd_self.width      = 240;          /* 默认宽度 */
        lcd_self.height     = 135;          /* 默认高度 */
        lcd_self.wramcmd    = 0X2C;
        lcd_self.setxcmd    = 0X2A;
        lcd_self.setycmd    = 0X2B;
    }

    lcd_scan_dir(lcd_self.dir);
}

/**
 * @brief       绘画一个像素点
 * @param       self_in：LCD结构体
 * @param       x：x轴坐标
 * @param       y：y轴坐标
 * @param       color：颜色值
 * @retval      无
 */
void lcd_draw_pixel(uint16_t x, uint16_t y, uint16_t color)
{
    if (x >= lcd_self.width || y >= lcd_self.height)
    {
        return;
    }
    lcd_buf[(uint32_t)y * lcd_self.width + x] = color;
}

/**
 * @brief       画线函数(直线、斜线)
 * @param       x1,y1   起点坐标
 * @param       x2,y2   终点坐标
 * @param       color 填充颜色
 * @retval      无
 */
void lcd_draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
    uint16_t t; 
    int xerr = 0, yerr = 0, delta_x, delta_y, distance; 
    
    int incx, incy, urow, ucol; 

    delta_x = x2 - x1;                      /* 计算坐标增量 */
    delta_y = y2 - y1; 
    urow = x1; 
    ucol = y1; 
    
    if (delta_x > 0)
    {
        incx = 1;                           /* 设置单步方向 */
    }
    else if (delta_x == 0)
    {
        incx = 0;                           /* 垂直线 */
    }
    else
    {
        incx =-1;
        delta_x =-delta_x;
    } 
    if(delta_y > 0)
    {
        incy = 1; 
    }
    else if(delta_y == 0)
    {
        incy = 0;                           /* 水平线 */
    }
    else
    {
        incy =-1;
        delta_y=-delta_y;
    } 
    
    if( delta_x>delta_y)
    {
        distance = delta_x;                 /* 选取基本增量坐标轴 */
    }
    else
    {
        distance = delta_y; 
    }
    
    for (t = 0;t <= distance + 1;t++ )      /* 画线输出 */
    {
        lcd_draw_pixel(urow,ucol,color);    /* 画点 */ 
        xerr += delta_x ; 
        yerr += delta_y ; 
        
        if(xerr>distance)
        { 
            xerr -= distance; 
            urow += incx; 
        } 
        
        if (yerr > distance)
        { 
            yerr -= distance; 
            ucol += incy; 
        } 
    } 
}

/**
 * @brief       画水平线
 * @param       x0,y0: 起点坐标
 * @param       len  : 线长度
 * @param       color: 矩形的颜色
 * @retval      无
 */
void lcd_draw_hline(uint16_t x, uint16_t y, uint16_t len, uint16_t color)
{
    if ((len == 0) || (x > lcd_self.width) || (y > lcd_self.height))return;

    lcd_fill(x, y, x + len - 1, y, color);
}

/**
 * @brief       画一个矩形
 * @param       x1,y1   起点坐标
 * @param       x2,y2   终点坐标
 * @param       color 填充颜色
 * @retval      无
 */
void lcd_draw_rectangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,uint16_t color)
{
    lcd_draw_line(x0, y0, x1, y0,color);
    lcd_draw_line(x0, y0, x0, y1,color);
    lcd_draw_line(x0, y1, x1, y1,color);
    lcd_draw_line(x1, y0, x1, y1,color);
}

/**
 * @brief       画一个圆
 * @param       x0,y0   圆心坐标
 * @param       r   圆半径
 * @param       color 填充颜色
 * @retval      无
 */
void lcd_draw_circle(uint16_t x0, uint16_t y0, uint16_t r, uint16_t color)
{
    int a, b;
    int di;
    a = 0;
    b = r;
    di = 3 - (r << 1);

    while (a <= b)
    {
        lcd_draw_pixel(x0 - b, y0 - a, color);
        lcd_draw_pixel(x0 + b, y0 - a, color);
        lcd_draw_pixel(x0 - a, y0 + b, color);
        lcd_draw_pixel(x0 - b, y0 - a, color);
        lcd_draw_pixel(x0 - a, y0 - b, color);
        lcd_draw_pixel(x0 + b, y0 + a, color);
        lcd_draw_pixel(x0 + a, y0 - b, color);
        lcd_draw_pixel(x0 + a, y0 + b, color);
        lcd_draw_pixel(x0 - b, y0 + a, color);
        a++;

        if (di < 0)
        {
            di += 4 * a + 6;
        }
        else
        {
            di += 10 + 4 * (a - b);
            b--;
        }

        lcd_draw_pixel(x0 + a, y0 + b, color);
    }
}

/**
 * @brief       在指定位置显示一个字符
 * @param       x,y  : 坐标
 * @param       chr  : 要显示的字符:" "--->"~"
 * @param       size : 字体大小 12/16/24/32
 * @param       mode : 叠加方式(1); 非叠加方式(0);
 * @param       color : 字符的颜色;
 * @retval      无
 */
void lcd_show_char(uint16_t x, uint16_t y, uint8_t chr, uint8_t size, uint8_t mode, uint16_t color)
{
    uint8_t *pfont = 0;
    uint8_t col;
    uint8_t row;
    uint8_t char_w = size / 2;
    uint8_t char_h = size;

    chr = chr - ' ';

    if ((x > (lcd_self.width - char_w)) || (y > (lcd_self.height - char_h)))
    {
        return;
    }

    switch (size)
    {
        case 12:
            pfont = (uint8_t *)asc2_1206[chr];
            break;

        case 16:
            pfont = (uint8_t *)asc2_1608[chr];
            break;

        case 24:
            pfont = (uint8_t *)asc2_2412[chr];
            break;

        case 32:
            pfont = (uint8_t *)asc2_3216[chr];
            break;

        default:
            return;
    }

    /* 字库是逐行：一行从左到右，高位在左。16 号一字一行；32 号一行两字节；24 号一行 8+4 位。 */
    if (size != 24)
    {
        uint8_t bytes_per_row = (char_w + 7) / 8;

        for (row = 0; row < char_h; row++)
        {
            for (col = 0; col < char_w; col++)
            {
                uint8_t temp = pfont[row * bytes_per_row + col / 8];

                if (temp & (0x80 >> (col % 8)))
                {
                    lcd_draw_pixel(x + col, y + row, color);
                }
                else if (mode == 0)
                {
                    lcd_draw_pixel(x + col, y + row, WHITE);
                }
            }
        }
    }
    else
    {
        uint8_t t;
        uint8_t t1;
        uint8_t sta;
        uint8_t temp;

        col = 0;
        row = 0;
        for (t = 0; t < 48; t++)
        {
            temp = pfont[t];
            sta = (t % 2 == 0) ? 8 : 4;
            for (t1 = 0; t1 < sta; t1++)
            {
                if (temp & 0x80)
                {
                    lcd_draw_pixel(x + col, y + row, color);
                }
                else if (mode == 0)
                {
                    lcd_draw_pixel(x + col, y + row, WHITE);
                }
                temp <<= 1;
                col++;
                if (col >= 12)
                {
                    col = 0;
                    row++;
                }
            }
        }
    }
}

/**
 * @brief       m^n函数
 * @param       m,n     输入参数
 * @retval      m^n次方
 */
uint32_t lcd_pow(uint8_t m, uint8_t n)
{
    uint32_t result = 1;

    while(n--)result *= m;

    return result;
}

/**
 * @brief       显示len个数字
 * @param       x,y : 起始坐标
 * @param       num : 数值(0 ~ 2^32)
 * @param       len : 显示数字的位数
 * @param       size: 选择字体 12/16/24/32
 * @retval      无
 */
void lcd_show_num(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint16_t color)
{
    uint8_t t, temp;
    uint8_t enshow = 0;

    for (t = 0; t < len; t++)                                               /* 按总显示位数循环 */
    {
        temp = (num / lcd_pow(10, len - t - 1)) % 10;                       /* 获取对应位的数字 */

        if (enshow == 0 && t < (len - 1))                                   /* 没有使能显示,且还有位要显示 */
        {
            if (temp == 0)
            {
                lcd_show_char(x + (size / 2)*t, y, ' ', size, 0, color);    /* 显示空格,占位 */
                continue;                                                   /* 继续下个一位 */
            }
            else
            {
                enshow = 1;                                                 /* 使能显示 */
            }

        }

        lcd_show_char(x + (size / 2)*t, y, temp + '0', size, 0, color);     /* 显示字符 */
    }
}

/**
 * @brief       扩展显示len个数字(高位是0也显示)
 * @param       x,y : 起始坐标
 * @param       num : 数值(0 ~ 2^32)
 * @param       len : 显示数字的位数
 * @param       size: 选择字体 12/16/24/32
 * @param       mode: 显示模式
 *              [7]:0,不填充;1,填充0.
 *              [6:1]:保留
 *              [0]:0,非叠加显示;1,叠加显示.
 * @param       color : 数字的颜色;
 * @retval      无
 */
void lcd_show_xnum(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint8_t mode, uint16_t color)
{
    uint8_t t, temp;
    uint8_t enshow = 0;

    for (t = 0; t < len; t++)                                                           /* 按总显示位数循环 */
    {
        temp = (num / lcd_pow(10, len - t - 1)) % 10;                                   /* 获取对应位的数字 */

        if (enshow == 0 && t < (len - 1))                                               /* 没有使能显示,且还有位要显示 */
        {
            if (temp == 0)
            {
                if (mode & 0X80)                                                        /* 高位需要填充0 */
                {
                    lcd_show_char(x + (size / 2)*t, y, '0', size, mode & 0X01, color);  /* 用0占位 */
                }
                else
                {
                    lcd_show_char(x + (size / 2)*t, y, ' ', size, mode & 0X01, color);  /* 用空格占位 */
                }
                continue;
            }
            else
            {
                enshow = 1;                                                             /* 使能显示 */
            }
        }
        lcd_show_char(x + (size / 2)*t, y, temp + '0', size, mode & 0X01, color);
    }
}


/**
 * @brief       显示字符串
 * @param       x,y         : 起始坐标
 * @param       width,height: 区域大小
 * @param       size        : 选择字体 12/16/24/32
 * @param       p           : 字符串首地址
 * @retval      无
 */
void lcd_show_string(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t size, char *p, uint16_t color)
{
    uint8_t x0 = x;
    width += x;
    height += y;

    while ((*p <= '~') && (*p >= ' '))   /* 判断是不是非法字符! */
    {
        if (x >= width)
        {
            x = x0;
            y += size;
        }

        if (y >= height)break;  /* 退出 */

        lcd_show_char(x, y, *p, size, 1, color);
        x += size / 2;
        p++;
    }
}

/**
 * @brief       打开LCD
 * @param       self_in：SPI控制块
 * @retval      mp_const_none：初始化成功
 */
void lcd_on(void)
{
    LCD_PWR(0);
    sleep_ms(10);
}

/**
 * @brief       关闭LCD
 * @param       self_in：SPI控制块
 * @retval      mp_const_none：初始化成功
 */
void lcd_off(void)
{
    LCD_PWR(1);
    sleep_ms(10);
}

/**
 * @brief       LCD初始化
 * @param       无
 * @retval      无
 */
void lcd_init(void)
{
   int cmd = 0;
   
   lcd_self.dir = 0;
   lcd_self.wr = LCD_NUM_WR;              /* 配置WR引脚 */
   lcd_self.cs = LCD_NUM_CS;              /* 配置CS引脚 */
   lcd_self.bl = LCD_NUM_BL;              /* 配置BL引脚 */

   /* WR管脚 */
   gpio_init(lcd_self.wr);                /* 设置GPIO引脚 */
   gpio_pull_up(lcd_self.wr);             /* 使能GPIO引脚上拉 */
   gpio_set_dir(lcd_self.wr, GPIO_OUT);   /* 设置GPIO引脚方向 */

   /* BL管脚 */
   gpio_init(lcd_self.bl);                /* 设置GPIO引脚 */
   gpio_pull_down(lcd_self.bl);           /* 使能GPIO引脚下拉 */
   gpio_set_dir(lcd_self.bl, GPIO_OUT);   /* 设置GPIO引脚方向 */

   /* CS管脚 */
   gpio_init(lcd_self.cs);                /* 设置GPIO引脚 */
   gpio_pull_up(lcd_self.cs);             /* 使能GPIO引脚上拉 */
   gpio_set_dir(lcd_self.cs, GPIO_OUT);   /* 设置GPIO引脚方向 */

   LCD_CS(1);
   LCD_WR(1);
   sleep_ms(200);

   /* 1.14寸lcd屏幕初始化序列 */
   lcd_init_cmd_t ili_init_cmds[] =
   {
       {0x11, {0}, 0x80},
       {0x36, {0x70}, 1},
       {0x3A, {0x05}, 1},
       {0xB2, {0x0C, 0x0C, 0x00, 0x33, 0x33}, 5},
       {0xB7, {0x35}, 1},
       {0xBB, {0x19}, 1},
       {0xC0, {0x2C}, 1},
       {0xC2, {0x01}, 1},
       {0xC3, {0x12}, 1},
       {0xC4, {0x20}, 1},
       {0xC6, {0x01}, 1},
       {0xD0, {0xA4,0xA1}, 2},
       {0xE0, {0xD0, 0x04, 0x0D, 0x11, 0x13, 0x2B, 0x3F, 0x54, 0x4C, 0x18, 0x0D, 0x0B, 0x1F, 0x23}, 14},
       {0xE1, {0xD0, 0x04, 0x0C, 0x11, 0x13, 0x2C, 0x3F, 0x44, 0x51, 0x2F, 0x1F, 0x1F, 0x20, 0x23}, 14},
       {0x21, {0}, 0x80},
       {0x29, {0}, 0x80},
       {0, {0}, 0xff},
   };

   /* 发送初始化序列 */
   while (ili_init_cmds[cmd].databytes != 0xff)
   {
       lcd_write_cmd(ili_init_cmds[cmd].cmd);
       lcd_write_data(ili_init_cmds[cmd].data, ili_init_cmds[cmd].databytes & 0x1F);
       
       if (ili_init_cmds[cmd].databytes & 0x80)
       {
           sleep_ms(120);
       }
       
       cmd++;
   }

   lcd_display_dir(1);                                             /* 设置屏幕方向 */
   lcd_clear(WHITE);
   lcd_flush();
   lcd_on();
}
 