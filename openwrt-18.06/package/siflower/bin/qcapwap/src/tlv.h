/*
 *  COPYRIGHT NOTICE
 *  Copyright (C) 2015, Jhuster, All Rights Reserved
 *  Author: Jhuster(lujun.hust@gmail.com)
 *
 *  https://github.com/Jhuster/TLV
 *
 *  This library is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation; either version 2.1 of the License,
 *  or (at your option) any later version.
 */
#ifndef _TLV_H_
#define _TLV_H_

#include <stdint.h>
#include "list.h"
#include "capwap_common.h"

/**
 * struct tlv:
 *   A type-length-value struct used to ease the message generation.
 *
 * @id: According to capwap protocol, a tlv struct may have vendor id with it.
 * @list: a list struct used to list into the tlv_box.
 * @type: the type of this tlv
 * @length: the length of this tlv
 * @value: data buffer of this tlv
 * @flag: used to control whether or not copy memory during initialization and whether
 * 	  free memory during destory. See tlv flags below. For optimize purpose.
 */
struct tlv {
	uint32_t id;
	struct list_head list;
	uint16_t type;
	uint16_t length;
	void *value;
	int flag;
};

/**
 * tlv_box:
 *   A tlv_box is a collection of tlvs, and these tlvs will be serialized in order.
 *   When sending messages, a tlv_box is created with the control of Vendor ID. Because the capwap message
 * is consists of many tlv parts, these tlvs are gathered into the tlv_list, and serialized into the serialized_buffer
 * when really send this message.
 *   When receive a message, the received data is put in the serialized_buffer, and then deserialize this
 * buffer into different tlvs, and these tlvs are linked in the tlv_list. We can then use tlv_box_for_each_tlv() to
 * iterator over these tlvs.
 *
 * @id: Vendor ID in Capwap message element.
 * @count: the number of total tlvs in this box.
 * @how: special flags to control behavior.
 * 	SERIAL_WITH_ID: a vendor ID in front of all the tlvs, such as WTP Board Data.
 * 	SERIAL_EACH_WITH_ID: a vendor ID in front of each tlvs.
 * 	NOT_FREE_BUFFER: don't free serialized_buffer when destory, because the buffer was transfered
 * 			to another tlv_box and will be freed there.
 * @tlv_list: list of tlvs.
 * @serialized_buffer: a buffer used to store the serialized data.
 * @serialized_len: length of the serialized_buffer.
 */
#define SERIAL_WITH_ID		BIT(0)
#define SERIAL_EACH_WITH_ID	BIT(1)
#define NOT_FREE_BUFFER		BIT(2)
struct tlv_box {
	uint32_t id;
	uint32_t count;
	uint32_t how;
	struct list_head tlv_list;
	void *serialized_buffer;
	int serialized_len;
};

/* A help macro to iterator over the tlvs in the tlv_box
 * _type, _len, _value are variables which reprensent each tlv's type, length and value */
#define tlv_box_for_each_tlv(box, _type, _len, _value)                                                             \
	for (struct tlv *__tlv = list_entry((&(box)->tlv_list)->next, struct tlv, list);                           \
	     _len = __tlv->length, _value = __tlv->value, _type = __tlv->type, &__tlv->list != (&(box)->tlv_list); \
	     __tlv = list_entry(__tlv->list.next, struct tlv, list))

struct tlv_box *tlv_box_create(uint32_t how);
void tlv_box_destroy(struct tlv_box *box);

/* Return the serialized_buffer pointer */
void *tlv_box_get_buffer(struct tlv_box *box);
/* Return serialized_len */
int tlv_box_get_size(struct tlv_box *box);

/**
 * tlv flags:
 * TLV_NOCPY: don't copy message buffer, use the pointer directly.
 * TLV_NOFREE_BIT: don't free this message buffer, this is used for literal strings.
 * TLV_NOFREE: it's obvious that we don't need to copy a literal string.
 */
#define TLV_NOCPY	BIT(0)
#define TLV_NOFREE_BIT	BIT(1)
#define TLV_NOFREE	(TLV_NOCPY | TLV_NOFREE_BIT)
/**
 * Append a tlv of type, length, value into this box.
 * @box: the box we want to add tlv into.
 * @type: the type of the tlv
 * @msg: a helper struct to indicate the length and value of the tlv
 * @flag: tlv flags
 */
int tlv_box_put_raw(struct tlv_box *box, uint16_t type, struct message *msg, int flag);
int tlv_box_put_string(struct tlv_box *box, uint16_t type, const char *value, int flag);
/**
 * Put a serialized box as a tlv struct into this box
 * @box: the box we want to add tlv into.
 * @type: the type of the tlv
 * @object: this box will be serialized and use serialized_buffer and serialized_len
 * 	    as the tlv's value and length.
 */
int tlv_box_put_box(struct tlv_box *box, uint16_t type, struct tlv_box *object);

/**
 * Parse a tlv format buffer into a tlv_box struct, each tlv was saved to tlv_list
 * @_box: a new, empty box to put deserialize data.
 * @buffer: the buffer contains raw message
 * @buffersize: length of the buffer
 */
int tlv_box_parse(struct tlv_box *_box, void *buffer, int buffersize);
/**
 * Serialize this box's tlv_list into serialized_buffer
 * This function is used in _capwap_send_ctrl_message() and other users don't need
 * to call this.
 */
int tlv_box_serialize(struct tlv_box *box);

/**
 * Find type in the tlv_list.
 * If there is a tlv whose type is the same with type, return this tlv;
 * return NULL if not found.
 */
struct tlv *tlv_box_find_type(struct tlv_box *box, uint16_t type);

#endif //_TLV_H_
