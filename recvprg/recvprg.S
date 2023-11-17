; Application memory is at $0000-$C6FF (saved to REU)
; LLVM stack is at $C700-$C7FF         (saved to REU)
; The contents of this file are located at $C800-$CBFF (not saved to REU)
; Screen RAM is located at $CC00-CFFF  (not saved to REU)
; $D000 - $DC00 memory area is also saved and restored from REU

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
next_fragment_idx        = $5D  ; uint8
mem_dst_address          = $5F  ; uint16
save_start_address       = $61  ; uint16, zero page address where we store save start for SAVE KERNAL call
tmp_reg_y                = $63  ; uint8, used for saving Y
waiting_for_first_packet = 2    ; uint8

prev_sp                  = $CBFF ; uint8
choose_run_prg           = $CBFE ; bool, if true, we will run the downloaded PRG, otherwise we are saving it to datassette
prg_entry_func_address   = $CBFC ; uint16
prev_cpu_processor_port  = $CBFB ; uint8 ; Save the value of the CPU processor port register (Memory address $0001). REU is unable to reload this

; # Clobbers A
.macro REU_SNAPSHOT value
  ; # Copy $0002 - $C7FF area (copying byte addresses 0 and 1 doesn't seem to work on real hardware)
  LDA #2
  STA $DF02 ; Set copy src address low byte in C64 memory
  STA $DF04 ; Set copy dst address low byte in REU memory

  LDA #0
  STA $DF0A ; Set flags (transfer address increments on both C64 and REU side)

  STA $DF03 ; Set copy src address high byte in C64 memory

  STA $DF05 ; Set copy dst address mid and high bytes in REU memory
  STA $DF06

  LDA #$FD  ; Set transfer length
  STA $DF07
  LDA #$C7
  STA $DF08

  LDA \value ; #Kick off the transfer immediately (this will be synchronous, C64 CPU halts until transfer is complete)
  STA $DF01

  LDA #0
  STA $DF02 ; Set copy src address low byte in C64 memory
  STA $DF04 ; Set copy dst address low byte in REU memory

  ; # Copy $D000 - $DC03 area
  LDA #$D0
  STA $DF03
  STA $DF05

  LDA #$04
  STA $DF07
  LDA #$0C
  STA $DF08

  LDA \value ; #Kick off the transfer immediately (this will be synchronous, C64 CPU halts until transfer is complete)
  STA $DF01
.endmacro


; # Clobbers A and X
.macro discard_n_rx_bytes num
  LDA \num
  LDX #0
  JSR discard_rx_bytes
.endmacro



.macro DEBUG_BORDER color
PHA
  LDA \color
  STA $D020
PLA
.endmacro


.macro DEBUG_CHAR ch
PHA
  LDA \ch
  JSR $FFD2
PLA
.endmacro

; # Clobbers A
.macro WRITE_U16 addr, value
  LDA #<\value
  STA \addr
  LDA #>\value
  STA \addr+1
.endmacro


; # Clobbers A
.macro CS_WRITE_PAGE addr, value
  WRITE_U16 CS_PACKET_PAGE, \addr
  WRITE_U16 CS_PACKET_DATA, \value
.endmacro


; # Clobbers A
.macro ADD_U16_U8 addr, value
  CLC
  LDA \addr
  ADC \value
  STA \addr
  LDA \addr+1
  ADC #0
  STA \addr+1
.endmacro


; # Clobbers A
.macro SUB_U16_U8 addr, value
  SEC
  LDA \addr
  SBC \value
  STA \addr
  LDA \addr+1
  SBC #0
  STA \addr+1
.endmacro


.section .restore_irq_overlay.1,"axR",@progbits

.globl receive_and_run_prg
.type receive_and_run_prg, @function
receive_and_run_prg:
  STA choose_run_prg ; Function argument: A=0:save to cassette, 1:run PRG from memory

  TSX         ; Save old stack pointer to memory
  STX prev_sp

  LDA $1      ; Save old CPU processor port state
  STA prev_cpu_processor_port

  REU_SNAPSHOT #$90 ; Snapshot memory from C64 to REU

;  LDA #1
;  ORA RRNET_CTL
;  STA RRNET_CTL ; Enable clock port

;  CS_WRITE_PAGE $0114, $0055 ; Reset RR-Net
;  CS_WRITE_PAGE $0104, $0D05 ; Accept individual and broadcast packets
;  CS_WRITE_PAGE $0112, $00D3 ; Enable RX and TX

  LDA #01
  STA waiting_for_first_packet ; Record that we are waiting for the first header packet to arrive
  JSR wait_for_msg ; Wait for the first header
  LDA #0
  STA waiting_for_first_packet ; Record that we are now waiting for subsequent datagram packets

  discard_n_rx_bytes #20 ; Discard filename

  LDA CS_RXTX_DATA      ; Read tx file id
  LDX CS_RXTX_DATA+1    ;
  STA tx_file_id
  STX tx_file_id+1

  LDA CS_RXTX_DATA      ; Read # of fragments
  LDX CS_RXTX_DATA+1    ;
  STA num_fragments

  LDA CS_RXTX_DATA      ; Read copy destination start address
  STA mem_dst_address
  LDA CS_RXTX_DATA+1
  STA mem_dst_address+1

  LDA CS_RXTX_DATA      ; Read PRG entry function address
  STA prg_entry_func_address
  LDA CS_RXTX_DATA+1
  STA prg_entry_func_address+1

  LDA CS_RXTX_DATA      ; Discard file size
  LDA CS_RXTX_DATA+1

  LDA CS_RXTX_DATA      ; Discard filetype and dummy padding byte
  LDA CS_RXTX_DATA+1

  LDA #$FF                            ; Send ACK for the header
  STA next_fragment_idx
  JSR ack_received_udp_fragment

  ; Now loop to receive file data UDP fragments
fragments_still_left_to_receive:
  JSR wait_for_msg ; Wait for a datagram

  LDA CS_RXTX_DATA     ; Get file ID
  LDX CS_RXTX_DATA+1

  LDA CS_RXTX_DATA     ; Get fragment index
  LDX CS_RXTX_DATA+1

  LSR packet_length+1  ; Divide packet length by two to convert from bytes to words (assume it was even)
  ROR packet_length

bytes_left_to_read:
  LDY #0
  LDA CS_RXTX_DATA     ; Read data word
  STA (mem_dst_address),y
  INY
  LDA CS_RXTX_DATA+1
  STA (mem_dst_address),y
  STA $D020
  ADD_U16_U8 mem_dst_address, #2
  SUB_U16_U8 packet_length, #1
  LDA packet_length
  ORA packet_length+1
  BNE bytes_left_to_read

  ; all bytes from this datagram have been read, ack the transfer
  JSR ack_received_udp_fragment

  LDA next_fragment_idx
  CMP num_fragments
  BNE fragments_still_left_to_receive

  ; All data has been received!
  LDA choose_run_prg
  BNE run_prg

save_prg_to_datassette:     ; Instead of running, we want to save the loaded PRG on to datasette
  LDA #$01
  STA save_start_address
  LDA #$08
  STA save_start_address+1
  LDA #save_start_address

  LDX mem_dst_address
  LDY mem_dst_address+1
  JSR $FFD8                    ; Call the 'SAVE' KERNAL ROM routine

  JMP continue_after_datassette_save

run_prg:                   ; Instead of saving to datasette, run the PRG
  LDX #$FF ; Clear the stack pointer for the child program
  TXS

  LDA #<abort_execution_on_restore_key ; Install IRQ handler for RESTORE key to resume execution
  STA $318
  LDA #>abort_execution_on_restore_key
  STA $319
  JSR $E544 ; Call CLRSCR to clear the screen
  LDA #14
  STA $D020 ; Set border color to default Light Blue
  LDA #6
  STA $D021 ; Set background color to default Dark Blue
  LDA #1
  STA $CC   ; Disable blinking cursor

  CLI
  LDX #$FF ; Clear stack pointer for the child program
  TXS
  JSR start_loaded_prg ; Start the downloaded program

  ; Print "Program Finished" message
  LDX #33
print_program_finished:
  LDA message,X
  JSR $FFD2
  DEX
  BNE print_program_finished

flush_keys:
  JSR $FFE4 ; # Consume key press
  LDA $C6 ; #Test if there is a key in queue
  BNE flush_keys
wait_key:
  LDA $C6 ; #Wait until there are no keys in queue
  BEQ wait_key

continue_after_datassette_save:
abort_execution_on_restore_key:
  SEI
  LDA #$2F ;# Restore CPU port Read/Write status to default value so we can write to the register
  STA $0
  LDA prev_cpu_processor_port ;# And the CPU Processor port bits to the previous value
  STA $1
  REU_SNAPSHOT #$91 ; Restore memory from REU to C64

  LDX prev_sp ; Restore old stack pointer
  TXS

  ; Set $314 to $EA31 ; Execution address of interrupt service routine.
  ; JSR $FF81 ; Call CINT, Initialize screen editor & 6567 video chip
  ; JSR $FF84 ; Call IOINIT, Initialize I/O devices
  CLI ; Restore interrupts

flush_keys2:
  JSR $FFE4 ; # Consume key press
  LDA $C6 ; #Test if there is a key in queue
  BNE flush_keys2

  RTS ; Return back to caller as usual

message:
  .ascii " .YEK YNA SSERP .DEHSINIF MARGORP\r"

start_loaded_prg:
  JMP (prg_entry_func_address)
  RTS

; #FUNCTION: Discards given number of bytes from RR-Net incoming packet queue
; #Clobbers A and X, preserves Y
discard_rx_bytes:
  STY tmp_reg_y
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
  LDY tmp_reg_y
  RTS



;# FUNCTION
discard_all_rx_bytes:
  LDA packet_length
  LDX packet_length+1
  JMP discard_rx_bytes



;# FUNCTION
ack_received_udp_fragment:
  WRITE_U16 CS_TX_CMD, $00c0
  WRITE_U16 CS_TX_LEN, 48
  WRITE_U16 CS_PACKET_PAGE, $0138

wait_for_tx_ready:
  LDA CS_PACKET_DATA+1
  ROR A
  BCC wait_for_tx_ready

  LDY #$FF
  STY CS_RXTX_DATA   ; Write the 6-byte dst MAC address (FF:FF:FF:FF:FF:FF for broadcast)
  STY CS_RXTX_DATA+1
  STY CS_RXTX_DATA
  STY CS_RXTX_DATA+1
  STY CS_RXTX_DATA
  STY CS_RXTX_DATA+1

  WRITE_U16 CS_RXTX_DATA, $CD28 ; Write src MAC address
  LDA #$4C
  STA CS_RXTX_DATA
  STY CS_RXTX_DATA+1
  LDA $DE0E
  STA CS_RXTX_DATA
  LDA $DE0F
  STA CS_RXTX_DATA+1

  LDA #8                  ; Write EtherType
  STA CS_RXTX_DATA
  LDX #0
  STX CS_RXTX_DATA+1

  LDA #$45                ; Write versionAndHeaderLength
  STA CS_RXTX_DATA
  STX CS_RXTX_DATA+1

  STX CS_RXTX_DATA        ; Write IP packet length
  LDA #$22
  STA CS_RXTX_DATA+1

  STX CS_RXTX_DATA        ; Write identification (0, unused)
  STX CS_RXTX_DATA+1

  STX CS_RXTX_DATA        ; Write flagsAndFragmentOffset (0, unused)
  STX CS_RXTX_DATA+1

  WRITE_U16 CS_RXTX_DATA, $1180 ; Write TTL (128) + Protocol (17 - UDP)

  WRITE_U16 CS_RXTX_DATA, $2479 ; Write IP checksum (this is precomputed, see recv.cpp for the calculation)

  WRITE_U16 CS_RXTX_DATA, $A8C0 ; Write src IP (182.168.0.255)
  STX CS_RXTX_DATA
  STY CS_RXTX_DATA+1

  STY CS_RXTX_DATA   ; Write dst IP (255.255.255.255)
  STY CS_RXTX_DATA+1
  STY CS_RXTX_DATA
  STY CS_RXTX_DATA+1

  STX CS_RXTX_DATA   ; Write src port (0, unused)
  STX CS_RXTX_DATA+1

  WRITE_U16 CS_RXTX_DATA, $A90F ; Write dst port (4009)

  STX CS_RXTX_DATA ; Write UDP packet length (14)
  LDA #$0E
  STA CS_RXTX_DATA+1

  STX CS_RXTX_DATA    ; Write UDP checksum (0, unused)
  STX CS_RXTX_DATA+1

  WRITE_U16 CS_RXTX_DATA, $5F02 ; Write FRAGMENT_RECEIVED_DATAGRAM_MAGIC low word
  WRITE_U16 CS_RXTX_DATA, $EE36 ; Write FRAGMENT_RECEIVED_DATAGRAM_MAGIC high word
  LDA next_fragment_idx ; Write the acked fragment index
  STA CS_RXTX_DATA
  STA CS_RXTX_DATA+1    ; Dummy padding
  INC next_fragment_idx ; Auto-increment next_fragment_idx
  RTS


;# FUNCTION
wait_for_msg:           ; Wait until RR-Net ethernet frame is available
  WRITE_U16 CS_PACKET_PAGE, $0124
wait_again:
  LDA CS_PACKET_DATA+1
  AND #$0D
  BEQ wait_again

  LDA CS_RXTX_DATA+1    ; Ignore RR-Net status uint16
  LDA CS_RXTX_DATA

  LDX CS_RXTX_DATA+1    ; read packet length
  STX packet_length+1
  LDA CS_RXTX_DATA
  STA packet_length

  ; # Ethernet frame = 14 bytes, IP frame = 20 bytes, UDP header = 8 bytes
  ; # UDP payload has 4 bytes of magic + fragment message is at minimum 4 bytes extra, total of 50 bytes
  ; # file transfer header is 24 bytes extra on top of that
  CMP #50                  ; if incoming packet is too small (< 50 bytes), it can't be intended for us
  BCS packet_large_enough
  CPX #0
  BNE packet_large_enough
  JSR discard_rx_bytes     ; The packet was too small that it cannot be intended for us. Discard all
  JMP wait_for_msg         ; bytes in the packet and start the wait again.

packet_large_enough:
  SUB_U16_U8 packet_length, #50

  ; # Parse Ethernet frame

debug_manual_skip_mac:
  LDA CS_RXTX_DATA      ; Read EtherType
  LDX CS_RXTX_DATA+1
  LDA CS_RXTX_DATA      ; Read EtherType
  LDX CS_RXTX_DATA+1
  LDA CS_RXTX_DATA      ; Read EtherType
  LDX CS_RXTX_DATA+1
  LDA CS_RXTX_DATA      ; Read EtherType
  LDX CS_RXTX_DATA+1
  LDA CS_RXTX_DATA      ; Read EtherType
  LDX CS_RXTX_DATA+1
  LDA CS_RXTX_DATA      ; Read EtherType
  LDX CS_RXTX_DATA+1


;  discard_n_rx_bytes #12 ; Discard SRC and DST MAC address fields of the Ethernet frame

  LDY #0                ; Y receives result of all testing

  LDA CS_RXTX_DATA      ; Read EtherType
  LDX CS_RXTX_DATA+1
  CMP #8                ; Test if EtherType equals $0800
  BEQ 1f
  INY                   ; EtherType test failed
1:
  CPX #0
  BEQ 1f
  INY                   ; EtherType test failed
1:

  ; # Parse IP frame
  discard_n_rx_bytes #8 ; Discard early parts of IP frame, destroys Y so save it on stack
  LDA CS_RXTX_DATA      ; Read Protocol
  LDX CS_RXTX_DATA+1    ;
  CPX #$11              ; Test against $11 (UDP)
  BEQ 1f
  INY                   ; Protocol test failed
1:

  discard_n_rx_bytes #18 ; Discard remaining 10 bytes of IP frame, and 8 bytes of UDP header, destroys Y so save it on stack
  LDA CS_RXTX_DATA      ; Read low u16 of magic
  LDX CS_RXTX_DATA+1    ;

  CLC
  ADC waiting_for_first_packet

  CMP #$24               ; Test if low u16 of magic matches
  BEQ 1f
  INY                   ; magic test failed
1:
  CPX #$14
  BEQ 1f
  INY                   ; magic test failed
1:

  LDA CS_RXTX_DATA      ; Read high u16 of magic
  LDX CS_RXTX_DATA+1    ;

  CMP #$29               ; Test if high u16 of magic matches
  BEQ 1f
  INY                   ; magic test failed
1:
  CPX #$25
  BEQ 1f
  INY                   ; magic test failed
1:

  LDA waiting_for_first_packet ; Finally, test if the datagram size is as expected
  BEQ size_good                ; If this is a datagram continuation packet, size is already validated to be good above

  LDA packet_length
  CMP #24
  BCS size_good

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



.globl restore_key_interrupt_handler
.type restore_key_interrupt_handler, @function
restore_key_interrupt_handler:
