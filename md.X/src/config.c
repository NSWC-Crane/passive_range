#include "../include/md.h"
#include "../include/config.h"
#include <xc.h>
#include <plib.h>

/*---------------Initialization Routines---------------*/
void init_ADC(void)
{
//    // ADC Configuration & Intialize
//    AD1CSSLbits.CSSL2 = 1; // Turn on AN2, AN4, AN8, AN10, AN12
//    AD1CSSLbits.CSSL4 = 1;
//    AD1CSSLbits.CSSL8 = 1;
//    AD1CSSLbits.CSSL10 = 1;
//    AD1CSSLbits.CSSL12 = 1;
//
    AD1PCFG = 0x0000FFFF;       // AN0-15 = digital; none = analog
//    //PMCONbits.ON = 0;
//    
//    AD1CON1bits.FORM = 0;       // Integer 16-bit (DOUT = 00dd dddd dddd)
//    AD1CON1bits.SSRC = 2;       // PR3 match ends sampling and starts conversion
//    AD1CON1bits.SIDL = 1;       // Discontinue module operation in Idle mode
//    AD1CON1bits.ASAM = 1;       // Sampling begins immediately after last conversion completes
//
//    AD1CON2bits.VCFG = 3;       // AVdd = Vref+, AVss = Vref-
//    AD1CON2bits.OFFCAL = 0;     // Disable Offset Cal mode
//    AD1CON2bits.CSCNA = 1;      // Scan Inputs
//    AD1CON2bits.SMPI = 5;       // Interrupts at the completion the 5th conversion
//    AD1CON2bits.BUFM = 0;       // Buffer configured as 16-word buffer
//    AD1CON2bits.ALTS = 0;       // Alternate Input Sample mode, 0-> Always use MUX A
//
//    AD1CON3bits.ADRC = 0;       // Clock from PBCLK
//    AD1CON3bits.SAMC = 14;      // 14 Tad
//    AD1CON3bits.ADCS = 159;     // TAD = TPB*2*(ADCS + 1)
//
//    AD1CHS = 0x00000000;        // MUXA: - = Vr-, + = Vr+
    AD1CON1bits.ADON = 0;       // Turn ADC on
}

void init_Timers(void)
{
    // Configure Timer 1 - used for general timing and delays
    T1CONbits.SIDL = 1;         // Discontinue operation in Idle mode
    T1CONbits.TGATE = 0;        // Gated time accumulation is disabled
    //T1CONbits.TWDIS = 1;        // Back-to-back writes are disabled
    T1CONbits.TCKPS = 2;        // 1:64 prescale value, FPBclk = 80MHz -> T1 period ~= 0.8us
    T1CONbits.TSYNC = 0;        // 
    T1CONbits.TCS = 0;          // Internal PBCLK
    PR1 = 0xFFFF;               // Period Register to max
    T1CONbits.ON = 1;           // Turn Timer1 on
    TMR1 = 0;                   // set Timer1 counter = 0

    // Configure Timer 2 - PR2 match stores max value from T3 ADC conversions in FIFO buffer
    //T2CONbits.TCS = 0;
    T2CONbits.SIDL = 0;         // Do not Discontinue operation in Idle mode
    T2CONbits.TGATE = 0;        // Gated time accumulation is disabled
    T2CONbits.TCKPS = 0;        // 1:2 prescale value, FPBclk = 80MHz -> T2 period ~= 0.01250us
    T2CONbits.T32 = 0;          // TMR2 and TMR3 form separate 16-bit timer
    PR2 = 15942;                // Period Register (0.01250us)*PR2 = ~200 us => 5kHz
                                // A3 -> 15942
                                // A5 -> 15940    
    tick_time = 200;            // sets the value of a tick in us - based on PR2 and T2 prescale
    T2CONbits.ON = 1;           // Turn Timer2 on
    TMR2 = 0;                   // set Timer2 counter = 0
    
    // Configure Timer 3 - PR3 match triggers ADC conversion
    T3CONbits.SIDL = 1;         // Discontinue operation in Idle mode
    T3CONbits.TGATE = 0;        // Gated time accumulation is disabled
    T3CONbits.TCKPS = 3;        // 1:8 prescale value -> T3 period = 0.1us; original 64/304
    PR3 = 190;                  // Period Register: T3per = (0.1us)*PR3 = ~20us => 50kHz
    T3CONbits.ON = 1;           // Turn Timer3 on
    TMR3 = 0;                   // set Timer3 counter = 0

}

void init_Clock(void)
{
    // Initialize & Enable OSCCON & RTCC
    SYSKEY = 0x0;               // ensure OSCCON is locked
    SYSKEY = 0xAA996655;        // Write Key1 to SYSKEY
    SYSKEY = 0x556699AA;        // Write Key2 to SYSKEY

    OSCCONbits.COSC = 3;        // Enable Primary OSC w/PLL
    OSCCONbits.SLPEN = 1;       // Enable Sleep on "wait"
    OSCCONbits.UFRCEN = 0;      // USB PLL as USB Clock Source
    OSCCONbits.SOSCEN = 1;      // Enable Secondary OSC
    OSCCONbits.PBDIV = 0;       // Peripheral Bus CLock diveded by 1

    RTCCONbits.RTCWREN = 1;     // Enable write operation for RTCC

    SYSKEY = 0x0;               // Re-Lock OSCCON

}

void init_RTCC(void)
{
    // RTCC Configuration
    RTCCONbits.CAL = 0;         // No Cal
    RTCCONbits.SIDL = 0;        // Continue normal operation in Idle mode
    RTCCONbits.RTSECSEL = 1;    // Seconds Clock is selected for the RTCC pin
    RTCCONbits.RTCOE = 1;       // RTCC clock output enabled

    RTCTIME = 0x08570000;       // Time set for 08:57:00
    RTCDATE = 0x06072805;       // Date set for 28 July 06, Friday

    RTCALRMbits.ALRMEN = 1;     // Alarm is enabled
    RTCALRMbits.AMASK = 1;      // Alarm Every second
    RTCALRMbits.CHIME = 1;      // Chime is enabled
    RTCALRMbits.ARPT = 0;       // Alarm Repeat Value

    RTCCONbits.ON = 0;          // Turn on RTCC module

}

void init_UART(void)
{
    // UART2 Configuration & Initialization - 8N1, Baud rate set in U2BRG
    U2MODEbits.SIDL = 1;        // Discontinue operation in Idle mode
    U2MODEbits.IREN = 0;        // IrDA is disabled
    U2MODEbits.RTSMD = 1;       // UxRTS pin is in Simplex mode
    U2MODEbits.UEN = 0;         // UxTX and UxRX pins are enabled and used
    U2MODEbits.WAKE = 0;        // Wake-up disabled
    U2MODEbits.LPBACK = 0;      // Loopback mode is disabled
    U2MODEbits.ABAUD = 0;       // Auto Baud rate measurement disabled
    U2MODEbits.RXINV = 0;       // UxRX Idle state is 1
    U2MODEbits.BRGH = 0;        // Standard Speed mode ? 16x baud clock enabled
    U2MODEbits.PDSEL = 0;       // 8-bit data, no parity
    U2MODEbits.STSEL = 0;       // 1 Stop bit

    U2STAbits.ADM_EN = 0;       // Automatic Address Detect mode is disabled
    U2STAbits.UTXISEL = 2;      // Interrupt while the transmit buffer is empty
    U2STAbits.UTXINV = 0;       // UxTX Idle state is ?1?
    U2STAbits.URXEN = 1;        // UARTx receiver is enabled
    U2STAbits.UTXBRK = 0;       // Break transmission is disabled
    U2STAbits.UTXEN = 1;        // UARTx transmitter is enabled
    U2STAbits.URXISEL = 0;      // Interrupt flag bit is set while receive buffer is not empty
    U2STAbits.ADDEN = 0;        // Address Detect mode is disabled

    // Fpb = 80MHz; BRG = Fpb/(16*baudrate) - 1
    U2BRG = 0x00000013;         // BRG = 42 (0x2A); 115.2k baud rate; 0.9% error
                                // BRG = 19 (0x13); 250k baud rate; 0.00% error
                                // BRG = 21 (0x15); 230.4k baud rate; -1.36% error
                                // BRG = 10 (0x0A); 460.8k baud rate; -1.36% error
                                // BRG = 9 (0x09); 500k baud rate; 0.00% error

    U2MODEbits.ON = 1;          // Turn UART2 On
}

//void init_CN(void)
//{
//    // Configure Change Notification
//    //TRISGbits.TRISG6 = 1;       // set TRIS status as input
//    TRISGbits.TRISG7 = 1;       // set TRIS status as input
//    //CN1 = 0;
//
//    // enable change notification pins
//    //CNENbits.CNEN8 = 1;         // enable CN8/RG6
//    //CNENbits.CNEN9 = 1;         // enable CN9/RG7
//
//    // enable pull-up resistors
//    //CNPUEbits.CNPUE8 = 1;
//    //CNPUEbits.CNPUE9 = 1;
//    
//    CNCONbits.SIDL = 1;         // Idle Mode halts CN operation
//    CNCONbits.ON = 1;           // turn change notification on
//
//}

void init_SPI3(void)
{
    // configure TRIS Registers
    TRISDbits.TRISD1 = 0;       // Clock = Outout
    TRISDbits.TRISD2 = 1;       // MISO/SDI = Input
    TRISDbits.TRISD3 = 0;       // MOSI/SDO = Output
    TRISDbits.TRISD4 = 0;       // HOLD = Output
    TRISDbits.TRISD5 = 0;       // Chip Select = Output

    // Initialize SPI3
    SPI3CONbits.FRMEN = 0;      // Framed SPI support is disabled
    SPI3CONbits.FRMSYNC = 0;    // Frame sync pulse output (Master mode)
    SPI3CONbits.FRMPOL = 0;     // Frame pulse is active-low
    SPI3CONbits.MSSEN = 0;      // Slave Select SPI support disabled
    SPI3CONbits.FRMSYPW = 0;    // Frame Sync Pulse Width 1 clock wide
    SPI3CONbits.FRMCNT = 0;     // Generate a frame sync pulse on every data character
    SPI3CONbits.SPIFE = 1;      // Frame synchronization pulse coincides with the first bit clock
    //SPI3CONbits.ENHBUF = 1;     // Enhanced Buffer mode is enabled
    SPI3CONbits.ENHBUF = 0;     // Enhanced Buffer mode is disabled
    SPI3CONbits.SIDL = 1;       // Discontinue operation in Idle mode
    SPI3CONbits.DISSDO = 0;     // SDOx pin is controlled by the module
    SPI3CONbits.MODE16 = 0;     // 8-bit Communication
    SPI3CONbits.MODE32 = 0;     // 8-bit Communication
    SPI3CONbits.SMP = 1;        // Input data sampled at middle of data output time
    SPI3CONbits.CKE = 1;        // Serial output data changes on transition from idle clock state to active clock state
    SPI3CONbits.SSEN = 0;       // SSx pin controlled by port function
    SPI3CONbits.CKP = 0;        // Idle state for clock is a high level
    SPI3CONbits.MSTEN = 1;      // Master mode
    SPI3CONbits.STXISEL = 3;    // SPIxTXIF is set when buffer is not full (has one or more empty elements)
    SPI3CONbits.SRXISEL = 3;    // SPIxRXIF is set when the buffer is full

    SPI3BRG = 1;             // SPI3 Baud Rate Generator:
                                // 15 => 2.5 MHz
                                // 7 => 5 MHz baud rate
                                // 3 => 10 MHz baud rate
                                // 1 => 20 MHz baud rate
                                // 0 => 40 MHz baud rate

    SPI3STATbits.SPIROV = 0;    // Clear the overflow bit

    SPI3CONbits.ON = 1;         // Turn SPI3 Module on

}


void init_ETH(void)
{
    ETHCON1bits.PTV = 0;        // PAUSE Timer Value bits
    ETHCON1bits.SIDL = 1;       // Ethernet module transfers are frozen during Idle
    ETHCON1bits.TXRTS = 0;      // Stop transmit (when cleared by software) or transmit done
    ETHCON1bits.RXEN = 0;       // Disable RX logic
    ETHCON1bits.AUTOFC = 0;     // Automatic flow control disabled
    ETHCON1bits.MANFC = 0;      // Manual Flow Control disabled
    ETHCON1bits.ON = 0;         // Ethernet module is disabled
      
    ETHCON2bits.RXBUF_SZ = 1;   // RX data Buffer size for descriptors is 16 bytes
  
}

void init_Comparator(void)
{
    CM1CONbits.ON = 0;          // Module is disabled
    CM1CONbits.COE = 0;         // Comparator output is not on the CxOUT pin
    CM1CONbits.CPOL = 0;        // Output is not inverted
    CMSTATbits.SIDL = 1;        // Comparator modules are disabled in IDLE mode
}

void init_PreCache(void)
{
    CHECONbits.CHECOH = 0;      // Invalidate data lines that are not locked
    CHECONbits.DCSZ = 3;        // Disable data caching 0
    CHECONbits.PREFEN = 3;      // Disable predictive prefetch 0
    CHECONbits.PFMWS = 4;       // Number of Wait states
    
}