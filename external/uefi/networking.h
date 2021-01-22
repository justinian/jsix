#pragma once
#ifndef _uefi_networking_h_
#define _uefi_networking_h_

// This Source Code Form is part of the j6-uefi-headers project and is subject
// to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was
// not distributed with this file, You can obtain one at
// http://mozilla.org/MPL/2.0/.

#include <stdint.h>
#include <uefi/types.h>

namespace uefi {

//
// IPv4 definitions
//
struct ipv4_address
{
	uint8_t addr[4];
};

//
// IPv6 definitions
//
struct ipv6_address
{
	uint8_t addr[16];
};

struct ip6_address_info
{
    ipv6_address address;
    uint8_t prefix_length;
};

struct ip6_route_table
{
    ipv6_address gateway;
    ipv6_address destination;
    uint8_t prefix_length;
};

enum class ip6_neighbor_state : int {
    incomplete,
    reachable,
    stale,
    delay,
    probe,
}

struct ip6_neighbor_cache
{
    ipv6_address neighbor;
    mac_address link_address;
    ip6_neighbor_state state;
};

enum class icmpv6_type : uint8_t
{
    dest_unreachable = 0x1,
    packet_too_big = 0x2,
    time_exceeded = 0x3,
    parameter_problem = 0x4,
    echo_request = 0x80,
    echo_reply = 0x81,
    listener_query = 0x82,
    listener_report = 0x83,
    listener_done = 0x84,
    router_solicit = 0x85,
    router_advertise = 0x86,
    neighbor_solicit = 0x87,
    neighbor_advertise = 0x88,
    redirect = 0x89,
    listener_report_2 = 0x8f,
};

enum class icmpv6_code : uint8_t
{
    // codes for icmpv6_type::dest_unreachable
    no_route_to_dest = 0x0,
    comm_prohibited = 0x1,
    beyond_scope = 0x2,
    addr_unreachable = 0x3,
    port_unreachable = 0x4,
    source_addr_failed = 0x5,
    route_rejected = 0x6,

    // codes for icmpv6_type::time_exceeded
    timeout_hop_limit = 0x0,
    timeout_reassemble = 0x1,

    // codes for icmpv6_type::parameter_problem
    erroneous_header = 0x0,
    unrecognize_next_hdr = 0x1,
    unrecognize_option = 0x2,
};

struct ip6_icmp_type
{
    icmpv6_type type;
    icmpv6_code code;
};

struct ip6_config_data
{
    uint8_t default_protocol;
    bool accept_any_protocol;
    bool accept_icmp_errors;
    bool accept_promiscuous;
    ipv6_address destination_address;
    ipv6_address station_address;
    uint8_t traffic_class;
    uint8_t hop_limit;
    uint32_t flow_label;
    uint32_t receive_timeout;
    uint32_t transmit_timeout;
};

struct ip6_mode_data
{
    bool is_started;
    uint32_t max_packet_size;
    ip6_config_data config_data;
    bool is_configured;
    uint32_t address_count;
    ip6_address_info * address_list;
    uint32_t group_count;
    ipv6_address * group_table;
    uint32_t route_count;
    ip6_route_table * route_table;
    uint32_t neighbor_count;
    ip6_neighbor_cache * neighbor_cache;
    uint32_t prefix_count;
    ip6_address_info * prefix_table;
    uint32_t icmp_type_count;
     * icmp_type_list;
};

struct ip6_header
{
    uint8_t traffic_class_h : 4;
    uint8_t version : 4;
    uint8_t flow_label_h : 4;
    uint8_t traffic_class_l : 4;
    uint16_t flow_label_l;
    uint16_t payload_length;
    uint8_t next_header;
    uint8_t hop_limit;
    ipv6_address source_address;
    ipv6_address destination_address;
} __attribute__ ((packed));

struct ip6_fragment_data
{
    uint32_t fragment_length;
    void *fragment_buffer;
};

struct ip6_override_data
{
    uint8_t protocol;
    uint8_t hop_limit;
    uint32_t flow_label;
};

struct ip6_receive_data
{
    time time_stamp;
    event recycle_signal;
    uint32_t header_length;
    ip6_header *header;
    uint32_t data_length;
    uint32_t fragment_count;
    ip6_fragment_data fragment_table[1];
};

struct ip6_transmit_data
{
    ipv6_address destination_address;
    ip6_override_data *override_data;

    uint32_t ext_hdrs_length;
    void *ext_hdrs;
    uint8_t next_header;
    uint32_t data_length;
    uint32_t fragment_count;
    ip6_fragment_data fragment_table[1];
};

struct ip6_completion_token
{
    event event;
    status status;
    union {
        ip6_receive_data *rx_data;
        ip6_transmit_data *tx_data;
    } packet;
};

enum class ip6_config_data_type : int
{
    interface_info,
    alt_interface_id,
    policy,
    dup_addr_detect_transmits,
    manual_address,
    gateway,
    dns_server,
    maximum
};

struct ip6_config_interface_info
{
    wchar_t name[32];
    uint8_t if_type;
    uint32_t hw_address_size;
    mac_address hw_address;
    uint32_t address_info_count;
    ip6_address_info *address_info;
    uint32_t route_count;
    ip6_route_table *route_table;
};

struct ip6_config_interface_id
{
    uint8_t id[8];
};

enum class ip6_config_policy : int
{
    manual,
    automatic
};

struct ip6_config_dup_addr_detect_transmits
{
    uint32_t dup_addr_detect_transmits;
};

struct ip6_config_manual_address
{
    ipv6_address address;
    bool is_anycast;
    uint8_t prefix_length;
};

//
// IP definitions
//
union ip_address
{
	uint8_t addr[4];
	ipv4_address v4;
	ipv6_address v6;
};

//
// HTTP definitions
//
struct httpv4_access_point
{
	bool use_default_address;
	ipv4_address local_address;
	ipv4_address local_subnet;
	uint16_t local_port;
};

struct httpv6_access_point
{
	ipv6_address local_address;
	uint16_t local_port;
};

enum class http_version : int {
	v10,
	v11,
	unsupported,
};

struct http_config_data
{
	http_version http_version;
	uint32_t time_out_millisec;
	bool local_address_is_ipv6;
	union {
		httpv4_access_point *ipv4_node;
		httpv6_access_point *ipv6_node;
	} access_point;
};

enum class http_method : int {
	get,
	post,
	patch,
	options,
	connect,
	head,
	put,
	delete_,
	trace,
};

struct http_request_data
{
	http_method method;
	wchar_t *url;
};

enum class http_status_code : int {
	unsupported,
	continue_,
	switching_protocols,
	ok,
	created,
	accepted,
	non_authoritative_information,
	no_content,
	reset_content,
	partial_content,
	multiple_choices,
	moved_permanently,
	found,
	see_other,
	not_modified,
	use_proxy,
	temporary_redirect,
	bad_request,
	unauthorized,
	payment_required,
	forbidden,
	not_found,
	method_not_allowed,
	not_acceptable,
	proxy_authentication_required,
	request_time_out,
	conflict,
	gone,
	length_required,
	precondition_failed,
	request_entity_too_large,
	request_uri_too_large,
	unsupported_media_type,
	requested_range_not_satisfied,
	expectation_failed,
	internal_server_error,
	not_implemented,
	bad_gateway,
	service_unavailable,
	gateway_timeout,
	http_version_not_supported,
	permanent_redirect,  // I hate your decisions, uefi.
};

struct http_response_data
{
	http_status_code status_code;
};

struct http_header
{
	char *field_name;
	char *field_value;
};

struct http_message
{
	union {
		http_request_data *request;
		http_response_data *response;
	} data;

	size_t header_count;
	http_header *headers;

	size_t body_length;
	void *body;
};

struct http_token
{
	event event;
	status status;
	http_message *message;
};


} // namespace uefi

#endif
