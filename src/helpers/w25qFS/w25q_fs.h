 #ifndef W25Q_FILESYSTEM_H_
 #define W25Q_FILESYSTEM_H_
 
 #include "Adafruit_LittleFS.h"
 #include <w25q_mem.h>

#ifndef LFS_FLASH_TOTAL_SIZE /* Flash size can be configured in platformio.ini */
  #define LFS_FLASH_TOTAL_SIZE (1024 * 1024) /* defaults to 1Mbyte flash */
#endif
    
 class W25Q_FileSystem : public Adafruit_LittleFS
 {
   public:
     W25Q_FileSystem(void);
 
     // overwrite to also perform low level format (sector erase of whole flash region)
     bool begin(void);
 };
 
 extern W25Q_FileSystem ExternalFS;
 
 #endif /* INTERNALFILESYSTEM_H_ */