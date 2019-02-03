/*
  serial.c - Low level functions for sending and recieving bytes via the serial port
  Part of Grbl

  Copyright (c) 2011-2016 Sungeun K. Jeon for Gnea Research LLC
  Copyright (c) 2009-2011 Simen Svale Skogsrud

  Grbl is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Grbl is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Grbl.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "grbl.h"

#define RX_RING_BUFFER (RX_BUFFER_SIZE+1)
#define TX_RING_BUFFER (TX_BUFFER_SIZE+1)

uint8_t serial_rx_buffer[RX_RING_BUFFER];
uint8_t serial_rx_buffer_head = 0;
volatile uint8_t serial_rx_buffer_tail = 0;

uint8_t serial_tx_buffer[TX_RING_BUFFER];
uint8_t serial_tx_buffer_head = 0;
volatile uint8_t serial_tx_buffer_tail = 0;


// Returns the number of bytes available in the RX serial buffer.
uint8_t serial_get_rx_buffer_available()
{
  uint8_t rtail = serial_rx_buffer_tail; // Copy to limit multiple calls to volatile
  if (serial_rx_buffer_head >= rtail) { return(RX_BUFFER_SIZE - (serial_rx_buffer_head-rtail)); }
  return((rtail-serial_rx_buffer_head-1));
}


// Returns the number of bytes used in the RX serial buffer.
// NOTE: Deprecated. Not used unless classic status reports are enabled in config.h.
uint8_t serial_get_rx_buffer_count()
{
  uint8_t rtail = serial_rx_buffer_tail; // Copy to limit multiple calls to volatile
  if (serial_rx_buffer_head >= rtail) { return(serial_rx_buffer_head-rtail); }
  return (RX_BUFFER_SIZE - (rtail-serial_rx_buffer_head));
}


// Returns the number of bytes used in the TX serial buffer.
// NOTE: Not used except for debugging and ensuring no TX bottlenecks.
uint8_t serial_get_tx_buffer_count()
{
  uint8_t ttail = serial_tx_buffer_tail; // Copy to limit multiple calls to volatile
  if (serial_tx_buffer_head >= ttail) { return(serial_tx_buffer_head-ttail); }
  return (TX_RING_BUFFER - (ttail-serial_tx_buffer_head));
}


void serial_init()
{
  //
  // Note: Clocks were turned on to the SCIA peripheral
  // in the InitSysCtrl() function
  //

  // Main SCI Init
  SciaRegs.SCICCR.all = 0x0007;   // 1 stop bit,  No loopback
                                  // No parity,8 char bits,
                                  // async mode, idle-line protocol
  SciaRegs.SCICTL1.all = 0x0003;  // enable TX, RX, internal SCICLK,
                                  // Disable RX ERR, SLEEP, TXWAKE
  SciaRegs.SCICTL2.all = 0x0000;  // Setup no interrupts here
  SciaRegs.SCICTL1.all = 0x0023;  // Relinquish SCI from Reset


  SciaRegs.SCIFFTX.all = 0x4043;  // SCI TX/RX Reset, Enable FIFOs, TX FIFO Reset, TX FIFO Clear, TX FIFO Interrupt Disable, TX FIFO Interrupt when 3 or less items in the TX FIFO
  SciaRegs.SCIFFRX.all = 0x0061;  // RX FIFO Reset, RX FIFO Int clear, RX FIFO Interrupt Enable, RX FIFO Interrupt when 1 or more items in RX FIFO

  //
  // SCIA at 115200
  // @LSPCLK = 50 MHz (200 MHz SYSCLK) Default LSPCLK divider.
  //
  uint16_t BBR = (((F_CPU / 4) * 1000000L) / ((BAUD_RATE * 8L) - 1));
  SciaRegs.SCIHBAUD.all = (BBR & 0xFF00) >> 8;
  SciaRegs.SCILBAUD.all = BBR & 0xFF;

  SciaRegs.SCIFFTX.all = 0xE003;  // Relinquish SCI and TX FIFO from Reset
  SciaRegs.SCIFFRX.all = 0x2021;  // Relinquish SCI RX from Reset
}


// Writes one byte to the TX serial buffer. Called by main program.
void serial_write(uint8_t data) {
  // Calculate next head
  uint8_t next_head = serial_tx_buffer_head + 1;
  if (next_head == TX_RING_BUFFER) { next_head = 0; }

  // Wait until there is space in the buffer
  while (next_head == serial_tx_buffer_tail) {
    // TODO: Restructure st_prep_buffer() calls to be executed here during a long print.
    if (sys_rt_exec_state & EXEC_RESET) { return; } // Only check for abort to avoid an endless loop.
  }

  // Store data and advance head
  serial_tx_buffer[serial_tx_buffer_head] = data;
  serial_tx_buffer_head = next_head;

  // Enable Data Register Empty Interrupt to make sure tx-streaming is running
  SciaRegs.SCIFFTX.bit.TXFFIENA = 1;
}


// Data Register Empty Interrupt handler
// 9.2 - SCIA Transmit Interrupt
// This interrupt is called when the TX FIFO has 3 or less items
interrupt void SCIA_TX_ISR(void)
{
  uint8_t tail = serial_tx_buffer_tail; // Temporary serial_tx_buffer_tail (to optimize for volatile)
  // While there is less than 16 bytes in the FIFO, fill it.
  while (SciaRegs.SCIFFTX.bit.TXFFST < 0x10 && tail != serial_tx_buffer_head) {
    tail = serial_tx_buffer_tail; // Temporary serial_tx_buffer_tail (to optimize for volatile)

    // Wait for the buffer to be ready
    while (SciaRegs.SCICTL2.bit.TXRDY != 1) {;}
    // Send a byte from the buffer
    SciaRegs.SCITXBUF.bit.TXDT = serial_tx_buffer[tail];

    // Update tail position
    tail++;
    if (tail == TX_RING_BUFFER) { tail = 0; }

    serial_tx_buffer_tail = tail;
  }
  // Turn off Data Register Empty Interrupt to stop tx-streaming if this concludes the transfer
  if (tail == serial_tx_buffer_head) { SciaRegs.SCIFFTX.bit.TXFFIENA = 0; }

  SciaRegs.SCIFFTX.bit.TXFFINTCLR = 1; // Clear Interrupt flag
  // To receive more interrupts from this PIE group,
  // acknowledge this interrupt.
  PieCtrlRegs.PIEACK.all = PIEACK_GROUP9;
}

// Fetches the first byte in the serial read buffer. Called by main program.
uint8_t serial_read()
{
  uint8_t tail = serial_rx_buffer_tail; // Temporary serial_rx_buffer_tail (to optimize for volatile)
  if (serial_rx_buffer_head == tail) {
    return SERIAL_NO_DATA;
  } else {
    uint8_t data = serial_rx_buffer[tail];

    tail++;
    if (tail == RX_RING_BUFFER) { tail = 0; }
    serial_rx_buffer_tail = tail;

    return data;
  }
}


//
// 9.1 - SCIA Receive Interrupt
// This interrupt is called when the RX FIFO has 1 or more items
interrupt void SCIA_RX_ISR(void)
{
  uint8_t data;
  uint8_t next_head;

  // While data is still in the SCI FIFO
  while ( SciaRegs.SCIFFRX.bit.RXFFST != 0x0 ) {
    data = SciaRegs.SCIRXBUF.bit.SAR;
    // Pick off realtime command characters directly from the serial stream. These characters are
    // not passed into the main buffer, but these set system state flag bits for realtime execution.
    switch (data) {
      case CMD_RESET:         mc_reset(); break; // Call motion control reset routine.
      case CMD_STATUS_REPORT: system_set_exec_state_flag(EXEC_STATUS_REPORT); break; // Set as true
      case CMD_CYCLE_START:   system_set_exec_state_flag(EXEC_CYCLE_START); break; // Set as true
      case CMD_FEED_HOLD:     system_set_exec_state_flag(EXEC_FEED_HOLD); break; // Set as true
      default :
        if (data > 0x7F) { // Real-time control characters are extended ACSII only.
          switch(data) {
            case CMD_SAFETY_DOOR:   system_set_exec_state_flag(EXEC_SAFETY_DOOR); break; // Set as true
            case CMD_JOG_CANCEL:
              if (sys.state & STATE_JOG) { // Block all other states from invoking motion cancel.
                system_set_exec_state_flag(EXEC_MOTION_CANCEL);
              }
              break;
            #ifdef DEBUG
              case CMD_DEBUG_REPORT: {uint8_t sreg = SREG; cli(); bit_true(sys_rt_exec_debug,EXEC_DEBUG_REPORT); SREG = sreg;} break;
            #endif
            case CMD_FEED_OVR_RESET: system_set_exec_motion_override_flag(EXEC_FEED_OVR_RESET); break;
            case CMD_FEED_OVR_COARSE_PLUS: system_set_exec_motion_override_flag(EXEC_FEED_OVR_COARSE_PLUS); break;
            case CMD_FEED_OVR_COARSE_MINUS: system_set_exec_motion_override_flag(EXEC_FEED_OVR_COARSE_MINUS); break;
            case CMD_FEED_OVR_FINE_PLUS: system_set_exec_motion_override_flag(EXEC_FEED_OVR_FINE_PLUS); break;
            case CMD_FEED_OVR_FINE_MINUS: system_set_exec_motion_override_flag(EXEC_FEED_OVR_FINE_MINUS); break;
            case CMD_RAPID_OVR_RESET: system_set_exec_motion_override_flag(EXEC_RAPID_OVR_RESET); break;
            case CMD_RAPID_OVR_MEDIUM: system_set_exec_motion_override_flag(EXEC_RAPID_OVR_MEDIUM); break;
            case CMD_RAPID_OVR_LOW: system_set_exec_motion_override_flag(EXEC_RAPID_OVR_LOW); break;
            case CMD_SPINDLE_OVR_RESET: system_set_exec_accessory_override_flag(EXEC_SPINDLE_OVR_RESET); break;
            case CMD_SPINDLE_OVR_COARSE_PLUS: system_set_exec_accessory_override_flag(EXEC_SPINDLE_OVR_COARSE_PLUS); break;
            case CMD_SPINDLE_OVR_COARSE_MINUS: system_set_exec_accessory_override_flag(EXEC_SPINDLE_OVR_COARSE_MINUS); break;
            case CMD_SPINDLE_OVR_FINE_PLUS: system_set_exec_accessory_override_flag(EXEC_SPINDLE_OVR_FINE_PLUS); break;
            case CMD_SPINDLE_OVR_FINE_MINUS: system_set_exec_accessory_override_flag(EXEC_SPINDLE_OVR_FINE_MINUS); break;
            case CMD_SPINDLE_OVR_STOP: system_set_exec_accessory_override_flag(EXEC_SPINDLE_OVR_STOP); break;
            case CMD_COOLANT_FLOOD_OVR_TOGGLE: system_set_exec_accessory_override_flag(EXEC_COOLANT_FLOOD_OVR_TOGGLE); break;
            #ifdef ENABLE_M7
              case CMD_COOLANT_MIST_OVR_TOGGLE: system_set_exec_accessory_override_flag(EXEC_COOLANT_MIST_OVR_TOGGLE); break;
            #endif
          }
          // Throw away any unfound extended-ASCII character by not passing it to the serial buffer.
        } else { // Write character to buffer
          next_head = serial_rx_buffer_head + 1;
          if (next_head == RX_RING_BUFFER) { next_head = 0; }

          // Write data to buffer unless it is full.
          if (next_head != serial_rx_buffer_tail) {
            serial_rx_buffer[serial_rx_buffer_head] = data;
            serial_rx_buffer_head = next_head;
          }
        }
    }
  }

  SciaRegs.SCIFFRX.bit.RXFFOVRCLR = 1; // Clear Overflow flag
  SciaRegs.SCIFFRX.bit.RXFFINTCLR = 1; // Clear Interrupt flag
  // To receive more interrupts from this PIE group,
  // acknowledge this interrupt.
  PieCtrlRegs.PIEACK.all = PIEACK_GROUP9;
}

void serial_reset_read_buffer()
{
  serial_rx_buffer_tail = serial_rx_buffer_head;
}
