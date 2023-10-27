#ifndef UTILS_H
#define UTILS_H


extern int gpio asm ("GPIO");
extern int debug asm ("DEBUG");
extern int timer asm ("TIMER");
extern int i2cbus asm ("I2CBUS");
const char digits[16] = {'0','1','2','3','4','5','6','7','8','9', 'A', 'B', 'C', 'D', 'E', 'F'};



void i2cbus_write(uint8_t dev_addr, uint8_t data_addr, uint8_t data){
  i2cbus = (data << 16) | (data_addr << 8) | dev_addr | 0;
}

uint8_t i2cbus_read(uint8_t dev_addr, uint8_t data_addr){
  i2cbus = (data_addr << 8) | dev_addr | 1;
  return(i2cbus & 0xFF);
}

void *
_sbrk (incr)
     int incr;
{
   extern char   end; /* Set by linker.  */
   static char * heap_end;
   char *        prev_heap_end;

   if (heap_end == 0)
     heap_end = & end;

   prev_heap_end = heap_end;
   heap_end += incr;

   return (void *) prev_heap_end;
}

void sleep(int microseconds){
    int start = timer;
    while ((timer-start) < microseconds);
    return;
}

void delay(int ms){
    int start = timer;
    int microseconds = ms*1000;
    while ((timer-start) < microseconds);
    return;
}
    
void prints (char* str){
    int i = 0;
    while ((int)str[i] != 0){
        debug = str[i];
        i = i+1;
    }
    return;
}

void printi(int val){
    if (val == 0){
        debug = '0';
        return;
    }

    if (val < 0){
        debug = '-';
        val = val*-1;
    }
    int num [10];
    for (int i=0;i<10;i=i+1){
        num[i] = val - 10*(val/10); val = val/10;
    }
    int start = 0;

    for (int i=10;i>0;i=i-1){
        if (start)
            debug = digits[num[i-1]];
        else if (num[i-1] > 0){
            debug = digits[num[i-1]];
            start = 1;
        }
        else 
            debug = 0;
        
    }
    return;
}

void done(){
    prints("!q!\n\r");
    return;
}


//https://stackoverflow.com/questions/46631410/how-to-write-custom-printf
void printf(char *c, ...)
{
    char *s;
    va_list lst;
    va_start(lst, c);
    while(*c != '\0')
    {
        if(*c != '%')
        {
            debug = *c;
            c++;
            continue;
        }

        c++;

        if(*c == '\0')
        {
            break;
        }

        switch(*c)
        {
            case 's': prints(va_arg(lst, char *)); break;
            case 'd': printi(va_arg(lst, int)); break;
        }
        c++;
    }
}

void putchar(int c){
    debug = c;
}

void puts(char* c){
    prints(c);
}
#endif
