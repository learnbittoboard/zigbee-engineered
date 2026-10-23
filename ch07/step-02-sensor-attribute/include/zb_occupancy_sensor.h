/*
 * Chapter 7 — the Occupancy Sensor device declaration.
 *
 * ZBOSS (as shipped in the Zigbee R23 add-on) defines the Occupancy
 * Sensing CLUSTER (zb_zcl_occupancy_sensing.h) but no ready-made
 * Occupancy Sensor DEVICE declaration — so, exactly as Chapter 6
 * vendored the range extender's declaration, the course declares
 * its own device from the stack's cluster macros. Nothing here is
 * invented API: every macro below is the same ZB_ZCL_CLUSTER_DESC /
 * ZB_AF_DECLARE_ENDPOINT_DESC machinery the add-on's own device
 * headers (zb_ha_dimmable_light.h, zb_range_extender.h) use.
 *
 * The device: an HA-profile Occupancy Sensor, device ID 0x0107
 * (ZCL device table, Lighting & Occupancy category). One endpoint,
 * three server clusters:
 *
 *   Basic (0x0000)              — identity, as always
 *   Identify (0x0003)           — "which desk is it?" blinking
 *   Occupancy Sensing (0x0406)  — the product: Chapter 7's subject
 *
 * Chapter 8 Step 1 carries a copy of this header whose only change
 * is a reporting context (ZB_OCCUPANCY_SENSOR_REPORT_ATTR_COUNT 1);
 * Chapter 8 Step 2's copy adds the On/Off CLIENT cluster. Watch the
 * header grow; the endpoint's story is the course's story.
 */

#ifndef ZB_OCCUPANCY_SENSOR_H
#define ZB_OCCUPANCY_SENSOR_H 1

/**
 *  @defgroup ZB_DEFINE_DEVICE_OCCUPANCY_SENSOR Occupancy Sensor
 *  @{
 *  @details
 *      - @ref ZB_ZCL_BASIC \n
 *      - @ref ZB_ZCL_IDENTIFY \n
 *      - @ref ZB_ZCL_OCCUPANCY_SENSING
 */

/** Occupancy Sensor Device ID (HA profile, ZCL device table) */
#define ZB_OCCUPANCY_SENSOR_DEVICE_ID 0x0107

/** Occupancy sensor device version */
#define ZB_DEVICE_VER_OCCUPANCY_SENSOR 0

/** @cond internals_doc */

/** Occupancy sensor IN (server) clusters number */
#define ZB_OCCUPANCY_SENSOR_IN_CLUSTER_NUM 3

/** Occupancy sensor OUT (client) clusters number */
#define ZB_OCCUPANCY_SENSOR_OUT_CLUSTER_NUM 0

#define ZB_OCCUPANCY_SENSOR_CLUSTER_NUM \
	(ZB_OCCUPANCY_SENSOR_IN_CLUSTER_NUM + ZB_OCCUPANCY_SENSOR_OUT_CLUSTER_NUM)

/** Number of attributes for reporting on Occupancy sensor device.
 * Chapter 7 does not report yet (Chapter 8, Step 1 turns this on).
 */
#define ZB_OCCUPANCY_SENSOR_REPORT_ATTR_COUNT 0

/** @endcond */ /* internals_doc */

/**
 * @brief Declare cluster list for Occupancy sensor device
 * @param cluster_list_name - cluster list variable name
 * @param basic_attr_list - attribute list for Basic cluster
 * @param identify_attr_list - attribute list for Identify cluster
 * @param occupancy_attr_list - attribute list for Occupancy Sensing cluster
 */
#define ZB_DECLARE_OCCUPANCY_SENSOR_CLUSTER_LIST(			      \
		cluster_list_name,					      \
		basic_attr_list,					      \
		identify_attr_list,					      \
		occupancy_attr_list)					      \
zb_zcl_cluster_desc_t cluster_list_name[] =				      \
{									      \
	ZB_ZCL_CLUSTER_DESC(						      \
		ZB_ZCL_CLUSTER_ID_BASIC,				      \
		ZB_ZCL_ARRAY_SIZE(basic_attr_list, zb_zcl_attr_t),	      \
		(basic_attr_list),					      \
		ZB_ZCL_CLUSTER_SERVER_ROLE,				      \
		ZB_ZCL_MANUF_CODE_INVALID				      \
	),								      \
	ZB_ZCL_CLUSTER_DESC(						      \
		ZB_ZCL_CLUSTER_ID_IDENTIFY,				      \
		ZB_ZCL_ARRAY_SIZE(identify_attr_list, zb_zcl_attr_t),	      \
		(identify_attr_list),					      \
		ZB_ZCL_CLUSTER_SERVER_ROLE,				      \
		ZB_ZCL_MANUF_CODE_INVALID				      \
	),								      \
	ZB_ZCL_CLUSTER_DESC(						      \
		ZB_ZCL_CLUSTER_ID_OCCUPANCY_SENSING,			      \
		ZB_ZCL_ARRAY_SIZE(occupancy_attr_list, zb_zcl_attr_t),	      \
		(occupancy_attr_list),					      \
		ZB_ZCL_CLUSTER_SERVER_ROLE,				      \
		ZB_ZCL_MANUF_CODE_INVALID				      \
	)								      \
}

/** @cond internals_doc */

/**
 * @brief Declare simple descriptor for Occupancy sensor device
 * @param ep_name - endpoint variable name
 * @param ep_id - endpoint ID
 * @param in_clust_num - number of supported input clusters
 * @param out_clust_num - number of supported output clusters
 */
#define ZB_ZCL_DECLARE_OCCUPANCY_SENSOR_SIMPLE_DESC(ep_name, ep_id, in_clust_num, out_clust_num) \
	ZB_DECLARE_SIMPLE_DESC(in_clust_num, out_clust_num);				       \
	ZB_AF_SIMPLE_DESC_TYPE(in_clust_num, out_clust_num) simple_desc_##ep_name =	       \
	{										       \
		ep_id,									       \
		ZB_AF_HA_PROFILE_ID,							       \
		ZB_OCCUPANCY_SENSOR_DEVICE_ID,						       \
		ZB_DEVICE_VER_OCCUPANCY_SENSOR,						       \
		0,									       \
		in_clust_num,								       \
		out_clust_num,								       \
		{									       \
			ZB_ZCL_CLUSTER_ID_BASIC,					       \
			ZB_ZCL_CLUSTER_ID_IDENTIFY,					       \
			ZB_ZCL_CLUSTER_ID_OCCUPANCY_SENSING				       \
		}									       \
	}

/** @endcond */

/**
 * @brief Declare endpoint for Occupancy sensor device
 * @param ep_name - endpoint variable name
 * @param ep_id - endpoint ID
 * @param cluster_list - endpoint cluster list
 */
#define ZB_DECLARE_OCCUPANCY_SENSOR_EP(ep_name, ep_id, cluster_list)			      \
	ZB_ZCL_DECLARE_OCCUPANCY_SENSOR_SIMPLE_DESC(ep_name, ep_id,			      \
		ZB_OCCUPANCY_SENSOR_IN_CLUSTER_NUM, ZB_OCCUPANCY_SENSOR_OUT_CLUSTER_NUM);     \
	ZB_AF_DECLARE_ENDPOINT_DESC(ep_name, ep_id, ZB_AF_HA_PROFILE_ID, 0, NULL,	      \
		ZB_ZCL_ARRAY_SIZE(cluster_list, zb_zcl_cluster_desc_t), cluster_list,	      \
			(zb_af_simple_desc_1_1_t *)&simple_desc_##ep_name,		      \
			0, NULL, /* No reporting ctx — Chapter 8, Step 1 */		      \
			0, NULL) /* No CVC ctx */

/*! @} */

#endif /* ZB_OCCUPANCY_SENSOR_H */
