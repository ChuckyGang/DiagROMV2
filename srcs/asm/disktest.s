       include "earlymacros.i"
       include "build/srcs/globalvars.i"
       section "disktest",code_p
       xref   DiskTest
       xref   DiskdriveTest
       xref   GayleTest
       xref   GayleExp
       EVEN
DiskTest:
       bra    MainMenu

DiskdriveTest:
       bra    MainMenu
GayleTest:
       bra    MainMenu
GayleExp:
       bra    MainMenu

       EVEN