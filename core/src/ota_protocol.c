#include "ota_protocol.h"
#include "uart_drv.h"

static void ota_send_response(uint8_t cmd, uint32_t seq)
{
    ota_frame_t resp;

    resp.magic    = OTA_MAGIC;
    resp.version  = OTA_VERSION;
    resp.command  = cmd;
    resp.sequence = seq;
    resp.length   = 0;
    resp.crc32    = 0;

    uart2_write((uint8_t *)&resp, sizeof(resp));
}

void ota_process_once(void)
{
    ota_frame_t frame;

    uart2_read_blocking((uint8_t *)&frame, sizeof(frame));

    if (frame.magic != OTA_MAGIC)
    {
        ota_send_response(CMD_NACK, frame.sequence);
        return;
    }

    if (frame.version != OTA_VERSION)
    {
        ota_send_response(CMD_NACK, frame.sequence);
        return;
    }

    if (frame.command == CMD_HELLO)
    {
        ota_send_response(CMD_ACK, frame.sequence);
    }
    else
    {
        ota_send_response(CMD_NACK, frame.sequence);
    }
}