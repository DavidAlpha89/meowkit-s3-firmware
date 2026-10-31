    .section .rodata
    .balign 4
    .global boot_sound_mp3_data
boot_sound_mp3_data:
    .incbin "/home/david/.hermes/cache/scratch/meowkit/fork/src/system/assets/boot_sound effects.mp3", 0, 75902
    .balign 4
    .global button_sound_mp3_data
button_sound_mp3_data:
    .incbin "/home/david/.hermes/cache/scratch/meowkit/fork/src/system/assets/button_sound effects.mp3", 0, 33024
    .balign 4
