// =============================================================================
// 							MINIATURE GREENHOUSE PROJECT
// 								  LUCAS BUECHLER
// 									Jan 2026
//
// 		ESP32 based environmental monitoring and control program
// 		Designed to use off-the-shelf components and libraries
// 		All actuators should default to OFF
// 		Soil state and Air state are separate 
//      Heater may be on while Fans are on (though unlikely)
//
// =============================================================================
// INCLUDE　領域=================================================================
#include "driver/adc_types_legacy.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "ds1307.h"
#include "ds18x20.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_log_level.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hal/adc_types.h"
#include "hal/gpio_types.h"
#include "i2cdev.h"
#include "onewire.h"
#include "sht3x.h"
#include "soc/clk_tree_defs.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
// =============================================================================
// DEFINE　領域==================================================================
// ==================
// =SENSOR VARIABLES=
// ==================
// Currently set for Wild Camelia Ideal range 10-22 C
#define IDEAL_AIR_TEMP_C 16.0f
// Currently set for Wild Camelia Ideal +- 6 C
#define AIR_TEMP_RANGE_C 6.0f

// Currently set for Wild Camelia Ideal range 50-70%
#define IDEAL_AIR_HUMIDITY_PERCENT 60.0f
// Currently set for Wild Camelia Ideal +- 10%
#define AIR_HUMIDITY_RANGE_PERCENT 10.0f

// Currently set for Wild Camelia Ideal range 12-20 C
#define IDEAL_SOIL_TEMP_C 16.0f
// Currently set for Wild Camelia Ideal +- 4 C
#define SOIL_TEMP_RANGE_C 4.0f

// Currently set for Wild Camelia Ideal range 30-45%
#define IDEAL_SOIL_MOISTURE_PERCENT 37.5f
// Currently set for Wild Camelia Ideal +- 7.5%
#define SOIL_MOISTURE_RANGE_PERCENT 7.5f

// RAW DATA REQUIRES CALIBRATION PER SENSOR / SUPPLY VOLTAGE
// Raw for 0% moisture soil
#define SOIL_MOISTURE_MIN_RAW 2700
// Raw for 100% moisture soil
#define SOIL_MOISTURE_MAX_RAW 675

// ==================
// =  GPIO   SETUP  =
// ==================
// IN ==========================================================================
// CAP-SW-12	ADC compatible priority 2 pin.
#define SOIL_MOISTURE_IN_GPIO GPIO_NUM_0
//(Currently internal pull up enabled for safe boot,
//				interference chance low but non-zero)

// 	DS1307 / SHT31-D I2C compatible priority 2 pin.
#define I2C_SDA_GPIO GPIO_NUM_1

// 	DS1307 / SHT31-D I2C compatible priority 2 pin.
#define I2C_SCL_GPIO GPIO_NUM_3

// 	DS18B20	OneWire	Using pin 10 as it is  priority 2.
#define SOIL_TEMP_IN_GPIO GPIO_NUM_10
// (Onewire compatibility not explicitly stated in datasheet)

// OUT =========================================================================
// Datasheet specifies pins 4-6 as PWM compatible
#define FAN_OUT_GPIO GPIO_NUM_4

// JTAG funcionality Lost on pins 4-7 (Priority 3 Pins)
#define LED_OUT_GPIO GPIO_NUM_5

//  Use USB JTAG interface for debugging
#define HEATER_OUT_GPIO GPIO_NUM_6

// Pin 7 acting as plain GPIO
#define SIGNAL_LED_OUT_GPIO GPIO_NUM_7

// ==================
// =  DUTY   SETUP  =
// ==================
#define PWM_MAX_DUTY 1023

#define PWM_DUTY_OFF 0									//   0%
#define PWM_DUTY_LOW ((uint32_t)(PWM_MAX_DUTY * 0.25f)) //  25%
#define PWM_DUTY_FAN_MIN ((uint32_t)(PWM_MAX_DUTY * 0.40f)) // 40% (Fans won't reliably start below 35%)
#define PWM_DUTY_MED ((uint32_t)(PWM_MAX_DUTY * 0.75f)) //  75%
#define PWM_DUTY_HIGH PWM_MAX_DUTY						// 100%

// ==================
// = PWM  FREQUENCY =
// ==================
// Coil noise may be audible below 20000HZ | Very little gain beyond 30000HZ
#define PWM_FAN_FREQ_HZ 30000
// Raise if flicker noticeable | Suggested: 300HZ to 20000HZ
#define PWM_LED_FREQ_HZ 5000
// Theoretically could be run as low as 10HZ, but 1000 prevents long
#define PWM_HEAT_FREQ_HZ 1000
// open MOSFETS / slightly lowers stress on power lines,
//                             Essentially no benefit to higher frequency

// ==================
// =I2C  BUS  SETUP =
// ==================

#define I2C_PORT I2C_NUM_0 // Default I2C Port
#define SHT31_ADDR 0x44	   // Possibly 0x45 depending on manufacturer
#define DS1307_ADDR 0x68   // DS1307 Standard Address

// =============================================================================
// TYPEDEF　領域=================================================================
// =============================================================================
// ENUM
typedef enum { FAN_OFF, FAN_LOW, FAN_MED, FAN_HIGH } fan_state_t;
typedef enum { NIGHT, MORNING, MIDDAY, EVENING } time_state_t;
typedef enum { LED_OFF, LED_LOW, LED_MED, LED_HIGH } led_state_t;
typedef enum { HEAT_OFF, HEAT_LOW, HEAT_MED, HEAT_HIGH } heater_state_t;
typedef enum { SIG_LED_OFF, SIG_LED_ON } signal_led_state_t;

// PROTOTYPE　領域===============================================================
// ======
// =INIT=		Initialize PWM / GPIO / Misc
// ======
void pwm_timers_init(void);
void pwm_channels_init(void);
void pwm_init(void);
void gpio_init(void);
// ======
// =READ=		Read sensor data | used by state update functions
// ======
float read_soil_moisture_percent(adc_oneshot_unit_handle_t adc_handle);
// ======
// UPDATE		Update internal actuator states | used by Run functions
// ======
time_state_t update_time_state(struct tm current_time);
fan_state_t update_fan_state(float temp_from_probe_c,
							 float humid_from_probe_percent,
							 fan_state_t current_fan_state);
led_state_t update_led_state(time_state_t current_time,
							 float temp_from_probe_c);
heater_state_t update_heater_state(float temp_from_probe_c,
								   heater_state_t current_heater_state);
signal_led_state_t update_soil_moisture_led_state(
	signal_led_state_t current_soil_moisture_led_state,
	float soil_moisture_from_probe_percent);
void pwm_set(ledc_channel_t channel, uint32_t duty);
// ======
// =RUN	=		Run actuators based on updated state
// ======
void run_fan(fan_state_t current_fan_state);
void run_led(led_state_t current_led_state);
void run_signal_led(signal_led_state_t current_soil_moisture_led_state);
void run_heater(heater_state_t current_heater_state);

// =============================================================================
// INITIALIZATION AND SENSOR READ FUNCTIONS
// =============================================================================

float read_soil_moisture_percent(adc_oneshot_unit_handle_t adc_handle) {
	int raw = 0;
	ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_CHANNEL_0, &raw));

	// Sensor malfunction check ( if 100% < raw < 0% )
	if ((raw < SOIL_MOISTURE_MAX_RAW) || (raw > SOIL_MOISTURE_MIN_RAW)) {
		return -1.0f; // value is impossible, will set state to OFF
	} else {
		return ((float)(SOIL_MOISTURE_MIN_RAW - raw) /
				(SOIL_MOISTURE_MIN_RAW - SOIL_MOISTURE_MAX_RAW) * 100.0f);
	}
}

void gpio_init(void) {
	// IN ====================================================================
	// SOIL_MOISTURE_IN_GPIO configured by ADC init (pull up for boot safety)
	// I2C_SDA_GPIO configured by I2C init (Internal pull up)
	// I2C_SCL_GPIO configured by I2C init (Internal pull up)
	// SOIL_TEMP_IN_GPIO configured by OneWire init (External pull up)
	// OUT ===================================================================
	// FAN_OUT_GPIO configured by PWM init
	// LED_OUT_GPIO configured by PWM init
	// HEATER_OUT_GPIO configured by PWM init
	gpio_reset_pin(SIGNAL_LED_OUT_GPIO);
	gpio_set_direction(SIGNAL_LED_OUT_GPIO, GPIO_MODE_OUTPUT);
	gpio_set_level(SIGNAL_LED_OUT_GPIO, 0);
	gpio_set_pull_mode(SOIL_TEMP_IN_GPIO, GPIO_PULLUP_ENABLE);
	gpio_set_pull_mode(SOIL_MOISTURE_IN_GPIO, GPIO_PULLUP_ENABLE);
	// May affect sensor | disable if readings are too noisy
}

void pwm_init() {
	pwm_timers_init();
	pwm_channels_init();
}

void pwm_timers_init(void) {
	ledc_timer_config_t led_timer = {.speed_mode = LEDC_LOW_SPEED_MODE,
									 .timer_num = LEDC_TIMER_0,
									 .duty_resolution = LEDC_TIMER_10_BIT,
									 .freq_hz = PWM_LED_FREQ_HZ,
									 .clk_cfg = LEDC_AUTO_CLK};
	ledc_timer_config_t fan_timer = {.speed_mode = LEDC_LOW_SPEED_MODE,
									 .timer_num = LEDC_TIMER_1,
									 .duty_resolution = LEDC_TIMER_10_BIT,
									 .freq_hz = PWM_FAN_FREQ_HZ,
									 .clk_cfg = LEDC_AUTO_CLK};
	ledc_timer_config_t heater_timer = {.speed_mode = LEDC_LOW_SPEED_MODE,
										.timer_num = LEDC_TIMER_2,
										.duty_resolution = LEDC_TIMER_10_BIT,
										.freq_hz = PWM_HEAT_FREQ_HZ,
										.clk_cfg = LEDC_AUTO_CLK};
	ESP_ERROR_CHECK(ledc_timer_config(&led_timer));
	ESP_ERROR_CHECK(ledc_timer_config(&fan_timer));
	ESP_ERROR_CHECK(ledc_timer_config(&heater_timer));
}

void pwm_channels_init(void) {
	ledc_channel_config_t led_channel = {.gpio_num = LED_OUT_GPIO,
										 .speed_mode = LEDC_LOW_SPEED_MODE,
										 .channel = LEDC_CHANNEL_0,
										 .timer_sel = LEDC_TIMER_0,
										 .duty = 0,
										 .hpoint = 0};
	ledc_channel_config_t fan_channel = {.gpio_num = FAN_OUT_GPIO,
										 .speed_mode = LEDC_LOW_SPEED_MODE,
										 .channel = LEDC_CHANNEL_1,
										 .timer_sel = LEDC_TIMER_1,
										 .duty = 0,
										 .hpoint = 0};
	ledc_channel_config_t heater_channel = {.gpio_num = HEATER_OUT_GPIO,
											.speed_mode = LEDC_LOW_SPEED_MODE,
											.channel = LEDC_CHANNEL_2,
											.timer_sel = LEDC_TIMER_2,
											.duty = 0,
											.hpoint = 0};
	ESP_ERROR_CHECK(ledc_channel_config(&led_channel));
	ESP_ERROR_CHECK(ledc_channel_config(&fan_channel));
	ESP_ERROR_CHECK(ledc_channel_config(&heater_channel));
}

void pwm_set(ledc_channel_t channel, uint32_t duty) {
	ledc_set_duty(LEDC_LOW_SPEED_MODE, channel, duty);
	ledc_update_duty(LEDC_LOW_SPEED_MODE, channel);
}

// =============================================================================
// STATE UPDATE FUNCTIONS
// =============================================================================

time_state_t update_time_state(struct tm current_time) {
	// NIGHT 	runs from 23:00-6:00
	// MORNING 	runs from 6:00-8:00
	// MIDDAY 	runs from 8:00-17:00
	// EVENING 	runs from 17:00-23:00
	if ((current_time.tm_hour >= 6) && (current_time.tm_hour < 8)) {
		return MORNING;
	} else if ((current_time.tm_hour >= 8) && (current_time.tm_hour < 17)) {
		return MIDDAY;
	} else if ((current_time.tm_hour >= 17) && (current_time.tm_hour < 23)) {
		return EVENING;
	} else {
		return NIGHT;
	}
}

fan_state_t update_fan_state(float temp_from_probe_c,
							 float humid_from_probe_percent,
							 fan_state_t current_fan_state) {
	// const float temp_min = IDEAL_AIR_TEMP_C - AIR_TEMP_RANGE_C; // Included
	// for posterity / possible future use
	const float temp_med = IDEAL_AIR_TEMP_C + AIR_TEMP_RANGE_C;
	const float temp_high = IDEAL_AIR_TEMP_C + (AIR_TEMP_RANGE_C * 2.0f);
	const float temp_max = IDEAL_AIR_TEMP_C + (AIR_TEMP_RANGE_C * 3.0f);

	// const float humid_min = IDEAL_AIR_HUMIDITY_PERCENT -
	// AIR_HUMIDITY_RANGE_PERCENT; // Included for posterity / possible future
	// use
	const float humid_med =
		IDEAL_AIR_HUMIDITY_PERCENT + AIR_HUMIDITY_RANGE_PERCENT;
	const float humid_high =
		IDEAL_AIR_HUMIDITY_PERCENT + (AIR_HUMIDITY_RANGE_PERCENT * 2.0f);
	const float humid_max =
		IDEAL_AIR_HUMIDITY_PERCENT + (AIR_HUMIDITY_RANGE_PERCENT * 3.0f);

	// catch condition for likely impossible readings
	// 0C indoors unlikely but possible 40C indoors unlikely but possible |
	// Ignore values outside this range / Default to OFF
	if ((temp_from_probe_c < 0) || (temp_from_probe_c > 40)) {
		return FAN_OFF;
	}
	// Percentage based RH sensing means we can cap at 0% and 100% with relative
	// safety | Ignore values outside this range / Default to OFF
	if ((humid_from_probe_percent < 0) || (humid_from_probe_percent > 100)) {
		return FAN_OFF;
	}

	switch (current_fan_state) {
	case FAN_OFF:
		// OFF -> ON (LOW)
		if ((temp_from_probe_c > (temp_med + 1.0f)) ||
			(humid_from_probe_percent > (humid_med + 5.0f))) {
			return FAN_LOW;
		}
		// OFF -> OFF
		else {
			return current_fan_state;
		}
	case FAN_LOW:
		//  ON (LOW) -> ON (MED)
		if ((temp_from_probe_c > (temp_high + 1.0f)) ||
			(humid_from_probe_percent > (humid_high + 5.0f))) {
			return FAN_MED;
		}
		//  ON (LOW) -> OFF
		if ((temp_from_probe_c < (temp_med - 1.0f)) &&
			(humid_from_probe_percent < (humid_med - 5.0f))) {
			return FAN_OFF;
		}
		//  ON (LOW) -> ON (LOW)
		else {
			return current_fan_state;
		}
	case FAN_MED:
		//  ON (MED) -> ON (HIGH)
		if ((temp_from_probe_c > (temp_max + 1.0f)) ||
			(humid_from_probe_percent > (humid_max + 5.0f))) {
			return FAN_HIGH;
		}
		//  ON (MED) -> ON (LOW)
		if ((temp_from_probe_c < (temp_high - 1.0f)) &&
			(humid_from_probe_percent < (humid_high - 5.0f))) {
			return FAN_LOW;
		}
		//  ON (MED) -> ON (MED)
		else {
			return current_fan_state;
		}
	case FAN_HIGH:
		//  ON (HIGH) -> ON (MED)
		if ((temp_from_probe_c < (temp_high - 1.0f)) &&
			(humid_from_probe_percent < (humid_high - 5.0f))) {
			return FAN_MED;
		}
		//  ON (HIGH) -> ON (HIGH)
		else {
			return current_fan_state;
		}
	default:
		// DEFAULT OFF
		return FAN_OFF;
	}
}

led_state_t update_led_state(time_state_t current_time,
							 float temp_from_probe_c) {
	// Catch condition for likely impossible readings
	// 0C indoors unlikely but possible 40C indoors unlikely but possible |
	// Ignore values outside this range / Default to OFF
	if ((temp_from_probe_c < 0) || (temp_from_probe_c > 40)) {
		return LED_OFF;
	}
	// Minor safety check for (likely minimal) heat addition from LEDs
	if (temp_from_probe_c > 35.0f) {
		return LED_OFF;
	}

	switch (current_time) {
	case MORNING:
		return LED_MED;
	case MIDDAY:
		return LED_HIGH;
	case EVENING:
		return LED_LOW;
	case NIGHT:
		// fall through
	default:
		return LED_OFF;
	}
}

heater_state_t update_heater_state(float temp_from_probe_c,
								   heater_state_t current_heater_state) {
	const float temp_max = IDEAL_SOIL_TEMP_C + SOIL_TEMP_RANGE_C;
	const float temp_low = IDEAL_SOIL_TEMP_C - SOIL_TEMP_RANGE_C;
	const float temp_med = IDEAL_SOIL_TEMP_C - (SOIL_TEMP_RANGE_C * 2.0f);
	const float temp_min = IDEAL_SOIL_TEMP_C - (SOIL_TEMP_RANGE_C * 3.0f);

	// Catch condition for likely impossible readings
	// 0C indoors unlikely but possible 40C indoors unlikely but possible |
	// Ignore values outside this range / Default to OFF
	if ((temp_from_probe_c < 0) || (temp_from_probe_c > 40)) {
		return HEAT_OFF;
	}

	if (temp_from_probe_c > temp_max) {
		return HEAT_OFF;
	}

	switch (current_heater_state) {
	case HEAT_OFF:
		// OFF -> ON (LOW)
		if ((temp_from_probe_c < (temp_low - 1.0f))) {
			return HEAT_LOW;
		}
		// OFF -> OFF
		else {
			return current_heater_state;
		}
	case HEAT_LOW:
		//  ON (LOW) -> ON (MED)
		if ((temp_from_probe_c < (temp_med - 1.0f))) {
			return HEAT_MED;
		}
		//  ON (LOW) -> OFF
		if ((temp_from_probe_c > (temp_low + 1.0f))) {
			return HEAT_OFF;
		}
		//  ON (LOW) -> ON (LOW)
		else {
			return current_heater_state;
		}
	case HEAT_MED:
		//  ON (MED) -> ON (HIGH)
		if ((temp_from_probe_c < (temp_min - 1.0f))) {
			return HEAT_HIGH;
		}
		//  ON (MED) -> ON (LOW)
		if ((temp_from_probe_c > (temp_med + 1.0f))) {
			return HEAT_LOW;
		}
		//  ON (MED) -> ON (MED)
		else {
			return current_heater_state;
		}
	case HEAT_HIGH:
		//  ON (HIGH) -> ON (MED)
		if ((temp_from_probe_c > (temp_med + 1.0f))) {
			return HEAT_MED;
		}
		//  ON (HIGH) -> ON (HIGH)
		else {
			return current_heater_state;
		}
	default:
		// DEFAULT OFF
		return HEAT_OFF;
	}
}

signal_led_state_t update_soil_moisture_led_state(
	signal_led_state_t current_soil_moisture_led_state,
	float soil_moisture_from_probe_percent) {

	float soil_moisture_min =
		IDEAL_SOIL_MOISTURE_PERCENT - SOIL_MOISTURE_RANGE_PERCENT;
	float soil_moisture_max =
		IDEAL_SOIL_MOISTURE_PERCENT + SOIL_MOISTURE_RANGE_PERCENT;

	// Catch condition for likely impossible readings
	// Percentage based moisture sensing means we can cap at 0% and 100% with
	// relative safety | Ignore values outside this range / Default to OFF
	if ((soil_moisture_from_probe_percent < 0) ||
		(soil_moisture_from_probe_percent > 100)) {
		return SIG_LED_OFF;
	}

	switch (current_soil_moisture_led_state) {
	case SIG_LED_OFF:
		// OFF -> ON
		if (soil_moisture_from_probe_percent < (soil_moisture_min - 1)) {
			return SIG_LED_ON;
		}
		// OFF -> OFF
		else {
			return current_soil_moisture_led_state;
		}
	case SIG_LED_ON:
		// ON -> OFF
		if (soil_moisture_from_probe_percent > (soil_moisture_max + 1)) {
			return SIG_LED_OFF;
		}
		// ON -> ON
		else {
			return current_soil_moisture_led_state;
		}
	default:
		return SIG_LED_OFF;
	}
}
// =============================================================================
// RUN ACTUATOR FUNCTIONS
// =============================================================================

void run_fan(fan_state_t current_fan_state) {
	// FAN USES LEDC_CHANNEL_1
	switch (current_fan_state) {
	case FAN_HIGH:
		pwm_set(LEDC_CHANNEL_1, PWM_DUTY_HIGH);
		break;
	case FAN_MED:
		pwm_set(LEDC_CHANNEL_1, PWM_DUTY_MED);
		break;
	case FAN_LOW:
		pwm_set(LEDC_CHANNEL_1, PWM_DUTY_FAN_MIN);
		break;
	case FAN_OFF:
		// fall through
	default:
		pwm_set(LEDC_CHANNEL_1, PWM_DUTY_OFF);
		break;
	}
}
void run_led(led_state_t current_led_state) {
	// LED USES LEDC_CHANNEL_0
	switch (current_led_state) {
	case LED_HIGH:
		pwm_set(LEDC_CHANNEL_0, PWM_DUTY_HIGH);
		break;
	case LED_MED:
		pwm_set(LEDC_CHANNEL_0, PWM_DUTY_MED);
		break;
	case LED_LOW:
		pwm_set(LEDC_CHANNEL_0, PWM_DUTY_LOW);
		break;
	case LED_OFF:
		// fall through
	default:
		pwm_set(LEDC_CHANNEL_0, PWM_DUTY_OFF);
		break;
	}
}
void run_signal_led(signal_led_state_t current_soil_moisture_led_state) {
	// NO PWM | PURE GPIO ON-OFF
	switch (current_soil_moisture_led_state) {
	case SIG_LED_ON:
		gpio_set_level(SIGNAL_LED_OUT_GPIO, 1);
		break;
	case SIG_LED_OFF:
	default:
		gpio_set_level(SIGNAL_LED_OUT_GPIO, 0);
		break;
	}
}
void run_heater(heater_state_t current_heater_state) {
	// HEATER USES LEDC_CHANNEL_2
	switch (current_heater_state) {
	case HEAT_HIGH:
		pwm_set(LEDC_CHANNEL_2, PWM_DUTY_HIGH);
		break;
	case HEAT_MED:
		pwm_set(LEDC_CHANNEL_2, PWM_DUTY_MED);
		break;
	case HEAT_LOW:
		pwm_set(LEDC_CHANNEL_2, PWM_DUTY_LOW);
		break;
	case HEAT_OFF:
		// fall through
	default:
		pwm_set(LEDC_CHANNEL_2, PWM_DUTY_OFF);
		break;
	}
}

// =============================================================================

void app_main(void) {

	esp_log_level_set("*", ESP_LOG_INFO);
	esp_log_level_set("SENSORS", ESP_LOG_INFO);
	esp_log_level_set("STATES", ESP_LOG_INFO);

	// GPIO INITIALIZATION =====================================================
	gpio_init();
	// PWM INITIALIZATION ======================================================
	pwm_init();
	// ADC INITIALIZATION ======================================================
	adc_oneshot_unit_handle_t adc_handle;
	adc_oneshot_unit_init_cfg_t unit_cfg = {
		.unit_id = ADC_UNIT_1,
		.clk_src = ADC_DIGI_CLK_SRC_DEFAULT,
		.ulp_mode = ADC_ULP_MODE_DISABLE,
	};
	ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_cfg, &adc_handle));
	adc_oneshot_chan_cfg_t chan_cfg = {
		.bitwidth = ADC_BITWIDTH_DEFAULT,
		.atten = ADC_ATTEN_DB_12, // ADC_ATTEN_DB_11 DEPRECIATED, DOCS SAY 12
								  // BEHAVES THE SAME
	};
	ESP_ERROR_CHECK(
		adc_oneshot_config_channel(adc_handle, ADC_CHANNEL_0, &chan_cfg));
     // I2C INITIALIZATION ========================================
	ESP_ERROR_CHECK(i2cdev_init());
	sht3x_t sht31_dev = {0};
	sht31_dev.i2c_dev.port = I2C_PORT;
	sht31_dev.i2c_dev.addr = SHT31_ADDR;
	sht31_dev.i2c_dev.cfg.sda_io_num = I2C_SDA_GPIO;
	sht31_dev.i2c_dev.cfg.scl_io_num = I2C_SCL_GPIO;
	sht31_dev.i2c_dev.cfg.master.clk_speed = 100000;
	//				I2C_SCL_GPIO);
	i2c_dev_t rtc_clock = {
    .port = I2C_PORT,
    .addr = DS1307_ADDR,
    .cfg = {
        .sda_io_num = I2C_SDA_GPIO,
        .scl_io_num = I2C_SCL_GPIO,
        .master.clk_speed = 100000
    }
};
ESP_ERROR_CHECK(i2c_dev_create_mutex(&sht31_dev.i2c_dev));
ESP_ERROR_CHECK(i2c_dev_create_mutex(&rtc_clock));
ESP_ERROR_CHECK(sht3x_init(&sht31_dev));
sht31_dev.mode = SHT3X_PERIODIC_1MPS;
sht31_dev.mode = SHT3X_HIGH;

	//ds1307_init_desc(&rtc_clock, I2C_PORT, I2C_SDA_GPIO, I2C_SCL_GPIO);
					
	// ONEWIRE / DS18B20 INITIALIZATION ========================================
	onewire_addr_t addr;
	size_t found = 0;
	esp_err_t err = ds18x20_scan_devices(SOIL_TEMP_IN_GPIO, &addr, 1, &found);
	if (err != ESP_OK || found == 0) {
		ESP_LOGE("DS18B20", "No DS18B20 devices found (%s)",
				 esp_err_to_name(err));
		return;
	}
	// STATE INITIALIZATION | DEFAULT OFF
	// ========================================

	fan_state_t current_fan_state = FAN_OFF;
	led_state_t current_led_state = LED_OFF;
	heater_state_t current_heater_state = HEAT_OFF;
	signal_led_state_t current_signal_led_state = SIG_LED_OFF;
	time_state_t current_time_state = NIGHT;
	// VARIABLE INITIALIZATION | DEFAULT 0.0
	// =====================================
	float current_soil_moisture_percent = 0.0f;
	float current_soil_temperature_c = 0.0f;
	float current_air_humidity_percent = 0.0f;
	float current_air_temperature_c = 0.0f;
	struct tm current_time;
	struct tm new_time;
	
	//============================================================================

	
	while (1) {
		// =======================================================================
		// SENSOR READS 
		// =======================================================================
		
		current_soil_moisture_percent = read_soil_moisture_percent(adc_handle);

		err = ds18b20_measure_and_read(SOIL_TEMP_IN_GPIO, addr,
									   &current_soil_temperature_c);
		if (err != ESP_OK) {
			current_soil_temperature_c = -1.0f;
		}

		if (sht3x_measure(&sht31_dev, &current_air_temperature_c,
						  &current_air_humidity_percent) != ESP_OK) {
			current_air_temperature_c = -1.0f;
			current_air_humidity_percent = -1.0f;
		}
		if (ds1307_get_time(&rtc_clock, &new_time) == ESP_OK) {
			current_time = new_time;
		}
		vTaskDelay(pdMS_TO_TICKS(500));
		static const char *TAG = "SENSORS";
		ESP_LOGI(TAG,
				 "SENSORS: temp=%.2fC humidity=%.2f%% soil=%.2fC soil "
				 "moisture=%.2f%% H%d:M%d:S%d",
				 current_air_temperature_c, current_air_humidity_percent,
				 current_soil_temperature_c, current_soil_moisture_percent,
				 current_time.tm_hour, current_time.tm_min,
				 current_time.tm_sec);
		// =======================================================================
		// STATE UPDATE 
		// =======================================================================
		current_time_state = update_time_state(current_time);
		current_fan_state =
			update_fan_state(current_air_temperature_c,
							 current_air_humidity_percent, current_fan_state);
		current_led_state =
			update_led_state(current_time_state, current_air_temperature_c);
		current_heater_state = update_heater_state(current_soil_temperature_c,
												   current_heater_state);
		current_signal_led_state = update_soil_moisture_led_state(
			current_signal_led_state, current_soil_moisture_percent);
		vTaskDelay(pdMS_TO_TICKS(500));

		static const char *TAG2 = "STATES";
		ESP_LOGI(TAG2, "STATES: LED=%d heater=%d fan=%d signalled=%d",
				 current_led_state, current_heater_state, current_fan_state,
				 current_signal_led_state

		);
		// =======================================================================
		// RUN FUNCTIONS 
		// =======================================================================
		run_fan(current_fan_state);
		run_led(current_led_state);
		run_heater(current_heater_state);
		run_signal_led(current_signal_led_state);
		vTaskDelay(pdMS_TO_TICKS(4000));
	}
}
