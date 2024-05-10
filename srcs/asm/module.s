       section "module",code_p
       xdef   Music
       xdef   EndMusic
       xdef   MusicSize
       MusicSize: EQU    EndMusic-Music
Music:
       incbin "data/Music.MOD"
EndMusic: