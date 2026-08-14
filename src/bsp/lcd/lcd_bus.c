/**
 * @file        lcd_bus.c
 * @brief       ST7789 SPI 命令/数据与 DMA 像素传输
 */

#include "bsp/lcd/lcd_priv.h"
#include "hardware/dma.h"
#include "hardware/irq.h"

#define LCD_DMA_TIMEOUT_US      50000u

static int lcd_dma_tx = -1;
static int lcd_dma_rx = -1;
static volatile bool lcd_dma_busy;
static uint16_t lcd_dma_rx_dump;

void lcd_spi_8bit(void)
{
    spi_set_format(SPI_PORT, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
}

void lcd_spi_16bit(void)
{
    spi_set_format(SPI_PORT, 16, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
}

static void lcd_dma_abort(void)
{
    if (lcd_dma_rx >= 0)
    {
        dma_channel_set_irq0_enabled(lcd_dma_rx, false);
    }
    if (lcd_dma_tx >= 0)
    {
        dma_channel_abort(lcd_dma_tx);
    }
    if (lcd_dma_rx >= 0)
    {
        dma_channel_abort(lcd_dma_rx);
        dma_channel_acknowledge_irq0(lcd_dma_rx);
        dma_channel_set_irq0_enabled(lcd_dma_rx, true);
    }
    lcd_spi_8bit();
    LCD_CS(1);
    lcd_dma_busy = false;
}

void lcd_wait_idle(void)
{
    uint32_t t0 = time_us_32();

    while (lcd_dma_busy)
    {
        if ((time_us_32() - t0) > LCD_DMA_TIMEOUT_US)
        {
            lcd_dma_abort();
            break;
        }
        tight_loop_contents();
    }
}

bool lcd_is_busy(void)
{
    return lcd_dma_busy;
}

static void lcd_dma_irq_handler(void)
{
    if (lcd_dma_rx < 0 || !dma_channel_get_irq0_status(lcd_dma_rx))
    {
        return;
    }
    dma_channel_acknowledge_irq0(lcd_dma_rx);
    lcd_spi_8bit();
    LCD_CS(1);
    lcd_dma_busy = false;
}

void lcd_bus_init(void)
{
    dma_channel_config tx_cfg;
    dma_channel_config rx_cfg;

    if (lcd_dma_tx >= 0)
    {
        return;
    }

    lcd_dma_tx = dma_claim_unused_channel(true);
    lcd_dma_rx = dma_claim_unused_channel(true);

    tx_cfg = dma_channel_get_default_config(lcd_dma_tx);
    channel_config_set_transfer_data_size(&tx_cfg, DMA_SIZE_16);
    channel_config_set_dreq(&tx_cfg, spi_get_dreq(SPI_PORT, true));
    channel_config_set_read_increment(&tx_cfg, true);
    channel_config_set_write_increment(&tx_cfg, false);
    dma_channel_configure(lcd_dma_tx, &tx_cfg, &spi_get_hw(SPI_PORT)->dr, NULL, 0, false);

    rx_cfg = dma_channel_get_default_config(lcd_dma_rx);
    channel_config_set_transfer_data_size(&rx_cfg, DMA_SIZE_16);
    channel_config_set_dreq(&rx_cfg, spi_get_dreq(SPI_PORT, false));
    channel_config_set_read_increment(&rx_cfg, false);
    channel_config_set_write_increment(&rx_cfg, false);
    dma_channel_configure(lcd_dma_rx, &rx_cfg, &lcd_dma_rx_dump, &spi_get_hw(SPI_PORT)->dr, 0, false);

    dma_channel_set_irq0_enabled(lcd_dma_rx, true);
    irq_add_shared_handler(DMA_IRQ_0, lcd_dma_irq_handler,
                           PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY);
    irq_set_enabled(DMA_IRQ_0, true);
}

void lcd_dma_start(const uint16_t *src, size_t count)
{
    if (src == NULL || count == 0)
    {
        return;
    }

    while (spi_is_readable(SPI_PORT))
    {
        (void)spi_get_hw(SPI_PORT)->dr;
    }

    lcd_dma_busy = true;
    dma_channel_set_read_addr(lcd_dma_tx, src, false);
    dma_channel_set_trans_count(lcd_dma_tx, (uint32_t)count, false);
    dma_channel_set_trans_count(lcd_dma_rx, (uint32_t)count, false);
    dma_start_channel_mask((1u << lcd_dma_tx) | (1u << lcd_dma_rx));
}

void lcd_write_cmd(uint8_t cmd)
{
    lcd_wait_idle();
    LCD_DC(0);
    LCD_CS(0);
    spi_write_blocking(SPI_PORT, &cmd, 1);
    LCD_CS(1);
}

void lcd_write_data(const uint8_t *data, int len)
{
    lcd_wait_idle();
    if (data == NULL || len <= 0)
    {
        return;
    }
    LCD_DC(1);
    LCD_CS(0);
    spi_write_blocking(SPI_PORT, data, (size_t)len);
    LCD_CS(1);
}
