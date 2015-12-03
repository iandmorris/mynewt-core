/**
 * Copyright (c) 2015 Runtime Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <errno.h>
#include <string.h>
#include "os/os.h"
#include "nimble/ble.h"
#include "ble_hs_uuid.h"
#include "ble_l2cap.h"
#include "ble_att_cmd.h"

int
ble_att_error_rsp_parse(void *payload, int len, struct ble_att_error_rsp *rsp)
{
    uint8_t *u8ptr;

    if (len < BLE_ATT_ERROR_RSP_SZ) {
        return EMSGSIZE;
    }

    u8ptr = payload;

    if (u8ptr[0] != BLE_ATT_OP_ERROR_RSP) {
        return EINVAL;
    }

    rsp->bhaep_req_op = u8ptr[1];
    rsp->bhaep_handle = le16toh(u8ptr + 2);
    rsp->bhaep_error_code = u8ptr[4];

    return 0;
}

int
ble_att_error_rsp_write(void *payload, int len, struct ble_att_error_rsp *rsp)
{
    uint8_t *u8ptr;

    if (len < BLE_ATT_ERROR_RSP_SZ) {
        return EMSGSIZE;
    }

    u8ptr = payload;

    u8ptr[0] = BLE_ATT_OP_ERROR_RSP;
    u8ptr[1] = rsp->bhaep_req_op;
    htole16(u8ptr + 2, rsp->bhaep_handle);
    u8ptr[4] = rsp->bhaep_error_code;

    return 0;
}

int
ble_att_mtu_cmd_parse(void *payload, int len, struct ble_att_mtu_cmd *cmd)
{
    uint8_t *u8ptr;

    if (len < BLE_ATT_MTU_CMD_SZ) {
        return EMSGSIZE;
    }

    u8ptr = payload;

    if (u8ptr[0] != BLE_ATT_OP_MTU_REQ &&
        u8ptr[0] != BLE_ATT_OP_MTU_RSP) {

        return EINVAL;
    }

    cmd->bhamc_mtu = le16toh(u8ptr + 1);

    return 0;
}

int
ble_att_mtu_req_write(void *payload, int len,
                      struct ble_att_mtu_cmd *cmd)
{
    uint8_t *u8ptr;

    if (len < BLE_ATT_MTU_CMD_SZ) {
        return EMSGSIZE;
    }

    u8ptr = payload;

    u8ptr[0] = BLE_ATT_OP_MTU_REQ;
    htole16(u8ptr + 1, cmd->bhamc_mtu);

    return 0;
}

int
ble_att_mtu_rsp_write(void *payload, int len, struct ble_att_mtu_cmd *cmd)
{
    uint8_t *u8ptr;

    if (len < BLE_ATT_MTU_CMD_SZ) {
        return EMSGSIZE;
    }

    u8ptr = payload;

    u8ptr[0] = BLE_ATT_OP_MTU_RSP;
    htole16(u8ptr + 1, cmd->bhamc_mtu);

    return 0;
}

int
ble_att_find_info_req_parse(void *payload, int len,
                            struct ble_att_find_info_req *req)
{
    uint8_t *u8ptr;

    if (len < BLE_ATT_FIND_INFO_REQ_SZ) {
        return EMSGSIZE;
    }

    u8ptr = payload;

    if (u8ptr[0] != BLE_ATT_OP_FIND_INFO_REQ) {
        return EINVAL;
    }

    req->bhafq_start_handle = le16toh(u8ptr + 1);
    req->bhafq_end_handle = le16toh(u8ptr + 3);

    return 0;
}

int
ble_att_find_info_req_write(void *payload, int len,
                            struct ble_att_find_info_req *req)
{
    uint8_t *u8ptr;

    if (len < BLE_ATT_FIND_INFO_REQ_SZ) {
        return EMSGSIZE;
    }

    u8ptr = payload;

    u8ptr[0] = BLE_ATT_OP_FIND_INFO_REQ;
    htole16(u8ptr + 1, req->bhafq_start_handle);
    htole16(u8ptr + 3, req->bhafq_end_handle);

    return 0;
}

int
ble_att_find_info_rsp_parse(void *payload, int len,
                            struct ble_att_find_info_rsp *rsp)
{
    uint8_t *u8ptr;

    if (len < BLE_ATT_FIND_INFO_RSP_BASE_SZ) {
        return EMSGSIZE;
    }

    u8ptr = payload;

    if (u8ptr[0] != BLE_ATT_OP_FIND_INFO_RSP) {
        return EINVAL;
    }

    rsp->bhafp_format = u8ptr[1];

    return 0;
}

int
ble_att_find_info_rsp_write(void *payload, int len,
                            struct ble_att_find_info_rsp *rsp)
{
    uint8_t *u8ptr;

    if (len < BLE_ATT_FIND_INFO_RSP_BASE_SZ) {
        return EMSGSIZE;
    }

    u8ptr = payload;

    u8ptr[0] = BLE_ATT_OP_FIND_INFO_RSP;
    u8ptr[1] = rsp->bhafp_format;

    return 0;
}

int
ble_att_find_type_value_req_parse(void *payload, int len,
                                  struct ble_att_find_type_value_req *req)
{
    uint8_t *u8ptr;

    if (len < BLE_ATT_FIND_TYPE_VALUE_REQ_MIN_SZ) {
        return EMSGSIZE;
    }

    u8ptr = payload;

    if (u8ptr[0] != BLE_ATT_OP_FIND_TYPE_VALUE_REQ) {
        return EINVAL;
    }

    req->bhavq_start_handle = le16toh(u8ptr + 1);
    req->bhavq_end_handle = le16toh(u8ptr + 3);
    req->bhavq_attr_type = le16toh(u8ptr + 5);

    return 0;
}

int
ble_att_find_type_value_req_write(void *payload, int len,
                                  struct ble_att_find_type_value_req *req)
{
    uint8_t *u8ptr;

    if (len < BLE_ATT_FIND_TYPE_VALUE_REQ_MIN_SZ) {
        return EMSGSIZE;
    }

    u8ptr = payload;

    u8ptr[0] = BLE_ATT_OP_FIND_TYPE_VALUE_REQ;
    htole16(u8ptr + 1, req->bhavq_start_handle);
    htole16(u8ptr + 3, req->bhavq_end_handle);
    htole16(u8ptr + 5, req->bhavq_attr_type);

    return 0;
}

int
ble_att_read_type_req_parse(void *payload, int len,
                            struct ble_att_read_type_req *req)
{
    uint8_t *u8ptr;

    if (len < BLE_ATT_READ_TYPE_REQ_BASE_SZ) {
        return EMSGSIZE;
    }

    u8ptr = payload;

    if (u8ptr[0] != BLE_ATT_OP_READ_TYPE_REQ) {
        return EINVAL;
    }

    req->bhatq_start_handle = le16toh(u8ptr + 1);
    req->bhatq_end_handle = le16toh(u8ptr + 3);

    return 0;
}

int
ble_att_read_type_req_write(void *payload, int len,
                            struct ble_att_read_type_req *req)
{
    uint8_t *u8ptr;

    if (len < BLE_ATT_READ_TYPE_REQ_BASE_SZ) {
        return EMSGSIZE;
    }

    u8ptr = payload;

    u8ptr[0] = BLE_ATT_OP_READ_TYPE_REQ;
    htole16(u8ptr + 1, req->bhatq_start_handle);
    htole16(u8ptr + 3, req->bhatq_end_handle);

    return 0;
}

int
ble_att_read_type_rsp_parse(void *payload, int len,
                            struct ble_att_read_type_rsp *rsp)
{
    uint8_t *u8ptr;

    if (len < BLE_ATT_READ_TYPE_RSP_MIN_SZ) {
        return EMSGSIZE;
    }

    u8ptr = payload;

    rsp->bhatp_len = u8ptr[1];

    return 0;
}

int
ble_att_read_type_rsp_write(void *payload, int len,
                            struct ble_att_read_type_rsp *rsp)
{
    uint8_t *u8ptr;

    if (len < BLE_ATT_READ_TYPE_RSP_MIN_SZ) {
        return EMSGSIZE;
    }

    u8ptr = payload;

    u8ptr[0] = BLE_ATT_OP_READ_TYPE_RSP;
    u8ptr[1] = rsp->bhatp_len;

    return 0;
}

int
ble_att_read_req_parse(void *payload, int len, struct ble_att_read_req *req)
{
    uint8_t *u8ptr;

    if (len < BLE_ATT_READ_REQ_SZ) {
        return EMSGSIZE;
    }

    u8ptr = payload;

    if (u8ptr[0] != BLE_ATT_OP_READ_REQ) {
        return EINVAL;
    }

    req->bharq_handle = le16toh(u8ptr + 1);

    return 0;
}

int
ble_att_read_req_write(void *payload, int len, struct ble_att_read_req *req)
{
    uint8_t *u8ptr;

    if (len < BLE_ATT_READ_REQ_SZ) {
        return EMSGSIZE;
    }

    u8ptr = payload;

    u8ptr[0] = BLE_ATT_OP_READ_REQ;
    htole16(u8ptr + 1, req->bharq_handle);

    return 0;
}

int
ble_att_read_group_type_req_parse(void *payload, int len,
                                  struct ble_att_read_group_type_req *req)
{
    uint8_t *u8ptr;

    if (len < BLE_ATT_READ_GROUP_TYPE_REQ_BASE_SZ) {
        return EMSGSIZE;
    }

    u8ptr = payload;

    if (u8ptr[0] != BLE_ATT_OP_READ_GROUP_TYPE_REQ) {
        return EINVAL;
    }

    req->bhagq_start_handle = le16toh(u8ptr + 1);
    req->bhagq_end_handle = le16toh(u8ptr + 3);

    return 0;
}

int
ble_att_read_group_type_req_write(void *payload, int len,
                                  struct ble_att_read_group_type_req *req)
{
    uint8_t *u8ptr;

    if (len < BLE_ATT_READ_GROUP_TYPE_REQ_BASE_SZ) {
        return EMSGSIZE;
    }

    u8ptr = payload;

    u8ptr[0] = BLE_ATT_OP_READ_GROUP_TYPE_REQ;
    htole16(u8ptr + 1, req->bhagq_start_handle);
    htole16(u8ptr + 3, req->bhagq_end_handle);

    return 0;
}

int
ble_att_read_group_type_rsp_parse(void *payload, int len,
                                  struct ble_att_read_group_type_rsp *rsp)
{
    uint8_t *u8ptr;

    if (len < BLE_ATT_READ_GROUP_TYPE_RSP_BASE_SZ) {
        return EMSGSIZE;
    }

    u8ptr = payload;

    if (u8ptr[0] != BLE_ATT_OP_READ_GROUP_TYPE_RSP) {
        return EINVAL;
    }

    rsp->bhagp_length = u8ptr[1];

    return 0;
}

int
ble_att_read_group_type_rsp_write(void *payload, int len,
                                  struct ble_att_read_group_type_rsp *rsp)
{
    uint8_t *u8ptr;

    if (len < BLE_ATT_READ_GROUP_TYPE_RSP_BASE_SZ) {
        return EMSGSIZE;
    }

    u8ptr = payload;

    u8ptr[0] = BLE_ATT_OP_READ_GROUP_TYPE_RSP;
    u8ptr[1] = rsp->bhagp_length;

    return 0;
}

int
ble_att_write_req_parse(void *payload, int len, struct ble_att_write_req *req)
{
    uint8_t *u8ptr;

    if (len < BLE_ATT_WRITE_REQ_MIN_SZ) {
        return EMSGSIZE;
    }

    u8ptr = payload;

    if (u8ptr[0] != BLE_ATT_OP_WRITE_REQ) {
        return EINVAL;
    }

    req->bhawq_handle = le16toh(u8ptr + 1);

    return 0;
}

int
ble_att_write_req_write(void *payload, int len, struct ble_att_write_req *req)
{
    uint8_t *u8ptr;

    if (len < BLE_ATT_WRITE_REQ_MIN_SZ) {
        return EMSGSIZE;
    }

    u8ptr = payload;

    u8ptr[0] = BLE_ATT_OP_WRITE_REQ;
    htole16(u8ptr + 1, req->bhawq_handle);

    return 0;
}
