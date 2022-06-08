

/**
 * main.c
 */
#include "DSP28x_Project.h"
#include <stdint.h>
#define DOWNLOAD_TO_FLASH 1

//0x3D7E82 - Slope (oC / LSB, fixed-point Q15 format)
//0x3D7E85 - Offset (0 oC LSB value)
#define LED_BLUE_OFF (GpioDataRegs.GPBSET.bit.GPIO39 = 1)
#define LED_BLUE_ON (GpioDataRegs.GPBCLEAR.bit.GPIO39 = 1)

#define getTempSlope() (*(int32_t (*)(void))0x3D7E82)()
#define getTempOffset() (*(int32_t (*)(void))0x3D7E85)()
volatile int DegreesC = 0;
//int32_t TempOffset = 0;
//int32_t TempSlope = 0;
//int sized = sizeof(int32_t*);
volatile int16_t buttonPressed = 0;
__interrupt void adc_isr(void);
interrupt void xint1_isr(void);

void pwm_init();


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
    PieVectTable.XINT1 = &xint1_isr;
    EDIS;

    EALLOW;

    GpioCtrlRegs.GPBMUX1.bit.GPIO34 = 0; //default GPIO
    GpioCtrlRegs.GPBDIR.bit.GPIO34 = 0; //input
    GpioCtrlRegs.GPBPUD.bit.GPIO34 = 1; //disable pull-up

    GpioCtrlRegs.GPBMUX1.bit.GPIO39 = 0;
    GpioCtrlRegs.GPBDIR.bit.GPIO39 = 1; //output
    GpioCtrlRegs.GPBPUD.bit.GPIO39 = 1; //disable pull-up

    GpioCtrlRegs.GPAMUX1.bit.GPIO2 = 0;         // GPIO2
    GpioCtrlRegs.GPAPUD.bit.GPIO2 = 0; //enable pull-up
    GpioCtrlRegs.GPADIR.bit.GPIO2 = 0;          // input
    GpioCtrlRegs.GPAQSEL1.bit.GPIO2 = 0;        // Synch to SYSCLOUT
    GpioIntRegs.GPIOXINT1SEL.bit.GPIOSEL = 2; // use GPIO2

    XIntruptRegs.XINT1CR.bit.POLARITY = 0; // Falling edge interrupt
    XIntruptRegs.XINT1CR.bit.ENABLE = 1;        // Enable XINT1
    EDIS;
    LED_BLUE_OFF;
    InitAdc();  // For this example, init the ADC
    AdcOffsetSelfCal();

    // Enable ADCINT1 in PIE
    // page 175
    PieCtrlRegs.PIEIER1.bit.INTx1 = 1;  // Enable INT 1.1 in the PIE
    //PieCtrlRegs.PIECTRL.bit.ENPIE
    PieCtrlRegs.PIEIER1.bit.INTx4 = 1;  // Enable INT 1.4 in the PIE
    IER |= M_INT1;                      // Enable CPU Interrupt 1
    //IER |= M_INT10;                      // Enable CPU Interrupt 1
    EINT;                               // Enable Global Interrupts
    ERTM;                               // Enable Global real-time debug DBGM.

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

    pwm_init();

    EPwm1Regs.CMPA.half.CMPA = 100;  //设置占空比
    EPwm1Regs.CMPB = 100;  //设置占空比

    DELAY_US(100000U);

	while(1){
	    //DELAY_US(100000U);
	    if(buttonPressed){
	        LED_BLUE_ON;
            DELAY_US(1000000U);
            LED_BLUE_OFF;
            buttonPressed = 0;
	    }
	    if(GpioDataRegs.GPBDAT.bit.GPIO34){
	        //GpioDataRegs.GPBSET.bit.GPIO39 = 1;
	    }
	    else{
	        //GpioDataRegs.GPBCLEAR.bit.GPIO39 = 1;
	    }



	}
}

void pwm_init()
{
    //manual page 388
    EALLOW;

    //ePWM Module Time Base Clock Sync
    SysCtrlRegs.PCLKCR0.bit.TBCLKSYNC = 0;   // Disable TBCLK within the ePWM
    SysCtrlRegs.PCLKCR1.bit.EPWM1ENCLK = 1;  // enable ePWM1 clock

    GpioCtrlRegs.GPAPUD.bit.GPIO0 = 1;    // Disable pull-up on GPIO0 (EPWM1A)
    GpioCtrlRegs.GPAPUD.bit.GPIO1 = 1;    // Disable pull-up on GPIO1 (EPWM1B)
    GpioCtrlRegs.GPAMUX1.bit.GPIO0 = 1;   // Configure GPIO0 as EPWM1A
    GpioCtrlRegs.GPAMUX1.bit.GPIO1 = 1;   // Configure GPIO1 as EPWM1B

    EDIS;

    // Interrupts that are used in this example are re-mapped to
    // ISR functions found within this file.
    //EALLOW;  // This is needed to write to EALLOW protected registers
    //PieVectTable.EPWM1_INT = &epwm1_isr;
    //EDIS;    // This is needed to disable write to EALLOW protected registers

    //EALLOW;
    //SysCtrlRegs.PCLKCR0.bit.TBCLKSYNC = 0;      // Stop all the TB clocks
    //EDIS;

    // Setup Sync

    //Time-Base Control Register
    EPwm1Regs.TBCTL.bit.SYNCOSEL = TB_SYNC_DISABLE;  // Pass through

    EPwm1Regs.TBCTL.bit.PHSEN = TB_DISABLE; // Allow each timer to be sync'ed

    // Time-Base Phase Register
    EPwm1Regs.TBPHS.half.TBPHS = 0;
    // Time-Base Counter Register
    EPwm1Regs.TBCTR = 0x0000;                  // Clear counter
    // Time-Base Period Register 0000~FFFF
    EPwm1Regs.TBPRD = 15000;
    EPwm1Regs.TBCTL.bit.HSPCLKDIV = TB_DIV1;
    EPwm1Regs.TBCTL.bit.CLKDIV = TB_DIV1;  // Frequency = 90MHz / 1 / 1 / 15000 = 6KHz
    EPwm1Regs.TBCTL.bit.CTRMODE = TB_COUNT_UP;


    // Setup shadow register load on ZERO

    // Counter-Compare Control Register
    EPwm1Regs.CMPCTL.bit.SHDWAMODE = CC_SHADOW;
    EPwm1Regs.CMPCTL.bit.SHDWBMODE = CC_SHADOW;
    EPwm1Regs.CMPCTL.bit.LOADAMODE = CC_CTR_ZERO;
    EPwm1Regs.CMPCTL.bit.LOADBMODE = CC_CTR_ZERO;

    // Set Compare values

    // Counter-Compare A Register, compare to TBCTR
    EPwm1Regs.CMPA.half.CMPA = 0;    // Set compare A value

    //Counter-Compare B Register
    EPwm1Regs.CMPB = 0;              // Set Compare B value

    // Set actions

    // Action-Qualifier Output A Control Register
    EPwm1Regs.AQCTLA.bit.ZRO = AQ_SET;            // Set PWM1A on Zero
    EPwm1Regs.AQCTLA.bit.CAU = AQ_CLEAR;     // Clear PWM1A on event A, up count

    // Action-Qualifier Output B Control Register
    EPwm1Regs.AQCTLB.bit.ZRO = AQ_SET;            // Set PWM1B on Zero
    EPwm1Regs.AQCTLB.bit.CBU = AQ_CLEAR;     // Clear PWM1B on event B, up count

    // Active Low PWMs - Setup Deadband
    /*    EPwm1Regs.DBCTL.bit.OUT_MODE = DB_FULL_ENABLE;
     EPwm1Regs.DBCTL.bit.POLSEL = DB_ACTV_LO;
     EPwm1Regs.DBCTL.bit.IN_MODE = DBA_ALL;
     EPwm1Regs.DBRED = 100;
     EPwm1Regs.DBFED = 100;*/

    //EPwm1Regs.ETSEL.bit.INTSEL = ET_CTR_ZERO;     // Select INT on Zero event
    //EPwm1Regs.ETSEL.bit.INTEN = 0;  // Enable INT
    //EPwm1Regs.ETPS.bit.INTPRD = ET_1ST;           // Generate INT on 1st event

    EALLOW;
    SysCtrlRegs.PCLKCR0.bit.TBCLKSYNC = 1;        // Start all the timers synced
    EDIS;
}
interrupt void xint1_isr(void)
{
    if(buttonPressed==0){
        buttonPressed = 1;
    }

    //DegreesC = 0;
    //XIntruptRegs.XINT1CTR.bit.
    //PieCtrlRegs.PIEIFR1.bit.INTx1
    // Acknowledge this interrupt to get more from group 1
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;
}
__interrupt void adc_isr(void)
{
    int32_t sensorSample = AdcResult.ADCRESULT0; //Get temp sensor sample result

    DegreesC = ((sensorSample - getTempOffset()) * getTempSlope()) / 32786; //0.162384 * 224

    AdcRegs.ADCINTFLGCLR.bit.ADCINT1 = 1;
    AdcRegs.ADCSOCFRC1.bit.SOC0 = 1; //Sample temp sensor

    PieCtrlRegs.PIEACK.bit.ACK1 = 1;//PIEACK_GROUP1;   // Acknowledge interrupt to PIE
}
