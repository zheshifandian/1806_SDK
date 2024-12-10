#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include "tlv.h"

struct tlv_box *tlv_box_create(uint32_t how)
{
	struct tlv_box *box = (struct tlv_box *)malloc(sizeof(struct tlv_box));

	if (!box)
		return NULL;

	memset(box, 0, sizeof(*box));
	INIT_LIST_HEAD(&box->tlv_list);
	box->how = how;

	return box;
}

int tlv_box_put_raw(struct tlv_box *box, uint16_t type, struct message *msg, int flag)
{
	struct tlv *tlv;

	if (!box || !msg)
		return -EINVAL;

	tlv = (struct tlv *)calloc(1, sizeof(struct tlv));
	if (!tlv)
		return -ENOMEM;
	tlv->type = type;
	tlv->length = msg->len;

	if (flag & TLV_NOCPY) {
		tlv->value = msg->data;
	} else {
		tlv->value = malloc(msg->len);
		if (!tlv->value)
			return -ENOMEM;
		memcpy(tlv->value, msg->data, msg->len);
	}
	tlv->flag = flag;
	list_add_tail(&tlv->list, &box->tlv_list);
	box->serialized_len += sizeof(tlv->type) + sizeof(tlv->length) + tlv->length;
	box->count++;

	return 0;
}

int tlv_box_put_string(struct tlv_box *box, uint16_t type, const char *value, int flag)
{
	struct message msg;

	msg.data = (void *)value;
	msg.len = strlen(value);
	return tlv_box_put_raw(box, type, &msg, flag);
}

int tlv_box_put_box(struct tlv_box *box, uint16_t type, struct tlv_box *object)
{
	struct message msg;
	int err = tlv_box_serialize(object);

	if (err)
		return err;
	msg.data = tlv_box_get_buffer(object);
	msg.len = tlv_box_get_size(object);
	object->how |= NOT_FREE_BUFFER;
	return tlv_box_put_raw(box, type, &msg, TLV_NOCPY);
}

// static uint8_t tlv_parse_u8(void *value)
// {
// 	return *(uint8_t *)value;
// }

static uint16_t tlv_parse_u16(void *value)
{
	uint16_t tmp;

	memcpy(&tmp, value, sizeof(uint16_t));
	return htons(tmp);
}

static uint32_t tlv_parse_u32(void *value)
{
	uint32_t tmp;

	memcpy(&tmp, value, sizeof(uint32_t));
	return htonl(tmp);
}

static struct tlv *get_last_tlv(struct tlv_box *box)
{
	return list_entry(box->tlv_list.prev, struct tlv, list);
}

/**
 * Parse a TLV format message to struct tlv_box
 */
int tlv_box_parse(struct tlv_box *box, void *buffer, int buffersize)
{
	struct message msg;
	struct tlv *tlv, *tmp;
	int offset = 0;
	int old_len = box->serialized_len;
	int i = 0;
	uint16_t type;
	uint32_t id;

	if (box->how & SERIAL_WITH_ID) {
		box->id = tlv_parse_u32(buffer);
		offset += sizeof(uint32_t);
		box->serialized_len += sizeof(uint32_t);
	}
	while (offset < buffersize) {
		if (box->how & SERIAL_EACH_WITH_ID) {
			id = tlv_parse_u32(buffer + offset);
			offset += sizeof(uint32_t);
			box->serialized_len += sizeof(uint32_t);
		}
		type = tlv_parse_u16(buffer + offset);
		offset += sizeof(uint16_t);
		msg.len = tlv_parse_u16(buffer + offset);
		offset += sizeof(uint16_t);
		msg.data = buffer + offset;
		if (tlv_box_put_raw(box, type, &msg, 0))
			goto fail;
		get_last_tlv(box)->id = id;
		offset += msg.len;
		i++;
	}

	if (box->serialized_len - old_len != buffersize)
		goto fail;

	return 0;

fail:
	list_for_each_entry_safe_reverse(tlv, tmp, &box->tlv_list, list) {
		if (i == 0)
			break;
		if (tlv->value)
			free(tlv->value);
		list_del(&tlv->list);
		free(tlv);
		i--;
	}
	return -ENOMEM;
}

void tlv_box_destroy(struct tlv_box *box)
{
	struct tlv *tlv, *tmp;

	if (!box)
		return;
	list_for_each_entry_safe(tlv, tmp, &box->tlv_list, list) {
		if ((!(tlv->flag & TLV_NOFREE_BIT)) && tlv->value)
			free(tlv->value);
		list_del(&tlv->list);
		free(tlv);
	}

	if (!(box->how & NOT_FREE_BUFFER) && box->serialized_buffer) {
		free(box->serialized_buffer);
		box->serialized_buffer = NULL;
	}
	free(box);
}

void *tlv_box_get_buffer(struct tlv_box *box)
{
	return box->serialized_buffer;
}

int tlv_box_get_size(struct tlv_box *box)
{
	return box->serialized_len;
}

#define TLV_BOX_PUT_U16(buffer, offset, value)                                                     \
	do {                                                                                       \
		uint16_t __value = htons(value);                                                   \
		memcpy(buffer + offset, &__value, sizeof(uint16_t));                               \
		offset += sizeof(uint16_t);                                                        \
	} while (0)
#define TLV_BOX_PUT_U32(buffer, offset, value)                                                     \
	do {                                                                                       \
		uint32_t __value = htonl(value);                                                   \
		memcpy(buffer + offset, &__value, sizeof(uint32_t));                               \
		offset += sizeof(uint32_t);                                                        \
	} while (0)

int tlv_box_serialize(struct tlv_box *box)
{
	int offset = 0;
	unsigned char *buffer;
	struct tlv *tlv;

	if (!box)
		return -EINVAL;

	if (box->serialized_buffer)
		return 0;

	if (box->how & SERIAL_WITH_ID)
		box->serialized_len += sizeof(box->id);
	else if (box->how & SERIAL_EACH_WITH_ID)
		box->serialized_len += box->count * sizeof(tlv->id);

	buffer = malloc(box->serialized_len);
	if (!buffer)
		return -ENOMEM;

	if (box->how & SERIAL_WITH_ID)
		TLV_BOX_PUT_U32(buffer, offset, box->id);

	list_for_each_entry(tlv, &box->tlv_list, list) {
		if (box->how & SERIAL_EACH_WITH_ID)
			TLV_BOX_PUT_U32(buffer, offset, box->id);
		TLV_BOX_PUT_U16(buffer, offset, tlv->type);
		TLV_BOX_PUT_U16(buffer, offset, tlv->length);
		memcpy(buffer + offset, tlv->value, tlv->length);
		offset += tlv->length;
	}

	box->serialized_buffer = buffer;

	return 0;
}

struct tlv *tlv_box_find_type(struct tlv_box *box, uint16_t type)
{
	struct tlv *tlv;

	list_for_each_entry(tlv, &box->tlv_list, list) {
		if (tlv->type == type)
			return tlv;
	}
	return NULL;
}

void tlv_box_print(struct tlv_box *box)
{
	struct tlv *tlv;
	uint16_t i;

	list_for_each_entry(tlv, &box->tlv_list, list) {
		printf("{type %d, len %d, value: ", tlv->type, tlv->length);
		for (i = 0; i < tlv->length; i++)
			printf("0x%02hhx ", *(uint8_t *)(tlv->value + i));

		printf("} -> ");
	}
	printf("end\n");
}
