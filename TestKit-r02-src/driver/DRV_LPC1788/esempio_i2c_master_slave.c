
/*********************************************************************//**
 * @brief		Enable/Disable I2C monitor mode
 * @param[in]	I2Cx	I2C peripheral selected, should be
 *				- LPC_I2C0
 *				- LPC_I2C1
 *				- LPC_I2C2
 * @param[in]	NewState New State of this function, should be:
 * 				- ENABLE: Enable monitor mode.
 * 				- DISABLE: Disable monitor mode.
 * @return		None
 **********************************************************************/
void I2C_MonitorModeCmd(en_I2C_unitId i2cId, FunctionalState NewState)
{
	LPC_I2C_TypeDef* I2Cx = I2C_GetPointer(i2cId);

	if (NewState == ENABLE)
	{
		I2Cx->MMCTRL |= I2C_I2MMCTRL_MM_ENA;
		I2Cx->CONSET = I2C_I2CONSET_AA;
		I2Cx->CONCLR = I2C_I2CONCLR_SIC | I2C_I2CONCLR_STAC;
	}
	else
	{
		I2Cx->MMCTRL &= (~I2C_I2MMCTRL_MM_ENA) & I2C_I2MMCTRL_BITMASK;
		I2Cx->CONCLR = I2C_I2CONCLR_SIC | I2C_I2CONCLR_STAC | I2C_I2CONCLR_AAC;
	}

	I2C_MonitorBufferIndex = 0;
}


/*********************************************************************//**
 * @brief		Get data from I2C data buffer in monitor mode.
 * @param[in]	I2Cx	I2C peripheral selected, should be
 *				- LPC_I2C0
 *				- LPC_I2C1
 *				- LPC_I2C2
 * @return		None
 * Note:	In monitor mode, the I2C module may lose the ability to stretch
 * the clock (stall the bus) if the ENA_SCL bit is not set. This means that
 * the processor will have a limited amount of time to read the contents of
 * the data received on the bus. If the processor reads the DAT shift
 * register, as it ordinarily would, it could have only one bit-time to
 * respond to the interrupt before the received data is overwritten by
 * new data.
 **********************************************************************/
uint8_t I2C_MonitorGetDatabuffer(en_I2C_unitId i2cId)
{
	LPC_I2C_TypeDef* I2Cx = I2C_GetPointer(i2cId);

	return ((uint8_t)(I2Cx->DATA_BUFFER));
}

/*********************************************************************//**
 * @brief		Get data from I2C data buffer in monitor mode.
 * @param[in]	I2Cx	I2C peripheral selected, should be
 *				- LPC_I2C0
 *				- LPC_I2C1
 *				- LPC_I2C2
 * @return		None
 * Note:	In monitor mode, the I2C module may lose the ability to stretch
 * the clock (stall the bus) if the ENA_SCL bit is not set. This means that
 * the processor will have a limited amount of time to read the contents of
 * the data received on the bus. If the processor reads the DAT shift
 * register, as it ordinarily would, it could have only one bit-time to
 * respond to the interrupt before the received data is overwritten by
 * new data.
 **********************************************************************/
BOOL_8 I2C_MonitorHandler(en_I2C_unitId i2cId, uint8_t *buffer, uint32_t size)
{
	LPC_I2C_TypeDef* I2Cx = I2C_GetPointer(i2cId);

	BOOL_8 ret=FALSE;

	I2Cx->CONCLR = I2C_I2CONCLR_SIC;

	buffer[I2C_MonitorBufferIndex] = (uint8_t)(I2Cx->DATA_BUFFER);

	I2C_MonitorBufferIndex++;

	if(I2C_MonitorBufferIndex >= size)
	{
		ret = TRUE;
	}
	return ret;
}
/*********************************************************************//**
 * @brief 		Get status of Master Transfer
 * @param[in]	I2Cx	I2C peripheral selected, should be:
 *				- LPC_I2C0
 *				- LPC_I2C1
 *				- LPC_I2C2
 * @return 		Master transfer status, could be:
 * 				- TRUE	master transfer completed
 * 				- FALSE master transfer have not completed yet
 **********************************************************************/
uint32_t I2C_MasterTransferComplete(en_I2C_unitId i2cId)
{
	uint32_t retval;

	retval = I2C_MasterComplete[i2cId];

	I2C_MasterComplete[i2cId] = FALSE;

	return retval;
}

/*********************************************************************//**
 * @brief 		Get status of Slave Transfer
 * @param[in]	I2Cx	I2C peripheral selected, should be:
 *				- LPC_I2C0
 *				- LPC_I2C1
 *				- LPC_I2C2
 * @return 		Complete status, could be: TRUE/FALSE
 **********************************************************************/
uint32_t I2C_SlaveTransferComplete(en_I2C_unitId i2cId)
{
	uint32_t retval;

	retval = I2C_SlaveComplete[i2cId];

	I2C_SlaveComplete[i2cId] = FALSE;

	return retval;
}

#endif /*_I2C*/

/**
 * @}
 */


/**
 * @}
 */

/* --------------------------------- End Of File ------------------------------ */

