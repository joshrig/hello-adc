set print pretty on
set target-async on

file AtmelStart.elf

define reset
       target extended-remote localhost:3333

       monitor reset

       load

       monitor reg sp 0x200118C0
       monitor reg pc 0x00000000

       monitor reg

end