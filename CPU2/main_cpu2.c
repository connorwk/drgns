//
// Included Files
//
#include "F28x_Project.h"

#ifdef _FLASH
//
// These are defined by the linker (see device linker command file)
//
extern Uint16 RamfuncsLoadStart;
extern Uint16 RamfuncsLoadSize;
extern Uint16 RamfuncsRunStart;
#endif

interrupt void IPC0ISR(void)
{
    if ( GpioDataRegs.GPBDAT.bit.GPIO34 == 1 )
    {
        GpioDataRegs.GPBDAT.bit.GPIO34 = 0;
    }
    else
    {
        GpioDataRegs.GPBDAT.bit.GPIO34 = 1;
    }
    DELAY_US(1000 * 500);
    IpcRegs.IPCACK.bit.IPC0 = 1;
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;
}

//*(unsigned *)SEND_MSG_RAM = x;
//((unsigned *)SEND_MSG_RAM)[n] = x;
typedef struct
{
    Uint32 counter;
    Uint16 direction;
} TSendMsgRam;

//
// Main
//
void main(void)
{
//
// Copy time critical code and Flash setup code to RAM
// This includes InitFlash(), Flash API functions and any functions that are
// assigned to ramfuncs section.
// The  RamfuncsLoadStart, RamfuncsLoadEnd, and RamfuncsRunStart
// symbols are created by the linker. Refer to the device .cmd file.
//
#ifdef _FLASH
  memcpy(&RamfuncsRunStart, &RamfuncsLoadStart, (size_t)&RamfuncsLoadSize);
#endif

//
// Step 1. Initialize System Control:
// PLL, WatchDog, enable Peripheral Clocks
// This example function is found in the F2837xD_SysCtrl.c file.
//
    InitSysCtrl();

//
// Call Flash Initialization to setup flash waitstates
// This function must reside in RAM
//
#ifdef _FLASH
   InitFlash();
#endif

//
// Step 2. Initialize GPIO:
// This example function is found in the F2837xD_Gpio.c file and
// illustrates how to set the GPIO to it's default state.
//
//    InitGpio(); // Skipped for this example

    EALLOW;
//
//TODO Add code to configure GPADIR through IPC
//
    //GPIO_WritePin(34, 1);

//
// Step 3. Clear all interrupts and initialize PIE vector table:
// Disable CPU interrupts
//
    DINT;

//
// Initialize the PIE control registers to their default state.
// The default state is all PIE interrupts disabled and flags
// are cleared.
// This function is found in the F2837xD_PieCtrl.c file.
//
    InitPieCtrl();

//
// Disable CPU interrupts and clear all CPU interrupt flags:
//
    IER = 0x0000;
    IFR = 0x0000;

//
// Initialize the PIE vector table with pointers to the shell Interrupt
// Service Routines (ISR).
// This will populate the entire table, even if the interrupt
// is not used in this example.  This is useful for debug purposes.
// The shell ISR routines are found in F2837xD_DefaultIsr.c.
// This function is found in F2837xD_PieVect.c.
//
    //PieVectTable.IPC0_INT = &IPC0ISR;
    InitPieVectTable();
    //PieCtrlRegs.PIEIER1.bit.INTx13 = 1;
    //PieCtrlRegs.PIECTRL.bit.ENPIE = 1;

//
// Enable global Interrupts and higher priority real-time debug events:
//
    //IER |= M_INT1;
    EINT;  // Enable Global interrupt INTM
    ERTM;  // Enable Global realtime interrupt DBGM

//
// Step 6. IDLE loop. Just sit and loop forever (optional):
//
    IpcRegs.IPCBOOTSTS = 0xFFFFFFFF;
    if ( IpcRegs.IPCBOOTSTS == 0xFFFFFFFF ){
        IpcRegs.IPCBOOTSTS = 0x00000000;
    }
    DELAY_US(1000 * 500);

    GpioDataRegs.GPADAT.bit.GPIO31 = 0;
    GpioDataRegs.GPBDAT.bit.GPIO34 = 1;
    DELAY_US(1000 * 100);
    GpioDataRegs.GPADAT.bit.GPIO31 = 1;
    GpioDataRegs.GPBDAT.bit.GPIO34 = 0;
    DELAY_US(1000 * 100);
    GpioDataRegs.GPADAT.bit.GPIO31 = 0;
    GpioDataRegs.GPBDAT.bit.GPIO34 = 1;
    DELAY_US(1000 * 100);
    GpioDataRegs.GPADAT.bit.GPIO31 = 1;
    GpioDataRegs.GPBDAT.bit.GPIO34 = 0;
    DELAY_US(1000 * 100);
    GpioDataRegs.GPADAT.bit.GPIO31 = 0;

    //
    // Initialize eQEP.
    //

    InitEQep();

    TSendMsgRam * const SendMsgRam = (TSendMsgRam *)SEND_MSG_RAM;

    for(;;)
    {
        //if ( EQep1Regs.QEPSTS.bit.QDF == 1 )
        //{
        //    GpioDataRegs.GPADAT.bit.GPIO31 = 0;
        //}
        //else
        //{
        //    GpioDataRegs.GPADAT.bit.GPIO31 = 1;
        //}

        //if ( EQep1Regs.QPOSCNT & 0x00000100 )
        //{
        //   GpioDataRegs.GPBDAT.bit.GPIO34 = 0;
        //}
        //else
        //{
        //    GpioDataRegs.GPBDAT.bit.GPIO34 = 1;
        //}
        SendMsgRam->counter = EQep1Regs.QPOSCNT;
        SendMsgRam->direction = EQep1Regs.QEPSTS.bit.QDF;
    }
}

//
// End of file
//
