/*
 * Chapter 8, Step 1 — the Occupancy Sensor device declaration,
 * reporting edition.
 *
 * This is Chapter 7's header with exactly one change, and the
 * change is the chapter: the endpoint declaration now carries a
 * REPORTING CONTEXT — a fixed-size table (ZBOSS_DEVICE_DECLARE_
 * REPORTING_CTX) in which the stack records who asked to be told
 * about which attribute, and how often.
 *
 * Why a table has to exist at all: reporting configuration
 * arrives over the air (a coordinator's Configure Reporting
 * command) and must live somewhere typed, bounded and persistent
 * across the stack's scheduling. ZBOSS allocates that storage at
 * compile time from ZB_OCCUPANCY_SENSOR_REPORT_ATTR_COUNT — here
 * 1, because the Occupancy Sensing declare macro marks exactly
 * one attribute reportable:
 * ZB_ZCL_OCCUPANCY_SENSING_REPORT_ATTR_COUNT is 1 (Occupancy;
 * see zb_zcl_occupancy_sensing.h — the attribute is declared
 * ZB_ZCL_ATTR_ACCESS_REPORTING, its neighbours are not).
 *
 * Without this context the endpoint still builds and reads fine,
 * and every Configure Reporting command is answered "unsupported"
 * — the classic Chapter 8 bug: a silent device, correctly wired.
 * Chapter 8, Step 2's copy of this header adds the On/Off client
 * cluster; the reporting context carries over untouched.
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
 * One: the Occupancy attribute is the cluster's only reportable
 * attribute (ZB_ZCL_OCCUPANCY_SENSING_REPORT_ATTR_COUNT).
 */
#define ZB_OCCUPANCY_SENSOR_REPORT_ATTR_COUNT \
	ZB_ZCL_OCCUPANCY_SENSING_REPORT_ATTR_COUNT

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
	ZBOSS_DEVICE_DECLARE_REPORTING_CTX(reporting_info## ep_name,			      \
		ZB_OCCUPANCY_SENSOR_REPORT_ATTR_COUNT);				      \
	ZB_AF_DECLARE_ENDPOINT_DESC(ep_name, ep_id, ZB_AF_HA_PROFILE_ID, 0, NULL,	      \
		ZB_ZCL_ARRAY_SIZE(cluster_list, zb_zcl_cluster_desc_t), cluster_list,	      \
			(zb_af_simple_desc_1_1_t *)&simple_desc_##ep_name,		      \
			ZB_OCCUPANCY_SENSOR_REPORT_ATTR_COUNT,				      \
				reporting_info## ep_name,				      \
			0, NULL) /* No CVC ctx */

/*! @} */

#endif /* ZB_OCCUPANCY_SENSOR_H */
