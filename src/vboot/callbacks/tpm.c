/*
 * Copyright 2012 Google Inc.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but without any warranty; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <libpayload.h>
#include <vboot_api.h>

#include "drivers/tpm/tpm.h"

VbError_t VbExTpmInit(void)
{
	return VBERROR_SUCCESS;
}

VbError_t VbExTpmClose(void)
{
	return VBERROR_SUCCESS;
}

VbError_t VbExTpmOpen(void)
{
	return VBERROR_SUCCESS;
}

VbError_t VbExTpmSendReceive(const uint8_t *request, uint32_t request_length,
			     uint8_t *response, uint32_t *response_length)
{
	if (tpm_xmit(request, request_length, response, response_length))
		return VBERROR_UNKNOWN;
	return VBERROR_SUCCESS;
}
