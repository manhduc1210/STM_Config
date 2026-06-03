#ifndef OTA_PROTOCOL_H
#define OTA_PROTOCOL_H

#include <stdint.h>

#define OTA_MAGIC        0x4F54U
#define OTA_VERSION      0x01U
#define OTA_MAX_PAYLOAD  1024U

#define CMD_HELLO        0x01U
#define CMD_ERASE_APP    0x04U
#define CMD_WRITE_CHUNK  0x05U
#define CMD_ACK          0x80U
#define CMD_NACK         0x81U

#define OTA_OK                   0x0000U
#define OTA_ERR_INVALID_CMD       0x1003U
#define OTA_ERR_INVALID_LENGTH    0x1004U
#define OTA_ERR_FLASH_ERASE       0x1008U
#define OTA_ERR_FLASH_WRITE       0x1009U
#define OTA_ERR_VERIFY            0x100AU

typedef struct __attribute__((packed))
{
    uint16_t magic;
    uint8_t  version;
    uint8_t  command;
    uint32_t sequence;
    uint32_t offset;
    uint16_t length;
    uint32_t crc32;
} ota_frame_t;

void ota_process_once(void);

#endif