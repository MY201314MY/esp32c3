# Copyright (C) Telit Communications S.p.A. Italy - All Rights Reserved.
# See LICENSE file in the project root for full license information.
# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(CONFIG_MODEM_TELIT_CELLULAR_MODEM tc_modem.c)
