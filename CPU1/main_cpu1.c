//
// Included Files
//
#include "F28x_Project.h"
#include "F2837xD_Ipc_drivers.h"
#include "I2C_SSD1306.h"
#include "SCI.h"
#include "stdio.h"

// Message RAM Struct
typedef struct
{
    Uint32 counter;
    Uint16 direction;
} TRecvMsgRam;

//
// Main
//
void main(void)
{
//
// Step 1. Initialize System Control:
// PLL, WatchDog, enable Peripheral Clocks
// This example function is found in the F2837xD_SysCtrl.c file.
//
   InitSysCtrl();

#ifdef _STANDALONE
#ifdef _FLASH
//
// Send boot command to allow the CPU2 application to begin execution
//
IPCBootCPU2(C1C2_BROM_BOOTMODE_BOOT_FROM_FLASH);
#else
//
// Send boot command to allow the CPU2 application to begin execution
//
IPCBootCPU2(C1C2_BROM_BOOTMODE_BOOT_FROM_RAM);
#endif
#endif

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
    InitGpio(); // Skipped for this example
    InitEQepGpio();
    I2C_Init();
    EALLOW;
    GpioCtrlRegs.GPADIR.bit.GPIO31 = 1; // Output
    GpioCtrlRegs.GPBDIR.bit.GPIO34 = 1; // Output
    GpioCtrlRegs.GPBCSEL1.bit.GPIO34 = 2; // CPU2
    //GpioCtrlRegs.GPBCSEL1.bit.GPIO34 = 0; // CPU1 TESTING TODO

    //GpioCtrlRegs.GPDPUD.bit.GPIO104 = 1; // Disable pull-up on GPIO104/SDAA
    //GpioCtrlRegs.GPDPUD.bit.GPIO105 = 1; // Disable pull-up on GPIO105/SCLA
    //GpioCtrlRegs.GPDDIR.bit.GPIO104 = 1; // Set as output.
    //GpioCtrlRegs.GPDDIR.bit.GPIO105 = 1; // Set as output.
    //GpioCtrlRegs.GPDODR.bit.GPIO104 = 1; // Set as open drain output.
    //GpioCtrlRegs.GPDODR.bit.GPIO105 = 1; // Set as open drain output.
    //GpioCtrlRegs.GPDGMUX1.bit.GPIO104 = 0; // Set MUX to row 01 for SDAA
    //GpioCtrlRegs.GPDGMUX1.bit.GPIO105 = 0; // Set MUX to row 01 for SCLA
    //GpioCtrlRegs.GPDMUX1.bit.GPIO104 = 1; // Set MUX to row 01 for SDAA
    //GpioCtrlRegs.GPDMUX1.bit.GPIO105 = 1; // Set MUX to row 01 for SCLA

    GPIO_SetupPinMux(104, GPIO_MUX_CPU1, 1);
    GPIO_SetupPinMux(105, GPIO_MUX_CPU1, 1);

    // Serial through FTDI setup.
    GPIO_SetupPinMux(43, GPIO_MUX_CPU1, 15);
    GPIO_SetupPinMux(42, GPIO_MUX_CPU1, 15);

    //GPIO_SetupPinOptions(34, GPIO_OUTPUT, GPIO_PUSHPULL);
    //GPIO_SetupPinMux(34, GPIO_MUX_CPU2, 0);
//
// TODO Add code to allow configuration of GPADIR from CPU02 using IPC
//
    EDIS;
    GpioDataRegs.GPADAT.bit.GPIO31 = 1;// turn off LED

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
    //PieVectTable.I2CA_INT = &I2CA_ISR; // Set interrupt vector.
    InitPieVectTable();
    //PieCtrlRegs.PIEIER8.bit.INTx1 = 1; // Enable interrupt according to 2.4.5 PIE Channel Mapping table.
    //PieCtrlRegs.PIECTRL.bit.ENPIE = 1; // Enable the PIE.
//
// Enable global Interrupts and higher priority real-time debug events:
//
    //IER |= M_INT8; // Enable INT8 from PIE
    EINT;  // Enable Global interrupt INTM
    ERTM;  // Enable Global realtime interrupt DBGM

    I2C_Display_Init();     // Initialize I2C Display
    scia_fifo_init();       // Initialize the SCI FIFO
    scia_init();            // Initialize SCI for echoback
//
// Step 6. IDLE loop. Just sit and loop forever (optional):
//
    // Wait for CPU2 to come up.
    while( IpcRegs.IPCBOOTSTS != 0xFFFFFFFF ){}
    GpioDataRegs.GPADAT.bit.GPIO31 = 0;
    DELAY_US(1000 * 100);
    GpioDataRegs.GPADAT.bit.GPIO31 = 1;
    DELAY_US(1000 * 100);
    GpioDataRegs.GPADAT.bit.GPIO31 = 0;
    DELAY_US(1000 * 100);
    GpioDataRegs.GPADAT.bit.GPIO31 = 1;

    EALLOW;
    GpioCtrlRegs.GPACSEL4.bit.GPIO31 = 2; // CPU2
    EDIS;


    I2C_Display_String("Encoder Dir:", 0, 0);
    I2C_Display_String("Encoder:", 0, 1);

    volatile TRecvMsgRam * const RecvMsgRam = (TRecvMsgRam *)RECV_MSG_RAM;
    char c[22];
    char *msg;
    char *ReceivedChar;
    for(;;)
    {
        //IpcRegs.IPCSET.bit.IPC0 = 1;
        //while( IpcRegs.IPCFLG.bit.IPC0 != 0 ){;}
        msg = "\r\n\n\nHello World!";
        scia_msg(msg);

        sprintf(c, "%08lX", RecvMsgRam->counter);
        I2C_Display_String(c, 8, 1);

        if ( RecvMsgRam->direction == 1 )
        {
            I2C_Display_String("Foward  ", 12, 0);
        }
        else
        {
            I2C_Display_String("Backward", 12, 0);
        }

        //
        // Wait for inc character
        //
        while(SciaRegs.SCIFFRX.bit.RXFFST == 0) { } // wait for empty state

        //
        // Get character
        //
        *ReceivedChar = SciaRegs.SCIRXBUF.all;
        I2C_Display_String(ReceivedChar, 0, 2);
    }
}

//
// End of file
//
