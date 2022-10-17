#include <Cross Platform Shim\compat.h>
#include <spb.h>
#include <controller.h>
#include <tcm/touch_tcm.h>
#include <trace.h>
#include <touch_tcm.tmh>

static void WaitForEvent(PKEVENT event, LONGLONG timeout_ms)
{
	if (event == NULL) return;

	LARGE_INTEGER Timeout;
	Timeout.QuadPart = WDF_REL_TIMEOUT_IN_MS(timeout_ms);
	KeWaitForSingleObject(
		event,
		Executive,
		KernelMode,
		FALSE,
		&Timeout
	);
}

static void SignalEvent(PKEVENT event)
{
	if (event) {
		KeSetEvent(event, 0, FALSE);
	}
}

NTSTATUS
TcmServiceInterrupts(
	IN TCM_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext,
	IN PREPORT_CONTEXT ReportContext
)
{
	NTSTATUS status = STATUS_SUCCESS;
	ControllerContext->ISRCount++;
	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_INTERRUPT,
		"ISRCount = %d",
		ControllerContext->ISRCount);
	if(ControllerContext->ControllerState.Init == TRUE)
		status = TcmReadMessage(ControllerContext,
							SpbContext,
							ReportContext);

	return status;
}

NTSTATUS
TcmReadMessage(
	IN TCM_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext,
	IN PREPORT_CONTEXT ReportContext
)
{
	NTSTATUS status = STATUS_NO_DATA_DETECTED;

	//
	// Grab a waitlock to ensure the ISR executes serially and is 
	// protected against power state transitions
	//
	WdfWaitLockAcquire(ControllerContext->ControllerLock, NULL);

	TCM_MSG_HEADER* messageHeader = NULL;
	UINT8 *payloadPtr = NULL, *payloadData = NULL;
	int readLength = 0;

	messageHeader = ExAllocatePoolWithTag(
		NonPagedPoolNx,
		MESSAGE_HEADER_SIZE,
		TOUCH_POOL_TAG_MSG
	);

	if (messageHeader == NULL)
	{
		status = STATUS_INSUFFICIENT_RESOURCES;
		goto exit;
	}

	status = SpbReadContinuedData(
		SpbContext,
		messageHeader,
		MESSAGE_HEADER_SIZE
	);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_SAMPLES,
			"Could not read message header- %X",
			status);

		goto free_buffer;
	}

	if (messageHeader->Marker != MESSAGE_MARKER) {
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_SAMPLES,
			"Invalid message header marker- 0x%x",
			messageHeader->Marker);
		status = STATUS_NO_DATA_DETECTED;
		goto free_buffer;
	}

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_SAMPLES,
		"Message header code = 0x%02x, Payload length = %d",
		messageHeader->Code,
		messageHeader->Length);

	if (messageHeader->Code <= TCM_STATUS_ERROR
		|| messageHeader->Code == TCM_STATUS_INVALID) {
		switch (messageHeader->Code) {
		case TCM_STATUS_OK:
			break;
		case TCM_STATUS_CONTINUED_READ:
			Trace(
				TRACE_LEVEL_VERBOSE,
				TRACE_SAMPLES,
				"Out-of-sync continued read");
		case TCM_STATUS_IDLE:
		case TCM_STATUS_BUSY:
			goto free_buffer;
			break;
		default:
			if(messageHeader->Code == TCM_STATUS_INVALID) {
				messageHeader->Length = 0;
			}
			break;
		}
	}

	readLength = messageHeader->Length + 3;

	payloadData = ExAllocatePoolWithTag(
		NonPagedPoolNx,
		readLength + 4,
		TOUCH_POOL_TAG_MSG
	);

	if (payloadData == NULL)
	{
		status = STATUS_INSUFFICIENT_RESOURCES;
		goto free_buffer;
	}
	RtlZeroMemory(payloadData, readLength + 4);
	RtlCopyMemory(payloadData, messageHeader, MESSAGE_HEADER_SIZE);
	payloadPtr = &(payloadData[4]);

	if (messageHeader->Length == 0) {
		payloadPtr[readLength - 1] = MESSAGE_PADDING;
	}
	else {
		status = SpbReadContinuedData(
			SpbContext,
			payloadPtr,
			readLength
		);

		if (!NT_SUCCESS(status))
		{
			Trace(
				TRACE_LEVEL_ERROR,
				TRACE_SAMPLES,
				"Could not read message payload- %X",
				status);

			goto free_buffer;
		}

		if (payloadPtr[0] != MESSAGE_MARKER || payloadPtr[1] != TCM_STATUS_CONTINUED_READ) {
			Trace(
				TRACE_LEVEL_ERROR,
				TRACE_SAMPLES,
				"Incorrect continued read header marker/code(0x%02x/0x%02x)",
				payloadPtr[0], payloadPtr[1]);
			status = STATUS_NO_DATA_DETECTED;
			goto free_buffer;
		}

		payloadPtr += 2;
		readLength -= 2;
	}

	UINT8 temp = payloadPtr[readLength - 1];

	if (temp != MESSAGE_PADDING) {
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_SAMPLES,
			"Incorrect message padding byte: 0x%02x",
			temp);
		status = STATUS_NO_DATA_DETECTED;
		goto free_buffer;
	}

	if (messageHeader->Code >= TCM_REPORT_IDENTIFY) {
		ControllerContext->ReportCode = messageHeader->Code;
		switch(messageHeader->Code) {
			case TCM_REPORT_TOUCH:
				if(ControllerContext->ControllerState.Init == TRUE) {
					status = TcmDispatchReport(ControllerContext,
											ReportContext,
											messageHeader,
											payloadPtr,
											readLength);
				} else {
					Trace(
						TRACE_LEVEL_ERROR,
						TRACE_SAMPLES,
						"Received report before initialization, report_code:0x%x",
						messageHeader->Code);
				}
				
				break;
			case TCM_REPORT_IDENTIFY:
				ControllerContext->ControllerState.Power = TCM_POWER_ON;
				if (readLength < sizeof(TCM_ID_INFO)) {
					Trace(
						TRACE_LEVEL_ERROR,
						TRACE_SAMPLES,
						"Received ID Info smaller than buffer");
					status = STATUS_INVALID_PARAMETER;
					goto free_buffer;
				}
				RtlCopyMemory(&ControllerContext->IDInfo, payloadPtr, sizeof(TCM_ID_INFO));
				ControllerContext->ChunkSize = MIN(ControllerContext->IDInfo.MaxWriteSize, DEFAULT_CHUNK_SIZE);
				
				Trace(
					TRACE_LEVEL_ERROR,
					TRACE_SAMPLES,
					"Received identify report (firmware mode = 0x%02x)",
					ControllerContext->IDInfo.Mode);
				
				if(ControllerContext->CommandStatus == CMD_BUSY) {
					switch(ControllerContext->CurrentCommand) {
						case CMD_RESET:
						case CMD_RUN_BOOTLOADER_FIRMWARE:
						case CMD_RUN_APPLICATION_FIRMWARE:
						case CMD_ENTER_PRODUCTION_TEST_MODE:
						case CMD_ROMBOOT_RUN_BOOTLOADER_FIRMWARE:
							ControllerContext->CurrentResponse = TCM_STATUS_OK;
							ControllerContext->CommandStatus = CMD_IDLE;
							break;
						default:
							ControllerContext->CommandStatus = CMD_ERROR;
							break;
					}
				}
				if(ControllerContext->ControllerState.Init == TRUE) {
					Trace(
						TRACE_LEVEL_ERROR,
						TRACE_SAMPLES,
						"Received identify report with Init done state (IC reset occured, need reinit)");
					ControllerContext->ControllerState.Init = FALSE;
				}
				break;
			case TCM_REPORT_RAW:
			case TCM_REPORT_DELTA:
				// TODO
			default:
				break;
		}
	}
	else { // Response
		ControllerContext->ResponseCode = messageHeader->Code;
		status = TcmDispatchResponse(ControllerContext,
								ReportContext,
								messageHeader,
								payloadPtr,
								readLength);
		if(!NT_SUCCESS(status)) {
			Trace(
				TRACE_LEVEL_ERROR,
				TRACE_SAMPLES,
				"Failed to dispatch response");
			if(ControllerContext->CommandStatus == CMD_BUSY)
				ControllerContext->CommandStatus = CMD_ERROR;
		}
	}

free_buffer:
	if (messageHeader != NULL) {
		ExFreePoolWithTag(
			messageHeader,
			TOUCH_POOL_TAG_MSG
		);
	}

exit:
	WdfWaitLockRelease(ControllerContext->ControllerLock);
	return status;
}

NTSTATUS
TcmDispatchReport(
	IN TCM_CONTROLLER_CONTEXT* ControllerContext,
	IN PREPORT_CONTEXT ReportContext,
	IN TCM_MSG_HEADER* MessageHeader,
	_In_reads_bytes_(PayloadLength) PVOID Payload,
	IN ULONG PayloadLength
)
{	
	NTSTATUS Status = STATUS_SUCCESS;
	DETECTED_OBJECTS data;
	UINT8 *ConfigData = ControllerContext->ConfigData.Buffer;
	ULONG Index = 0, Next = 0, EndOfForeach = 0, BufIdx = 0;
	ULONG BitsToRead = 0, BitsOffset = 0;
	ULONG ObjectIndex = 0, ActiveObjectsNum = 0, ActiveObjectsIndex = 0;
	UINT32 DataByte = 0;
	UINT8 Code = 0;
	INT32 DataInt = 0;
	BOOLEAN ActiveOnly = FALSE, NeedReport = FALSE;
	
	(void*)MessageHeader;
	(void*)ControllerContext;

	if (ControllerContext->ConfigData.DataLength <= 0) {
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_DRIVER,
			"Report config data not ready");
		return STATUS_UNSUCCESSFUL;
	}

	RtlZeroMemory(&data, sizeof(data));

	while(Index < ControllerContext->ConfigData.DataLength) {
		Trace(
				TRACE_LEVEL_VERBOSE,
				TRACE_SAMPLES,
				"Packet: ConfigData[%d]=0x%x, [%d]=%dbits",
				Index, ConfigData[Index],
				Index + 1, ConfigData[Index + 1]);

		Code = ConfigData[Index];
		Index++;
		switch (Code) {
			case TOUCH_END:
				goto exit;
				break;
			case TOUCH_FOREACH_ACTIVE_OBJECT:
				ObjectIndex = 0;
				Next = Index;
				ActiveOnly = TRUE;
				break;
			case TOUCH_FOREACH_OBJECT:
				ObjectIndex = 0;
				Next = Index;
				ActiveOnly = FALSE;
				break;
			case TOUCH_FOREACH_END:
				EndOfForeach = Index;
				if (ActiveOnly) {
					if (ActiveObjectsNum) {
						ActiveObjectsIndex++;
						if (ActiveObjectsIndex < ActiveObjectsNum)
							Index = Next;
					} else if (BitsOffset < PayloadLength * 8) {
						Index = Next;
					}
				} else {
					ObjectIndex++;
					if (ObjectIndex < MAX_FINGER)
						Index = Next;
				}
				break;
			case TOUCH_PAD_TO_NEXT_BYTE:
				BitsOffset = ceil_div(BitsOffset, 8) * 8;
				break;
			case TOUCH_CUSTOMER_GESTURE_DETECTED:
				BitsToRead = ConfigData[Index];
				Index++;
				Status = TcmParseSingleByte(Payload, PayloadLength, BitsOffset, BitsToRead, &DataByte);
				if (Status < 0) {
					Trace(
						TRACE_LEVEL_ERROR,
						TRACE_SAMPLES,
						"Failed to get customer gesture Detected");
					goto exit;
				}
				if(DataByte == DETECT_NORMAL_TOUCH) {
					NeedReport = TRUE;
				}
				BitsOffset += BitsToRead;
				break;

			case TOUCH_CUSTOMER_GESTURE_INFO:
				BitsToRead = ConfigData[Index];
				Index++;
				BufIdx = 0;
				while (BitsToRead > 0) {
					if (BufIdx >= 20)
						break;
					Status = TcmParseSingleByte(Payload, PayloadLength, BitsOffset, 8, &DataByte);
					if (Status < 0) {
						Trace(
							TRACE_LEVEL_ERROR,
							TRACE_SAMPLES,
							"Failed to get customer gesture Detected");
						goto exit;
					}
					//read into lpwg data buf here
					// lpwg_data.buf[buf_idx] = DataByte;
					BitsOffset += 8;
					BitsToRead -= 8;
					BufIdx++;
				}
				break;

			case TOUCH_CUSTOMER_GESTURE_INFO2:
				BitsToRead = ConfigData[Index];
				Index++;
				BufIdx = 20;
				while (BitsToRead > 0) {
					if (BufIdx >= 40)
						break;
					Status = TcmParseSingleByte(Payload, PayloadLength, BitsOffset, 8, &DataByte);
					if (Status < 0) {
						Trace(
							TRACE_LEVEL_ERROR,
							TRACE_SAMPLES,
							"Failed to get customer gesture Detected");
						goto exit;
					}
					//read into lpwg data buf here
					// lpwg_data.buf[buf_idx] = DataByte;
					BitsOffset += 8;
					BitsToRead -= 8;
					BufIdx++;
				}
				break;

			case TOUCH_NUM_OF_ACTIVE_OBJECTS:
				BitsToRead = ConfigData[Index];
				Index++;
				Status = TcmParseSingleByte(Payload, PayloadLength, BitsOffset, BitsToRead, &DataByte);
				if (Status < 0) {
					Trace(
						TRACE_LEVEL_ERROR,
						TRACE_SAMPLES,
						"Failed to get number of active objects");
					goto exit;
				}

				DataInt = (INT32)DataByte;

				if (DataInt < 0) {
					Trace(
						TRACE_LEVEL_ERROR,
						TRACE_SAMPLES,
						"Invalid ActiveObjectsNum = %d < 0", DataInt);
					ActiveObjectsNum = 0;
				} else if (DataInt > MAX_FINGER) {
					Trace(
						TRACE_LEVEL_ERROR,
						TRACE_SAMPLES,
						"Invalid ActiveObjectsNum = %d > MAX_FINGER", DataInt);
					ActiveObjectsNum = MAX_FINGER;
				} else {
					ActiveObjectsNum = DataInt;
				}

				BitsOffset += BitsToRead;
				if (ActiveObjectsNum == 0) {
					if (EndOfForeach != 0) {
						Index = EndOfForeach;
					} else {
						Trace(
							TRACE_LEVEL_VERBOSE,
							TRACE_SAMPLES,
							"ActiveObjectsNum & EndOfForeach are 0");
						goto exit;
					}
				}
				break;
			case TOUCH_OBJECT_N_INDEX:
				BitsToRead = ConfigData[Index];
				Index++;
				Status = TcmParseSingleByte(Payload, PayloadLength, BitsOffset, BitsToRead, &DataByte);
				if (Status < 0) {
					Trace(
						TRACE_LEVEL_ERROR,
						TRACE_SAMPLES,
						"Failed to get object index");
					goto exit;
				}
				DataInt = (INT32)DataByte;
				if (DataInt < 0) {
					Trace(
						TRACE_LEVEL_ERROR,
						TRACE_SAMPLES,
						"Invalid ObjectIndex = %d -> 0", DataInt);
					ObjectIndex = 0;
				} else if (DataInt >= MAX_FINGER) {
					Trace(
						TRACE_LEVEL_ERROR,
						TRACE_SAMPLES,
						"Invalid ObjectIndex = %d -> MAX_FINGER", DataInt);
					ObjectIndex = MAX_FINGER - 1;
				} else {
					ObjectIndex = DataInt;
					Trace(
						TRACE_LEVEL_ERROR,
						TRACE_SAMPLES,
						"TOUCH_OBJECT_N_INDEX: %d", DataInt);
				}

				BitsOffset += BitsToRead;
				break;

			case TOUCH_OBJECT_N_CLASSIFICATION:
				BitsToRead = ConfigData[Index];
				Index++;
				Status = TcmParseSingleByte(Payload, PayloadLength, BitsOffset, BitsToRead, &DataByte);
				if (Status < 0) {
					Trace(
						TRACE_LEVEL_ERROR,
						TRACE_SAMPLES,
						"Failed to get object N classification");
					goto exit;
				}
				data.States[ObjectIndex] = DataByte >= 1 ?
					OBJECT_STATE_FINGER_PRESENT_WITH_ACCURATE_POS : OBJECT_STATE_NOT_PRESENT;
				if (data.States[ObjectIndex]) NeedReport = TRUE;
				BitsOffset += BitsToRead;
				break;

			case TOUCH_OBJECT_N_X_POSITION:
				BitsToRead = ConfigData[Index];
				Index++;
				Status = TcmParseSingleByte(Payload, PayloadLength, BitsOffset, BitsToRead, &DataByte);
				if (Status < 0) {
					Trace(
						TRACE_LEVEL_ERROR,
						TRACE_SAMPLES,
						"Failed to get object x position");
					goto exit;
				}
				data.Positions[ObjectIndex].X = DataByte;
				BitsOffset += BitsToRead;
				break;
			case TOUCH_OBJECT_N_Y_POSITION:
				BitsToRead = ConfigData[Index];
				Index++;
				Status = TcmParseSingleByte(Payload, PayloadLength, BitsOffset, BitsToRead, &DataByte);
				if (Status < 0) {
					Trace(
						TRACE_LEVEL_ERROR,
						TRACE_SAMPLES,
						"Failed to get object y position");
					goto exit;
				}
				data.Positions[ObjectIndex].Y = DataByte;
				BitsOffset += BitsToRead;
				break;
			case TOUCH_OBJECT_N_Z:
				BitsToRead = ConfigData[Index];
				Index++;
				Status = TcmParseSingleByte(Payload, PayloadLength, BitsOffset, BitsToRead, &DataByte);
				if (Status < 0) {
					Trace(
						TRACE_LEVEL_ERROR,
						TRACE_SAMPLES,
						"Failed to get object z");
					goto exit;
				}
				BitsOffset += BitsToRead;
				break;
			case TOUCH_OBJECT_N_ANGLE:
			case TOUCH_OBJECT_N_MAJOR:
			case TOUCH_OBJECT_N_MINOR:
			case TOUCH_OBJECT_N_X_WIDTH:
			case TOUCH_OBJECT_N_Y_WIDTH:
			case TOUCH_OBJECT_N_AREA:
			case TOUCH_OBJECT_N_FORCE:
			case TOUCH_TIMESTAMP:
			case TOUCH_GESTURE_ID:
			case TOUCH_FINGERPRINT_AREA_MEET:
			case TOUCH_0D_BUTTONS_STATE:
			case TOUCH_OBJECT_N_TX_POSITION_TIXELS:
			case TOUCH_OBJECT_N_RX_POSITION_TIXELS:
			default:
				BitsToRead = ConfigData[Index];
				Index++;
				BitsOffset += BitsToRead;
				break;
		}
	
	}
exit:
	// if(NeedReport && ReportContext != NULL) {
	if(ReportContext != NULL) {
		Status = ReportObjects(
			ReportContext,
			data);

		if (!NT_SUCCESS(Status))
		{
			Trace(
				TRACE_LEVEL_VERBOSE,
				TRACE_SAMPLES,
				"Error while reporting objects - 0x%08lX",
				Status);
		}
	}

	return Status;
}

NTSTATUS
TcmDispatchResponse(
	IN TCM_CONTROLLER_CONTEXT* ControllerContext,
	IN PREPORT_CONTEXT ReportContext,
	IN TCM_MSG_HEADER* MessageHeader,
	_In_reads_bytes_(PayloadLength) PVOID Payload,
	IN ULONG PayloadLength
)
{
	(void*)Payload;
	(void*)MessageHeader;
	(void*)ReportContext;
	NTSTATUS status = STATUS_SUCCESS;
	UINT8* PayloadData = (UINT8*)Payload;
	if(ControllerContext->CommandStatus != CMD_BUSY) {
		return STATUS_SUCCESS;
	}

	if(PayloadLength == 0) {
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_SAMPLES,
			"TcmDispatchResponse: PayloadLength = 0");
		ControllerContext->CommandStatus = CMD_IDLE;
		return status;
	}

	RtlZeroMemory(ControllerContext->ResponseData.Buffer, sizeof(ControllerContext->ResponseData.Buffer));
	ControllerContext->ResponseData.DataLength = PayloadLength;
	RtlCopyMemory(ControllerContext->ResponseData.Buffer, PayloadData, PayloadLength);

	ControllerContext->CommandStatus = CMD_IDLE;
	// WdfWaitLockRelease(ControllerContext->ResponseLock);
	SignalEvent(&ControllerContext->ResponseSignal->Event);
	return status;
}

NTSTATUS
TcmParseSingleByte(
	_In_reads_bytes_(PayloadLength) PVOID Payload,
	IN UINT32 PayloadLength,
	IN UINT32 BitsOffset,
	IN UINT32 BitsToRead,
	IN UINT32 *OutputData
) 
{
	UINT8 mask = 0, byte_data = 0;
	UINT32 bit_offset = 0, byte_offset = 0, data_bits = 0, output_data = 0;
	INT32 remaining_bits = 0, available_bits = 0;
	UINT8 *PayloadData = (UINT8*) Payload;

	if (BitsToRead == 0 || BitsToRead > 32) {
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_SAMPLES,
			"Invalid number of BitsToRead");
		return STATUS_INVALID_PARAMETER;
	}

	if (BitsOffset + BitsToRead > PayloadLength * 8) {
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_SAMPLES,
			"Overflow PayloadLength*8, BitsOffset+BitsToRead:%d+%d",
			BitsOffset,
			BitsToRead);

		*OutputData = 0;
		return STATUS_UNSUCCESSFUL;
	}

	output_data = 0;
	remaining_bits = BitsToRead;

	bit_offset = BitsOffset % 8;
	byte_offset = BitsOffset / 8;

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_SAMPLES,
		"TcmParseSingleByte: read %d bits at offset: bit %d at byte %d",
		BitsToRead,
		bit_offset,
		byte_offset);

	while (remaining_bits > 0) {
		byte_data = PayloadData[byte_offset];
		byte_data >>= bit_offset;

		available_bits = 8 - bit_offset;
		data_bits = MIN(available_bits, remaining_bits);
		mask = 0xff >> (8 - data_bits);

		byte_data &= mask;

		output_data |= byte_data << (BitsToRead - remaining_bits);

		bit_offset = 0;
		byte_offset += 1;
		remaining_bits -= data_bits;
		if (remaining_bits < 0) {
			Trace(
				TRACE_LEVEL_ERROR,
				TRACE_SAMPLES,
				"remaining_bits is negative value:%d", remaining_bits);
			break;
		}
	}

	*OutputData = output_data;

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_SAMPLES,
		"TcmParseSingleByte: OutputData = 0x%x", *OutputData);


	return STATUS_SUCCESS;
}

NTSTATUS
TcmWriteMessage(
	IN TCM_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext,
	IN UINT8 Command,
	_In_reads_bytes_(PayloadLength) PVOID Payload,
	IN ULONG PayloadLength,
	IN UINT8 *ResponseBuffer,
	IN ULONG *ResponseLength
)
{
	NTSTATUS status = STATUS_SUCCESS;
	ULONG WriteLength = 0;
	UINT8 *PayloadData = (UINT8*)Payload;
	UINT8* Buffer = NULL;
	LONGLONG Timeout;

	WdfWaitLockAcquire(ControllerContext->ControllerLock, NULL);

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_DRIVER,
		"TcmWriteMessage: command=0x%x, payload=%d, length=%d",
		Command, (Payload!=NULL) ? 1 : 0, PayloadLength);

	WriteLength = PayloadLength + 2;

	Buffer = ExAllocatePoolWithTag(
		NonPagedPoolNx,
		WriteLength,
		TOUCH_POOL_TAG_MSG
	);

	if (Buffer == NULL) {
		status = STATUS_INSUFFICIENT_RESOURCES;
		goto exit;
	}

	ControllerContext->CurrentCommand = Command;
	ControllerContext->CommandStatus = CMD_BUSY;

	// Buffer[0] = Command;
	Buffer[0] = (UINT8)PayloadLength;
	Buffer[1] = (UINT8)(PayloadLength >> 8);
	
	if (WriteLength > 2 && PayloadData != NULL) {
		RtlCopyMemory(&Buffer[2], PayloadData, PayloadLength);
	}

	if (Command == CMD_GET_BOOT_INFO ||
		Command == CMD_GET_APPLICATION_INFO ||
		Command == CMD_READ_FLASH ||
		Command == CMD_WRITE_FLASH ||
		Command == CMD_ERASE_FLASH ||
		Command == CMD_PRODUCTION_TEST) {
		Timeout = RESPONSE_TIMEOUT_LONG;
	}
	else Timeout = RESPONSE_TIMEOUT;

	status = SpbWriteDataSynchronously(
		SpbContext,
		Command,
		Buffer,
		WriteLength
	);

	WdfWaitLockRelease(ControllerContext->ControllerLock);

	WaitForEvent(&ControllerContext->ResponseSignal->Event, Timeout);

	if (ControllerContext->CommandStatus != CMD_IDLE) {
		status = TcmReadMessage(ControllerContext,
			SpbContext,
			NULL);

		if (!NT_SUCCESS(status)) {
			Trace(
				TRACE_LEVEL_ERROR,
				TRACE_DRIVER,
				"TcmWriteMessage: timed out waiting for response");
			status = STATUS_UNSUCCESSFUL;
			goto free_buffer;
		}

		if (ControllerContext->CommandStatus != CMD_IDLE) {
			Trace(
				TRACE_LEVEL_ERROR,
				TRACE_DRIVER,
				"TcmWriteMessage: CommandStatus is not CMD_IDLE: Command = 0x%02x, CommandStatus = 0x%02x",
				Command, ControllerContext->CommandStatus);
			status = STATUS_UNSUCCESSFUL;
			goto free_buffer;
		}
	}

	if (ControllerContext->ResponseCode != TCM_STATUS_OK) {
		if (ControllerContext->ResponseData.DataLength != 0) {

		}
		status = STATUS_UNSUCCESSFUL;
		goto free_buffer;
	}

	if (ResponseBuffer != NULL && ResponseBuffer != ControllerContext->ResponseData.Buffer
		&& ResponseLength != NULL) {
		RtlCopyMemory(&ResponseBuffer[0], &(ControllerContext->ResponseData.Buffer[0]), ControllerContext->ResponseData.DataLength);
		*ResponseLength = ControllerContext->ResponseData.DataLength;
	}

free_buffer:
	if (Buffer != NULL) {
		ExFreePoolWithTag(
			Buffer,
			TOUCH_POOL_TAG_MSG
		);
	}

exit:
	return status;
}

NTSTATUS
TcmSwitchMode(
	IN TCM_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext,
	IN UINT8 Mode
)
{
	NTSTATUS status = STATUS_SUCCESS;
	UINT8 Command = 0;
	
	switch (Mode) {
		case MODE_APPLICATION_FIRMWARE:
			Command = CMD_RUN_APPLICATION_FIRMWARE;
			break;
		case MODE_ROMBOOTLOADER:
			Command = CMD_ROMBOOT_RUN_BOOTLOADER_FIRMWARE;
			break;
		case MODE_BOOTLOADER:
			Command = CMD_RUN_BOOTLOADER_FIRMWARE;
			break;
		case MODE_PRODUCTIONTEST_FIRMWARE:
			Command = CMD_ENTER_PRODUCTION_TEST_MODE;
			break;
		default:
			status = STATUS_INVALID_PARAMETER;
			return status;
	}
	status = TcmWriteMessage(ControllerContext,
		SpbContext,
		Command,
		NULL,
		0,
		NULL,
		NULL);
	if (!NT_SUCCESS(status)) goto exit;

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_DRIVER,
		"TcmSwitchMode: Response: 0x%02x / Command: 0x%02x",
		ControllerContext->ResponseCode,
		Command);

	status = TcmGetIcInfo(ControllerContext, SpbContext);

exit:
	return status;
}

NTSTATUS
TcmGetIcInfo(
	IN TCM_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext
)
{
	(void*)SpbContext;
	NTSTATUS status = STATUS_SUCCESS;

	switch (ControllerContext->IDInfo.Mode) {
		case MODE_APPLICATION_FIRMWARE:
		case MODE_HOSTDOWNLOAD_FIRMWARE:
			Trace(
				TRACE_LEVEL_ERROR,
				TRACE_DRIVER,
				"Current FW Mode: Application mode");
			status = TcmGetAppInfo(ControllerContext, SpbContext);
			if (!NT_SUCCESS(status)) {
				Trace(
					TRACE_LEVEL_ERROR,
					TRACE_DRIVER,
					"App info status is not ok");
				if (ControllerContext->AppInfo.Status == APP_STATUS_BAD_APP_CONFIG) {
					Trace(
						TRACE_LEVEL_ERROR,
						TRACE_DRIVER,
						"Bad App config!");
				}
			}
			break;
		case MODE_ROMBOOTLOADER:
			Trace(
				TRACE_LEVEL_ERROR,
				TRACE_DRIVER,
				"Current FW Mode: ROM Bootloader mode");
			/*status = TcmGetROMBootInfo(ControllerContext, SpbContext);
			if (!NT_SUCCESS(status)) {
				Trace(
					TRACE_LEVEL_ERROR,
					TRACE_DRIVER,
					"Failed to get romboot_info");
				goto error;
			}*/
			break;
		case MODE_BOOTLOADER:
		case MODE_TDDI_BOOTLOADER:
			Trace(
				TRACE_LEVEL_ERROR,
				TRACE_DRIVER,
				"Current FW Mode: Bootloader mode");
			// TODO
			break;
		default:
			Trace(
				TRACE_LEVEL_ERROR,
				TRACE_DRIVER,
				"Current FW Mode: Unknown mode 0x%02x",
				ControllerContext->IDInfo.Mode);
			break;
	}

//error:
	return status;
}

NTSTATUS
TcmGetAppInfo(
	IN TCM_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext
)
{
	NTSTATUS status = STATUS_SUCCESS;
	INT RetryCount = 0;
	TCM_APP_INFO* Info;
	LARGE_INTEGER PollInterval;

	PollInterval.QuadPart = STATUS_POLL_INTERVAL;

retry:
	RetryCount++;
	status = TcmWriteMessage(ControllerContext,
		SpbContext,
		CMD_GET_APPLICATION_INFO,
		NULL,
		0,
		NULL,
		NULL);

	if (!NT_SUCCESS(status)) {
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_DRIVER,
			"Failed to get Application info");
		goto exit;
	}

	RtlCopyMemory(&ControllerContext->AppInfo,
		ControllerContext->ResponseData.Buffer,
		MIN(sizeof(TCM_APP_INFO), ControllerContext->ResponseData.DataLength));

	if (ControllerContext->AppInfo.Status != APP_STATUS_OK) {
		if (RetryCount < 5)
		{
			Trace(
				TRACE_LEVEL_ERROR,
				TRACE_DRIVER,
				"Retry reading App info, RetryCount = %d", RetryCount);
			KeDelayExecutionThread(KernelMode, TRUE, &PollInterval);
			goto retry;
		}
		else {
			Trace(
				TRACE_LEVEL_ERROR,
				TRACE_DRIVER,
				"Failed to wait for APP_STATUS_OK");
			status = STATUS_TIMEOUT;
			goto exit;
		}
	}

	Info = &ControllerContext->AppInfo;

	Trace(
		TRACE_LEVEL_ERROR,
		TRACE_DRIVER,
		"TcmGetAppInfo: IC Version: v%d.%02d, IC Build_id: %d",
		Info->CustomerConfigID.Release, Info->CustomerConfigID.Version, ControllerContext->IDInfo.BuildId);

exit:
	return status;
}

NTSTATUS
TcmGetReportConfig(
	IN TCM_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext
)
{
	NTSTATUS status = STATUS_SUCCESS;

	status = TcmWriteMessage(ControllerContext,
		SpbContext,
		CMD_GET_TOUCH_REPORT_CONFIG,
		NULL,
		0,
		&ControllerContext->ConfigData.Buffer[0],
		&ControllerContext->ConfigData.DataLength);

	if (NT_SUCCESS(status)) {
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_DRIVER,
			"TcmGetReportConfig: Response: 0x%x / Command: 0x%x",
			ControllerContext->ResponseCode,
			ControllerContext->CurrentCommand);
	}

	return status;
}