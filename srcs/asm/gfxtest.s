       include "earlymacros.i"
       include "build/srcs/globalvars.i"
       section "gfxtest",code_p
       xref   GFXtestMenu
       xref   GFXTestScreen
       xref   GFXtest320x200
       xref   GFXTestScroll
       xref   GFXTestRaster
       xref   GFXTestRGB
       EVEN
       
GFXtestMenu:
       bra    MainMenu

GFXTestScreen:
       bra    MainMenu
GFXtest320x200:
       bra    MainMenu
GFXTestScroll:
       bra    MainMenu
GFXTestRaster:
       bra    MainMenu
GFXTestRGB:
       bra    MainMenu

       EVEN