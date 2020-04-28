#include "ipp_local_device_info.h"
#include "local_common.h"
#include "local_tcp.h"
#include "device_manager.h"

ipp_local_device *create_local_device()
{
	ipp_local_device *d = (ipp_local_device *)malloc(sizeof(ipp_local_device));
	MALLOC_ERROR_RETURN_WTTH_RET(d, NULL);
	init_local_device(d);
	return d;
}

void init_local_device(ipp_local_device *profile)
{
	profile->guid = NULL;
	profile->product_id = NULL;
	profile->software_ver = NULL;
	profile->device_type = 0;
	profile->mac = NULL;
	profile->connet_type = 0;
	profile->name = 0;
	profile->is_master = 0;
	profile->gateway_guid = NULL;
	profile->reserved = NULL;
	profile->reserved_len = 0;
}

int init_string_from_protocol(char **dest, ipp_protocol *protocol)
{
	char *tempstr = NULL;
	int ret = -1;
	if (protocol_success == get_string(protocol, &tempstr)){
		ret = copy_string(dest, tempstr);
	} else {
		ret = copy_string(dest, "");
	}
	FREE_POINTER(tempstr);
	return ret;
}

int init_local_device_from_protocol(ipp_local_device *profile, ipp_protocol *protocol) {
	if (init_string_from_protocol(&profile->guid, protocol) < 0)
		return -1;

	if (init_string_from_protocol(&profile->product_id, protocol) < 0)
		return -1;

	if (init_string_from_protocol(&profile->software_ver, protocol) < 0)
		return -1;

	if (protocol_success != get_int32(protocol, &profile->device_type))
		return -1;

	if (init_string_from_protocol(&profile->mac, protocol) < 0)
		return -1;

	if (protocol_success != get_char(protocol, (char *)&profile->connet_type))
		return -1;

	if (init_string_from_protocol(&profile->name, protocol) < 0)
		return -1;

	if (protocol_success != get_char(protocol, (char *)&profile->is_master))
		return -1;

	if (init_string_from_protocol(&profile->gateway_guid, protocol) < 0)
		return -1;

	if (protocol_success != get_char_array(protocol, (char **)&profile->reserved, &profile->reserved_len))
		return -1;

	return 1;
}

void free_local_device(ipp_local_device **profile)
{
	if (NULL != *profile) {
		FREE_POINTER((*profile)->guid);
		FREE_POINTER((*profile)->product_id);
		FREE_POINTER((*profile)->software_ver);
		FREE_POINTER((*profile)->mac);
		FREE_POINTER((*profile)->name);
		FREE_POINTER((*profile)->gateway_guid);
		FREE_POINTER((*profile)->reserved);
		FREE_POINTER(*profile);
	}
}

void free_local_device_list(ipp_device_list **profile_list)
{
	ipp_device_list *next;
	for (; *profile_list; *profile_list = next) {
		next = (*profile_list)->next;
		free_local_device(&(*profile_list)->device);
		FREE_POINTER(*profile_list);
	}
}

ipp_local_device *get_device_clone_from_other(ipp_local_device *other) {
	if (NULL != other) {
		ipp_local_device *p = create_local_device();
		MALLOC_ERROR_RETURN_WTTH_RET(p, NULL)

		if (copy_string(&p->guid, other->guid) < 0) {
			free_local_device(&p);
			return NULL;
		}

		if (copy_string(&p->product_id, other->product_id) < 0) {
			free_local_device(&p);
			return NULL;
		}
		
		if (copy_string(&p->software_ver, other->software_ver) < 0) {
			free_local_device(&p);
			return NULL;
		}

		p->device_type = other->device_type;

		if (copy_string(&p->mac, other->mac) < 0) {
			free_local_device(&p);
			return NULL;
		}

		p->connet_type = other->connet_type;

		if (copy_string(&p->name, other->name) < 0) {
			free_local_device(&p);
			return NULL;
		}

		p->is_master = other->is_master;

		if (copy_string(&p->gateway_guid, other->gateway_guid) < 0) {
			free_local_device(&p);
			return NULL;
		}

		if (other->reserved_len > 0) {
			p->reserved_len = other->reserved_len;
			p->reserved = (int8_t *)malloc(p->reserved_len);
			if (NULL == p->reserved) {
				ipp_LogE_Prefix("malloc:%s(%d)\r\n",__FUNCTION__,__LINE__);
				free_local_device(&p);
				return NULL;
			}

			memcpy(p->reserved, other->reserved, p->reserved_len);
		}
		return p;
	}

	return NULL;
}

int ipp_device_list_append(ipp_device_list **head, ipp_local_device *element) {
	if (NULL != element && NULL != element->guid) {
		ipp_device_list *node = (ipp_device_list *)malloc(sizeof(ipp_device_list));
		MALLOC_ERROR_RETURN_WTTH_RET(node, -1)

		node->device = get_device_clone_from_other(element);
		node->next = NULL;

		if (*head == NULL) {
			*head = node;
		} else {
			ipp_device_list *e = *head, *tail_e = NULL;
			for (; e; e = e->next) {
				if (NULL != e->device && NULL != e->device->guid && 
					NULL != node->device && NULL != node->device->guid) {
					if (0 == strcmp(e->device->guid, node->device->guid)) {
						free_local_device(&node->device);
						FREE_POINTER(node);
						return 0;
					}
				}
				
				if (e->next == NULL)
					tail_e = e;
			}

			if (NULL != tail_e)
				tail_e->next = node;
			else {
				free_local_device(&node->device);
				FREE_POINTER(node);
			}
		}
		return 1;
	}

	return -1;
}

ipp_device_list *ipp_device_list_remove(ipp_device_list **head, char *guid)
{
	if (NULL != *head) {
		ipp_device_list *le = NULL, *pe = NULL;
		le = *head, pe = NULL;
		while (le) {
			ipp_device_list *next = le->next;
			if (0 == strcmp(le->device->guid, guid)) {
				if (NULL == pe) {
					*head = next;
				} else {
					pe->next = next;
				}

				free_local_device(&le->device);
				FREE_POINTER(le);
				return next;
			} else {
				pe = le;
			}

			le = next;
		}
		return NULL;
	}

	return NULL;
}

ipp_local_device *ipp_device_list_find(ipp_device_list **head, char *guid)
{
	if (NULL != *head) {
		ipp_device_list *le = *head;
		while (le) {
			ipp_device_list *next = le->next;
			if (0 == strcmp(le->device->guid, guid)) {
				return le->device;
			}

			le = next;
		}
		return NULL;
	}

	return NULL;
}

int get_device_info_protocol_len(ipp_local_device *device){
	if (NULL != device) {
		int len = (int)(
			sizeof(int32_t) + (NULL == device->guid ? 0 : strlen(device->guid)) +
			sizeof(int32_t) + (NULL == device->product_id ? 0 : strlen(device->product_id)) +
			sizeof(int32_t) + (NULL == device->software_ver ? 0 : strlen(device->software_ver)) +
			sizeof(int32_t) +	//device_type
			sizeof(int32_t) + (NULL == device->mac ? 0 : strlen(device->mac)) +
			sizeof(int8_t) +	//connet_type
			sizeof(int32_t) + (NULL == device->name ? 0 : strlen(device->name)) +
			sizeof(int8_t) +	//is_master
			sizeof(int32_t) + (NULL == device->gateway_guid ? 0 : strlen(device->gateway_guid)) +
			sizeof(int32_t) + device->reserved_len);

		return len;
	}
	
	return 0;
}

int32_t fill_to_protocol(ipp_protocol *p, ipp_local_device *device)
{
	int32_t ret = put_string(p, (char*)(NULL == device->guid ? "" : device->guid));
	if (protocol_success != ret)
		return ret;

	ret = put_string(p, (char*)(NULL == device->product_id ? "" : device->product_id));
	if (protocol_success != ret)
		return ret;

	ret = put_string(p, (char*)(NULL == device->software_ver ? "" : device->software_ver));
	if (protocol_success != ret)
		return ret;

	ret = put_int32(p, device->device_type);
	if (protocol_success != ret)
		return ret;

	ret = put_string(p, (char*)(NULL == device->mac ? "" : device->mac));
	if (protocol_success != ret)
		return ret;

	ret = put_char(p, device->connet_type);
	if (protocol_success != ret)
		return ret;

	ret = put_string(p, (char*)(NULL == device->name ? "" : device->name));
	if (protocol_success != ret)
		return ret;

	ret = put_char(p, device->is_master);
	if (protocol_success != ret)
		return ret;

	ret = put_string(p, (char*)(NULL == device->gateway_guid ? "" : device->gateway_guid));
	if (protocol_success != ret)
		return ret;

	ret = put_char_array(p, (char *)device->reserved, device->reserved_len);
	if (protocol_success != ret)
		return ret;

	return protocol_success;
}

