       include "earlymacros.i"
       include "build/srcs/globalvars.i"
       section "porttest",code_p
       xref   PortTestMenu
       xref   PortTestPar
       xref   PortTestSer
       xref   PortTestJoystick

PortTestMenu:
       bra    MainMenu

PortTestPar:
       bra    MainMenu
PortTestSer:
       bra    MainMenu
PortTestJoystick:
       bra    MainMenu