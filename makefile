#
# Makefile for cc65 samples
#
# This Makefile requires GNU make
#

# Enter the target system here
SYS	= c64

# Determine the path to the executables and libraries. If the samples
# directory is part of a complete source tree, use the stuff from that
# source tree; otherwise, use the "install" directories.

CLIB = --lib $(SYS).lib
CL   = cl65
CC   = cc65
AS   = ca65
LD   = ld65
EXE  = ggj.prg
C1541  	= /Applications/VICE_2_4/tools/c1541

# --------------------------------------------------------------------------
# Generic rules

%.o:   	%.c
	@echo $<
	@$(CC) $(MY_INC) -DC64 -Or -g -Cl --codesize 210  $<
	@$(AS) $(basename $<).s

%.o:	%.s
	@$(AS) -o $@ $(AFLAGS) $<

# --------------------------------------------------------------------------
# List of executables. This list could be made target dependent by checking
# $(SYS).

C_OBJS =	main.o

S_OBJS  =   crt0.o c64_asm.o

EXELIST =	$(EXE)

$(EXE): $(S_OBJS) $(C_OBJS)
	@$(LD) $^ -v -o $(EXE) -C mem.cfg -m $(EXE).map $(CLIB) 

# --------------------------------------------------------------------------
# Rules how to make each one of the binaries

.PHONY:	all
all:   $(EXELIST)
    
# --------------------------------------------------------------------------
# Cleanup rules

.PHONY:	clean
clean:
	$(RM) $(C_OBJS:.o=.s) $(C_OBJS) $(S_OBJS) $(EXELIST) *.map 


