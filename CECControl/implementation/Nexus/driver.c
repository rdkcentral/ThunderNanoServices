/******************************************************************************
 *  Copyright (C) 2017 Broadcom. The term "Broadcom" refers to Broadcom Limited and/or its subsidiaries.
 *
 *  This program is the proprietary software of Broadcom and/or its licensors,
 *  and may only be used, duplicated, modified or distributed pursuant to the terms and
 *  conditions of a separate, written license agreement executed between you and Broadcom
 *  (an "Authorized License").  Except as set forth in an Authorized License, Broadcom grants
 *  no license (express or implied), right to use, or waiver of any kind with respect to the
 *  Software, and Broadcom expressly reserves all rights in and to the Software and all
 *  intellectual property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU
 *  HAVE NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY
 *  NOTIFY BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.
 *
 *  Except as expressly set forth in the Authorized License,
 *
 *  1.     This program, including its structure, sequence and organization, constitutes the valuable trade
 *  secrets of Broadcom, and you shall use all reasonable efforts to protect the confidentiality thereof,
 *  and to use this information only in connection with your use of Broadcom integrated circuit products.
 *
 *  2.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
 *  AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
 *  WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH RESPECT TO
 *  THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND ALL IMPLIED WARRANTIES
 *  OF TITLE, MERCHANTABILITY, NONINFRINGEMENT, FITNESS FOR A PARTICULAR PURPOSE,
 *  LACK OF VIRUSES, ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION
 *  OR CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF
 *  USE OR PERFORMANCE OF THE SOFTWARE.
 *
 *  3.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR ITS
 *  LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL, INDIRECT, OR
 *  EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY WAY RELATING TO YOUR
 *  USE OF OR INABILITY TO USE THE SOFTWARE EVEN IF BROADCOM HAS BEEN ADVISED OF
 *  THE POSSIBILITY OF SUCH DAMAGES; OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT
 *  ACTUALLY PAID FOR THE SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE
 *  LIMITATIONS SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF
 *  ANY LIMITED REMEDY.
 ******************************************************************************/

#include "nexus_platform.h"
#include "bstd.h"
#include "bkni.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include "nexus_hdmi_output.h"
#include "nexus_cec.h"
#include "nscclient.h"
#include "ccec/drivers/hdmi_cec_driver.h"

#undef BDBG_WRN
#define BDBG_WRN2(x) do{printf x;printf("\r\n");}while(0)
#define BDBG_WRN(x) do{/*printf x;printf("\r\n");*/}while(0)
pthread_mutex_t DriverMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t CallbackMutex = PTHREAD_MUTEX_INITIALIZER;

#if (NEXUS_PLATFORM_VERSION_MAJOR<14)
	#error "This requires Nexus Major version > 14 !!!"
#endif


#define DRIVER_LOCK() do{/*printf("LOCKING DRV [%s]\r\n", __FUNCTION__);*/pthread_mutex_lock(&DriverMutex);}while(0)
#define DRIVER_UNLOCK() do{/*printf("UNLOCKING DRV [%s]\r\n", __FUNCTION__);*/pthread_mutex_unlock(&DriverMutex);}while(0)

typedef struct NEXUS_CecContext_t {
    BKNI_EventHandle event;
    NEXUS_HdmiOutputHandle hdmiOutput;
    NEXUS_CecHandle hCec;
    bool deviceReady;
    NEXUS_CecStatus cecStatus;

    HdmiCecRxCallback_t userRxCallback;
    void *userRxCallbackData;

    HdmiCecTxCallback_t userTxCallback;
    void *userTxCallbackData;
    int transmitResult;

    int opened;
} NEXUS_CecContext_t;

static NEXUS_CecContext_t driverContext;
static NEXUS_CecContext_t *pDriverContext = &driverContext;

static int logical_address = 15;

static void msgReceived_callback(void *context, int param);
static void msgTransmitted_callback(void *context, int param);
static void deviceReady_callback(void *context, int param);

static void deviceReady_callback(void *context, int param)
{
	if (!pDriverContext->opened) return;

	CALLBACK_LOCK();
	NEXUS_CecStatus status;
    BSTD_UNUSED(param);
    BKNI_SetEvent((BKNI_EventHandle)context);
    NEXUS_Cec_GetStatus(pDriverContext->hCec, &status);

    BDBG_WRN2(("BCM%d Logical Address <%d> Acquired",
        BCHP_CHIP,
        status.logicalAddress)) ;

    BDBG_WRN2(("BCM%d Physical Address: %X.%X.%X.%X",
        BCHP_CHIP,
        (status.physicalAddress[0] & 0xF0) >> 4,
        (status.physicalAddress[0] & 0x0F),
        (status.physicalAddress[1] & 0xF0) >> 4,
        (status.physicalAddress[1] & 0x0F))) ;

    if ((status.physicalAddress[0] = 0xFF)
    && (status.physicalAddress[1] = 0xFF))
    {
	    BDBG_WRN2(("CEC Device Ready!")) ;
	       pDriverContext->deviceReady = true ;
	       pDriverContext->cecStatus = status;
    }

    BDBG_WRN2(("deviceReady_callback -- Device Type : %d , Logical Address : %d \n",status.deviceType, status.logicalAddress));
    logical_address = status.logicalAddress;

	CALLBACK_UNLOCK();
}

static void msgReceived_callback(void *context, int param)
{
	if (!pDriverContext->opened) return;

	CALLBACK_LOCK();
    NEXUS_CecStatus status;
    NEXUS_CecReceivedMessage receivedMessage;
    NEXUS_Error rc ;
    uint8_t i, j ;
    char msgBuffer[3*(NEXUS_CEC_MESSAGE_DATA_SIZE +1)];
    char rawBuffer[NEXUS_CEC_MESSAGE_DATA_SIZE + 1];

    BSTD_UNUSED(param);
    BKNI_SetEvent((BKNI_EventHandle)context);
    NEXUS_Cec_GetStatus(pDriverContext->hCec, &status);

    BDBG_WRN(("Message Received: %s", status.messageReceived ? "Yes" : "No")) ;

    rc = NEXUS_Cec_ReceiveMessage(pDriverContext->hCec, &receivedMessage);
    BDBG_ASSERT(!rc);

    /* For debugging purposes */
    for (i = 0, j = 0; i <= receivedMessage.data.length && j<(sizeof(msgBuffer)-1); i++)
    {
        j += BKNI_Snprintf(msgBuffer + j, sizeof(msgBuffer)-j, "%02X ",
        receivedMessage.data.buffer[i]) ;
    }

    rawBuffer[0] = (((receivedMessage.data.initiatorAddr & 0x0F) << 4) | (receivedMessage.data.destinationAddr & 0x0F));

    BDBG_WRN(("CEC Message Length %d Received: %s",
        receivedMessage.data.length, msgBuffer)) ;

    BDBG_WRN(("Msg Recd Status for Phys/Logical Addrs: %X.%X.%X.%X / %d with [src=%x][dst=%x]",
        (status.physicalAddress[0] & 0xF0) >> 4, (status.physicalAddress[0] & 0x0F),
        (status.physicalAddress[1] & 0xF0) >> 4, (status.physicalAddress[1] & 0x0F),
        status.logicalAddress, receivedMessage.data.initiatorAddr, receivedMessage.data.destinationAddr)) ;

    memcpy(&rawBuffer[1], &receivedMessage.data.buffer[0], receivedMessage.data.length);


    if (pDriverContext->userRxCallback != 0) {

#if defined (LOG_ENABLED)
    	printf("<<<<<<<<< <<<<< <<<< <<< <\r\n");
    	int ii = 0;
        for (ii = 0; ii < receivedMessage.data.length + 1; ii++) {
        	printf("%02X ", rawBuffer[ii]);
        }
    	printf("==========================\r\n");
#endif

    	pDriverContext->userRxCallback((int)pDriverContext, pDriverContext->userRxCallbackData, (unsigned char *)&rawBuffer[0], receivedMessage.data.length + 1);
    }
    CALLBACK_UNLOCK();
}

static void msgTransmitted_callback(void *context, int param)
{
	if (!pDriverContext->opened) return;

	CALLBACK_LOCK();

	NEXUS_CecStatus status;

    BSTD_UNUSED(param);
    BKNI_SetEvent((BKNI_EventHandle)context);
    NEXUS_Cec_GetStatus(pDriverContext->hCec, &status);

    BDBG_WRN(("Msg Xmit Status for Phys/Logical Addrs: %X.%X.%X.%X / %d",
        (status.physicalAddress[0] & 0xF0) >> 4, (status.physicalAddress[0] & 0x0F),
        (status.physicalAddress[1] & 0xF0) >> 4, (status.physicalAddress[1] & 0x0F),
        status.logicalAddress)) ;

    BDBG_WRN(("Xmit Msg Acknowledged: %s",
		status.transmitMessageAcknowledged ? "Yes" : "No")) ;
    BDBG_WRN(("Xmit Msg Pending: %s",
		status.messageTransmitPending ? "Yes" : "No")) ;

    int result = HDMI_CEC_IO_SUCCESS;

    if (status.transmitMessageAcknowledged) {
    	result = HDMI_CEC_IO_SENT_AND_ACKD;
    }
    else {
    	if (!(status.messageTransmitPending)) {
        	result = HDMI_CEC_IO_SENT_BUT_NOT_ACKD;
    	}
    	else {
        	result = HDMI_CEC_IO_SENT_FAILED;
    	}
    }

    pDriverContext->transmitResult = result;

    if (pDriverContext->userTxCallback != 0) {
    	pDriverContext->userTxCallback((int)pDriverContext, pDriverContext->userTxCallbackData, result);
    }
    CALLBACK_UNLOCK();
}

int HdmiCecOpen(int *handle)
{
	DRIVER_LOCK();
	CALLBACK_LOCK();

    int retErr = HDMI_CEC_IO_SUCCESS;
    NEXUS_Error rc = 0;

    NEXUS_CecMessage transmitMessage;
    NEXUS_CecSettings cecSettings;

    NEXUS_HdmiOutputStatus status;
    NEXUS_HdmiOutputHandle hdmiOutput;

	/* nxserver does not open CEC, so a client may. */
	NxClient_Join(NULL);
	if(pDriverContext->hCec) {
	   printf("HdmiCecOpen: alredy opened. return error \r\n");
           retErr = HDMI_CEC_IO_GENERAL_ERROR;
           return retErr;
	}
	NEXUS_Cec_GetDefaultSettings(&cecSettings);

	pDriverContext->hCec = NEXUS_Cec_Open(0, &cecSettings);
       if (!pDriverContext->hCec) {
           printf("unable to open CEC ! Error from nexus driver \r\n");
           retErr = HDMI_CEC_IO_GENERAL_ERROR;
           return retErr;
	}
        printf("HdmiCecOpen: open success");
	
        /* nxserver opens HDMI output, so a client can only open as a read-only alias. */
        hdmiOutput = NEXUS_HdmiOutput_Open(0 + NEXUS_ALIAS_ID, NULL);
	rc = NEXUS_HdmiOutput_GetStatus(hdmiOutput, &status);
	BDBG_ASSERT(!rc);

    if (retErr == HDMI_CEC_IO_SUCCESS) {
        if (!status.connected) {
            /*retErr = HDMI_CEC_IO_INVALID_STATE;*/
            BDBG_WRN(("********************")) ;
            BDBG_WRN(("HDMI is not connected"));
            BDBG_WRN(("********************")) ;
            while(!status.connected)
            {
               sleep(10);
#if (NEXUS_PLATFORM_VERSION_MAJOR<14)
               NscClienGetHdmiStatus(&status);
#else
	       rc = NEXUS_HdmiOutput_GetStatus(hdmiOutput, &status);
#endif
            }
            BDBG_WRN(("********************")) ;
            BDBG_WRN(("HDMI is connected"));
            BDBG_WRN(("********************")) ;
        }
        else {
            BDBG_WRN(("********************")) ;
            BDBG_WRN(("HDMI is connected"));
            BDBG_WRN(("********************")) ;
        }
    }

    if (retErr == HDMI_CEC_IO_SUCCESS) {

        NEXUS_CecSettings cecSettings;

        BKNI_CreateEvent(&(pDriverContext->event));

        NEXUS_Cec_GetSettings(pDriverContext->hCec, &cecSettings);
		cecSettings.messageReceivedCallback			.callback 	= msgReceived_callback ;
		cecSettings.messageReceivedCallback			.context	= pDriverContext->event;

		cecSettings.messageTransmittedCallback		.callback 	= msgTransmitted_callback;
		cecSettings.messageTransmittedCallback		.context 	= pDriverContext->event;

		cecSettings.logicalAddressAcquiredCallback	.callback 	= deviceReady_callback ;
		cecSettings.logicalAddressAcquiredCallback	.context 	= pDriverContext->event;

		cecSettings.physicalAddress[0] = (status.physicalAddressA << 4) | status.physicalAddressB;
		cecSettings.physicalAddress[1] = (status.physicalAddressC << 4) | status.physicalAddressD;
        rc = NEXUS_Cec_SetSettings(pDriverContext->hCec, &cecSettings);

        BDBG_ASSERT(!rc);


        /* Enable CEC core */
        NEXUS_Cec_GetSettings(pDriverContext->hCec, &cecSettings);
		cecSettings.enabled = true;
		rc = NEXUS_Cec_SetSettings(pDriverContext->hCec, &cecSettings);

		pDriverContext->opened = 1;
        BDBG_ASSERT(!rc);
    }

    CALLBACK_UNLOCK();

    BDBG_WRN(("*************************")) ;
    BDBG_WRN(("Wait for logical address before starting test..."));
    BDBG_WRN(("*************************\n\n")) ;
    int loops = 0;

    for (loops = 0; loops < 10; loops++) {
		if (pDriverContext->deviceReady)
			break;
        BKNI_Sleep(1000);
    }

    if (pDriverContext->cecStatus.logicalAddress == 0xFF)
    {
        BDBG_WRN(("No CEC capable device found on HDMI output."));
        retErr = HDMI_CEC_IO_LOGICALADDRESS_UNAVAILABLE;
    }
    else {
        BDBG_WRN(("*************************")) ;
        BDBG_WRN(("Logical Address Set..."));
        BDBG_WRN(("*************************\n\n")) ;
    }


    if (retErr != HDMI_CEC_IO_SUCCESS) {
        if(NULL != pDriverContext->event)
        {
            BKNI_DestroyEvent(pDriverContext->event);
        }
        NEXUS_Platform_Uninit();
    }
    else {
    	*handle = (int)pDriverContext;
    }

	DRIVER_UNLOCK();

    return retErr;
}

int HdmiCecClose(int handle)
{
    if(pDriverContext->hCec) {
         NEXUS_Cec_Close(pDriverContext->hCec);
         printf("HdmiCecClose: closed CEC handle \r\n");
         pDriverContext->hCec = NULL;
    }
    else
         printf("HdmiCecClose: handle invalid! error! \r\n");

	pDriverContext->opened = 0;
	DRIVER_LOCK();
    CALLBACK_LOCK();
    BDBG_ASSERT(handle == ((int)pDriverContext));
	BKNI_DestroyEvent(pDriverContext->event);

	logical_address = 15;

#ifdef RDK_USE_NXCLIENT
    NscClientShutdown();
#endif

#ifdef RDK_USE_NXCLIENT
#else
	NEXUS_Platform_Uninit();
#endif
    CALLBACK_UNLOCK();
	DRIVER_UNLOCK();
	return HDMI_CEC_IO_SUCCESS;
}

int HdmiCecSetRxCallback(int handle, HdmiCecRxCallback_t cbfunc, void *data)
{
	DRIVER_LOCK();
    CALLBACK_LOCK();
    BDBG_ASSERT(handle == ((int)pDriverContext));
    pDriverContext->userRxCallback = cbfunc;
    pDriverContext->userRxCallbackData = data;
    CALLBACK_UNLOCK();
	DRIVER_UNLOCK();
}

int HdmiCecSetTxCallback(int handle, HdmiCecTxCallback_t cbfunc, void *data)
{
	DRIVER_LOCK();
    CALLBACK_LOCK();
    BDBG_ASSERT(handle == ((int)pDriverContext));
    pDriverContext->userTxCallback = cbfunc;
    pDriverContext->userTxCallbackData = data;
    CALLBACK_UNLOCK();
	DRIVER_UNLOCK();
}

int HdmiCecTxAsync(int handle, const unsigned char *buf, int len)
{
	DRIVER_LOCK();
    BDBG_ASSERT(handle == ((int)pDriverContext));

    BDBG_WRN(("*************************")) ;
    BDBG_WRN(("HdmiCecTxAsync...\r\n"));

	int retErr = HDMI_CEC_IO_SUCCESS;
    NEXUS_Error rc;
    NEXUS_CecMessage transmitMessage;
	transmitMessage.initiatorAddr 	= ((buf[0] & 0xF0) >> 4) ;
    transmitMessage.destinationAddr = ((buf[0] & 0x0F) >> 0);
    transmitMessage.length = len;
    memcpy(&transmitMessage.buffer[0], &buf[1], len - 1);

#if defined (LOG_ENABLED)
    int ii = 0;
	printf(">>>>>>> >>>>> >>>> >> >> ><<<<<<\r\n");
    for (ii = 0; ii < len; ii++) {
    	printf("%02X ", buf[ii]);
    }
	printf("==========================\r\n");
#endif
    rc = NEXUS_Cec_TransmitMessage(pDriverContext->hCec, &transmitMessage);

    if(rc) {
    	retErr = HDMI_CEC_IO_GENERAL_ERROR;
    }

    BDBG_WRN(("HdmiCecTxAsync... DONE"));
    BDBG_WRN(("*************************\n\n")) ;
	DRIVER_UNLOCK();

    return retErr;
}

int HdmiCecTx(int handle, const unsigned char *buf, int len, int *result)
{
	DRIVER_LOCK();

    BDBG_ASSERT(handle == ((int)pDriverContext));

	int retErr = HDMI_CEC_IO_SUCCESS;
    NEXUS_Error rc;
    NEXUS_CecMessage transmitMessage;
	transmitMessage.initiatorAddr 	= ((buf[0] & 0xF0) >> 4) ;
    transmitMessage.destinationAddr = ((buf[0] & 0x0F) >> 0);
    transmitMessage.length = len - 1;
    memcpy(&transmitMessage.buffer[0], &buf[1], transmitMessage.length);
#if defined (LOG_ENABLED)
    int ii = 0;
	printf(">>>>>>> >>>>> >>>> >> >> >\r\n");
    for (ii = 0; ii < len; ii++) {
    	printf("%02X ", buf[ii]);
    }
	printf("==========================\r\n");

    printf("Writing to [%x] from [%x] of %d bytes\r\n", transmitMessage.destinationAddr, transmitMessage.initiatorAddr, transmitMessage.length);
#endif
    pDriverContext->transmitResult = HDMI_CEC_IO_GENERAL_ERROR;
    rc = NEXUS_Cec_TransmitMessage(pDriverContext->hCec, &transmitMessage);

    if(rc) {
    	retErr = HDMI_CEC_IO_GENERAL_ERROR;
    }
    else {
    	/*
    	 * IMPORTANT:
    	 *
    	 * PLEASE REPLACE THIS LOOP WITH CONCITIONAL VARIABLE IN YOUR IMPLEMENTATION
    	 *
    	 */
    	int waitLoop = 0;
    	while((waitLoop < 10) && (pDriverContext->transmitResult == HDMI_CEC_IO_GENERAL_ERROR)) {
    		usleep(100*1000);
            BDBG_WRN(("Waiting for trans complete..."));
            waitLoop++;
    	}
    }

    *result = pDriverContext->transmitResult;
	DRIVER_UNLOCK();

    return retErr;
}

int HdmiCecSetLogicalAddress(int handle, int *logicalAddresses, int num)
{
    return 0;
}

int HdmiCecAddLogicalAddress(int handle, int logicalAddresses)
{
    return 0;
}

int HdmiCecRemoveLogicalAddress(int handle, int logicalAddresses)
{
    return 0;
}

int HdmiCecGetLogicalAddress(int handle, int devType,  int *logicalAddress)
{
    DRIVER_LOCK();

    BDBG_WRN(("Entered SOC HdmiCecGetLogicalAddress \n"));
    BDBG_ASSERT(handle == ((int)pDriverContext));

    *logicalAddress = logical_address;

    DRIVER_UNLOCK();
    return 0;
}
