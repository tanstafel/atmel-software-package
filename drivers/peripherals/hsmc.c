/* ----------------------------------------------------------------------------
 *         SAM Software Package License
 * ----------------------------------------------------------------------------
 * Copyright (c) 2015, Atmel Corporation
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the disclaimer below.
 *
 * Atmel's name may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * DISCLAIMER: THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ----------------------------------------------------------------------------
 */

/**
  *  \file
  *
  *  Implementation of HSMC functions.
  */

/*----------------------------------------------------------------------------
 *        Headers
 *----------------------------------------------------------------------------*/

#include "chip.h"
#include "trace.h"

#include "peripherals/pmc.h"
#include "peripherals/hsmc.h"

#include <assert.h>

/*----------------------------------------------------------------------------
 *        Local functions
 *----------------------------------------------------------------------------*/

/* Reading the NFC Command Register (to any address) will give the status of
 * the NFC. */
static uint32_t _nfc_read_status(void)
{
	return *(volatile uint32_t*)(NFC_ADDR);
}

static void _nfc_write_cmd(uint32_t cmd, uint32_t value)
{
	*(volatile uint32_t*)(NFC_ADDR + cmd) = value;
}

/*----------------------------------------------------------------------------
 *        Exported functions
 *----------------------------------------------------------------------------*/

/**
 * \brief Sets SMC timing for NAND FLASH.
 * \param bus_width  bus width 8/16.
 */
void hsmc_nand_configure(uint8_t bus_width)
{
	pmc_enable_peripheral(ID_HSMC);

	HSMC->SMC_CS_NUMBER[NAND_EBI_CS].HSMC_SETUP =
		HSMC_SETUP_NWE_SETUP(2) |
		HSMC_SETUP_NCS_WR_SETUP(2) |
		HSMC_SETUP_NRD_SETUP(2) |
		HSMC_SETUP_NCS_RD_SETUP(2);

	HSMC->SMC_CS_NUMBER[NAND_EBI_CS].HSMC_PULSE =
		HSMC_PULSE_NWE_PULSE(7) |
		HSMC_PULSE_NCS_WR_PULSE(7) |
		HSMC_PULSE_NRD_PULSE(7) |
		HSMC_PULSE_NCS_RD_PULSE(7);

	HSMC->SMC_CS_NUMBER[NAND_EBI_CS].HSMC_CYCLE =
		HSMC_CYCLE_NWE_CYCLE(13) |
		HSMC_CYCLE_NRD_CYCLE(13);

	HSMC->SMC_CS_NUMBER[NAND_EBI_CS].HSMC_TIMINGS =
		HSMC_TIMINGS_TCLR(3) |
		HSMC_TIMINGS_TADL(27) |
		HSMC_TIMINGS_TAR(3) |
		HSMC_TIMINGS_TRR(6) |
		HSMC_TIMINGS_TWB(5) |
		HSMC_TIMINGS_NFSEL;

	HSMC->SMC_CS_NUMBER[NAND_EBI_CS].HSMC_MODE =
		HSMC_MODE_READ_MODE |
		HSMC_MODE_WRITE_MODE |
		((bus_width == 8 ) ? HSMC_MODE_DBW_BIT_8 : HSMC_MODE_DBW_BIT_16) |
		HSMC_MODE_TDF_CYCLES(1);
}

/**
 * \brief Sets SMC timing for NOR FLASH.
 * \param cs  chip select.
 * \param bus_width  bus width 8/16.
 */
void hsmc_nor_configure(uint8_t cs, uint8_t bus_width)
{
	pmc_enable_peripheral(ID_HSMC);

	HSMC->SMC_CS_NUMBER[cs].HSMC_SETUP =
		HSMC_SETUP_NWE_SETUP(1) |
		HSMC_SETUP_NCS_WR_SETUP(0) |
		HSMC_SETUP_NRD_SETUP(2) |
		HSMC_SETUP_NCS_RD_SETUP(0);

	HSMC->SMC_CS_NUMBER[cs].HSMC_PULSE =
		HSMC_PULSE_NWE_PULSE(10) |
		HSMC_PULSE_NCS_WR_PULSE(10) |
		HSMC_PULSE_NRD_PULSE(11) |
		HSMC_PULSE_NCS_RD_PULSE(11);

	HSMC->SMC_CS_NUMBER[cs].HSMC_CYCLE =
		HSMC_CYCLE_NWE_CYCLE(11) |
		HSMC_CYCLE_NRD_CYCLE(14);

	HSMC->SMC_CS_NUMBER[cs].HSMC_TIMINGS = 0;

	HSMC->SMC_CS_NUMBER[cs].HSMC_MODE =
		HSMC_MODE_READ_MODE |
		HSMC_MODE_WRITE_MODE |
		(bus_width == 8 ? HSMC_MODE_DBW_BIT_8 : HSMC_MODE_DBW_BIT_16) |
		HSMC_MODE_EXNW_MODE_DISABLED |
		HSMC_MODE_TDF_CYCLES(1);
}

void hsmc_nfc_configure(uint32_t data_size, uint32_t spare_size,
		bool read_spare, bool write_spare)
{
	uint32_t cfg;

	/* cannot read and write spare at the same time */
	assert(!read_spare || !write_spare);

	cfg = HSMC_CFG_NFCSPARESIZE((spare_size - 1) >> 2) |
	      HSMC_CFG_DTOCYC(0xF) |
	      HSMC_CFG_DTOMUL_X1048576 |
	      HSMC_CFG_RBEDGE;

	if (read_spare)
		cfg |= HSMC_CFG_RSPARE;

	if (write_spare)
		cfg |= HSMC_CFG_WSPARE;

	switch (data_size) {
	case 512:
		cfg |= HSMC_CFG_PAGESIZE_PS512;
		break;
	case 1024:
		cfg |= HSMC_CFG_PAGESIZE_PS1024;
		break;
	case 2048:
		cfg |= HSMC_CFG_PAGESIZE_PS2048;
		break;
	case 4096:
		cfg |= HSMC_CFG_PAGESIZE_PS4096;
		break;
#ifdef HSMC_CFG_PAGESIZE_PS8192
	case 8192:
		cfg |= HSMC_CFG_PAGESIZE_PS8192;
		break;
#endif
	default:
		trace_fatal("Data size %d unsupported!\r\n",
				(unsigned)data_size);
	}

	HSMC->HSMC_CFG = cfg;
}

/**
 * \brief Reset NFC controller.
 */
void hsmc_nfc_reset(void)
{
	/* Disable all the SMC NFC interrupts */
	HSMC->HSMC_IDR = 0xFFFFFFFF;
	HSMC->HSMC_CTRL = 0;
}

/**
 * \brief Check if spare area be read in read mode.
 *
 * \return Returns true if NFC controller reads both main and spare area in
 *         read mode, otherwise returns false.
 */
bool hsmc_nfc_is_spare_read_enabled(void)
{
	return (((HSMC->HSMC_CFG) >> 9) & 0x1) != 0;
}

/**
 * \brief Check if spare area be written in write mode.
 *
 * \return Returns true if NFC controller writes both main and spare area in
 *         write mode, otherwise returns false.
 */
bool hsmc_nfc_is_spare_write_enabled(void)
{
	return (((HSMC->HSMC_CFG) >> 8) & 0x1) != 0;
}

/**
 * \brief Check if NFC Controller is busy.
 *
 * \return Returns 1 if NFC Controller is activated and accesses the memory device,
 *         otherwise returns 0.
 */
bool hsmc_nfc_is_nfc_busy(void)
{
	return ((HSMC->HSMC_SR & HSMC_SR_NFCBUSY) == HSMC_SR_NFCBUSY);
}

/**
 * \brief Check if the host controller is busy.
 * \return Returns 1 if the host controller is busy, otherwise returns 0.
 */
static bool smc_nfc_is_host_busy(void)
{
	return (_nfc_read_status() & NFCDATA_STATUS_NFCBUSY) == NFCDATA_STATUS_NFCBUSY;
}

/**
 * \brief Wait for Ready-busy pin falling and then rising.
 */
void hsmc_wait_rb(void)
{
	/* Wait for RB pin falling */
	while ((HSMC->HSMC_SR & HSMC_SR_RB_FALL) != HSMC_SR_RB_FALL);

	/* Wait for RB pin rising */
	while ((HSMC->HSMC_SR & HSMC_SR_RB_RISE) != HSMC_SR_RB_RISE);
}

/**
 * \brief Wait for NFC command has done.
 */
void hsmc_nfc_wait_cmd_done(void)
{
	while ((HSMC->HSMC_SR & HSMC_SR_CMDDONE) != HSMC_SR_CMDDONE);
}

/**
 * \brief Wait for NFC Data Transfer Terminated.
 */
void hsmc_nfc_wait_xfr_done(void)
{
	while ((HSMC->HSMC_SR & HSMC_SR_XFRDONE) != HSMC_SR_XFRDONE);
}

/**
 * \brief Wait for NFC Ready/Busy Line 0 Edge Detected.
 */
void hsmc_nfc_wait_rb_busy(void)
{
	while ((HSMC->HSMC_SR & HSMC_SR_RB_EDGE0) != HSMC_SR_RB_EDGE0);
}

/**
 * \brief Wait for PMECC ready.
 */
void hsmc_pmecc_wait_ready(void)
{
	while((HSMC->HSMC_PMECCSR) & HSMC_PMECCSR_BUSY);
}

/**
 * \brief Uses the HOST NANDFLASH controller to send a command to the NFC.
 * \param cmd  command to send.
 * \param address_cycle address cycle when command access id decoded.
 * \param cycle0 address at first cycle.
 */
void hsmc_nfc_send_cmd(uint32_t cmd, uint8_t *cycle_bytes)
{
	int cycle_offset = 0;
	uint32_t nfcdata_addr = 0;

	/* Wait until host controller is not busy. */
	while (smc_nfc_is_host_busy());

	/* Send the command */
	switch (cmd & NFCADDR_CMD_ACYCLE_Msk) {
	case NFCADDR_CMD_ACYCLE_FIVE:
		HSMC->HSMC_ADDR = cycle_bytes[0];
		cycle_offset = 1;
		// fall-through
	case NFCADDR_CMD_ACYCLE_FOUR:
		nfcdata_addr |= NFCDATA_ADDR_ACYCLE4(cycle_bytes[cycle_offset + 3]);
		// fall-through
	case NFCADDR_CMD_ACYCLE_THREE:
		nfcdata_addr |= NFCDATA_ADDR_ACYCLE3(cycle_bytes[cycle_offset + 2]);
		// fall-through
	case NFCADDR_CMD_ACYCLE_TWO:
		nfcdata_addr |= NFCDATA_ADDR_ACYCLE2(cycle_bytes[cycle_offset + 1]);
		// fall-through
	case NFCADDR_CMD_ACYCLE_ONE:
		nfcdata_addr |= NFCDATA_ADDR_ACYCLE1(cycle_bytes[cycle_offset + 0]);
		break;
	case NFCADDR_CMD_ACYCLE_NONE:
		break;
	default:
		assert(0);
		break;
	}
	_nfc_write_cmd(cmd, nfcdata_addr);

	/* Wait for command completion */
	hsmc_nfc_wait_cmd_done();
}
