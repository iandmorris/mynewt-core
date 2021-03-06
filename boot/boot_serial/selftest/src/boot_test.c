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
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "os/mynewt.h"
#include "base64/base64.h"
#include "crc/crc16.h"
#include "testutil/testutil.h"
#include "hal/hal_flash.h"
#include "flash_map/flash_map.h"
#include "boot_test.h"

#include "boot_serial_priv.h"

void
tx_msg(void *src, int len)
{
    boot_serial_input(src, len);
}

TEST_SUITE(boot_serial_suite)
{
    boot_serial_setup();
    boot_serial_empty_msg();
    boot_serial_empty_img_msg();
    boot_serial_img_msg();
    boot_serial_upload_bigger_image();
}

int
main(void)
{
    boot_serial_suite();
    return tu_any_failed;
}
