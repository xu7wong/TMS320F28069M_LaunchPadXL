//
// Included Files
//
#include "DSP28x_Project.h"         // DSP280xx Headerfile
#include "SFO_V6.h"

//
//                   *!!IMPORTANT!!*
// UPDATE NUMBER OF HRPWM CHANNELS + 1  USED IN SFO_V6.H

//
// Configured for 4 HRPWM channels, but FF2806x has a maximum of
// 7 HRPWM channels (8=7+1)
//
// i.e.: #define PWM_CH 5

//
// Function Prototypes
//
void HRPWM_Config(Uint16);
void error(void);

//
// Globals
//
Uint16 UpdateFine, status;
Uint32 PeriodFine;
//
// The following declarations are required in order to use the SFO
// library functions:
//

//
// Global variable used by the SFO library. Result can be used for all HRPWM
// channels. This variable is also copied to HRMSTEP register by SFO() function
//
int MEP_ScaleFactor;

//
// Array of pointers to EPwm register structures:
// *ePWM[0] is defined as dummy value not used in the example
//
volatile struct EPWM_REGS *ePWM[PWM_CH] = { &EPwm1Regs, &EPwm1Regs, &EPwm2Regs,
                                            &EPwm3Regs, &EPwm4Regs };

//
// Main
//
void main(void)
{
    //
    // Step 1. Initialize System Control:
    // PLL, WatchDog, enable Peripheral Clocks
    // This example function is found in the F2806x_SysCtrl.c file.
    //
    InitSysCtrl();

    //
    // Step 2. Initalize GPIO:
    // This example function is found in the F2806x_Gpio.c file and
    // illustrates how to set the GPIO to it's default state.
    //
    //InitGpio();  // Skipped for this example
    InitEPwmGpio();

    //
    // Step 3. Clear all interrupts and initialize PIE vector table:
    // Disable CPU interrupts
    //
    DINT;

    //
    // Initialize the PIE control registers to their default state.
    // The default state is all PIE interrupts disabled and flags
    // are cleared.
    // This function is found in the F2806x_PieCtrl.c file.
    //
    InitPieCtrl();

    //
    // Disable CPU interrupts and clear all CPU interrupt flags
    //
    IER = 0x0000;
    IFR = 0x0000;

    //
    // Initialize the PIE vector table with pointers to the shell Interrupt
    // Service Routines (ISR).
    // This will populate the entire table, even if the interrupt
    // is not used in this example.  This is useful for debug purposes.
    // The shell ISR routines are found in F2806x_DefaultIsr.c.
    // This function is found in F2806x_PieVect.c.
    //
    InitPieVectTable();

    //
    // Step 4. Initialize all the Device Peripherals:
    // This function is found in F2806x_InitPeripherals.c
    //
    //InitPeripherals();  // Not required for this example

    //
    // For this example, only initialize the ePWM
    // Step 5. User specific code, enable interrupts:
    //
    UpdateFine = 1;
    PeriodFine = 0;
    status = SFO_INCOMPLETE;

    //
    // Calling SFO() updates the HRMSTEP register with calibrated MEP_ScaleFactor.
    // HRMSTEP must be populated with a scale factor value prior to enabling
    // high resolution period control.
    //
    while (status == SFO_INCOMPLETE)
    {
        //
        // Call until complete
        //
        status = SFO();
        if (status == SFO_ERROR)
        {
            //
            // SFO function returns 2 if an error occurs & # of MEP
            // steps/coarse step exceeds maximum of 255.
            //
            error();
        }
    }

    //
    // Some useful Period vs Frequency values
    //  SYSCLKOUT =     80 MHz
    //  ---------------------------
    //  Period          Frequency
    //  1000            80 kHz
    //  800             100 kHz
    //  600             133 kHz
    //  500             160 kHz
    //  250             320 kHz
    //  200             400 kHz
    //  100             800 kHz
    //  50              1.6 Mhz
    //  25              3.2 Mhz
    //  20              4.0 Mhz
    //  12              6.7 MHz
    //  10              8.0 MHz
    //  9               8.9 MHz
    //  8               10.0 MHz
    //  7               11.4 MHz
    //  6               13.3 MHz
    //  5               16.0 MHz
    //

    //
    // ePWM and HRPWM register initialization
    //
    HRPWM_Config(20);        // ePWMx target
    for (;;)
    {

        //
        // Sweep PeriodFine as a Q16 number from 0.2 - 0.999
        //
        //Uint32 i = (18U*256*256);
        //Uint32 j = (18U*256*256)+(2U*256*256);
        for (PeriodFine = 579648; PeriodFine < 1379648; PeriodFine++)
        {
            DELAY_US(5);
            if (UpdateFine)
            {
                //
                // Because auto-conversion is enabled, the desired
                // fractional period must be written directly to the
                // TBPRDHR (or TBPRDHRM) register in Q16 format
                // (lower 8-bits are ignored)
                //EPwm1Regs.TBPRDHR = PeriodFine;

                //
                // The hardware will automatically scale
                // the fractional period by the MEP_ScaleFactor
                // in the HRMSTEP register (which is updated
                // by the SFO calibration software).

                //
                // Hardware conversion:
                // MEP delay movement = ((TBPRDHR(15:0) >> 8) *
                //                       HRMSTEP(7:0) + 0x80) >> 8
                //
                Uint16 p_c = (Uint16)((PeriodFine & 0xFFFF0000)>>16);
                Uint16 p_f = (Uint16)(PeriodFine & 0x0000FFFF);
                EPwm1Regs.CMPA.half.CMPA = p_c / 2;     // set duty 50% initially
                EPwm1Regs.CMPA.half.CMPAHR = p_f/2;//(1 << 8);
                EPwm1Regs.TBPRD = p_c;
                EPwm1Regs.TBPRDHR = p_f;//PeriodFine; //In Q16 format
            }
            else
            {
                //
                // No high-resolution movement on TBPRDHR.
                //
                EPwm1Regs.TBPRDHR = 0;
            }

            //
            // Call the scale factor optimizer lib function SFO(0)
            // periodically to track for any change due to temp/voltage.
            // This function generates MEP_ScaleFactor by running the
            // MEP calibration module in the HRPWM logic. This scale
            // factor can be used for all HRPWM channels. HRMSTEP
            // register is automatically updated by the SFO function.
            //

            //
            // in background, MEP calibration module continuously updates
            // MEP_ScaleFactor
            //
            status = SFO();
            if (status == SFO_ERROR)
            {
                //
                // SFO function returns 2 if an error occurs & # of MEP
                // steps/coarse step exceeds maximum of 255.
                //
                error();
            }
        }
    }
}

//
// HRPWM_Config - Configures ePWM1 and sets up HRPWM on ePWM1A
//
// PARAMETERS:  period - desired PWM period in TBCLK counts
// RETURN:      N/A
//
void HRPWM_Config(Uint16 period)
{
    //
    // ePWM channel register configuration with HRPWM
    // ePWMxA toggle low/high with MEP control on Rising edge
    //
    EALLOW;
    SysCtrlRegs.PCLKCR0.bit.TBCLKSYNC = 0;     // Disable TBCLK within the EPWM
    EDIS;

    EPwm1Regs.TBCTL.bit.PRDLD = TB_SHADOW;     // set Shadow load
    EPwm1Regs.TBPRD = period;                  // PWM frequency = 1/(2*TBPRD)
    EPwm1Regs.CMPA.half.CMPA = period / 2;     // set duty 50% initially
    EPwm1Regs.CMPA.half.CMPAHR = (1 << 8);     // initialize HRPWM extension
    EPwm1Regs.CMPB = period / 2;               // set duty 50% initially
    EPwm1Regs.TBPHS.all = 0;
    EPwm1Regs.TBCTR = 0;

    EPwm1Regs.TBCTL.bit.CTRMODE = TB_COUNT_UPDOWN; // Select up-down count mode

    //
    // TBCTR phase load on SYNC (required for updown count HR control
    //
    EPwm1Regs.TBCTL.bit.PHSEN = TB_ENABLE;

    EPwm1Regs.TBCTL.bit.SYNCOSEL = TB_SYNC_DISABLE;
    EPwm1Regs.TBCTL.bit.HSPCLKDIV = TB_DIV1;
    EPwm1Regs.TBCTL.bit.CLKDIV = TB_DIV1;         // TBCLK = SYSCLKOUT
    EPwm1Regs.TBCTL.bit.FREE_SOFT = 11;

    EPwm1Regs.CMPCTL.bit.LOADAMODE = CC_CTR_ZERO; // LOAD CMPA on CTR = 0
    EPwm1Regs.CMPCTL.bit.LOADBMODE = CC_CTR_ZERO;
    EPwm1Regs.CMPCTL.bit.SHDWAMODE = CC_SHADOW;
    EPwm1Regs.CMPCTL.bit.SHDWBMODE = CC_SHADOW;

    EPwm1Regs.AQCTLA.bit.CAU = AQ_SET;            // PWM toggle high/low
    EPwm1Regs.AQCTLA.bit.CAD = AQ_CLEAR;
    EPwm1Regs.AQCTLB.bit.CAU = AQ_SET;            // PWM toggle high/low
    EPwm1Regs.AQCTLB.bit.CAD = AQ_CLEAR;

    EALLOW;
    EPwm1Regs.HRCNFG.all = 0x0;
    EPwm1Regs.HRCNFG.bit.EDGMODE = HR_BEP;    // MEP control on both edges
    EPwm1Regs.HRCNFG.bit.CTLMODE = HR_CMP;    // CMPAHR and TBPRDHR HR control

    //
    // load on CTR = 0 and CTR = TBPRD
    //
    EPwm1Regs.HRCNFG.bit.HRLOAD = HR_CTR_ZERO_PRD;

    EPwm1Regs.HRCNFG.bit.AUTOCONV = 1;   // Enable autoconversion for HR period

    //
    // Enable TBPHSHR sync (required for updwn count HR control)
    //
    EPwm1Regs.HRPCTL.bit.TBPHSHRLOADE = 1;

    //
    // Turn on high-resolution period control
    //
    EPwm1Regs.HRPCTL.bit.HRPE = 1;

    SysCtrlRegs.PCLKCR0.bit.TBCLKSYNC = 1;  // Enable TBCLK within the EPWM

    //
    // Synchronize high resolution phase to start HR period
    //
    EPwm1Regs.TBCTL.bit.SWFSYNC = 1;

    EDIS;
}

//
// error -
//
void error(void)
{
    ESTOP0;
    // Stop here and handle error
}

//
// End of File
//

