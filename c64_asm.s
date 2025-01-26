.export   _wait_raster
.export   _init_audio
.export   _init_nmi
.export   _start_music

PARAM1 = $33C
sid_init = $A200
sid_play = $A203

.segment	"SID"
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
   SEI             
   lda #<nmi_nop
   sta $fffa
   lda #>nmi_nop
   sta $fffb

   lda #$00
   sta $dd0e       
   sta $dd04       
   sta $dd0e

   lda #$81
   sta $dd0d       

   lda #$01
   sta $dd0e       
   cli
rts

nmi_nop:
rti            

_init_audio:
   sei        

   lda #$7f
   sta $dc0d  
   sta $dd0d  

   lda $dc0d  
   lda $dd0d  

   lda #$01   
   sta $d01a

   lda #$64   
   sta $d012

   lda #<irq  
   sta $fffe  
   lda #>irq
   sta $ffff
rts

irq:
   pha        
   txa
   pha        
   tya
   pha        

   lda #$ff   
   sta $d019  
   jsr sid_play

   pla
   tay        
   pla
   tax        
   pla        
rti     


