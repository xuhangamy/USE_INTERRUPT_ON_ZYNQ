#include "xparameters.h"
#include "xgpio.h"
#include "xil_exception.h"
#include "xscugic.h"
#include "sleep.h"

XGpio GpioPtr;
XScuGic Intc;

u8 Led = 0xf;

/*pre define of the interrupt function*/
void GpioIsr(void *InstancePtr);

int main(void)
{
	/*when initialize, we usually define a variable named with status to judge the device is initialized or not*/
	int Status;

	/*define the pointer which point to the general interrupt controller*/
	XScuGic_Config *IntcConfig;

	/*first initialize the GPIO, in specifically set their data direction and connet to the device ID*/
	XGpio_Initialize(&GpioPtr,XPAR_AXI_GPIO_0_DEVICE_ID);
	XGpio_SetDataDirection(&GpioPtr,1,0xf);
	XGpio_SetDataDirection(&GpioPtr,2,0);
	/*in this program we can first light on all the LED in the board*/
	XGpio_DiscreteWrite(&GpioPtr, 2, Led);

	/*Initialize the interrupt controller driver so that it is ready to use*/
	/*it is to use the device ID from the PS perspective*/
	IntcConfig = XScuGic_LookupConfig(XPAR_PS7_SCUGIC_0_DEVICE_ID);
	if (NULL == IntcConfig) {
		return XST_FAILURE;
	}

	/*then initialize it, also we should use it from the PS perspective*/
	Status = XScuGic_CfgInitialize(&Intc, IntcConfig,IntcConfig->CpuBaseAddress);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*use the interrupt number to set up the priority of this interrupt*/
	XScuGic_SetPriorityTriggerType(&Intc, XPAR_FABRIC_AXI_GPIO_0_IP2INTC_IRPT_INTR,0xA0, 0x3);

	/* Connect the interrupt handler that will be called when an interrupt occurs for the device.*/
	Status = XScuGic_Connect(&Intc, XPAR_FABRIC_AXI_GPIO_0_IP2INTC_IRPT_INTR,
				 (Xil_ExceptionHandler)GpioIsr, &Intc);
	if (Status != XST_SUCCESS) {
		return Status;
	}

	/*
	 * Enable the interrupt for the GPIO device.
	 */
	/*when enable a interrupt, we should also pay attention to the general interrupt controlle, so we should use the parameter &Intc*/
	XScuGic_Enable(&Intc, XPAR_FABRIC_AXI_GPIO_0_IP2INTC_IRPT_INTR);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*Since the Interrupt */
	XGpio_InterruptEnable(&GpioPtr, XGPIO_IR_CH1_MASK);
	XGpio_InterruptGlobalEnable(&GpioPtr);

	/*the interrupt was treated as the exception, so after connect we shoud initial the exception*/
	/*
	 * Initialize the exception table and register the interrupt
	 * controller handler with the exception table
	 */
	Xil_ExceptionInit();
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
			 (Xil_ExceptionHandler)XScuGic_InterruptHandler, &Intc);
	/* Enable non-critical exceptions */
	Xil_ExceptionEnable();

	/*
	 * Loop forever while the button changes are handled by the interrupt
	 * level processing
	 */
	/*in the program we set the light is on all the time and when the button was pushed down, we can see the light is off*/
	while (1) {
		XGpio_DiscreteWrite(&GpioPtr, 2, Led);
	}

	return XST_SUCCESS;
}



/****************************************************************************/
/**
* This function is the Interrupt Service Routine for the GPIO device.  It
* will be called by the processor whenever an interrupt is asserted by the
* device.
*
* This function will detect the push button on the board has changed state
* and then turn on or off the LED.
*
* @param	InstancePtr is the GPIO instance pointer to operate on.
*		It is a void pointer to meet the interface of an interrupt
*		processing function.
*
* @return	None.
*
* @note		None.
*
*****************************************************************************/
void GpioIsr(void *InstancePtr)
{
	/*
	 * Disable the interrupt
	 */
	/*when enter the interrupt service program we should disable the interrupt immediately, it is the requires from the interrupt logic*/
	XGpio_InterruptDisable(&GpioPtr, XGPIO_IR_CH1_MASK);

	 /* Clear the interrupt such that it is no longer pending in the GPIO */
	/*change the on-off state of the light*/
	Led = ~Led;
	usleep(20000);
	/*standard way, clear the interrupt after execute the main function of the interrupt service*/
	XGpio_InterruptClear(&GpioPtr, XGPIO_IR_CH1_MASK);

	 /*
	  * Enable the interrupt
	  */
	/*enable it once again*/
	XGpio_InterruptEnable(&GpioPtr, XGPIO_IR_CH1_MASK);
}

