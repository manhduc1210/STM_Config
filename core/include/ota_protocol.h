#ifndef OTA_PROTOCOL_H
#define OTA_PROTOCOL_H

#include <stdint.h>

#define OTA_MAGIC      0x4F54U
#define OTA_VERSION    0x01U

#define CMD_HELLO      0x01U
#define CMD_ACK        0x80U
#define CMD_NACK       0x81U

typedef struct __attribute__((packed))
{
    uint16_t magic;
    uint8_t  version;
    uint8_t  command;
    uint32_t sequence;
    uint16_t length;
    uint32_t crc32;
} ota_frame_t;

void ota_process_once(void);

#endif