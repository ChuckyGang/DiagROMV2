       include "earlymacros.i"
       include "build/srcs/globalvars.i"
       section "memtest",code_p
       xref   MemtestMenu
       xref   CheckDetectedChip
       xref   CheckExtendedChip
       xref   CheckDetectedMBMem
       xref   CheckExtended16MBMem
       xref   ForceExtended16MBMem
       xref   Detectallmemory
       xref   CheckMemManual
       xref   CheckMemEdit
       xref   AutoConfig

MemtestMenu:
       bra    MainMenu
CheckDetectedChip:
       bra    MainMenu
CheckExtendedChip:
       bra    MainMenu
CheckDetectedMBMem:
       bra    MainMenu
CheckExtended16MBMem:
       bra    MainMenu
ForceExtended16MBMem:
       bra    MainMenu
Detectallmemory:
       bra    MainMenu
CheckMemManual:
       bra    MainMenu
CheckMemEdit:
       bra    MainMenu
AutoConfig:
       bra    MainMenu