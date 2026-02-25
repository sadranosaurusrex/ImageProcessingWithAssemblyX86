## Command for running [[kernelConvolution.asm]]: 
- `nasm -f elf64 kernelConvolution.asm -o kernel.o && gcc -c test.c -o test.o && gcc -no-pie test.o kernel.o -o my_program -lm && ./my_program`

## command for runnign [[highlevelTestConvolution.c]]:
- `gcc -c highLevelTestConvolution.c -o test.o && gcc test.o -o my_program -lm && ./my_program`

## Command for running [[CVR.c]]:
- ```nasm -f elf64 kernelConvolution.asm -o kernel.o && gcc -c CVR.c -o test.o && gcc -no-pie test.o kernel.o -o my_program -lm && ./my_program```