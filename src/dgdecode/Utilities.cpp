#include <windows.h>

#define MAGIC_NUMBER (0xdeadbeef)

bool PutHintingData(unsigned char *video, unsigned int hint)
{
	unsigned char *p;
	unsigned int i, magic_number = MAGIC_NUMBER;
	bool error = false;

	p = video;
	for (i = 0; i < 32; i++)
	{
		*p &= ~1; 
		*p++ |= ((magic_number & (1 << i)) >> i);
	}
	for (i = 0; i < 32; i++)
	{
		*p &= ~1;
		*p++ |= ((hint & (1 << i)) >> i);
	}
	return error;
}

bool GetHintingData(unsigned char *video, unsigned int *hint)
{
	unsigned char *p;
	unsigned int i, magic_number = 0;
	bool error = false;

	p = video;
	for (i = 0; i < 32; i++)
	{
		magic_number |= ((*p++ & 1) << i);
	}
	if (magic_number != MAGIC_NUMBER)
	{
		error = true;
	}
	else
	{
		*hint = 0;
		for (i = 0; i < 32; i++)
		{
			*hint |= ((*p++ & 1) << i);
		}
	}
	return error;
}
