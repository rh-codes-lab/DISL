#include <stdarg.h> 
#include <stdint.h>
#include <stddef.h>
#include "utils.h"
#include "test.h"
int main( )
{
	uint32_t rs1 = 0;
	uint32_t rs2 = 0;
	uint32_t rd;
	uint32_t valid = 0;
	__asm__ (".insn r 0x2B , 0x0, 0, %[rd] , %[rs1], %[rs2]" : [rd] "=r" (rd) : [rs1] "r" (rs1), [rs2] "r" (rs2));
	delay(100);
	printf("Detected\tExpected\n\r");
	for (int test = 0; test < NUM_TESTS; test++){
		for (int i = 0; i < 16; i++){
			for (int j = 0; j < 7; j++){
				rs2 = rows[test][i][j*2+0];
				rs1 = rows[test][i][j*2+1];
			 	__asm__ (".insn r 0x2B , 0x2, 0, %[rd] , %[rs1], %[rs2]" : [rd] "=r" (rd) : [rs1] "r" (rs1), [rs2] "r" (rs2));
			}
			__asm__ (".insn r 0x2B , 0x3, 0, %[rd] , %[rs1], %[rs2]" : [rd] "=r" (rd) : [rs1] "r" (rs1), [rs2] "r" (rs2));
			valid = 0;
			while (!valid){
				__asm__ (".insn r 0x2B , 0x1, 0, %[rd] , %[rs1], %[rs2]" : [rd] "=r" (rd) : [rs1] "r" (rs1), [rs2] "r" (rs2));
				valid = rd&512;
			}
		}
		valid = 0;
		while (!valid){
			__asm__ (".insn r 0x2B , 0x1, 0, %[rd] , %[rs1], %[rs2]" : [rd] "=r" (rd) : [rs1] "r" (rs1), [rs2] "r" (rs2));
			valid = rd&256;
		}
		__asm__ (".insn r 0x2B , 0x4, 0, %[rd] , %[rs1], %[rs2]" : [rd] "=r" (rd) : [rs1] "r" (rs1), [rs2] "r" (rs2));
		printf("%d\t%d\n\r", rd&255,results[test]);
		delay(10);
	}
	while(1);
}


