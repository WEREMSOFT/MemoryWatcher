#define BUFFER_SIZE 1000
#define BUFFER_SIZE_MINUS_1 999

#include <string.h>
#include <stdint.h>


int main(void)
{
	char *text = "ThIS IS A TEST!!!!";
	int i = 0;
	int phase = 1;

	uint32_t buffer[BUFFER_SIZE] = {0xff0000};
    uint32_t fill_value = 0xFF0000; // Rojo en RGB

    for (size_t i = 0; i < BUFFER_SIZE; ++i) {
        buffer[i] = fill_value;
    }
	
	while(1)
	{
		i++;
		int j = i;
		i %= BUFFER_SIZE;
		if(i != j)
		{
			phase == 0?1:0;	
		}
		
		// buffer[i] = 0xFF * phase;
	}

	return 0;
}
