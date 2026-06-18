#include "ota_protocol.h"
#include "uart_drv.h"
#include "flash_manager.h"

static uint8_t g_payload[OTA_MAX_PAYLOAD];

static void ota_send_response(uint8_t cmd, uint32_t seq, uint16_t status)
{
    ota_frame_t resp;

    resp.magic    = OTA_MAGIC;
    resp.version  = OTA_VERSION;
    resp.command  = cmd;
    resp.sequence = seq;
    resp.offset   = 0;
    resp.length   = sizeof(status);
    resp.crc32    = 0;

    uart2_write((uint8_t *)&resp, sizeof(resp));
    uart2_write((uint8_t *)&status, sizeof(status));
}

static void ota_send_ack(uint32_t seq)
{
    uint16_t status = OTA_OK;
    ota_send_response(CMD_ACK, seq, status);
}

static void ota_send_nack(uint32_t seq, uint16_t error)
{
    ota_send_response(CMD_NACK, seq, error);
}

static void ota_read_frame_resync(ota_frame_t *frame)
{
    uint8_t *raw = (uint8_t *)frame;
    uint8_t byte;

    while (1)
    {
        byte = uart2_read_byte_blocking();
        if (byte != 0x54U)
        {
            continue;
        }

        byte = uart2_read_byte_blocking();
        if (byte != 0x4FU)
        {
            continue;
        }

        raw[0] = 0x54U;
        raw[1] = 0x4FU;
        uart2_read_blocking(&raw[2], sizeof(*frame) - 2U);
        return;
    }
}

static void ota_handle_hello(const ota_frame_t *frame)
{
    ota_send_ack(frame->sequence);
}

static void ota_handle_erase_app(const ota_frame_t *frame)
{
    flash_status_t ret;

    ret = flash_erase_app_region();

    if (ret == FLASH_OK)
    {
        ota_send_ack(frame->sequence);
    }
    else
    {
        ota_send_nack(frame->sequence, OTA_ERR_FLASH_ERASE);
    }
}

static void ota_handle_write_chunk(const ota_frame_t *frame)
{
    uint32_t write_addr;
    flash_status_t ret;

    if (frame->length == 0 || frame->length > OTA_MAX_PAYLOAD)
    {
        ota_send_nack(frame->sequence, OTA_ERR_INVALID_LENGTH);
        return;
    }

    uart2_read_blocking(g_payload, frame->length);

    write_addr = APP_START_ADDRESS + frame->offset;

    ret = flash_write_bytes(write_addr, g_payload, frame->length);
    if (ret != FLASH_OK)
    {
        ota_send_nack(frame->sequence, OTA_ERR_FLASH_WRITE);
        return;
    }

    ret = flash_verify_bytes(write_addr, g_payload, frame->length);
    if (ret != FLASH_OK)
    {
        ota_send_nack(frame->sequence, OTA_ERR_VERIFY);
        return;
    }

    ota_send_ack(frame->sequence);
}

void ota_process_once(void)
{
    ota_frame_t frame;

    ota_read_frame_resync(&frame);
    // const char msg[] = "FRAME_RX\r\n";
    // uart2_write((uint8_t*)msg, sizeof(msg)-1);

    if (frame.version != OTA_VERSION)
    {
        ota_send_nack(frame.sequence, OTA_ERR_INVALID_CMD);
        return;
    }

    switch (frame.command)
    {
        case CMD_HELLO:
            ota_handle_hello(&frame);
            break;

        case CMD_ERASE_APP:
            ota_handle_erase_app(&frame);
            break;

        case CMD_WRITE_CHUNK:
            ota_handle_write_chunk(&frame);
            break;

        default:
            ota_send_nack(frame.sequence, OTA_ERR_INVALID_CMD);
            break;
    }
}
