# DESCRIPTION
# -----------
# MAKEFILE._1 for CHARED, Birthright Character Editor.

# If this makefile is called "makefile" then just run WMAKE.
# If it has another name such as "example.mak" then you have to
# give the file name at the command line:
#   WMAKE /f example.mak

# BEGINNING OF MAKE FILE....

##########################
## User settable macros ##
##########################

ASM = ml
##AFLAGS = -5p
AFLAGS = /Gz /c /Cp /Zm /Zp4 /DDEBUG
ExeFile  =

CCompiler = wcc386
CPPCompiler = wpp386

# This is a wmake style makefile

# Provide helpful messages for runtime mistakes.
!ifndef OS
!error "Please define OS"
!endif

!ifeq RAND ON
Rand_Flag = -d_RANDLOG
!else
Rand_Flag =
!endif


!ifeq OS DOS
op_sys = dos
!else
!ifeq OS WIN95
op_sys = win
!else
!error "OS must be DOS or WIN95"
!endif
!endif

!ifndef DEBUG
!error "Please define DEBUG"
!endif

!ifeq DEBUG ON
version = debugging
Mem_Check = -dMEMORY_CHK
Vec_Check = -d_VECTOR_CHK
!else
!ifeq DEBUG OFF
version = production
Mem_Check =
Vec_Check =
!else
!error "DEBUG must either be ON or OFF"
!endif
!endif

soundsys = on
!ifeq SOUND OFF
soundsys = off
!endif

ReleaseVer =
!ifeq RELEASE ON
ReleaseVer = -d_RELEASE
!endif

DemoVer =
!ifeq DEMO ON
DemoVer = -d_DEMO
!endif

NetDemoVer =
!ifeq NETDEMO ON
NetDemoVer = -d_NETDEMO
!endif

CRC_Check =
!ifeq CRCCHECK ON
CRC_Check = -dCRC_CHK
!endif

!ifeq PROFILE ON
Mem_Check =
Vec_Check =
!endif

StatusFlag =
!ifeq STATUS ON
StatusFlag = -d_STATUS
!endif

ChrEdFlag =
!ifeq CHARED ON
ChrEdFlag = -d_CHARED
!endif

msg_debugging  = linking $(op_sys) debug version ...
msg_production = linking $(op_sys) production version ...

dos_debugging_object_dir = dosobjsd
dos_production_object_dir = dosobjsr
dos_browser_dir = dosmbr
dos_browser_database = nova.dbr
dos_executable = nova.exe
dos_system = dos4g
dos_sys_dir = .\pcsys\
dos_debugging_sound_on_lib =
dos_debugging_sound_off_lib =
dos_production_sound_on_lib =
dos_production_sound_off_lib =
dos_sound_on = -dAUDIO_LIB
dos_sound_off =
dos_graphics_lib =

win_debugging_object_dir = winobjsd
win_production_object_dir = winobjsr
win_browser_dir = winmbr
win_browser_database = nova95.dbr
win_executable = chared.exe
win_system = nt_win
win_sys_dir = .\winsys\
win_debugging_sound_on_lib =
win_debugging_sound_off_lib =
win_production_sound_on_lib =
win_production_sound_off_lib =
win_sound_on = -dAUDIO_LIB
win_sound_off =
win_graphics_lib = lib ddraw.lib


OBJD = $($(op_sys)_$(version)_object_dir)
MBRD = $($(op_sys)_browser_dir)

SYS_DIR = $($(op_sys)_sys_dir)

Exe_file     = $($(op_sys)_executable)
System       = $($(op_sys)_system)
#SoundFlag    = $($(op_sys)_sound_$(soundsys))
SoundFlag    =

!ifndef LIBRARIES
LIBRARIES    = $($(op_sys)_graphics_lib) $($(op_sys)_$(version)_sound_$(soundsys)_lib)
!endif

!ifdef %LIB
LIBPATH = libpath $(%LIB)
!else
LIBPATH =
!endif


dos_production_sound_on_Includes=
dos_production_sound_off_Includes=
dos_debugging_sound_on_Includes=
dos_debugging_sound_off_Includes=

win_production_sound_on_Includes=
win_production_sound_off_Includes=
win_debugging_sound_on_Includes=
win_debugging_sound_off_Includes=


#compiler option flags
# makefile flag to just preprocess a file.
preprocess_flag = -pcl
pentium_profiling = et

browser_info = -db
remove_stack_checking = -s
# save & restore segment registers
save_registers = -r
optimize_on = -5s -otexan
optimize_off = -5s -od
warning_level = -w3
# Note: -d2 implies -od (No optimizations.)
debugging_on = -d2 -d_DEBUG -hw
large_memory_model = -mf
# Note: -za for Ansi C implies that enum are int.
no_lang_extension = -za
# pack structures at one byte
one_byte_data_packing = -zp1
# minimum int enumerations
int_enumerations = -ei
default_signed_chars = -j
quiet = -zq

# Following doesn't work for DOS debugger
#	$(optimize_on) &

# Following is best setup for WATCOM DOS debugger.
!ifdef PREPROCESS
dos_debugging_Compiler_Options = &
	$(warning_level) &
	$(quiet) &
	$(preprocess_flag) &
	$(SoundFlag) $(ReleaseVer) $(Mem_Check) $(Vec_Check) $(CRC_Check) $(NetDemoVer) $(DemoVer) $($(op_sys)_$(version)_sound_$(sound_sys)_Includes)

dos_production_Compiler_Options = &
	$(dos_debugging_Compiler_Options)
!else
dos_debugging_Compiler_Options = &
	$(optimize_off) &
	$(debugging_on) &
	$(remove_stack_checking) &
	$(warning_level) &
	$(save_registers) &
	$(quiet) &
	$(one_byte_data_packing) &
	$(default_signed_chars) &
	-bt=$(System) -fo$(OBJD)\ &
	$(SoundFlag) $(ReleaseVer) $(Mem_Check) $(Vec_Check) $(CRC_Check) $(NetDemoVer) $(DemoVer) $($(op_sys)_$(version)_sound_$(soundsys)_Includes)

# Following is more optimized
dos_production_Compiler_Options = &
	$(optimize_on) &
	$(remove_stack_checking) &
	$(warning_level) &
	$(quiet) &
	$(one_byte_data_packing) &
	$(default_signed_chars) &
	-bt=$(System) -fo=$(OBJD)\ &
	$(SoundFlag) $(ReleaseVer) $(Mem_Check) $(Vec_Check) $(CRC_Check) $(NetDemoVer) $(DemoVer) &
	$($(op_sys)_$(version)_sound_$(soundsys)_Includes)
!endif

dos_browser_options = &
	$(browser_info) &
	$(optimize_off) &
	$(debugging_on) &
	$(warning_level) &
	$(quiet) &
	$(one_byte_data_packing) &
	$(default_signed_chars) &
	-bt=$(System) -fo=$(MBRD)\ &
	$(SoundFlag) $(ReleaseVer) $(Mem_Check) $(Vec_Check) $(CRC_Check) $(NetDemoVer) $(DemoVer) $($(op_sys)_$(version)_sound_$(soundsys)_Includes)

# Linker_options   =
dos_debugging_Linker_options  = debug all option stack=32k option map
dos_production_Linker_Options = option stack=32k option map


RCCompiler = wrc
RSC_FLAGS = -q -ad -bt=nt -d=_CHARED
#RSC_FLAGS =
RES_OPTIONS = -r -q -ad -bt=nt
#RES_OPTIONS =

!ifdef PREPROCESS
win_debugging_Compiler_Options = &
	$($(op_sys)_$(version)_sound_$(soundsys)_Includes)&
	-dWIN32 -d_WINDOWS $(SoundFlag) $(ReleaseVer) $(Rand_Flag) $(Mem_Check) $(Vec_Check) $(CRC_Check) $(NetDemoVer) $(DemoVer) $(StatusFlag) $(ChrEdFlag) &
	$(warning_level) &
	$(quiet) &
	$(preprocess_flag)
	
win_production_Compiler_Options = &
	$(win_debugging_Compiler_Options)
	
!else
win_debugging_Compiler_Options = &
	$($(op_sys)_$(version)_sound_$(soundsys)_Includes)&
	-dWIN32 -d_WINDOWS $(SoundFlag) $(ReleaseVer) $(Rand_Flag) $(Mem_Check) $(Vec_Check) $(CRC_Check) $(NetDemoVer) $(DemoVer) $(StatusFlag) $(ChrEdFlag)&
	$(optimize_on) &
	$(debugging_on) &
	$(warning_level) &
	$(quiet) &
	$(one_byte_data_packing) &
	$(default_signed_chars) &
	$(large_memory_model) &
	-bt=nt -fo=$(OBJD)\
	
win_production_Compiler_Options = &
	$($(op_sys)_$(version)_sound_$(soundsys)_Includes)&
	-dWIN32 -d_WINDOWS $(SoundFlag) $(ReleaseVer) $(Rand_Flag) $(Mem_Check) $(Vec_Check) $(CRC_Check) $(NetDemoVer) $(DemoVer) $(StatusFlag) $(ChrEdFlag)&
	$(optimize_on) &
	$(warning_level) &
	$(quiet) &
	$(one_byte_data_packing) &
	$(default_signed_chars) &
	$(large_memory_model) &
	-bt=nt -fo=$(OBJD)\
!endif

win_browser_options = &
	$(browser_info) &
	$(optimize_off) &
	$(debugging_on) &
	$($(op_sys)_$(version)_sound_$(soundsys)_Includes)&
	-dWIN32 -d_WINDOWS $(SoundFlag) $(ReleaseVer) $(Rand_Flag) $(Mem_Check) $(Vec_Check) $(CRC_Check) $(NetDemoVer) $(DemoVer) $(StatusFlag) $(ChrEdFlag)&
	$(quiet) &
	$(warning_level) &
	$(one_byte_data_packing) &
	$(default_signed_chars) &
	$(large_memory_model) &
	-bt=nt -fo=$(MBRD)\
	
win_debugging_Linker_Options = debug all option map option q option symf
win_production_Linker_Options = option q option map


SYSTEM_OBJ =	$(OBJD)\memmanag.obj &
				$(OBJD)\resmanag.obj  &
				$(OBJD)\systemx.obj  &
				lzssdcod.obj  &
				$(OBJD)\strmgr.obj &
				$(OBJD)\fopen.obj &
				$(OBJD)\assert.obj &
				$(OBJD)\vdpmi.obj &
				$(OBJD)\random.obj

MACHINE_OBJ =	$(OBJD)\keyint.obj   &
				$(OBJD)\machinex.obj  &
				$(OBJD)\mousex.obj    &
				$(OBJD)\sysgrapx.obj &
				$(OBJD)\statisti.obj &
				$(OBJD)\util.obj

WIN95_OBJ =	$(OBJD)\crc.obj &
				$(OBJD)\mciutil.obj &
				$(OBJD)\redbookx.obj &
            $(OBJD)\chared.obj  &
            $(OBJD)\appear.obj  &
            $(OBJD)\bloodreq.obj  &
            $(OBJD)\ava.obj  &
            $(OBJD)\getfile.obj  &
            $(OBJD)\playrace.obj  &
            $(OBJD)\charedin.obj
				
ENGINE_OBJ =	$(OBJD)\cuex.obj


#converted back to real file
# $(OBJD)\itemtypx.obj
# $(OBJD)\itemeffx.obj

GAME_OBJ =	$(OBJD)\dice.obj  &
				$(OBJD)\fileutil.obj    &
				$(OBJD)\gametypx.obj    &
				$(OBJD)\itemtypx.obj    &
				$(OBJD)\itemeqp.obj    &
				$(OBJD)\itemeffx.obj    &
				$(OBJD)\mapeffx.obj    &
				$(OBJD)\bloodefx.obj    &
				$(OBJD)\relicefx.obj    &
				$(OBJD)\spellefx.obj    &
				$(OBJD)\regentsx.obj    &
				$(OBJD)\mainched.obj    &
				$(OBJD)\realm.obj
				
D_AND_D_OBJ = 	$(OBJD)\armor.obj &
				$(OBJD)\ability.obj &
				$(OBJD)\alignmnt.obj &
				$(OBJD)\blood.obj &
				$(OBJD)\charisma.obj &
				$(OBJD)\constitn.obj &
				$(OBJD)\dexterit.obj  &
				$(OBJD)\experien.obj  &
				$(OBJD)\intelli.obj    &
				$(OBJD)\playid.obj &
				$(OBJD)\playclas.obj &
				$(OBJD)\rlmspell.obj  &
				$(OBJD)\spell.obj   &
				$(OBJD)\spellpro.obj &
				$(OBJD)\strength.obj &
				$(OBJD)\weapon.obj  &
				$(OBJD)\wisdom.obj
				


dos_debugging_OBJ =
dos_production_OBJ =

win_debugging_OBJ = $(OBJD)\debug.obj
			
win_production_OBJ = $(OBJD)\debug.obj &
			
STRMGR_OBJ = $(OBJD)\strmake.obj

####################
## Makefile rules ##
####################

chared.exe: $(SYSTEM_OBJ) $(MACHINE_OBJ) $(WIN95_OBJ) $(ENGINE_OBJ) $(GAME_OBJ) $(D_AND_D_OBJ) chared.res $($(op_sys)_$(version)_OBJ)
        echo $(msg_$(version))
        *wlink system $(System) $(win_$(version)_Linker_Options) name $^. &
                file {$(SYSTEM_OBJ) $(MACHINE_OBJ) $(WIN95_OBJ) $(ENGINE_OBJ) $(GAME_OBJ) $(D_AND_D_OBJ) $(sound_$(soundsys)_objs) $($(op_sys)_$(version)_OBJ) } &
                $(LIBPATH) $(LIBRARIES)
        *wrc $(RSC_FLAGS) chared.rc


strdat.dat: strdat.txt strmake.exe
	echo Making $^.
	strmake

strenum.h: strdat.txt strmake.exe
	echo Making $^.
	strmake

strmake.exe: $(STRMGR_OBJ)
	echo Making $^.
        *wlink system $(System) $(dos_$(version)_Linker_Options) name $^. &
                file {$(STRMGR_OBJ) } &
                $(LIBPATH) $(LIBRARIES)

.EXTENSIONS:
.EXTENSIONS: .exe .obj .mbr .asm .res .c .cpp .h .hxx .hpp .rc .avd .ava

# Search Paths.
.c: $(SYS_DIR);.
.cpp: $(SYS_DIR);.
.ava: $(SCN_AVD);.

.c.obj: .AUTODEPEND
	*$(CCompiler) $($(op_sys)_$(version)_Compiler_Options) $<

.cpp.obj: .AUTODEPEND
	*$(CPPCompiler) $($(op_sys)_$(version)_Compiler_Options) -ew $<

.rc.res: .AUTODEPEND
	*$(RCCompiler) $(RES_OPTIONS) $<

.c.mbr: .AUTODEPEND
	*$(CCompiler) $($(op_sys)_browser_options) $<

.cpp.mbr: .AUTODEPEND
	*$(CPPCompiler) $($(op_sys)_browser_options) $<

.asm.obj: .SYMBOLIC
        *$(ASM) $(AFLAGS) /Fo$(OBJD)\ $<

.ava.avd: .AUTODEPEND makeavd.exe
	makeavd.exe $<
	
$(win_production_object_dir) : .SYMBOLIC
	 MKDIR $^.
	
$(win_debugging_object_dir) : .SYMBOLIC
	 MKDIR $^.
	
$(dos_production_object_dir) : .SYMBOLIC
	 MKDIR $^.
	
$(dos_debugging_object_dir) : .SYMBOLIC
	 MKDIR $^.
	
	
##      proto : .SYMBOLIC
##              protit *.c pcsys\*.c
