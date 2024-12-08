/************************************************************************************************
 * Copyright (c) 2006-2009 Laboratorio di Sistemi di Elaborazione e Bioingegneria Informatica	*
 *                          Universita' Campus BioMedico - Italy								*
 *																								*
 * This program is free software; you can redistribute it and/or modify it under the terms		*
 * of the GNU General Public License as published by the Free Software Foundation; either		*
 * version 2 of the License, or (at your option) any later version.								*
 *																								*
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY				*
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A				*
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.						*
 *																								*
 * You should have received a copy of the GNU General Public License along with this			*
 * program; if not, write to the:																*
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,							*
 * MA  02111-1307, USA.																			*
 *
 * In addition, as a special exception, the copyright holders give permission to link the  *
 * code of portions of this program with the OpenSSL library under certain conditions as   *
 * described in each individual source file, and distribute linked combinations including  *
 * the two. You must obey the GNU General Public License in all respects for all of the    *
 * code used other than OpenSSL.  If you modify file(s) with this exception, you may       *
 * extend this exception to your version of the file(s), but you are not obligated to do   *
 * so.  If you do not wish to do so, delete this exception statement from your version.    *
 * If you delete this exception statement from all source files in the program, then also  *
 * delete it here.                                                                         *
 *
 * -------------------------------------------------------------------------------------------- *
 * Project:  Capwap																				*
 *																								*
 * Authors : Ludovico Rossi (ludo@bluepixysw.com)												*
 *           Del Moro Andrea (andrea_delmoro@libero.it)											*
 *           Giovannini Federica (giovannini.federica@gmail.com)								*
 *           Massimo Vellucci (m.vellucci@unicampus.it)											*
 *           Mauro Bisson (mauro.bis@gmail.com)													*
 *	         Antonio Davoli (antonio.davoli@gmail.com)											*
 ************************************************************************************************/


#ifndef __CAPWAP_CWProtocol_HEADER__
#define __CAPWAP_CWProtocol_HEADER__

/*_________________________________________________________*/
/*  *******************___CONSTANTS___*******************  */

// to be defined
#define	MAX_UDP_PACKET_SIZE					65536
#define	CW_CONTROL_PORT						5246
#define	CW_DATA_PORT						5247
#define	CW_PROTOCOL_VERSION					0
#define	CW_IANA_ENTERPRISE_NUMBER				13277
#define	CW_IANA_ENTERPRISE_NUMBER_VENDOR_IANA			18357
#define	CW_CONTROL_HEADER_OFFSET_FOR_MSG_ELEMS			3		//Offset "Seq Num" - "Message Elements"
#define	CW_MAX_SEQ_NUM						255
#define	CW_MAX_FRAGMENT_ID					65535
/*
 * Elena Agostini - 02/2014
 * DTLS_ENABLED DATA hasn't right value
 */
#define	CLEAR_DATA						2
#define	DTLS_ENABLED_DATA					4
#define	CW_PACKET_PLAIN						0
#define	CW_PACKET_CRYPT						1
#define	CW_DATA_MSG_FRAME_TYPE					1
#define	CW_DATA_MSG_STATS_TYPE					2
#define	CW_DATA_MSG_FREQ_STATS_TYPE				3 /* 2009 Update */
#define	CW_IEEE_802_3_FRAME_TYPE				4
#define	CW_DATA_MSG_KEEP_ALIVE_TYPE				5

#define	CW_IEEE_802_11_FRAME_TYPE				6

// <TRANSPORT_HEADER_FIELDS>
// CAPWAP version (currently 0)
#define	CW_TRANSPORT_HEADER_VERSION_START			0
#define	CW_TRANSPORT_HEADER_VERSION_LEN				4

// Mauro
#define	CW_TRANSPORT_HEADER_TYPE_START				4
#define	CW_TRANSPORT_HEADER_TYPE_LEN				4

// Radio ID number (for WTPs with multiple radios)
#define	CW_TRANSPORT_HEADER_RID_START				13
#define	CW_TRANSPORT_HEADER_RID_LEN				5

// Length of CAPWAP tunnel header in 4 byte words
#define	CW_TRANSPORT_HEADER_HLEN_START				8
#define	CW_TRANSPORT_HEADER_HLEN_LEN				5

// Wireless Binding ID
#define	CW_TRANSPORT_HEADER_WBID_START				18
#define	CW_TRANSPORT_HEADER_WBID_LEN				5

// Format of the frame
#define	CW_TRANSPORT_HEADER_T_START				23
#define	CW_TRANSPORT_HEADER_T_LEN				1

// Is a fragment?
#define	CW_TRANSPORT_HEADER_F_START				24
#define	CW_TRANSPORT_HEADER_F_LEN				1

// Is NOT the last fragment?
#define	CW_TRANSPORT_HEADER_L_START				25
#define	CW_TRANSPORT_HEADER_L_LEN				1

// Is the Wireless optional header present?
#define	CW_TRANSPORT_HEADER_W_START				26
#define	CW_TRANSPORT_HEADER_W_LEN				1

// Is the Radio MAC Address optional field present?
#define	CW_TRANSPORT_HEADER_M_START				27
#define	CW_TRANSPORT_HEADER_M_LEN				1

// Is the message a keep alive?
#define	CW_TRANSPORT_HEADER_K_START				28
#define	CW_TRANSPORT_HEADER_K_LEN				1

// Set to 0 in this version of the protocol
#define	CW_TRANSPORT_HEADER_FLAGS_START				29
#define	CW_TRANSPORT_HEADER_FLAGS_LEN				3

// ID of the group of fragments
#define	CW_TRANSPORT_HEADER_FRAGMENT_ID_START			0
#define	CW_TRANSPORT_HEADER_FRAGMENT_ID_LEN			16

// Position of this fragment in the group
#define	CW_TRANSPORT_HEADER_FRAGMENT_OFFSET_START		16
#define	CW_TRANSPORT_HEADER_FRAGMENT_OFFSET_LEN			13

// Set to 0 in this version of the protocol
#define	CW_TRANSPORT_HEADER_RESERVED_START			29
#define	CW_TRANSPORT_HEADER_RESERVED_LEN			3
// </TRANSPORT_HEADER_FIELDS>


// Message Type Values
#define	CW_TYPE_DISCOVERY_REQUEST			1
#define	CW_TYPE_DISCOVERY_RESPONSE			2
#define	CW_TYPE_JOIN_REQUEST				3
#define	CW_TYPE_JOIN_RESPONSE				4
#define	CW_TYPE_CONFIGURE_REQUEST			5
#define	CW_TYPE_CONFIGURE_RESPONSE			6
#define	CW_TYPE_CONFIGURE_UPDATE_REQUEST		7
#define	CW_TYPE_CONFIGURE_UPDATE_RESPONSE		8
#define	CW_TYPE_WTP_EVENT_REQUEST			9
#define	CW_TYPE_WTP_EVENT_RESPONSE			10
#define	CW_TYPE_CHANGE_STATE_EVENT_REQUEST		11
#define	CW_TYPE_CHANGE_STATE_EVENT_RESPONSE		12
#define	CW_TYPE_ECHO_REQUEST				13
#define	CW_TYPE_ECHO_RESPONSE				14
#define	CW_TYPE_IMAGE_DATA_REQUEST			15
#define	CW_TYPE_IMAGE_DATA_RESPONSE			16
#define	CW_TYPE_RESET_REQUEST				17
#define	CW_TYPE_RESET_RESPONSE				18
#define	CW_TYPE_PRIMARY_DISCOVERY_REQUEST		19
#define	CW_TYPE_PRIMARY_DISCOVERY_RESPONSE		20
#define	CW_TYPE_DATA_TRANSFER_REQUEST			21
#define	CW_TYPE_DATA_TRANSFER_RESPONSE			22
#define	CW_TYPE_CLEAR_CONFIGURATION_REQUEST		23
#define	CW_TYPE_CLEAR_CONFIGURATION_RESPONSE		24
#define	CW_TYPE_STATION_CONFIGURATION_REQUEST		25
#define	CW_TYPE_STATION_CONFIGURATION_RESPONSE		26

#define	CW_TYPE_WTP_EXECUTE_COMMAND_REQUEST		257
#define	CW_TYPE_WTP_EXECUTE_COMMAND_RESPONSE		258
#define	CW_TYPE_AP_ROAM_REQUEST				259
#define	CW_TYPE_AP_ROAM_RESPONSE			260

// IEEE 802.11 Binding Type
#define	CW_TYPE_WLAN_CONFIGURATION_REQUEST		3398913
#define	CW_TYPE_WLAN_CONFIGURATION_RESPONSE		3398914

// Message Elements Type Values
#define	CW_ELEM_AC_DESCRIPTOR_CW_TYPE			1
#define	CW_ELEM_AC_IPV4_LIST_CW_TYPE			2
#define	CW_ELEM_AC_IPV6_LIST_CW_TYPE			3
#define	CW_ELEM_AC_NAME_CW_TYPE				4
#define	CW_ELEM_AC_NAME_INDEX_CW_TYPE			5
#define	CW_ELEM_TIMESTAMP_CW_TYPE			6
#define	CW_ELEM_ADD_MAC_ACL_CW_TYPE			7
#define	CW_ELEM_ADD_STATION_CW_TYPE			8
#define	CW_ELEM_ADD_STATIC_MAC_ACL_CW_TYPE		9
#define	CW_ELEM_CW_CONTROL_IPV4_ADDRESS_CW_TYPE		10
#define	CW_ELEM_CW_CONTROL_IPV6_ADDRESS_CW_TYPE		11
#define	CW_ELEM_CW_TIMERS_CW_TYPE			12
#define	CW_ELEM_DATA_TRANSFER_DATA_CW_TYPE		13
#define	CW_ELEM_DATA_TRANSFER_MODE_CW_TYPE		14
#define	CW_ELEM_CW_DECRYPT_ER_REPORT_CW_TYPE		15
#define	CW_ELEM_CW_DECRYPT_ER_REPORT_PERIOD_CW_TYPE	16
#define	CW_ELEM_DELETE_MAC_ACL_CW_TYPE			17
#define	CW_ELEM_DELETE_STATION_CW_TYPE			18
#define	CW_ELEM_DELETE_STATIC_MAC_ACL_CW_TYPE		19
#define	CW_ELEM_DISCOVERY_TYPE_CW_TYPE			20
#define	CW_ELEM_DUPLICATE_IPV4_ADDRESS_CW_TYPE		21
#define	CW_ELEM_DUPLICATE_IPV6_ADDRESS_CW_TYPE		22
#define	CW_ELEM_IDLE_TIMEOUT_CW_TYPE			23
#define	CW_ELEM_IMAGE_DATA_CW_TYPE			24
#define	CW_ELEM_IMAGE_IDENTIFIER_CW_TYPE		25
#define	CW_ELEM_IMAGE_INFO_CW_TYPE			26
#define	CW_ELEM_INITIATED_DOWNLOAD_CW_TYPE		27
#define	CW_ELEM_LOCATION_DATA_CW_TYPE			28
#define	CW_ELEM_MAX_MSG_LEN_CW_TYPE			29
#define	CW_ELEM_AP_ROAM_LIST				30
#define	CW_ELEM_MOBILITY_DOMAIN				31
/*
 * Elena Agostini - 03/2014: Add AC local IPv4 Address Msg. Elem.
 */
#define	CW_ELEM_LOCAL_IPV4_ADDRESS_CW_TYPE		30
#define	CW_ELEM_RADIO_ADMIN_STATE_CW_TYPE		31
#define	CW_ELEM_RADIO_OPERAT_STATE_CW_TYPE		32
#define	CW_ELEM_RESULT_CODE_CW_TYPE			33
#define	CW_ELEM_RETURNED_MSG_ELEM_CW_TYPE		34
#define	CW_ELEM_SESSION_ID_CW_TYPE			35
#define	CW_ELEM_STATISTICS_TIMER_CW_TYPE		36
#define	CW_ELEM_VENDOR_SPEC_PAYLOAD_BW_CW_TYPE		37
#define	CW_ELEM_WTP_BOARD_DATA_CW_TYPE			38
#define	CW_ELEM_WTP_DESCRIPTOR_CW_TYPE			39
#define	CW_ELEM_WTP_FALLBACK_CW_TYPE			40
#define	CW_ELEM_WTP_FRAME_TUNNEL_MODE_CW_TYPE		41
#define	CW_ELEM_WTP_MAC_TYPE_CW_TYPE			44
#define	CW_ELEM_WTP_NAME_CW_TYPE			45
#define	CW_ELEM_WTP_OPERAT_STATISTICS_CW_TYPE		46
#define	CW_ELEM_WTP_RADIO_STATISTICS_CW_TYPE		47
#define	CW_ELEM_WTP_REBOOT_STATISTICS_CW_TYPE		48
#define	CW_ELEM_WTP_STATIC_IP_CW_TYPE			49
/*
 * Elena Agostini - 02/2014
 *
 * ECN Support Msg Elem MUST be included in Join Request/Response Messages
 */
#define	CW_ELEM_ECN_SUPPORT_CW_TYPE			53

/*Update 2009:
		Message type to return a payload together with the
		configuration update response*/
#define	CW_ELEM_VENDOR_SPEC_PAYLOAD_CW_TYPE		57
#define	CW_ELEM_RESULT_CODE_CW_TYPE_WITH_PAYLOAD	58
#define	CW_ELEM_MTU_DISCOVERY_PADDING_CW_TYPE		59
#define	CW_ELEM_RESULT_STRING				60
#define	CW_ELEM_STATION_ONLINE_CW_TYPE			127
// Vendor Specific data
#define	CW_ELEM_VENDOR_SPEC_DOWNLOAD_STATUS		128
// #define	CW_ELEM_VENDOR_SPEC_PAYLOAD_AC_PRIORITY		128
#define	CW_ELEM_VENDOR_SPEC_PAYLOAD_AC_PRIORITY_LEN	1
#define	CW_ELEM_VENDOR_SPEC_PAYLOAD_AC_PRIORITY_DEFAULT	3
#define	CW_ELEM_VENDOR_SPEC_PAYLOAD_LED			5
#define	CW_ELEM_VENDOR_SPEC_PAYLOAD_CLIENT_ALIVE	6
#define	CW_ELEM_VENDOR_SPEC_PAYLOAD_UPDATE_STATUS	7
#define	CW_ELEM_VENDOR_SPEC_PAYLOAD_WTP_CMD		8
#define	CW_ELEM_VENDOR_SPEC_PAYLOAD_WPA			1
#define	CW_ELEM_VENDOR_SPEC_PAYLOAD_WPA_PASSPHRASE	2

#define	CW_ELEM_VENDOR_SPEC_PAYLOAD_UCI			1
#define	CW_ELEM_VENDOR_SPEC_PAYLOAD_WUM			2
#define	CW_ELEM_VENDOR_SPEC_UPDATE			3

// IEEE 802.11 Message Element
#define	CW_ELEM_IEEE80211_ADD_WLAN_CW_TYPE			1024
#define	CW_ELEM_IEEE80211_ASSIGNED_WTP_BSSID_CW_TYPE		1026
#define	CW_ELEM_IEEE80211_DELETE_WLAN_CW_TYPE			1027
#define	CW_ELEM_IEEE80211_MAC_OPERATION_CW_TYPE			1030
#define	CW_ELEM_IEEE80211_MULTI_DOMAIN_CAPABILITY_CW_TYPE	1032
#define	CW_ELEM_IEEE80211_STATION				1036
#define	CW_ELEM_IEEE80211_SUPPORTED_RATES_CW_TYPE		1040
#define	CW_ELEM_IEEE80211_UPDATE_WLAN_CW_TYPE			1044
#define	CW_ELEM_IEEE80211_WTP_RADIO_INFORMATION_CW_TYPE		1048

#define	CW_ELEM_VENDOR_SPEC_WTP_VERSION	60000
#define	CW_ELEM_VENDOR_SPEC_AC_VERSION	60001
#define	CW_ELEM_VENDOR_SPEC_HTMODE	60002

// CAPWAP Protocol Variables
#define	CW_MAX_RETRANSMIT_DEFAULT		5
#define	CW_WAIT_JOIN_DEFAULT			30
#define	CW_REPORT_INTERVAL_DEFAULT		120
#define	CW_STATISTIC_TIMER_DEFAULT		120

#define	CW_JOIN_INTERVAL_DEFAULT		30

//Elena TORESET
#define	CW_CHANGE_STATE_INTERVAL_DEFAULT	10
#define	CW_RETRANSMIT_INTERVAL_DEFAULT		10
#define	CW_SESSION_ID_LENGTH			16

/*
* Elena Agostini - 03/2014
* DataChannel Dead Timer && Echo retransmit
*/
#define	CW_ECHO_MAX_RETRANSMIT_DEFAULT			3
#define	CW_ECHO_INTERVAL_DEFAULT			10
#define	CW_DATA_CHANNEL_KEEP_ALIVE_DEFAULT		CW_ECHO_INTERVAL_DEFAULT
#define	CW_NEIGHBORDEAD_INTERVAL_DEFAULT		(CW_ECHO_MAX_RETRANSMIT_DEFAULT * CW_ECHO_INTERVAL_DEFAULT)
#define	CW_DATACHANNELDEAD_INTERVAL_DEFAULT 		(CW_ECHO_MAX_RETRANSMIT_DEFAULT * CW_NEIGHBORDEAD_INTERVAL_DEFAULT)
#define	CW_NEIGHBORDEAD_RESTART_DISCOVERY_DELTA_DEFAULT	((CW_NEIGHBORDEAD_INTERVAL_DEFAULT) + 40)
/*_________________________________________________________*/
/*  *******************___VARIABLES___*******************  */

enum CWProtocolResultCode {
	CW_PROTOCOL_SUCCESS				= 0, //	Success
	CW_PROTOCOL_FAILURE_AC_LIST			= 1, // AC List message MUST be present
	CW_PROTOCOL_SUCCESS_NAT				= 2, // NAT detected
	CW_PROTOCOL_FAILURE				= 3, // unspecified
	CW_PROTOCOL_FAILURE_RES_DEPLETION		= 4, // Resource Depletion
	CW_PROTOCOL_FAILURE_UNKNOWN_SRC			= 5, // Unknown Source
	CW_PROTOCOL_FAILURE_INCORRECT_DATA		= 6, // Incorrect Data
	CW_PROTOCOL_FAILURE_ID_IN_USE			= 7, // Session ID Alreadyin Use
	CW_PROTOCOL_FAILURE_WTP_HW_UNSUPP		= 8, // WTP Hardware not supported
	CW_PROTOCOL_FAILURE_BINDING_UNSUPP		= 9, // Binding not supported
	CW_PROTOCOL_FAILURE_UNABLE_TO_RESET		= 10, // Unable to reset
	CW_PROTOCOL_FAILURE_FIRM_WRT_ERROR		= 11, // Firmware write error
	CW_PROTOCOL_FAILURE_SERVICE_PROVIDED_ANYHOW	= 12, // Unable to apply requested configuration
	CW_PROTOCOL_FAILURE_SERVICE_NOT_PROVIDED	= 13, // Unable to apply requested configuration
	CW_PROTOCOL_FAILURE_INVALID_CHECKSUM		= 14, // Image Data Error: invalid checksum
	CW_PROTOCOL_FAILURE_INVALID_DATA_LEN		= 15, // Image Data Error: invalid data length
	CW_PROTOCOL_FAILURE_OTHER_ERROR			= 16, // Image Data Error: other error
	CW_PROTOCOL_FAILURE_IMAGE_ALREADY_PRESENT	= 17, // Image Data Error: image already present
	CW_PROTOCOL_FAILURE_INVALID_STATE		= 18, // Message unexpected: invalid in current state
	CW_PROTOCOL_FAILURE_UNRECOGNIZED_REQ		= 19, // Message unexpected: unrecognized request
	CW_PROTOCOL_FAILURE_MISSING_MSG_ELEM		= 20, // Failure: missing mandatory message element
	CW_PROTOCOL_FAILURE_UNRECOGNIZED_MSG_ELEM	= 21, // Failure: unrecognized message element
};

enum vendor_info_type{
	CW_WTP_MODEL_NUMBER	= 0,
	CW_WTP_SERIAL_NUMBER	= 1,
	CW_BOARD_ID		= 2,
	CW_BOARD_REVISION	= 3,
	CW_BASE_MAC		= 4,
	CW_WTP_HARDWARE_VERSION	= 5,
	CW_WTP_SOFTWARE_VERSION	= 6,
	CW_BOOT_VERSION		= 7
};

#define CW_AC_HARDWARE_VERSION		4
#define CW_AC_SOFTWARE_VERSION		5

#define WTP_COMMIT_SIFLOWER_UPDATE	17
#define WTP_COMMIT_SIFLOWER_ACK		18

#define WTP_UPDATE_NONE			0
#define WTP_UPDATE_INPROCESS		1
#define WTP_UPDATE_TRANSFER_DONE	2
#define WTP_UPDATE_ERROR		3
#define WTP_UPDATE_SUCCESS		4
#define WTP_UPDATE_FAIL			5

#endif
