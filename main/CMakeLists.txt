set(COMPONENT_SRCS
	"app_main.c"
	"mqtt_demo_mutual_auth.c"

	"aws_iot_handler.h"
	"StationMode.c"
	"AccessPointMode.c"
	"TcpServer.c"
	"UdpServer.c"
	"udp_response_handler.c"
	"wifi_info_handler.c"
	"GPIO.c"
	"ota_lib/tcp_ota_handler.c"
	"ota_lib/ota_handler.c"
	"ota_lib/flash_boot_handler.c"
	"checksum_handler.c"
	"memory_handler/flash_wifi_info_handler.c"
	"memory_handler/flash_reset_handler.c"
	"reset_handler.c"

	)

set(COMPONENT_ADD_INCLUDEDIRS
	"."
	"${CMAKE_CURRENT_LIST_DIR}"
	)

idf_component_register(SRCS "account_handler.c" "aws_iot_handler.c" "${COMPONENT_SRCS}"
					   INCLUDE_DIRS ${COMPONENT_ADD_INCLUDEDIRS}
					  )