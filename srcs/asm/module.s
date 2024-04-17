       section "module",code_p
       xref   Music
       xref   EndMusic
       xref   MusicSize
       MusicSize: EQU    EndMusic-Music
Music:
       incbin "data/Music.MOD"
EndMusic: