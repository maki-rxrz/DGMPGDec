/* 
 *  Mutated into DGIndex. Modifications Copyright (C) 2004, Donald Graft
 * 
 *	Copyright (C) Chia-chen Kuo - April 2001
 *
 *  This file is part of DVD2AVI, a free MPEG-2 decoder
 *	
 *  DVD2AVI is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  DVD2AVI is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *
 */

void Initialize_Buffer(void);
void Fill_Buffer(void);
void Next_Packet(void);
void Flush_Buffer_All(unsigned int N);
unsigned int Get_Bits_All(unsigned int N);
void Next_File(void);

unsigned char Rdbfr[BUFFER_SIZE], *Rdptr, *Rdmax;
unsigned int BitsLeft, CurrentBfr, NextBfr, Val, Read;

__forceinline static unsigned int Show_Bits(unsigned int N)
{
	if (N <= BitsLeft)
	{
		return (CurrentBfr << (32 - BitsLeft)) >> (32 - N);
	}
	else
	{
		N -= BitsLeft;
		return (((CurrentBfr << (32 - BitsLeft)) >> (32 - BitsLeft)) << N) + (NextBfr >> (32 - N));
	}
}

__forceinline static unsigned int Get_Bits(unsigned int N)
{
	if (N < BitsLeft)
	{
		Val = (CurrentBfr << (32 - BitsLeft)) >> (32 - N);
		BitsLeft -= N;
		return Val;
	}
	else
		return Get_Bits_All(N);
}

__forceinline static void Flush_Buffer(unsigned int N)
{
	if (N < BitsLeft)
		BitsLeft -= N;
	else
		Flush_Buffer_All(N);	
}

int _donread(int fd, void *buffer, unsigned int count);

__forceinline static unsigned int Get_Byte()
{
	extern unsigned char *buffer_invalid;

	if (Rdptr >= buffer_invalid)
	{
		// Ran out of good data.
//		ThreadKill();
		Stop_Flag = 1;
		return 0xff;
	}

	while (Rdptr >= Rdbfr+BUFFER_SIZE)
	{
		Read = _donread(Infile[CurrentFile], Rdbfr, BUFFER_SIZE);
		if (Read < BUFFER_SIZE)	Next_File();

		Rdptr -= BUFFER_SIZE;
		Rdmax -= BUFFER_SIZE;
	}

	return *Rdptr++;
}

__forceinline static void Fill_Next()
{
	extern unsigned char *buffer_invalid;

	if (Rdptr >= buffer_invalid)
	{
		// Ran out of good data.
//		ThreadKill();
		Stop_Flag = 1;
		NextBfr = 0xffffffff;
		return;
	}

	if (SystemStream_Flag && Rdptr > Rdmax - 4)
	{
		if (Rdptr >= Rdmax)
			Next_Packet();
		NextBfr = Get_Byte() << 24;

		if (Rdptr >= Rdmax)
			Next_Packet();
		NextBfr += Get_Byte() << 16;

		if (Rdptr >= Rdmax)
			Next_Packet();
		NextBfr += Get_Byte() << 8;

		if (Rdptr >= Rdmax)
			Next_Packet();
		NextBfr += Get_Byte();
	}
	else if (Rdptr <= Rdbfr+BUFFER_SIZE - 4)
	{
		NextBfr = (*Rdptr << 24) + (*(Rdptr+1) << 16) + (*(Rdptr+2) << 8) + *(Rdptr+3);
		Rdptr += 4;
	}
	else
	{
		if (Rdptr >= Rdbfr+BUFFER_SIZE)
			Fill_Buffer();
		NextBfr = *Rdptr++ << 24;

		if (Rdptr >= Rdbfr+BUFFER_SIZE)
			Fill_Buffer();
		NextBfr += *Rdptr++ << 16;

		if (Rdptr >= Rdbfr+BUFFER_SIZE)
			Fill_Buffer();
		NextBfr += *Rdptr++ << 8;

		if (Rdptr >= Rdbfr+BUFFER_SIZE)
			Fill_Buffer();
		NextBfr += *Rdptr++;
	}
}

__forceinline static void next_start_code()
{
	unsigned int show;

	Flush_Buffer(BitsLeft & 7);

	while ((show = Show_Bits(24)) != 1)
	{
		if (Stop_Flag == true)
			return;
		Flush_Buffer(8);
	}
}

__forceinline static unsigned int Get_Short()
{
	unsigned int i, j;
	
	i = Get_Byte();
	j = Get_Byte();
	return ((i << 8) | j);
}
