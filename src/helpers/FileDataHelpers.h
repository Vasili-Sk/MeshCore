#pragma once

#include <stddef.h>
#include <stdint.h>

struct FileDataPacket_t {
  uint32_t Timestep;
  union {
    uint16_t Offset;
    uint16_t Size;
  };
  union {
    struct {
      uint8_t Flag_FileDescriptor : 1; // included data is a size (3 byte), modified date (4 byte) and name of file
      uint8_t Flag_IsFile : 1;         // included data is a file data
      uint8_t Flag_IsOTA : 1;          // over-the-air update binary data
      uint8_t Flag_24bOffset : 1;      // for offset more than 16kB, uses 3 bytes
      uint8_t Flag_CompressedData : 1; // for future
    };
    uint8_t Flags;
  };
  union {
    uint8_t Offset_3b;
    uint8_t Size_3b;
  };
  union {
    uint32_t FileDate;
    uint32_t DataPtr;
  };
};
