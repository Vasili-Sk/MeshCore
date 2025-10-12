#include <Arduino.h>
#include "w25q_fs.h"

//--------------------------------------------------------------------+
// LFS Disk IO
//--------------------------------------------------------------------+
static int _internal_flash_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size)
{
  W25Q_STATE state = W25Q_ReadData((uint8_t *)buffer, size, off, block);
  return state == W25Q_OK ? LFS_ERR_OK : LFS_ERR_IO;
}    

// Program a region in a block. The block must have previously
// been erased. Negative error codes are propogated to the user.
// May return LFS_ERR_CORRUPT if the block should be considered bad.
static int _internal_flash_prog(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size)
{    
    W25Q_STATE state = W25Q_ProgramData((uint8_t *)buffer, size, off, block );
    return state == W25Q_OK ? LFS_ERR_OK : LFS_ERR_IO;
}    

// Erase a block. A block must be erased before being programmed.
// The state of an erased block is undefined. Negative error codes
// are propogated to the user.
// May return LFS_ERR_CORRUPT if the block should be considered bad.
static int _internal_flash_erase(const struct lfs_config *c, lfs_block_t block)
{       
     W25Q_STATE state = W25Q_ErasePage(block);

     return state == W25Q_OK ? LFS_ERR_OK : LFS_ERR_IO;
}   

// Sync the state of the underlying block device. Negative error codes
// are propogated to the user.
static int _internal_flash_sync(const struct lfs_config *c)
{    
    return LFS_ERR_OK; // don't need sync
}    

struct lfs_config _ExternalFSConfig = {
    .context = NULL, 
    .read = _internal_flash_read,
    .prog = _internal_flash_prog,
    .erase = _internal_flash_erase,
    .sync = _internal_flash_sync,

    .read_size = 16,
    .prog_size = W25Q_PAGE_SIZE,
    .block_size = W25Q_PAGE_SIZE,
    .block_count = LFS_FLASH_TOTAL_SIZE / W25Q_PAGE_SIZE,
    .lookahead = 128,

    .read_buffer = NULL,
    .prog_buffer = NULL,
    .lookahead_buffer = NULL,
    .file_buffer = NULL
};

W25Q_FileSystem ExternalFS;

//--------------------------------------------------------------------+
//
//--------------------------------------------------------------------+

W25Q_FileSystem::W25Q_FileSystem(void)
  : Adafruit_LittleFS(&_ExternalFSConfig)
{

}

bool W25Q_FileSystem::begin(void)
{
  volatile bool format_fs;
  #ifdef FORMAT_FS
  format_fs = true;
  #else
  format_fs = false; // you can always use debugger to force formatting ;)
  #endif
  // failed to mount, erase all sector then format and mount again
  if ( format_fs || !Adafruit_LittleFS::begin() )
  {
    // lfs format
    this->format();
    // mount again if still failed, give up
    if ( !Adafruit_LittleFS::begin() ) return false;
  }

  return true;
}