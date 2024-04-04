# PCMU project README
ESP32 모듈을 활용한 전원제어 및 모니터링 보드 개발 프로젝트입니다.

## Overview
- Hardware : RS485 2 port, Relay 2ch, Current sensor(Uart), GPIO
- Firmware : C (FreeRTOS)
- Workspace : Visual studio code, esp-idf 4.0
- 빌드된 이미지들은 flash download tool 로 다운로드 합니다. [flash downloader](https://www.espressif.com/en/support/download/other-tools)

## Features
- AC전원 전류감시
- AC전원 On/Off 제어, 수동개폐 버튼
- 모니터링 데이터 전송 (RS485)
- LED 상태표시

## Program flow
  펌웨어 구동 순서에 관한 플로우는 아래 링크를 참조
  [diagram](https://drive.google.com/file/d/19-ZbDikXsktoTc2qnoBGVGnfmlUCmKA7/view?usp=drive_link)

## Envirments & Build
- Install ESP-IDF for vscode [참조](https://github.com/espressif/vscode-esp-idf-extension/blob/HEAD/docs/tutorial/install.md)
- 프로젝트 폴더 생성 및 git download
- compilePath 지정 (file: c_cpp_properties.json)
  
  컴파일 실패시, 크로스컴파일 버전을 확인하고 경로를 지정해 주어야 합니다.
  ```
  "compilerPath": "C:\\Users\\[user_folder_name]\\.espressif\\tools\\xtensa-esp32-elf\\esp-2021r2-patch2-8.4.0\\xtensa-esp32-elf\\bin\\xtensa-esp32-elf-gcc.exe",
  
- settings.json 파일의 시리얼 포트 설정
  ```
  {
    "idf.adapterTargetName": "esp32",
    "idf.openOcdConfigs": [
        "board/esp32-wrover-kit-3.3v.cfg"
    ],
    "idf.flashType": "UART",
    "idf.portWin": "COM5",
    "files.associations": {
        "*.ipp": "c",
        "esp_log.h": "c",
        "gpio.h": "c",
        "*.inc": "c",
        "safe_iop.h": "c",
        "esp_netif.h": "c",
        "stdint.h": "c",
        "esp_system.h": "c",
        "esp_err.h": "c"
    },
    "C_Cpp.errorSquiggles": "disabled"
  }

- CMaskLists.txt 파일에 추가 소스코드 및 스토리지 파일 경로 설정
  ```
  idf_component_register(SRCS "safe_iop.c" "fs.c" "main.c"
					"protocol.c"
                    INCLUDE_DIRS ".")
  # Create a SPIFFS image from the contents of the 'spiffs_image' directory
  # 20220727 leedg : 파티션 추가
  spiffs_create_partition_image(storage ../spiffs_image FLASH_IN_PROJECT)

- vscode 창 하단에 메뉴를 통해 **menuconfig**, **clean**, **build**, **flash** 작업을 진행

## Flash Download 사용법
  이미지 다운로드 방법은 아래 링크를 참조
  [다운로드 방법](https://drive.google.com/file/d/1CqSedxag_xgJu6Ys-tJ91rqqmVvL2GL5/view?usp=sharing)

  
