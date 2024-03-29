#define _SUPPRESS_PLIB_WARNING 1

#include <xc.h>
#include <plib.h>

//#include "../include/md.h"
#include "../include/pic32mx695f_config.h"

// ----------------------------------------------------------------------------
// Initialization Routines
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
void init_CLOCK(void)
{
    // Initialize & Enable OSCCON & RTCC
    SYSKEY = 0x0;               // ensure OSCCON is locked
    SYSKEY = 0xAA996655;        // Write Key1 to SYSKEY
    SYSKEY = 0x556699AA;        // Write Key2 to SYSKEY

    OSCCONbits.COSC = 1;        // Internal Fast RC Oscillator with PLL module via Postscaler (FRCPLL)
    OSCCONbits.SLPEN = 1;       // Enable Sleep on "wait"
    OSCCONbits.UFRCEN = 0;      // USB PLL as USB Clock Source
    OSCCONbits.SOSCEN = 0;      // disable Secondary OSC
    OSCCONbits.PBDIV = 0;       // Peripheral Bus CLock diveded by 1

    RTCCONbits.RTCWREN = 0;     // Enable write operation for RTCC

    SYSKEY = 0x0;               // Re-Lock OSCCON

}

// ----------------------------------------------------------------------------
void init_ADC(void)
{
    // ADC Configuration
    AD1PCFG = 0x0000FFFF;       // AN0-15 = digital; none = analog
    AD1CON1bits.ADON = 0;       // Turn ADC on
}

// ----------------------------------------------------------------------------
void init_TMR1(void)
{
    // Configure Timer 1 - used for general timing and delays
    T1CONbits.SIDL = 1;         // Discontinue operation in Idle mode
    T1CONbits.TWDIS = 0;        // Back-to-back writes are enabled
    T1CONbits.TGATE = 0;        // Gated time accumulation is disabled
    T1CONbits.TCKPS = 1;        // 1:8 prescale value, FPBclk = 80MHz -> T1 period ~= 0.1us
    T1CONbits.TSYNC = 0;        // When TCS = 0: This bit is ignored
    T1CONbits.TCS = 0;          // Internal Peripheral Clock
    PR1 = 0xFFFF;               // Period Register to max
    T1CONbits.ON = 1;           // Turn Timer1 on
    TMR1 = 0;                   // set Timer1 counter = 0    
}

// ----------------------------------------------------------------------------
void init_TMR23(void)
{
    // Configure Timer 2 
    T2CONbits.SIDL = 1;         // Do not Discontinue operation in Idle mode
    T2CONbits.TGATE = 0;        // Gated time accumulation is disabled
    T2CONbits.TCKPS = 3;        // 1:8 prescale value, TPBclk = 80MHz -> T23 period ~= 0.1 us
    T2CONbits.T32 = 1;          // TMR2 and TMR3 form a combined 32-bit timer
    PR2 = 0xFFFFFFFF;           // Period Register (0.1 us)*PR2

    // Configure Timer 3
    T3CONbits.SIDL = 1;         // Discontinue operation in Idle mode
    T3CONbits.TGATE = 0;        // Gated time accumulation is disabled

    T2CONbits.ON = 1;           // Turn Timer2 on
    TMR2 = 0;                   // set Timer2 counter = 0 
}

// ----------------------------------------------------------------------------
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

    RTCCONbits.ON = 0;          // Turn off RTCC module

}

// ----------------------------------------------------------------------------
void init_UART1(void)
{
    // UART2 Configuration & Initialization - 8N1, Baud rate set in U2BRG
    U1MODEbits.SIDL = 1;        // Discontinue operation in Idle mode
    U1MODEbits.IREN = 0;        // IrDA is disabled
    U1MODEbits.RTSMD = 1;       // UxRTS pin is in Simplex mode
    U1MODEbits.UEN = 0;         // UxTX and UxRX pins are enabled and used
    U1MODEbits.WAKE = 0;        // Wake-up disabled
    U1MODEbits.LPBACK = 0;      // Loopback mode is disabled
    U1MODEbits.ABAUD = 0;       // Auto Baud rate measurement disabled
    U1MODEbits.RXINV = 0;       // UxRX Idle state is 1
    U1MODEbits.BRGH = 1;        // High Speed mode - 4x baud clock enabled
    U1MODEbits.PDSEL = 0;       // 8-bit data, no parity
    U1MODEbits.STSEL = 0;       // 1 Stop bit

    U1STAbits.ADM_EN = 0;       // Automatic Address Detect mode is disabled
    U1STAbits.UTXISEL = 2;      // Interrupt while the transmit buffer is empty
    U1STAbits.UTXINV = 0;       // UxTX Idle state is ?1?
    U1STAbits.URXEN = 1;        // UARTx receiver is enabled
    U1STAbits.UTXBRK = 0;       // Break transmission is disabled
    U1STAbits.UTXEN = 1;        // UARTx transmitter is enabled
    U1STAbits.URXISEL = 0;      // Interrupt flag bit is set while receive buffer is not empty
    U1STAbits.ADDEN = 0;        // Address Detect mode is disabled

    // Fpb = 80MHz; BRG = Fpb/(4*baudrate) - 1
    U1BRG = 346;                // BRG = 79; 250k baud rate; 0.00% error
                                // BRG = 173; 115.2k baud rate; -0.22% error
                                // BRG = 346; 57.6k baud rate; 0.06% error

    U1MODEbits.ON = 1;          // Turn UART1 On
}

// ----------------------------------------------------------------------------
void init_UART2(void)
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
                                // BRG =  (0x); 1M baud rate)

    U2MODEbits.ON = 1;          // Turn UART2 On
}

// ----------------------------------------------------------------------------
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

    SPI3CONbits.ON = 0;         // Turn SPI3 Module off

}

// ----------------------------------------------------------------------------
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

// ----------------------------------------------------------------------------
void init_COMP1(void)
{
    CM1CONbits.COE = 0;         // Comparator output is not on the CxOUT pin
    CM1CONbits.CPOL = 0;        // Output is not inverted
    CMSTATbits.SIDL = 1;        // Comparator modules are disabled in IDLE mode
    CM1CONbits.ON = 0;          // Module is disabled
}

// ----------------------------------------------------------------------------
void init_PRECACHE(void)
{
    CHECONbits.CHECOH = 0;      // Invalidate data lines that are not locked
    CHECONbits.DCSZ = 3;        // Disable data caching 0
    CHECONbits.PREFEN = 3;      // Disable predictive prefetch 0
    CHECONbits.PFMWS = 4;       // Number of Wait states
    
}