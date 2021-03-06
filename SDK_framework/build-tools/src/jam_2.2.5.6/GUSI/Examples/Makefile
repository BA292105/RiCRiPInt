#######################################################################
#	Project	:	GUSI				-	Grand Unified Socket Interface
#	File		:	Makefile			-	Build examples
#	Author	:	Matthias Neeracher
#	Language	:	MPW Shell/Make
#	
#	$Log: Examples:Makefile,v $
#	Revision 1.2  1999/04/23  08:49:52  peterg
#	Automatic checkin:
#	changed attribute _comment to '#	'
#
#	Revision 1.3  1994/12/31  03:57:01  neeri
#	Link with 1.5.0 release.
#
#	Revision 1.2  1994/12/31  01:03:04  neeri
#	Add PowerPC examples.
#
#	Revision 1.1  1994/02/25  02:48:56  neeri
#	Initial revision
#
#######################################################################

#######################################################################
# You might not need to change anything below this line 
#

#
# Specify one of:
#  68K			Build 68K version only
#	PPC			Build PPC version only
#	Both			Build both 68K and PPC versions
#

Focus		=	Both

#
# 68K debugging options
#

Debug68K				=	-sym on -mbg on

#
# PowerPC debugging options. Note that -sym on disables optimization!
#

DebugPPC				=	-sym on

#
# PowerPC optimization options. Note that -sym on above disables optimization!
#

PPCCOpt				=	-opt local

#
# Tool options
#

GUSIInc				=	-i : -i ::include
COptions 			= 	-r {GUSIInc} {Debug68K}
CPlusOptions 		= 	-mf {GUSIInc} {Debug68K}
PPCCExt				=	-d macintosh -appleext on
PPCCOptions 		=  -shared_lib_export on {PPCCOpt} {PPCCExt} {GUSIInc} {DebugPPC}
LOptions				= 	-sym 3.2 -mf -w
ROptions 			= 	-i :

PPCLibs = 											�
		"{PPCLibraries}"InterfaceLib.xcoff	�
		"{PPCLibraries}"StdCLib.xcoff			�
		"{PPCLibraries}"PPCSIOW.o				�
		"{PPCLibraries}"StdCRuntime.o			�
		"{PPCLibraries}"CPlusLib.o				�
		"{PPCLibraries}"PPCCRuntime.o			�
		"{PPCLibraries}"PPCToolLibs.o			�
		"{PPCLibraries}"MathLib.xcoff

PPCLibEqu = 													�
		-l "GUSI.xcoff=GUSI#0x01508000-0x01508000" 	�
		-l InterfaceLib.xcoff=InterfaceLib 				�
      -l StdCLib.xcoff=StdCLib 							�
		-l MathLib.xcoff=MathLib
		
:Obj:			�	:
.c.o.68K	�	.c
	{C} {COptions} {DepDir}{Default}.c -o {TargDir}{Default}.c.o.68K
.c.o.PPC	�	.c
	PPCC {PPCCOptions} {DepDir}{Default}.c -o {TargDir}{Default}.c.o.PPC

GUSITest			�	GUSIFileTest	GUSIFinger		�
						GUSIUnixTest	GUSIUnixTest2	�
						GUSIAtlkTest	GUSIAtlkTest2	�
						GUSIPPCTest		GUSIPPCTest2	�
						GUSIINETTest	GUSIINETTest2	�
						GUSIPAPTest

GUSITest.68K	�	GUSIFileTest.68K	GUSIFinger.68K		�
						GUSIUnixTest.68K	GUSIUnixTest2.68K	�
						GUSIAtlkTest.68K	GUSIAtlkTest2.68K	�
						GUSIPPCTest.68K	GUSIPPCTest2.68K	�
						GUSIINETTest.68K	GUSIINETTest2.68K	�
						GUSIPAPTest.68K

GUSITest.PPC	�	GUSIFileTest.PPC	�
						GUSIAtlkTest.PPC	GUSIAtlkTest2.PPC	�
						GUSIPPCTest.PPC	GUSIPPCTest2.PPC	�
						GUSIINETTest.PPC	GUSIINETTest2.PPC	�
						GUSIPAPTest.PPC

GUSITest.Both	�	GUSIFileTest.Both	GUSIFinger.Both		�
						GUSIUnixTest.Both	GUSIUnixTest2.Both	�
						GUSIAtlkTest.Both	GUSIAtlkTest2.Both	�
						GUSIPPCTest.Both	GUSIPPCTest2.Both		�
						GUSIINETTest.Both	GUSIINETTest2.Both	�
						GUSIPAPTest.Both

GUSIFileTest.68K 	��	::Lib:GUSI.o GUSIFileTest.c.o GUSITest.c.o
	Link {LOptions} -t APPL -c '????' -o GUSIFileTest.68K	�
		GUSIFileTest.c.o											�
		GUSITest.c.o												�
		::Lib:GUSI.o												�
		"{Libraries}SIOW.o"										�
		"{Libraries}Runtime.o"									�
		"{Libraries}Interface.o"								�
		"{CLibraries}StdCLib.o"									�
		"{Libraries}IntEnv.o"									�
		"{Libraries}ToolLibs.o"

GUSIFileTest.68K	�� ::GUSI.rsrc ::GUSI.r GUSITest.rsrc GUSITest.r
	Rez {ROptions} -a -o GUSIFileTest.68K GUSITest.r

GUSIFileTest.xcoff �� ::Lib:GUSI.xcoff :Obj:GUSIFileTest.c.o.PPC :Obj:GUSITest.c.o.PPC
	PPCLink									�
		:Obj:GUSIFileTest.c.o.PPC		�
		:Obj:GUSITest.c.o.PPC			�
		::Lib:GUSISIOW.xcoff				�
		::Lib:GUSI.xcoff 					�
		{PPCLibs}				 			�
		-sym on				 			 	�
		-map GUSIFileTest.map			�
		-o GUSIFileTest.xcoff
	MakeSYM -cttes -i :: GUSIFileTest.xcoff

GUSIFileTest.PPC �� GUSIFileTest.xcoff
	MakePEF -ft APPL -fc '????' �
	  GUSIFileTest.xcoff �
     {PPCLibEqu} �
   -o GUSIFileTest.PPC					

GUSIFileTest.PPC	�� ::GUSI.rsrc ::GUSI.r GUSITest.rsrc GUSITest.r
	Rez {ROptions} -a -d PowerPC -d APPNAME=�"GUSIFileTest�" -o GUSIFileTest.PPC GUSITest.r

GUSIFileTest.Both	�	GUSIFileTest.68K GUSIFileTest.PPC

GUSIFileTest		�	GUSIFileTest.{Focus}

GUSIUnixTest.68K �� ::Lib:GUSI.o GUSIUnixTest.c.o GUSITest.c.o GUSISocketTest.c.o
	Link {LOptions} -t APPL -c '????' -o GUSIUnixTest.68K	�
		GUSIUnixTest.c.o											�
		GUSITest.c.o												�
		GUSISocketTest.c.o										�
		::Lib:GUSI.o												�
		::Lib:GUSI_Unix.cfg										�
		"{Libraries}SIOW.o"										�
		"{Libraries}Runtime.o"									�
		"{Libraries}Interface.o"								�
		"{CLibraries}StdCLib.o"									�
		"{Libraries}IntEnv.o"									�
		"{Libraries}ToolLibs.o"	
	
GUSIUnixTest.68K	��	::GUSI.rsrc ::GUSI.r GUSITest.rsrc GUSITest.r	
	Rez {ROptions} -a -o GUSIUnixTest.68K GUSITest.r

GUSIUnixTest.Both	�	GUSIUnixTest.68K

GUSIUnixTest		�	GUSIUnixTest.68K

GUSIUnixTest2.68K	�	GUSIUnixTest.68K
	Duplicate -y GUSIUnixTest.68K GUSIUnixTest2.68K

GUSIUnixTest2.Both	�	GUSIUnixTest2.68K

GUSIUnixTest2		�	GUSIUnixTest2.68K

GUSIAtlkTest.68K �� ::Lib:GUSI.o  GUSIAtlkTest.c.o GUSITest.c.o GUSISocketTest.c.o
	Link {LOptions} -t APPL -c '????' -o GUSIAtlkTest.68K	�
		GUSIAtlkTest.c.o											�
		GUSITest.c.o												�
		GUSISocketTest.c.o										�
		::Lib:GUSI.o 												�
		::Lib:GUSI_Appletalk.cfg								�
		"{Libraries}SIOW.o"										�
		"{Libraries}Runtime.o"									�
		"{Libraries}Interface.o"								�
		"{CLibraries}StdCLib.o"									�
		"{Libraries}IntEnv.o"									�
		"{Libraries}ToolLibs.o"	

GUSIAtlkTest.68K	��	::GUSI.rsrc ::GUSI.r GUSITest.rsrc GUSITest.r
	Rez {ROptions} -a -o GUSIAtlkTest.68K GUSITest.r

GUSIAtlkTest.xcoff �� ::Lib:GUSI.xcoff :Obj:GUSIAtlkTest.c.o.PPC :Obj:GUSITest.c.o.PPC :Obj:GUSISocketTest.c.o.PPC
	PPCLink									�
		:Obj:GUSIAtlkTest.c.o.PPC		�
		:Obj:GUSITest.c.o.PPC			�
		:Obj:GUSISocketTest.c.o.PPC	�
		::Lib:GUSISIOW.xcoff				�
		::Lib:GUSI.xcoff 					�
		{PPCLibs}				 			�
		-sym on				 			 	�
		-o GUSIAtlkTest.xcoff
	MakeSYM -cttes -i :: GUSIAtlkTest.xcoff

GUSIAtlkTest.PPC �� GUSIAtlkTest.xcoff
	MakePEF -ft APPL -fc '????' �
	  GUSIAtlkTest.xcoff �
     {PPCLibEqu} �
   -o GUSIAtlkTest.PPC					
	
GUSIAtlkTest.PPC	��	::GUSI.rsrc ::GUSI.r GUSITest.rsrc GUSITest.r
	Rez {ROptions} -a -d PowerPC -d APPNAME=�"GUSIAtlkTest�" -o GUSIAtlkTest.PPC GUSITest.r

GUSIAtlkTest.Both	�	GUSIAtlkTest.68K GUSIAtlkTest.PPC

GUSIAtlkTest		�	GUSIAtlkTest.{Focus}

GUSIAtlkTest2.68K	�	GUSIAtlkTest.68K
	Duplicate -y GUSIAtlkTest.68K GUSIAtlkTest2.68K

GUSIAtlkTest2.PPC	�	GUSIAtlkTest.PPC
	Duplicate -y GUSIAtlkTest.PPC GUSIAtlkTest2.PPC

GUSIAtlkTest2.Both	�	GUSIAtlkTest2.68K GUSIAtlkTest2.PPC

GUSIAtlkTest2		�	GUSIAtlkTest2.{Focus}

GUSIPPCTest.68K �� ::Lib:GUSI.o  GUSIPPCTest.c.o GUSITest.c.o GUSISocketTest.c.o
	Link {LOptions} -t APPL -c '????' -o GUSIPPCTest.68K	�
		GUSIPPCTest.c.o												�
		GUSITest.c.o													�
		GUSISocketTest.c.o											�
		::Lib:GUSI.o 													�
		::Lib:GUSI_PPC.cfg											�
		"{Libraries}SIOW.o"											�
		"{Libraries}Runtime.o"										�
		"{Libraries}Interface.o"									�
		"{CLibraries}StdCLib.o"										�
		"{Libraries}IntEnv.o"									�
		"{Libraries}ToolLibs.o"	
	
GUSIPPCTest.68K	��	::GUSI.rsrc ::GUSI.r GUSITest.rsrc GUSITest.r
	Rez {ROptions} -a -o GUSIPPCTest.68K GUSITest.r

GUSIPPCTest.xcoff �� ::Lib:GUSI.xcoff :Obj:GUSIPPCTest.c.o.PPC :Obj:GUSITest.c.o.PPC :Obj:GUSISocketTest.c.o.PPC
	PPCLink									�
		:Obj:GUSIPPCTest.c.o.PPC		�
		:Obj:GUSITest.c.o.PPC			�
		:Obj:GUSISocketTest.c.o.PPC	�
		::Lib:GUSISIOW.xcoff				�
		::Lib:GUSI.xcoff 					�
		{PPCLibs}				 			�
		-sym on				 			 	�
		-o GUSIPPCTest.xcoff
	MakeSYM -cttes -i :: GUSIPPCTest.xcoff

GUSIPPCTest.PPC �� GUSIPPCTest.xcoff
	MakePEF -ft APPL -fc '????' �
	  GUSIPPCTest.xcoff �
     {PPCLibEqu} �
   -o GUSIPPCTest.PPC					
	
GUSIPPCTest.PPC	��	::GUSI.rsrc ::GUSI.r GUSITest.rsrc GUSITest.r
	Rez {ROptions} -a -d PowerPC -d APPNAME=�"GUSIPPCTest�" -o GUSIPPCTest.PPC GUSITest.r

GUSIPPCTest.Both	�	GUSIPPCTest.68K GUSIPPCTest.PPC

GUSIPPCTest			�	GUSIPPCTest.{Focus}

GUSIPPCTest2.68K	�	GUSIPPCTest.68K
	Duplicate -y GUSIPPCTest.68K GUSIPPCTest2.68K

GUSIPPCTest2.PPC	�	GUSIPPCTest.PPC
	Duplicate -y GUSIPPCTest.PPC GUSIPPCTest2.PPC

GUSIPPCTest2.Both	�	GUSIPPCTest2.68K GUSIPPCTest2.PPC

GUSIPPCTest2		�	GUSIPPCTest2.{Focus}

GUSIPAPTest.68K �� ::Lib:GUSI.o GUSIPAPTest.c.o GUSITest.c.o GUSISocketTest.c.o
	Link {LOptions} -t APPL -c '????' -o GUSIPAPTest.68K	�
		GUSIPAPTest.c.o												�
		GUSITest.c.o													�
		GUSISocketTest.c.o											�
		::Lib:GUSI.o 													�
		::Lib:GUSI_PAP.cfg											�
		"{Libraries}SIOW.o"											�
		"{Libraries}Runtime.o"										�
		"{Libraries}Interface.o"									�
		"{CLibraries}StdCLib.o"										�
		"{Libraries}IntEnv.o"									�
		"{Libraries}ToolLibs.o"	
	
GUSIPAPTest.68K	��	::GUSI.rsrc ::GUSI.r GUSITest.rsrc GUSITest.r
	Rez {ROptions} -a -o GUSIPAPTest.68K GUSITest.r

GUSIPAPTest.xcoff �� ::Lib:GUSI.xcoff :Obj:GUSIPAPTest.c.o.PPC :Obj:GUSITest.c.o.PPC :Obj:GUSISocketTest.c.o.PPC
	PPCLink									�
		:Obj:GUSIPAPTest.c.o.PPC		�
		:Obj:GUSITest.c.o.PPC			�
		:Obj:GUSISocketTest.c.o.PPC	�
		::Lib:GUSISIOW.xcoff				�
		::Lib:GUSI.xcoff 					�
		{PPCLibs}				 			�
		-sym on				 			 	�
		-o GUSIPAPTest.xcoff
	MakeSYM -cttes -i :: GUSIPAPTest.xcoff

GUSIPAPTest.PPC �� GUSIPAPTest.xcoff
	MakePEF -ft APPL -fc '????' �
	  GUSIPAPTest.xcoff �
     {PPCLibEqu} �
   -o GUSIPAPTest.PPC				
	
GUSIPAPTest.PPC	��	::GUSI.rsrc ::GUSI.r GUSITest.rsrc GUSITest.r
	Rez {ROptions} -a -d PowerPC -d APPNAME=�"GUSIPAPTest�" -o GUSIPAPTest.PPC GUSITest.r

GUSIPAPTest.Both	�	GUSIPAPTest.68K GUSIPAPTest.PPC

GUSIPAPTest		�	GUSIPAPTest.{Focus}

GUSIINETTest.68K �� ::Lib:GUSI.o GUSIINETTest.c.o GUSITest.c.o GUSISocketTest.c.o
	Link {LOptions} -t APPL -c '????' -o GUSIINETTest.68K	�
		GUSIINETTest.c.o												�
		GUSITest.c.o													�
		GUSISocketTest.c.o											�
		::Lib:GUSI.o 													�
		::Lib:GUSI_Internet.cfg										�
		"{Libraries}SIOW.o"											�
		"{Libraries}Runtime.o"										�
		"{Libraries}Interface.o"									�
		"{CLibraries}StdCLib.o"										�
		"{Libraries}IntEnv.o"										�
		"{Libraries}ToolLibs.o"	
	
GUSIINETTest.68K	��	::GUSI.rsrc ::GUSI.r GUSITest.rsrc GUSITest.r
	Rez {ROptions} -a -o GUSIINETTest.68K GUSITest.r

GUSIINETTest.xcoff �� ::Lib:GUSI.xcoff :Obj:GUSIINETTest.c.o.PPC :Obj:GUSITest.c.o.PPC :Obj:GUSISocketTest.c.o.PPC
	PPCLink									�
		:Obj:GUSIINETTest.c.o.PPC		�
		:Obj:GUSITest.c.o.PPC			�
		:Obj:GUSISocketTest.c.o.PPC	�
		::Lib:GUSISIOW.xcoff				�
		::Lib:GUSI.xcoff 					�
		{PPCLibs}				 			�
		-sym on				 			 	�
		-o GUSIINETTest.xcoff
	MakeSYM -cttes -i :: GUSIINETTest.xcoff

GUSIINETTest.PPC �� GUSIINETTest.xcoff
	MakePEF -ft APPL -fc '????' �
	  GUSIINETTest.xcoff �
     {PPCLibEqu} �
   -o GUSIINETTest.PPC					
	
GUSIINETTest.PPC	��	::GUSI.rsrc ::GUSI.r GUSITest.rsrc GUSITest.r
	Rez {ROptions} -a -d PowerPC -d APPNAME=�"GUSIINETTest�" -o GUSIINETTest.PPC GUSITest.r

GUSIINETTest.Both	�	GUSIINETTest.68K GUSIINETTest.PPC

GUSIINETTest		�	GUSIINETTest.{Focus}

GUSIINETTest2.68K	�	GUSIINETTest.68K
	Duplicate -y GUSIINETTest.68K GUSIINETTest2.68K

GUSIINETTest2.PPC	�	GUSIINETTest.PPC
	Duplicate -y GUSIINETTest.PPC GUSIINETTest2.PPC

GUSIINETTest2.Both	�	GUSIINETTest2.68K GUSIINETTest2.PPC

GUSIINETTest2		�	GUSIINETTest2.{Focus}

GUSIFinger.68K �� ::Lib:GUSI.o GUSIFinger.c.o 
	Link {LOptions} -t APPL -c '????' -o GUSIFinger.68K	�
		GUSIFinger.c.o													�
		::Lib:GUSI.o 													�
		::Lib:GUSI_Internet.cfg										�
		"{Libraries}Runtime.o"										�
		"{Libraries}Interface.o"									�
		"{CLibraries}StdCLib.o"										�
		"{Libraries}IntEnv.o"									�
		"{Libraries}ToolLibs.o"	
	
GUSIFinger.68K	��	::GUSI.rsrc ::GUSI.r GUSIFinger.r
	Rez {ROptions} -a -o GUSIFinger.68K GUSIFinger.r

GUSIFinger.Both	�	GUSIFinger.68K

GUSIFinger		�	GUSIFinger.68K

