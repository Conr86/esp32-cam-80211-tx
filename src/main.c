#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_event_loop.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_camera.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "string.h"
#include <sys/param.h>
#include "math.h"

#define BOARD_ESP32CAM_AITHINKER

// ESP32Cam (AiThinker) PIN Map
#define CAM_PIN_PWDN 32
#define CAM_PIN_RESET -1 //software reset will be performed
#define CAM_PIN_XCLK 0
#define CAM_PIN_SIOD 26
#define CAM_PIN_SIOC 27

#define CAM_PIN_D7 35
#define CAM_PIN_D6 34
#define CAM_PIN_D5 39
#define CAM_PIN_D4 36
#define CAM_PIN_D3 21
#define CAM_PIN_D2 19
#define CAM_PIN_D1 18
#define CAM_PIN_D0 5
#define CAM_PIN_VSYNC 25
#define CAM_PIN_HREF 23
#define CAM_PIN_PCLK 22


static const char *TAG = "camera_LRC";

static camera_config_t camera_config = {
    .pin_pwdn = CAM_PIN_PWDN,
    .pin_reset = CAM_PIN_RESET,
    .pin_xclk = CAM_PIN_XCLK,
    .pin_sscb_sda = CAM_PIN_SIOD,
    .pin_sscb_scl = CAM_PIN_SIOC,

    .pin_d7 = CAM_PIN_D7,
    .pin_d6 = CAM_PIN_D6,
    .pin_d5 = CAM_PIN_D5,
    .pin_d4 = CAM_PIN_D4,
    .pin_d3 = CAM_PIN_D3,
    .pin_d2 = CAM_PIN_D2,
    .pin_d1 = CAM_PIN_D1,
    .pin_d0 = CAM_PIN_D0,
    .pin_vsync = CAM_PIN_VSYNC,
    .pin_href = CAM_PIN_HREF,
    .pin_pclk = CAM_PIN_PCLK,

    //XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental)
    .xclk_freq_hz = 20000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_JPEG, //YUV422,GRAYSCALE,RGB565,JPEG
    .frame_size = FRAMESIZE_QQVGA,    //QQVGA-UXGA Do not use sizes above QVGA when not JPEG

    .jpeg_quality = 18, //0-63 lower number means higher quality
    .fb_count = 1       //if more than one, i2s runs in continuous mode. Use only with JPEG
};

static esp_err_t init_camera()
{
    //initialize the camera
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Camera Init Failed");
        return err;
    }

    return ESP_OK;
}

esp_err_t esp_wifi_80211_tx(wifi_interface_t ifx, const void *buffer, int len, bool en_sys_seq);

uint8_t beacon_raw[] = {
	0x80, 0x00,							// 0-1: Frame Control
	0x00, 0x00,							// 2-3: Duration
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff,				// 4-9: Destination address (broadcast)
	0xb0, 0x0b, 0xb0, 0x0b, 0xb0, 0x0b,				// 10-15: Source address
	0xb0, 0x0b, 0xb0, 0x0b, 0xb0, 0x0b,				// 16-21: BSSID
	0x00, 0x00,							// 22-23: Sequence / fragment number
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,			// 24-31: Timestamp (GETS OVERWRITTEN TO 0 BY HARDWARE)
	0x64, 0x00,							// 32-33: Beacon interval
	0x31, 0x04,							// 34-35: Capability info
	0x00, 0x00, /* FILL CONTENT HERE */				// 36-38: SSID parameter set, 0x00:length:content
	0x01, 0x08, 0x82, 0x84,	0x8b, 0x96, 0x0c, 0x12, 0x18, 0x24,	// 39-48: Supported rates
	0x03, 0x01, 0x01,						// 49-51: DS Parameter set, current channel 1 (= 0x01),
	0x05, 0x04, 0x01, 0x02, 0xDD, 0x00,				// 52-57: Traffic Indication Map
};

#define BEACON_SSID_OFFSET 38
#define SRCADDR_OFFSET 10
#define BSSID_OFFSET 16
#define SEQNUM_OFFSET 22

// Length of camera buffer to get
#define CAMERA_BUFF_LEN 256

#define RETRY_COUNT 100

esp_err_t event_handler(void *ctx, system_event_t *event) {
	return ESP_OK;
}

void spam_task(void *pvParameter) {
    uint16_t uid = 0;

	for (;;) {
		vTaskDelay(100 / portTICK_PERIOD_MS);

		camera_fb_t *pic = esp_camera_fb_get();
	
        ESP_LOGI(TAG, "Picture taken! Its size was: %zu bytes", pic->len);

		uint8_t num_packets = ceil(pic->len / 256) + 1;

		// send every packet RETRY_COUNT times
		for (uint16_t t = 0; t < RETRY_COUNT; t++) {
			// Split image up into `num` packets
			for (uint16_t seqnum = 0; seqnum <= num_packets; seqnum++) {
				// Create the 802.11 frame
				uint8_t new_beacon[sizeof(beacon_raw) + CAMERA_BUFF_LEN];
				// Copy the header defined above (beacon_raw) into the start of our frame
				memcpy(new_beacon, beacon_raw, BEACON_SSID_OFFSET - 1);
				// Set the length of the "SSID" to be the length of our camera data
				//new_beacon[BEACON_SSID_OFFSET - 1] = CAMERA_BUFF_LEN;//pic->len;
				// Copy some of the camera data into the frame
				memcpy(&new_beacon[BEACON_SSID_OFFSET], pic->buf + CAMERA_BUFF_LEN * seqnum, CAMERA_BUFF_LEN);
				// Append the end of the raw frame to our frame, this defines some beacon parameters
				memcpy(&new_beacon[BEACON_SSID_OFFSET + CAMERA_BUFF_LEN], &beacon_raw[BEACON_SSID_OFFSET], sizeof(beacon_raw) - BEACON_SSID_OFFSET);
				// Store number of packets that this camera frame has been sent over, and also the frame uid
				new_beacon[sizeof(new_beacon) - 2] = num_packets;
				new_beacon[sizeof(new_beacon) - 1] = uid;
				// Update sequence number with current index of packet
				new_beacon[SEQNUM_OFFSET] = (seqnum & 0x0f) << 4;
				new_beacon[SEQNUM_OFFSET + 1] = (seqnum & 0xff0) >> 4;
				// Send the data
				esp_wifi_80211_tx(WIFI_IF_AP, new_beacon, sizeof(new_beacon), false);

				vTaskDelay(10 / portTICK_PERIOD_MS);
			}
			vTaskDelay(10 / portTICK_PERIOD_MS);
		}
		uid++;
	}
}

void app_main(void) {
	nvs_flash_init();
	tcpip_adapter_init();
    init_camera();

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

	ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

	// Init dummy AP to specify a channel and get WiFi hardware into
	// a mode where we can send the actual fake beacon frames.
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
	wifi_config_t ap_config = {
		.ap = {
			.ssid = "esp32-beaconspam",
			.ssid_len = 0,
			.password = "dummypassword",
			.channel = 1,
			.authmode = WIFI_AUTH_WPA2_PSK,
			.ssid_hidden = 1,
			.max_connection = 4,
			.beacon_interval = 60000
		}
	};

	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
	ESP_ERROR_CHECK(esp_wifi_start());
	ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));

	xTaskCreate(&spam_task, "spam_task", 2048, NULL, 5, NULL);
}