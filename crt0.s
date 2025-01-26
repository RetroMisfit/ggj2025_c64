	.import   _main
	.export   __STARTUP__ : absolute = 1        ; Mark as startup
	.include  "zeropage.inc"

; ------------------------------------------------------------------------
; Place the startup code in a special segment.
.segment       	"CODE"
	; BASIC header with a SYS call
    .word   $801; Load address
    .byte $0B, $10, $00, $00, $9E, '2','0','6','1',$00, $00, $00

	lda #$F1
	sta	sp
	lda	#$00
   	sta	sp+1   		; Set argument stack ptr
    sei
	jsr _main

