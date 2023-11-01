RRNET_CTL = $DE01
CS_PACKET_PAGE = $DE02
CS_PACKET_DATA = $DE04
CS_RXTX_DATA   = $DE08
CS_TX_CMD      = $DE0C
CS_TX_LEN      = $DE0E

; Global data
packet_length            = $57  ; uint16
tx_file_id               = $59  ; uint16
num_fragments            = $5B  ; uint16
next_fragment_idx        = $5D  ; uint16
waiting_for_first_packet = 2    ; uint8


.macro DEBUG_BORDER color
  LDA \color
  STA $D020
.endmacro




.macro CS_SET_PAGE addr
  LDA #<\addr
  STA CS_PACKET_PAGE
  LDA #>\addr
  STA CS_PACKET_PAGE+1
.endmacro



.macro CS_WRITE_PAGE addr, value
  CS_SET_PAGE \addr
  LDA #<\value
  STA CS_PACKET_DATA
  LDA #>\value
  STA CS_PACKET_DATA+1
.endmacro



.macro SUB_U16_U8 addr, value
  SEC
  LDA \value
  SBC \addr
  SBC \addr+1
.endmacro



; Discards given number of bytes from RR-Net incoming packet queue
discard_rx_bytes:
  ; A and X contain number of bytes to discard
  CMP #0          ; Test if A is zero (have an even multiple of 256 bytes to discard)
  BEQ high_part   ; Jump to deal directly with high part

  LSR A           ; Calculate (A+1)/2 by shifting once right, and
  ADC #0          ; adding the lowest bit (now in Carry) back to A.
  TAY            ; Move A to Y, since A cannot be used as a loop counter
loop_lo:         ; Loop Y times to read the $DE09 hardware port
  LDA $DE08      ; Discards one uint16 from CS8900A receive buffer
  LDA $DE09      ; Discards one uint16 from CS8900A receive buffer
  DEY
  BNE loop_lo

high_part:       ; Now discard full multiples of 256 bytes
  CPX #00        ; No high part to discard
  BEQ end

loop_hi:
  LDY #128       ; 256 bytes == 128 uint16s
times_128:       ; Loop just like before
  LDA $DE08      ; Discard a uint16 from CS8900A receive buffer
  LDA $DE09      ; Discard a uint16 from CS8900A receive buffer
  DEY
  BNE times_128
   
  DEX
  BNE loop_hi    ; If we still have multiples of 256 left, do it again
end:
  RTS



.macro discard_n_rx_bytes num
  LDA \num
  LDX #0
  JSR discard_rx_bytes
.endmacro



discard_all_rx_bytes:
  LDA packet_length
  LDX packet_length+1
  JMP discard_rx_bytes



wait_for_msg:           ; Wait until RR-Net ethernet frame is available
  CS_SET_PAGE $0124
  LDA CS_PACKET_DATA+1
  AND #$0D
  BEQ wait_for_msg

  DEBUG_BORDER #1

  LDA CS_RXTX_DATA+1    ; Ignore RR-Net status uint16
  LDA CS_RXTX_DATA

  LDX CS_RXTX_DATA+1    ; read packet length
  STX packet_length+1
  LDA CS_RXTX_DATA
  STA packet_length

  CMP #50                  ; if incoming packet is too small (< 50 bytes), it can't be intended for us
  BPL packet_large_enough
  CPX #0
  BNE packet_large_enough
  JSR discard_rx_bytes     ; The packet was too small that it cannot be intended for us. Discard all
  JMP wait_for_msg         ; bytes in the packet and start the wait again.

packet_large_enough:
  SUB_U16_U8 packet_length, #50
  discard_n_rx_bytes #12

  LDY #0                ; Y receives result of all testing

  LDA CS_RXTX_DATA      ; Read EtherType
  LDX CS_RXTX_DATA+1    ;
  CMP #0                ; Test if EtherType equals $0800
  BEQ pass1
  CPX #8
  BEQ pass1
  INY                   ; EtherType test failed
pass1:

  discard_n_rx_bytes #8
  LDA CS_RXTX_DATA      ; Read Protocol
  LDX CS_RXTX_DATA+1    ;
  CPX #$11              ; Test against $11 (UDP)
  BEQ pass2
  INY                   ; Protocol test failed
pass2:

  discard_n_rx_bytes #18
  LDA CS_RXTX_DATA      ; Read low u16 of magic
  LDX CS_RXTX_DATA+1    ;

  CLC
  ADC waiting_for_first_packet

  CMP #$24               ; Test if low u16 of magic matches
  BEQ pass3
  INY                   ; magic test failed
pass3:
  CPX #$14
  BEQ pass32
  INY                   ; magic test failed
pass32:

  LDA CS_RXTX_DATA      ; Read high u16 of magic
  LDX CS_RXTX_DATA+1    ;

  CMP #$29               ; Test if high u16 of magic matches
  BEQ pass4
  INY                   ; magic test failed
pass4:
  CPX #$25
  BEQ pass42
  INY                   ; magic test failed
pass42:

  LDA waiting_for_first_packet ; Finally, test if the datagram size is as expected
  BNE size_good                ; If this is a datagram continuation packet, size is already validated to be good above

  LDA packet_length
  CMP #20
  BPL size_good

  LDX packet_length+1
  CPX #0
  BNE size_good

  INY ; Size test failed

size_good:
  CPY #0                   ; All tests finished. Now check if Y contains an error code
  BEQ good_datagram
  JSR discard_all_rx_bytes ; The packet failed one or more of checks above. Discard its bytes from queue.
  JMP wait_for_msg         ; And loop back to start the wait again.

good_datagram:
  RTS

.globl recv_prg
.type recv_prg, @function
recv_prg:
  LDA #1
  ORA RRNET_CTL
  STA RRNET_CTL ; Enable clock port

  CS_WRITE_PAGE $0114, $0055 ; Reset RR-Net
  CS_WRITE_PAGE $0104, $0D05 ; Accept individual and broadcast packets
  CS_WRITE_PAGE $0112, $00D3 ; Enable RX and TX

  LDA #01
  STA waiting_for_first_packet ; Record that we are waiting for the first header packet to arrive

  JSR wait_for_msg ; Wait for the first header

  LDA #0
  STA waiting_for_first_packet ; Record that we are now waiting for subsequent datagram packets

  DEBUG_BORDER #4

  RTS
