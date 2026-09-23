Differences entre les 2 projets
'https://github.com/espressif/arduino-esp32/tree/master/libraries/SD' et '.../Arduino15/packages/esp32/hardware/esp32/1.0.4/libraries/SD'
-----------------

diff -br src/SD.h C:\Users\Sara RUDEL\Documents\Arduino\ESP32\GpsCompass_1.2.5\src/SD.h
31c31
<     bool begin(uint8_t ssPin=SS, SPIClass &spi=SPI, uint32_t frequency=4000000, const char * mountpoint="/sd", uint8_t max_files=5);
---
>     bool begin(uint8_t ssPin=SS, SPIClass &spi=SPI, uint32_t frequency=4000000, const char * mountpoint="/sd", uint8_t max_files=5, bool format_if_empty=false);
34a35,36
>     size_t numSectors();
>     size_t sectorSize();
36a39,40
>     bool readRAW(uint8_t* buffer, uint32_t sector);
>     bool writeRAW(uint8_t* buffer, uint32_t sector);
diff -br src/sd_diskio.cpp C:\Users\Sara RUDEL\Documents\Arduino\ESP32\GpsCompass_1.2.5\src/sd_diskio.cpp
14a15
> #include "esp_system.h"
16,17d16
<     #include "diskio.h"
<     #include "ffconf.h"
18a18,21
>     #include "diskio.h"
> #if ESP_IDF_VERSION_MAJOR > 3
>     #include "diskio_impl.h"
> #endif
61a65,89
> #if ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_ERROR
> const char * fferr2str[] = {
>     "(0) Succeeded",
>     "(1) A hard error occurred in the low level disk I/O layer",
>     "(2) Assertion failed",
>     "(3) The physical drive cannot work",
>     "(4) Could not find the file",
>     "(5) Could not find the path",
>     "(6) The path name format is invalid",
>     "(7) Access denied due to prohibited access or directory full",
>     "(8) Access denied due to prohibited access",
>     "(9) The file/directory object is invalid",
>     "(10) The physical drive is write protected",
>     "(11) The logical drive number is invalid",
>     "(12) The volume has no work area",
>     "(13) There is no valid FAT volume",
>     "(14) The f_mkfs() aborted due to any problem",
>     "(15) Could not get a grant to access the volume within defined period",
>     "(16) The operation is rejected according to the file sharing policy",
>     "(17) LFN working buffer could not be allocated",
>     "(18) Number of open files > FF_FS_LOCK",
>     "(19) Given parameter is invalid"
> };
> #endif
>
74a103,105
>     if (!resp) {
>         log_w("Wait Failed");
>     }
93c124,129
<     sdWait(pdrv, 300);
---
>     bool s = sdWait(pdrv, 500);
>     if (!s) {
>         log_e("Select Failed");
>         digitalWrite(card->ssPin, HIGH);
>         return false;
>     }
107c143
<                 return token;
---
>                 break;
110c146,147
<                 return 0xFF;
---
>                 token = 0xFF;
>                 break;
161c198,201
<
---
>     if (token == 0xFF) {
>         log_e("Card Failed! cmd: 0x%02x", cmd);
>         card->status = STA_NOINIT;
>     }
217c257
<             break;
---
>             return false;
237c277
<             break;
---
>             return false;
273c313
<             break;
---
>             return false;
309c349
<                 break;
---
>                 return false;
314c354
<             break;
---
>             return false;
346,347d385
<                 sdDeselectCard(pdrv);
<
348a387
>                     sdDeselectCard(pdrv);
367c406
<                     return false;
---
>                     break;
382c421
<             break;
---
>             return false;
470c509,515
<     if (sdTransaction(pdrv, GO_IDLE_STATE, 0, NULL) != 1) {
---
>     // Fix mount issue - sdWait fail ignored before command GO_IDLE_STATE
>     digitalWrite(card->ssPin, LOW);
>     if(!sdWait(pdrv, 500)){
>         log_w("sdWait fail ignored, card initialize continues");
>     }
>     if (sdCommand(pdrv, GO_IDLE_STATE, 0, NULL) != 1){
>         sdDeselectCard(pdrv);
473a519
>     sdDeselectCard(pdrv);
572a619,623
>     if(sdTransaction(pdrv, SEND_STATUS, 0, NULL))
>     {
>         log_e("Check status failed");
>         return STA_NOINIT;
>     }
610c661
<     }
---
>     } else {
611a663
>     }
639a692,700
> bool sd_read_raw(uint8_t pdrv, uint8_t* buffer, DWORD sector)
> {
>     return ff_sd_read(pdrv, buffer, sector, 1) == ESP_OK;
> }
>
> bool sd_write_raw(uint8_t pdrv, uint8_t* buffer, DWORD sector)
> {
>     return ff_sd_write(pdrv, buffer, sector, 1) == ESP_OK;
> }
650a712
>     sdTransaction(pdrv, GO_IDLE_STATE, 0, NULL);
655a718
>         free(card->base_path);
714c777
< bool sdcard_mount(uint8_t pdrv, const char* path, uint8_t max_files)
---
> bool sdcard_mount(uint8_t pdrv, const char* path, uint8_t max_files, bool format_if_empty)
739c802,812
<         log_e("f_mount failed 0x(%x)", res);
---
>         log_e("f_mount failed: %s", fferr2str[res]);
>         if(res == 13 && format_if_empty){
>             BYTE* work = (BYTE*) malloc(sizeof(BYTE) * FF_MAX_SS);
>             if (!work) {
>               log_e("alloc for f_mkfs failed");
>               return false;
>             }
>             res = f_mkfs(drv, FM_ANY, 0, work, sizeof(work));
>             free(work);
>             if (res != FR_OK) {
>                 log_e("f_mkfs failed: %s", fferr2str[res]);
742a816,826
>             res = f_mount(fs, drv, 1);
>             if (res != FR_OK) {
>                 log_e("f_mount failed: %s", fferr2str[res]);
>                 esp_vfs_fat_unregister_path(path);
>                 return false;
>             }
>         } else {
>             esp_vfs_fat_unregister_path(path);
>             return false;
>         }
>     }
diff -br src/sd_diskio.h C:\Users\Sara RUDEL\Documents\Arduino\ESP32\GpsCompass_1.2.5\src/sd_diskio.h
19a20
> // #include "diskio.h"
24c25
< bool sdcard_mount(uint8_t pdrv, const char* path, uint8_t max_files);
---
> bool sdcard_mount(uint8_t pdrv, const char* path, uint8_t max_files, bool format_if_empty);
29a31,32
> bool sd_read_raw(uint8_t pdrv, uint8_t* buffer, uint32_t sector);
> bool sd_write_raw(uint8_t pdrv, uint8_t* buffer, uint32_t sector);

Erreurs
-------
09:46:35.053 -> All infos GPS available...
09:46:35.053 -> [W][sd_diskio.cpp:143] sdCommand(): crc error
09:46:35.155 -> [W][sd_diskio.cpp:143] sdCommand(): crc error
09:46:35.257 -> [W][sd_diskio.cpp:143] sdCommand(): crc error
09:46:35.359 -> [W][sd_diskio.cpp:143] sdCommand(): crc error
09:46:35.461 -> [W][sd_diskio.cpp:143] sdCommand(): crc error
09:46:35.563 -> [W][sd_diskio.cpp:143] sdCommand(): crc error
09:46:35.665 -> [W][sd_diskio.cpp:143] sdCommand(): crc error
09:46:35.767 -> [W][sd_diskio.cpp:143] sdCommand(): crc error
09:46:36.089 -> [W][sd_diskio.cpp:143] sdCommand(): crc error
09:46:36.089 -> [W][sd_diskio.cpp:143] sdCommand(): crc error
09:46:36.089 -> [E][vfs_api.cpp:265] VFSFileImpl(): fopen(/sd/FRAME/GpsFrames.txt) failed

=> Reparation avec:
- SDCard setInhAppendGpsFrame
- SDCard end
- SDCard init
- SDCard setInhAppendGpsFrame false
=> Ne fonctionne pas toujours ;-(

Warning:
10:34:31.021 -> SDFS::begin(5, ...)
10:34:31.021 -> [W][sd_diskio.cpp:143] sdCommand(): crc error
10:34:31.122 -> [W][sd_diskio.cpp:137] sdCommand(): no token received
10:34:31.224 -> 	=> init(): [Ok]
=> TODO: Propagation de l'erreur 'sdCommand(): crc error'...

10:35:16.520 -> SDFS::begin(5, ...)
10:35:16.520 -> 	=> init(): [Ok]

10:47:04.529 -> SDFS::begin(5, ...)
10:47:04.529 -> [W][sd_diskio.cpp:143] sdCommand(): crc error
10:47:04.631 -> [W][sd_diskio.cpp:137] sdCommand(): no token received
10:47:04.733 -> [W][sd_diskio.cpp:143] sdCommand(): crc error
10:47:04.835 -> [W][sd_diskio.cpp:143] sdCommand(): crc error
10:47:04.936 -> [W][sd_diskio.cpp:143] sdCommand(): crc error
10:47:05.038 -> [E][sd_diskio.cpp:739] sdcard_mount(): f_mount failed 0x(1)
10:47:05.038 -> SDCard::init(): Error: SD Card mount failed
10:47:05.038 -> 	=> init(): [Ko]

18:02:54.178 -> >>> [SDCard readFile  /LOG/log.txt ] (length: 30)
18:02:54.224 -> [E][vfs_api.cpp:64] open(): /sd/LOG/log.txt does not exist
18:02:54.224 -> SDCard::readFile(/LOG/log.txt): Failed to open file for reading
18:02:54.224 -> 	=> readFile(/LOG/log.txt): [Ko]
18:02:54.224 -> >>> Type the command...

# 'init' avec 'crc error'
# => TODO: Reporter l'erreur dans le retour ;-)
#    => Le retour de 'SD.begin(SD_CS)' est correct
#       => Ignore en attendant les retours d'exprerience

18:34:21.511 -> SDFS::begin(5, ...)
18:34:21.817 -> [W][sd_diskio.cpp:143] sdCommand(): crc error
18:34:22.020 -> 	=> init(): [Ok]
18:34:22.020 -> >>> Type the command...
18:34:32.151 -> Timers::test(): Expiration #10/#24 (Timer for error #1) -> call [0x400d3e60]
18:34:32.151 -> Timers::start(#10-Timer for error #1, 3000, 0x400d3e60)
18:34:32.151 -> activity_error_1(): 1 error(s)
18:34:32.151 -> 	#0: Error [3872 - 0xf20]
18:34:37.148 -> >>> [SDCard init] (length: 11)
18:34:37.148 -> SDFS::begin(5, ...)
18:34:37.148 -> 	=> init(): [Ok]

#end of file