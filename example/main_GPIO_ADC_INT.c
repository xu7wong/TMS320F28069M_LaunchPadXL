

/**
 * main.c
 */
#include "DSP28x_Project.h"
#include <stdint.h>
#define DOWNLOAD_TO_FLASH 1

//0x3D7E82 - Slope (oC / LSB, fixed-point Q15 format)
//0x3D7E85 - Offset (0 oC LSB value)

#define getTempSlope() (*(int32_t (*)(void))0x3D7E82)()
#define getTempOffset() (*(int32_t (*)(void))0x3D7E85)()
int DegreesC = 0;
//int32_t TempOffset = 0;
//int32_t TempSlope = 0;
//int sized = sizeof(int32_t*);

__interrupt void adc_isr(void);

void main(void)
{
    InitSysCtrl();

#if DOWNLOAD_TO_FLASH
    memcpy(&RamfuncsRunStart, &RamfuncsLoadStart, (Uint32)&RamfuncsLoadSize);
    InitFlash();
#endif

    // Disable CPU interrupts
    DINT;
    //Initialize the PIE control registers
    InitPieCtrl();
    // Disable CPU interrupts and clear all CPU interrupt flags:
    IER = 0x0000;
    IFR = 0x0000;
    InitPieVectTable();
    // Initialize the PIE vector table
    EALLOW;  // This is needed to write to EALLOW protected register
    PieVectTable.ADCINT1 = &adc_isr;
    EDIS;

    EALLOW;

    GpioCtrlRegs.GPBMUX1.bit.GPIO34 = 0; //default GPIO
    GpioCtrlRegs.GPBDIR.bit.GPIO34 = 0; //input
    GpioCtrlRegs.GPBPUD.bit.GPIO34 = 1; //disable pull-up

    GpioCtrlRegs.GPBMUX1.bit.GPIO39 = 0;
    GpioCtrlRegs.GPBDIR.bit.GPIO39 = 1; //output
    GpioCtrlRegs.GPBPUD.bit.GPIO39 = 1; //disable pull-up

    EDIS;

    InitAdc();  // For this example, init the ADC
    AdcOffsetSelfCal();

    // Enable ADCINT1 in PIE
    PieCtrlRegs.PIEIER1.bit.INTx1 = 1; // Enable INT 1.1 in the PIE
    IER |= M_INT1;                     // Enable CPU Interrupt 1
    EINT;

    // Enable Global interrupt INTM
    ERTM;

    EALLOW;

    FlashRegs.FOTPWAIT.bit.OTPWAIT = 1;


    AdcRegs.ADCCTL1.bit.INTPULSEPOS = 1; // ADCINT1 trips after AdcResults latch

    AdcRegs.ADCCTL2.bit.ADCNONOVERLAP = 1; // Enable non-overlap mode

    AdcRegs.ADCCTL1.bit.TEMPCONV = 1;

    AdcRegs.ADCSOC0CTL.bit.CHSEL = 5; //Set SOC0 to sample A5

    AdcRegs.ADCSOC0CTL.bit.ACQPS = 25; //Set SOC0 ACQPS to 26 ADCCLK
    AdcRegs.ADCSOC0CTL.bit.TRIGSEL = 0; // software only


    AdcRegs.INTSEL1N2.bit.INT1SEL = 0; //Connect ADCINT1 to EOC0
    AdcRegs.INTSEL1N2.bit.INT1E = 1; //Enable ADCINT1
    EDIS;

    AdcRegs.ADCSOCFRC1.bit.SOC0 = 1; //Sample temp sensor
    //DELAY_US(10U);
    //while (AdcRegs.ADCINTFLG.bit.ADCINT1 == 0){}
    //Wait for ADCINT1
    //AdcRegs.ADCINTFLGCLR.bit.ADCINT1 = 1; //Clear ADCINT1

    //int32_t sensorSample = AdcResult.ADCRESULT0; //Get temp sensor sample result

    //DegreesC = ((sensorSample - getTempOffset()) * getTempSlope()) / 32786; //0.162384 * 224


    DELAY_US(100000U);

	while(1){
	    //DELAY_US(100000U);

	    GpioDataRegs.GPBTOGGLE.bit.GPIO39 = 1;

	    if(GpioDataRegs.GPBDAT.bit.GPIO34){
	        //GpioDataRegs.GPBSET.bit.GPIO39 = 1;
	    }
	    else{
	        //GpioDataRegs.GPBCLEAR.bit.GPIO39 = 1;
	    }


	    DELAY_US(100000U);
	}
}
__interrupt void adc_isr(void)
{
    AdcRegs.ADCINTFLGCLR.bit.ADCINT1 = 1;

    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;   // Acknowledge interrupt to PIE

    AdcRegs.ADCSOCFRC1.bit.SOC0 = 1; //Sample temp sensor
    return;
}
