       include "earlymacros.i"
       include "build/srcs/globalvars.i"
       section "irqcia",code_p
       xref   IRQCIAtestMenu
       xref   IRQCIAIRQTest
       xref   IRQCIACIATest
       xref   IRQCIATest
       EVEN

IRQCIAtestMenu:
       bra    MainMenu

IRQCIAIRQTest:
       bra    MainMenu
IRQCIACIATest:
       bra    MainMenu
IRQCIATest:
       bra    MainMenu

       EVEN