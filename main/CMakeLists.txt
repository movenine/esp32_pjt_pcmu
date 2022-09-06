idf_component_register(SRCS "fs.c" "main.c"
					"protocol.c"
                    INCLUDE_DIRS ".")
# Create a SPIFFS image from the contents of the 'spiffs_image' directory
# 20220727 leedg : 파티션 추가
spiffs_create_partition_image(storage ../spiffs_image FLASH_IN_PROJECT)