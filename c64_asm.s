.export   _wait_raster
.export   _init_audio
.export   _init_nmi
.export   _start_music

PARAM1 = $33C
sid_init = $A200
sid_play = $A203

.segment		"SID"
.incbin "audio/ggj.bin"

.segment	"CODE"

_wait_raster:
   ldx PARAM1
   check_again:
   cpx $D012
   bne check_again;
rts

_start_music:
   sei
   lda #$0
   tax
   tay
   jsr sid_init
   cli
rts

_init_nmi:
   SEI             ; disable IRQ
   lda #<nmi_nop
   sta $fffa
   lda #>nmi_nop
   sta $fffb

   lda #$00
   sta $dd0e       ;// Stop timer A
   sta $dd04       ;// Set timer A to 0, NMI will occure immediately after start
   sta $dd0e

   lda #$81
   sta $dd0d       ;// Set timer A as source for NMI

   lda #$01
   sta $dd0e       ;// Start timer A -> NMI
   cli
rts
                        ; from here on NMI is disabled
nmi_nop:
rti            

_init_audio:
   sei        ;disable maskable IRQs

   lda #$7f
   sta $dc0d  ;disable timer interrupts which can be generated by the two CIA chips
   sta $dd0d  ;the kernal uses such an interrupt to flash the cursor and scan the keyboard, so we better
         ;stop it.

   lda $dc0d  ;by reading this two registers we negate any pending CIA irqs.
   lda $dd0d  ;if we don't do this, a pending CIA irq might occur after we finish setting up our irq.
         ;we don't want that to happen.

   lda #$01   ;this is how to tell the VICII to generate a raster interrupt
   sta $d01a

   lda #$64   ;this is how to tell at which rasterline we want the irq to be triggered
   sta $d012

   ;lda #$1b   ;as there are more than 256 rasterlines, the topmost bit of $d011 serves as
   ;sta $d011  ;the 9th bit for the rasterline we want our irq to be triggered.
         ;here we simply set up a character screen, leaving the topmost bit 0.

   lda #<irq  ;this is how we set up
   sta $fffe  ;the address of our interrupt code
   lda #>irq
   sta $ffff
rts

irq:
	;dec 53280 ; flash border to see we are live
   pha        ;store register A in stack
   txa
   pha        ;store register X in stack
   tya
   pha        ;store register Y in stack

   lda #$ff   ;this is the orthodox and safe way of clearing the interrupt condition of the VICII.
   sta $d019  ;if you don't do this the interrupt condition will be present all the time and you end
         ;up having the CPU running the interrupt code all the time, as when it exists the
         ;interrupt, the interrupt request from the VICII will be there again regardless of the
         ;rasterline counter.

         ;it's pretty safe to use inc $d019 (or any other rmw instruction) for brevity, they
         ;will only fail on hardware like c65 or supercpu. c64dtv is ok with this though.
   jsr sid_play

   pla
   tay        ;restore register Y from stack (remember stack is FIFO: First In First Out)
   pla
   tax        ;restore register X from stack
   pla        ;restore register A from stack
rti     


