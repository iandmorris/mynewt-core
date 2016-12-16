/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <syscfg/syscfg.h>
#if (MYNEWT_VAL(OC_TRANSPORT_GATT) == 1)
#include <assert.h>
#include <os/os.h>
#include <string.h>

#include <stats/stats.h>
#include "oic/oc_gatt.h"
#include "oic/oc_log.h"
#include "messaging/coap/coap.h"
#include "api/oc_buffer.h"
#include "port/oc_connectivity.h"
#include "adaptor.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

/* OIC Transport Profile GATT */

/* service UUID */
/* ADE3D529-C784-4F63-A987-EB69F70EE816 */
const uint8_t oc_gatt_svc_uuid[16] = {
    0x16, 0xe8, 0x0e, 0xf7, 0x69, 0xeb, 0x87, 0xa9,
    0x63, 0x4f, 0x84, 0xc7, 0x29, 0xd5, 0xe3, 0xad
};

/* request characteristic UUID */
/* AD7B334F-4637-4B86-90B6-9D787F03D218 */
static const uint8_t oc_gatt_req_chr_uuid[16] = {
    0x18, 0xd2, 0x03, 0x7f, 0x78, 0x9d, 0xb6, 0x90,
    0x86, 0x4b, 0x37, 0x46, 0x4f, 0x33, 0x7b, 0xad
};

/* response characteristic UUID */
/* E9241982-4580-42C4-8831-95048216B256 */
static const uint8_t oc_gatt_rsp_chr_uuid[16] = {
    0x56, 0xb2, 0x16, 0x82, 0x04, 0x95, 0x31, 0x88,
    0xc4, 0x42, 0x80, 0x45, 0x82, 0x19, 0x24, 0xe9
};

STATS_SECT_START(oc_ble_stats)
    STATS_SECT_ENTRY(iseg)
    STATS_SECT_ENTRY(ibytes)
    STATS_SECT_ENTRY(ierr)
    STATS_SECT_ENTRY(oseg)
    STATS_SECT_ENTRY(obytes)
    STATS_SECT_ENTRY(oerr)
STATS_SECT_END
static STATS_SECT_DECL(oc_ble_stats) oc_ble_stats;
STATS_NAME_START(oc_ble_stats)
    STATS_NAME(oc_ble_stats, iseg)
    STATS_NAME(oc_ble_stats, ibytes)
    STATS_NAME(oc_ble_stats, ierr)
    STATS_NAME(oc_ble_stats, oseg)
    STATS_NAME(oc_ble_stats, obytes)
    STATS_NAME(oc_ble_stats, oerr)
STATS_NAME_END(oc_ble_stats)

static STAILQ_HEAD(, os_mbuf_pkthdr) oc_ble_reass_q;

#if (MYNEWT_VAL(OC_SERVER) == 1)
/* ble nmgr attr handle */
static uint16_t oc_ble_coap_req_handle;
static uint16_t oc_ble_coap_rsp_handle;

static int oc_gatt_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                   struct ble_gatt_access_ctxt *ctxt, void *arg);

static const struct ble_gatt_svc_def gatt_svr_svcs[] = { {
        /* Service: newtmgr */
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid128 = (void *)oc_gatt_svc_uuid,
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                /* Characteristic: Request */
                .uuid128 = (void *)oc_gatt_req_chr_uuid,
                .access_cb = oc_gatt_chr_access,
                .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_NO_RSP |
                         BLE_GATT_CHR_F_NOTIFY,
                .val_handle = &oc_ble_coap_req_handle,
            },{
                /* Characteristic: Response */
                .uuid128 = (void *)oc_gatt_rsp_chr_uuid,
                .access_cb = oc_gatt_chr_access,
                .flags = BLE_GATT_CHR_F_NOTIFY,
                .val_handle = &oc_ble_coap_rsp_handle,
            },{
                0, /* No more characteristics in this service */
            }
        },
    },
    {
        0, /* No more services */
    },
};

int
oc_ble_reass(struct os_mbuf *om1, uint16_t conn_handle)
{
    struct os_mbuf_pkthdr *pkt1;
    struct oc_endpoint_ble *oe_ble;
    struct os_mbuf *om2;
    struct os_mbuf_pkthdr *pkt2;
    uint8_t hdr[6]; /* sizeof(coap_tcp_hdr32) */

    pkt1 = OS_MBUF_PKTHDR(om1);
    assert(pkt1);

    STATS_INC(oc_ble_stats, iseg);
    STATS_INCN(oc_ble_stats, ibytes, pkt1->omp_len);

    OC_LOG_DEBUG("oc_gatt rx seg %u-%x-%u\n", conn_handle,
                 (unsigned)pkt1, pkt1->omp_len);

    STAILQ_FOREACH(pkt2, &oc_ble_reass_q, omp_next) {
        om2 = OS_MBUF_PKTHDR_TO_MBUF(pkt2);
        oe_ble = (struct oc_endpoint_ble *)OC_MBUF_ENDPOINT(om2);
        if (conn_handle == oe_ble->conn_handle) {
            /*
             * Data from same connection. Append.
             */
            os_mbuf_concat(om2, om1);
            os_mbuf_copydata(om2, 0, sizeof(hdr), hdr);

            if (coap_tcp_msg_size(hdr, sizeof(hdr)) <= pkt2->omp_len) {
                STAILQ_REMOVE(&oc_ble_reass_q, pkt2, os_mbuf_pkthdr, omp_next);
                oc_recv_message(om2);
            }
            pkt1 = NULL;
            break;
        }
    }
    if (pkt1) {
        /*
         * New frame, need to add oc_endpoint_ble in the front.
         * Check if there is enough space available. If not, allocate a
         * new pkthdr.
         */
        if (OS_MBUF_USRHDR_LEN(om1) < sizeof(struct oc_endpoint_ble)) {
            om2 = os_msys_get_pkthdr(0, sizeof(struct oc_endpoint_ble));
            if (!om2) {
                OC_LOG_ERROR("oc_gatt_rx: Could not allocate mbuf\n");
                STATS_INC(oc_ble_stats, ierr);
                return -1;
            }
            OS_MBUF_PKTHDR(om2)->omp_len = pkt1->omp_len;
            SLIST_NEXT(om2, om_next) = om1;
        } else {
            om2 = om1;
        }
        oe_ble = (struct oc_endpoint_ble *)OC_MBUF_ENDPOINT(om2);
        oe_ble->flags = GATT;
        oe_ble->conn_handle = conn_handle;
        pkt2 = OS_MBUF_PKTHDR(om2);

        if (os_mbuf_copydata(om2, 0, sizeof(hdr), hdr) ||
          coap_tcp_msg_size(hdr, sizeof(hdr)) > pkt2->omp_len) {
            STAILQ_INSERT_TAIL(&oc_ble_reass_q, pkt2, omp_next);
        } else {
            oc_recv_message(om2);
        }
    }
    return 0;
}

static int
oc_gatt_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                   struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    struct os_mbuf *m;
    int rc;
    (void) attr_handle; /* xxx req should only come in via req handle */

    switch (ctxt->op) {
    case BLE_GATT_ACCESS_OP_WRITE_CHR:
        m = ctxt->om;

        rc = oc_ble_reass(m, conn_handle);
        if (rc) {
            return BLE_ATT_ERR_INSUFFICIENT_RES;
        }

        /* tell nimble we are keeping the mbuf */
        ctxt->om = NULL;

        break;
    default:
        assert(0);
        return BLE_ATT_ERR_UNLIKELY;
    }
    return 0;
}
#endif

int
oc_ble_coap_gatt_srv_init(void)
{
#if (MYNEWT_VAL(OC_SERVER) == 1)
    int rc;

    rc = ble_gatts_count_cfg(gatt_svr_svcs);
    if (rc != 0) {
        return rc;
    }

    rc = ble_gatts_add_svcs(gatt_svr_svcs);
    if (rc != 0) {
        return rc;
    }
#endif
    stats_init_and_reg(STATS_HDR(oc_ble_stats),
      STATS_SIZE_INIT_PARMS(oc_ble_stats, STATS_SIZE_32),
      STATS_NAME_INIT_PARMS(oc_ble_stats), "oc_ble");
    return 0;
}

void
oc_ble_coap_conn_new(uint16_t conn_handle)
{
    OC_LOG_DEBUG("oc_gatt newconn %x\n", conn_handle);
}

void
oc_ble_coap_conn_del(uint16_t conn_handle)
{
    struct os_mbuf_pkthdr *pkt;
    struct os_mbuf *m;
    struct oc_endpoint_ble *oe_ble;

    OC_LOG_DEBUG("oc_gatt endconn %x\n", conn_handle);
    STAILQ_FOREACH(pkt, &oc_ble_reass_q, omp_next) {
        m = OS_MBUF_PKTHDR_TO_MBUF(pkt);
        oe_ble = (struct oc_endpoint_ble *)OC_MBUF_ENDPOINT(m);
        if (oe_ble->conn_handle == conn_handle) {
            STAILQ_REMOVE(&oc_ble_reass_q, pkt, os_mbuf_pkthdr, omp_next);
            os_mbuf_free_chain(m);
            break;
        }
    }
}

int
oc_connectivity_init_gatt(void)
{
    STAILQ_INIT(&oc_ble_reass_q);
    return 0;
}

void
oc_connectivity_shutdown_gatt(void)
{
    /* there is not unregister for BLE */
}

static int
oc_ble_frag(struct os_mbuf *m, uint16_t mtu)
{
    struct os_mbuf_pkthdr *pkt;
    struct os_mbuf *n;
    uint16_t off, blk;

    pkt = OS_MBUF_PKTHDR(m);
    if (pkt->omp_len <= mtu) {
        STAILQ_NEXT(pkt, omp_next) = NULL;
        return 0;
    }
    off = pkt->omp_len % mtu;

    while (off > mtu) {
        n = os_msys_get_pkthdr(mtu, 0);
        if (!n) {
            goto err;
        }
        STAILQ_NEXT(OS_MBUF_PKTHDR(n), omp_next) = STAILQ_NEXT(pkt, omp_next);
        STAILQ_NEXT(pkt, omp_next) = OS_MBUF_PKTHDR(n);

        blk = pkt->omp_len - off;
        if (os_mbuf_appendfrom(n, m, off, blk)) {
            goto err;
        }
        off -= blk;
        os_mbuf_adj(m, -blk);
    }
    os_mbuf_adj(m, mtu - OS_MBUF_PKTLEN(m));
    return 0;
err:
    pkt = OS_MBUF_PKTHDR(m);
    while (1) {
        pkt = STAILQ_NEXT(pkt, omp_next);
        os_mbuf_free_chain(m);
        if (!pkt) {
            break;
        }
        m = OS_MBUF_PKTHDR_TO_MBUF(pkt);
    };
    return -1;
}

void
oc_send_buffer_gatt(struct os_mbuf *m)
{
    struct oc_endpoint *oe;
    struct os_mbuf_pkthdr *pkt;
    uint16_t mtu;
    uint16_t conn_handle;

    assert(OS_MBUF_USRHDR_LEN(m) >= sizeof(struct oc_endpoint_ble));
    oe = OC_MBUF_ENDPOINT(m);
    conn_handle = oe->oe_ble.conn_handle;

#if (MYNEWT_VAL(OC_CLIENT) == 1)
    OC_LOG_ERROR("oc_gatt send not supported on client");
#endif

#if (MYNEWT_VAL(OC_SERVER) == 1)

    STATS_INC(oc_ble_stats, oseg);
    STATS_INCN(oc_ble_stats, obytes, OS_MBUF_PKTLEN(m));

    mtu = ble_att_mtu(conn_handle);
    assert(mtu > 4);
    mtu -= 3; /* # of bytes for ATT notification base */

    if (oc_ble_frag(m, mtu)) {
        STATS_INC(oc_ble_stats, oerr);
        return;
    }
    while (1) {
        pkt = STAILQ_NEXT(OS_MBUF_PKTHDR(m), omp_next);

        ble_gattc_notify_custom(conn_handle, oc_ble_coap_rsp_handle, m);
        if (pkt) {
            m = OS_MBUF_PKTHDR_TO_MBUF(pkt);
        } else {
            break;
        }
    }
#endif
}

#endif
