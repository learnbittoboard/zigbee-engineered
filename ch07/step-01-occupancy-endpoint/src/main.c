/*
 * Chapter 7, Step 1 — the Occupancy Sensing endpoint, declared.
 *
 * The Chapter 6 skeleton joins a network and answers to Basic and
 * Identify. This step gives the endpoint its product: the ZCL
 * Occupancy Sensing cluster (0x0406), server side, declared the
 * ZBOSS way — an attribute list, a cluster list, an endpoint —
 * with its storage in our device context struct.
 *
 * There is deliberately NO sensor in this step yet. The Occupancy
 * attribute is flipped by DK Button 1 ("simulated occupancy"), so
 * the Zigbee side can be proven end-to-end from the coordinator
 * shell — reads, descriptor, the works — before any UART wiring
 * is involved. Chapter 7, Step 2 swaps the thumb for Chapter 4's
 * LD2410 driver and changes nothing about the endpoint.
 *
 * Startup, steering, identify and factory reset are Chapter 6's,
 * carried over unchanged. That is the skeleton paying rent.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <dk_buttons_and_leds.h>

#include <zboss_api.h>
#include <zigbee/zigbee_error_handler.h>
#include <zigbee/zigbee_app_utils.h>
#include <zb_nrf_platform.h>
#include "zb_occupancy_sensor.h"


/* Device endpoint — the same number Chapter 6 chose. */
#define SENSOR_ENDPOINT                     10

/* Type of power sources available for the device.
 * For possible values see section 3.2.2.2.8 of ZCL specification.
 */
#define SENSOR_INIT_BASIC_POWER_SOURCE      ZB_ZCL_BASIC_POWER_SOURCE_DC_SOURCE

/* LED indicating that device successfully joined Zigbee network. */
#define ZIGBEE_NETWORK_STATE_LED            DK_LED3

/* LED used for device identification. */
#define IDENTIFY_LED                        DK_LED4

/* Button used to enter the Identify mode. */
#define IDENTIFY_MODE_BUTTON                DK_BTN4_MSK

/* A long press of the same button is the factory reset. */
#define FACTORY_RESET_BUTTON                IDENTIFY_MODE_BUTTON

/* Button 1 is this step's "sensor": each press flips the
 * Occupancy attribute between occupied and unoccupied.
 */
#define SIMULATED_OCCUPANCY_BUTTON          DK_BTN1_MSK

LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

/* Application context: the attribute storage behind the endpoint's
 * clusters. ZBOSS reads and writes these structs; the application
 * owns them. Every attribute the endpoint exposes has a field
 * here — the cluster declarations below point INTO this struct.
 */
struct zb_device_ctx {
	zb_zcl_basic_attrs_t basic_attr;
	zb_zcl_identify_attrs_t identify_attr;
	/* Occupancy Sensing attribute storage. The stack's declare
	 * macro (ZB_ZCL_DECLARE_OCCUPANCY_SENSING_ATTRIB_LIST) takes
	 * one pointer per attribute, which is why these are three
	 * plain fields rather than a nested struct.
	 */
	zb_uint8_t occupancy;
	zb_uint8_t occupancy_sensor_type;
	zb_uint8_t occupancy_sensor_type_bitmap;
};

/* Zigbee device application context storage. */
static struct zb_device_ctx dev_ctx;

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(
	identify_attr_list,
	&dev_ctx.identify_attr.identify_time);

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(
	basic_attr_list,
	&dev_ctx.basic_attr.zcl_version,
	&dev_ctx.basic_attr.power_source);

ZB_ZCL_DECLARE_OCCUPANCY_SENSING_ATTRIB_LIST(
	occupancy_attr_list,
	&dev_ctx.occupancy,
	&dev_ctx.occupancy_sensor_type,
	&dev_ctx.occupancy_sensor_type_bitmap);

/* The endpoint declaration — our own device type this time
 * (include/zb_occupancy_sensor.h): Occupancy Sensor, 0x0107.
 */
ZB_DECLARE_OCCUPANCY_SENSOR_CLUSTER_LIST(
	sensor_clusters,
	basic_attr_list,
	identify_attr_list,
	occupancy_attr_list);

ZB_DECLARE_OCCUPANCY_SENSOR_EP(
	sensor_ep,
	SENSOR_ENDPOINT,
	sensor_clusters);

ZBOSS_DECLARE_DEVICE_CTX_1_EP(
	sensor_ctx,
	sensor_ep);


/**@brief Function for initializing all clusters attributes. */
static void app_clusters_attr_init(void)
{
	/* Basic cluster attributes data */
	dev_ctx.basic_attr.zcl_version = ZB_ZCL_VERSION;
	dev_ctx.basic_attr.power_source = SENSOR_INIT_BASIC_POWER_SOURCE;

	/* Identify cluster attributes data. */
	dev_ctx.identify_attr.identify_time =
		ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE;

	/* Occupancy Sensing attributes.
	 *
	 * Occupancy (0x0000) is a bitmap8; bit 0 is the whole story
	 * for a single-sensor device: 1 = someone is there.
	 *
	 * OccupancySensorType (0x0001): the ZCL enum (spec
	 * 4.8.2.2.1.2) knows four values — PIR (0), Ultrasonic (1),
	 * PIR+Ultrasonic (2), Physical contact (3). There is NO
	 * radar value: the enum predates consumer mmWave sensors.
	 * The LD2410 is an ACTIVE wave sensor — it transmits and
	 * reads reflections, which is the ultrasonic family's
	 * mechanism, and the exact opposite of a passive PIR.
	 * ULTRASONIC is therefore the spec-correct value to report:
	 * it tells the network the truth about how this device
	 * senses, in the only vocabulary the spec offers. The
	 * bitmap attribute (0x0002) says the same thing as a flag.
	 */
	dev_ctx.occupancy =
		ZB_ZCL_OCCUPANCY_SENSING_OCCUPANCY_UNOCCUPIED;
	dev_ctx.occupancy_sensor_type =
		ZB_ZCL_OCCUPANCY_SENSING_OCCUPANCY_SENSOR_TYPE_ULTRASONIC;
	dev_ctx.occupancy_sensor_type_bitmap =
		ZB_ZCL_OCCUPANCY_SENSING_OCCUPANCY_SENSOR_TYPE_BITMAP_ULTRASONIC;
}

/**@brief Publish a new Occupancy value through the stack.
 *
 * Runs in ZBOSS scheduler context (scheduled by the button
 * handler), which is where attribute writes belong:
 * ZB_ZCL_SET_ATTRIBUTE stores the value AND wakes the stack's
 * reporting machinery for attributes declared reportable —
 * Chapter 8 turns that machinery on.
 *
 * @param  bufid  The new occupancy value (0 or 1), riding the
 *                scheduler's parameter slot.
 */
static void publish_occupancy(zb_bufid_t bufid)
{
	zb_uint8_t occupied = (zb_uint8_t)bufid;

	ZB_ZCL_SET_ATTRIBUTE(
		SENSOR_ENDPOINT,
		ZB_ZCL_CLUSTER_ID_OCCUPANCY_SENSING,
		ZB_ZCL_CLUSTER_SERVER_ROLE,
		ZB_ZCL_ATTR_OCCUPANCY_SENSING_OCCUPANCY_ID,
		&occupied,
		ZB_FALSE);

	LOG_INF("Occupancy attribute -> %s",
		occupied ? "OCCUPIED" : "UNOCCUPIED");
}

/**@brief Function to toggle the identify LED
 *
 * @param  bufid  Unused parameter, required by ZBOSS scheduler API.
 */
static void toggle_identify_led(zb_bufid_t bufid)
{
	static int blink_status;

	dk_set_led(IDENTIFY_LED, (++blink_status) % 2);
	ZB_SCHEDULE_APP_ALARM(toggle_identify_led, bufid, ZB_MILLISECONDS_TO_BEACON_INTERVAL(100));
}

/**@brief Function to handle identify notification events on the endpoint.
 *
 * @param  bufid  Unused parameter, required by ZBOSS scheduler API.
 */
static void identify_cb(zb_bufid_t bufid)
{
	zb_ret_t zb_err_code;

	if (bufid) {
		/* Schedule a self-scheduling function that will toggle the LED */
		ZB_SCHEDULE_APP_CALLBACK(toggle_identify_led, bufid);
	} else {
		/* Cancel the toggling function alarm and turn off LED */
		zb_err_code = ZB_SCHEDULE_APP_ALARM_CANCEL(toggle_identify_led, ZB_ALARM_ANY_PARAM);
		ZVUNUSED(zb_err_code);

		dk_set_led(IDENTIFY_LED, 0);
	}
}

/**@brief Starts identifying the device.
 *
 * @param  bufid  Unused parameter, required by ZBOSS scheduler API.
 */
static void start_identifying(zb_bufid_t bufid)
{
	ZVUNUSED(bufid);

	if (ZB_JOINED()) {
		/* Check if endpoint is in identifying mode,
		 * if not put desired endpoint in identifying mode.
		 */
		if (dev_ctx.identify_attr.identify_time ==
		ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE) {

			zb_ret_t zb_err_code = zb_bdb_finding_binding_target(
				SENSOR_ENDPOINT);

			if (zb_err_code == RET_OK) {
				LOG_INF("Enter identify mode");
			} else if (zb_err_code == RET_INVALID_STATE) {
				LOG_WRN("RET_INVALID_STATE - Cannot enter identify mode");
			} else {
				ZB_ERROR_CHECK(zb_err_code);
			}
		} else {
			LOG_INF("Cancel identify mode");
			zb_bdb_finding_binding_target_cancel();
		}
	} else {
		LOG_WRN("Device not in a network - cannot enter identify mode");
	}
}

/**@brief Callback for button events.
 *
 * @param[in]   button_state  Bitmask containing buttons state.
 * @param[in]   has_changed   Bitmask containing buttons
 *                            that have changed their state.
 */
static void button_changed(uint32_t button_state, uint32_t has_changed)
{
	if ((SIMULATED_OCCUPANCY_BUTTON & has_changed) &&
	    (SIMULATED_OCCUPANCY_BUTTON & button_state)) {
		/* Button 1 pressed: flip the simulated occupancy.
		 * The attribute write itself is scheduled into the
		 * ZBOSS context — button callbacks record and defer,
		 * exactly as Chapter 3 taught.
		 */
		static zb_uint8_t simulated;

		simulated = simulated ?
			ZB_ZCL_OCCUPANCY_SENSING_OCCUPANCY_UNOCCUPIED :
			ZB_ZCL_OCCUPANCY_SENSING_OCCUPANCY_OCCUPIED;
		ZB_SCHEDULE_APP_CALLBACK(publish_occupancy, simulated);
	}

	if (IDENTIFY_MODE_BUTTON & has_changed) {
		if (IDENTIFY_MODE_BUTTON & button_state) {
			/* Button changed its state to pressed */
		} else {
			/* Button changed its state to released */
			if (was_factory_reset_done()) {
				/* The long press was for Factory Reset */
				LOG_DBG("After Factory Reset - ignore button release");
			} else   {
				/* Button released before Factory Reset */

				/* Start identification mode */
				ZB_SCHEDULE_APP_CALLBACK(start_identifying, 0);
			}
		}
	}

	check_factory_reset_button(button_state, has_changed);
}

/**@brief Function for initializing LEDs and Buttons. */
static void configure_gpio(void)
{
	int err;

	err = dk_buttons_init(button_changed);
	if (err) {
		LOG_ERR("Cannot init buttons (err: %d)", err);
	}

	err = dk_leds_init();
	if (err) {
		LOG_ERR("Cannot init LEDs (err: %d)", err);
	}
}

/**@brief Zigbee stack event handler.
 *
 * @param[in]   bufid   Reference to the Zigbee stack buffer
 *                      used to pass signal.
 *
 * Unchanged from Chapter 6: steering is the default handler's
 * state machine; this handler keeps the LED honest and delegates.
 */
void zboss_signal_handler(zb_bufid_t bufid)
{
	/* Update network status LED. */
	zigbee_led_status_update(bufid, ZIGBEE_NETWORK_STATE_LED);

	/* No application-specific behavior is required.
	 * Call default signal handler.
	 */
	ZB_ERROR_CHECK(zigbee_default_signal_handler(bufid));

	/* All callbacks should either reuse or free passed buffers.
	 * If bufid == 0, the buffer is invalid (not passed).
	 */
	if (bufid) {
		zb_buf_free(bufid);
	}
}

int main(void)
{
	LOG_INF("Starting Zigbee, Engineered - occupancy endpoint (simulated sensor)");

	/* Initialize */
	configure_gpio();
	register_factory_reset_button(FACTORY_RESET_BUTTON);

	/* Register device context (endpoints). */
	ZB_AF_REGISTER_DEVICE_CTX(&sensor_ctx);

	app_clusters_attr_init();

	/* Register handlers to identify notifications */
	ZB_AF_SET_IDENTIFY_NOTIFICATION_HANDLER(SENSOR_ENDPOINT, identify_cb);

	/* Start Zigbee default thread */
	zigbee_enable();

	LOG_INF("Endpoint 10 live: Basic + Identify + Occupancy Sensing. "
		"Button 1 flips the Occupancy attribute");

	return 0;
}
