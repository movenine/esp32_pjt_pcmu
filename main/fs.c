/* SPIFFS filesystem example.
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_spiffs.h"

#include "define.h"
#include "protocol.h"
#include "fs.h"

static const char *PCMU_FILE_LOG = "pcmu_file";

void init_fileconfig(void)
{
   ESP_LOGI(PCMU_FILE_LOG, "Initializing spiffs");
   esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",     // spiffs image root 폴더
      .partition_label = NULL,
      .max_files = 6,
      .format_if_mount_failed = true
   };
   // Register and mount SPIFFS to VFS with given path prefix.
   esp_err_t ret = esp_vfs_spiffs_register(&conf);

   if (ret != ESP_OK) {
      if (ret == ESP_FAIL) {
         ESP_LOGE(PCMU_FILE_LOG, "Failed to mount or format filesystem");
      }
      else if (ret == ESP_ERR_NOT_FOUND) {
         ESP_LOGE(PCMU_FILE_LOG, "Failed to find SPIFFS partition");
      } 
      else {
         ESP_LOGE(PCMU_FILE_LOG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
      }
      return;
   }
   // Get information for SPIFFS
   size_t total = 0, used = 0;
   ret = esp_spiffs_info(conf.partition_label, &total, &used);
   if (ret != ESP_OK) {
      ESP_LOGE(PCMU_FILE_LOG, "Failed to get SPIFFS partition information (%s). Formatting...", esp_err_to_name(ret));
      esp_spiffs_format(conf.partition_label);
      return;
   } else {
      ESP_LOGI(PCMU_FILE_LOG, "Partition size: total: %d, used: %d", total, used);
   }
}

// file read using spiffs filesystem
int fs_read(void)
{
   char buf[READ_LENGH];
   char tmp[READ_LENGH];

   ESP_LOGI(PCMU_FILE_LOG, "file reading");
   FILE* f = fopen("/spiffs/status.txt", "r");
   if (f == NULL) {
      ESP_LOGE(PCMU_FILE_LOG, "Failed to open file for reading");
      return 0;
   } else {
      memset(buf, 0, sizeof(buf));
      fread(buf, 1, sizeof(buf), f);
      fclose(f);
      ESP_LOGI(PCMU_FILE_LOG, "read from file: '%s'", buf);
      // buf 값을 파싱해서 pcmu 구조체에 저장
      // buf : 106,0 (ID : 106, 릴레이on/off: 0 or 1)
      strncpy(tmp, buf, 3);   //ID 값 복사
      pcmu.board_id = (uint8_t)atoi(tmp);
      ESP_LOGI(PCMU_FILE_LOG, "board ID : 0x'%x'", pcmu.board_id);

      memset(tmp, 0, sizeof(tmp));
      strncpy(tmp, buf+4, 1); //relay 값 복사
      pcmu.output_relay = atoi(tmp);
      ESP_LOGI(PCMU_FILE_LOG, "relay : 0x'%d'", pcmu.output_relay);

      // assert failed: xQueueSemaphoreTake queue.c:1549 (pxQueue->uxItemSize == 0) 발생원인 
      // char *p = strtok(buf, ",");
      // while (p != NULL) {
      //    sArr[i] = p;
      //    i++;
      //    p = strtok(buf, ",");
      // }
      // // ID
      // if(sArr[0] != NULL) {
      //    pcmu.board_id = (uint8_t)atoi(sArr[0]);
      //    ESP_LOGI(PCMU_FILE_LOG, "board ID : 0x'%x'", pcmu.board_id);
      // }
      // if(sArr[1] != NULL) {
      //    pcmu.output_relay = (uint8_t)atoi(sArr[1]);
      //    ESP_LOGI(PCMU_FILE_LOG, "relay : 0x'%x'", pcmu.output_relay);
      // }
      return 1;
   }
}

// file write using filesystem
int fs_write(uint8_t id, int relay)
{
   char buf[READ_LENGH];
   ESP_LOGI(PCMU_FILE_LOG, "Opening file");
   FILE* f = fopen("/spiffs/status.txt", "w");
   if(f == NULL) {
      ESP_LOGE(PCMU_FILE_LOG, "Failed to open file for writing");
      return 0;
   } else {
      fprintf(f, "%c,%d", id, relay);
      fread(buf, 1, sizeof(buf), f);
      fclose(f);
      ESP_LOGI(PCMU_FILE_LOG, "file written: '%s'", buf);
      return 1;
   }
}

