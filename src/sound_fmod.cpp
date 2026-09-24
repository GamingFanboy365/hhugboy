/*
   hhugboy Game Boy emulator
   copyright 2013 taizou

   Based on GEST
   Copyright (C) 2003-2010 TM

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

// FMOD audio output used by the Windows frontend

#define WIN32_LEAN_AND_MEAN

#include <string.h>
#include <windows.h>

#include "sound.h"

extern int speedup;

int channel_n = 0;

FSOUND_SAMPLE* FSbuffer;

int sound_next_position = 0;

void sound_output_reset()
{
   sound_next_position = 0;
}

void sound_output_write(signed short* samples, int bytes)
{
   void* ptr1 = NULL; 
   unsigned int bytes1 = 0; 
   void* ptr2 = NULL; 
   unsigned int bytes2 = 0; 
            
   if(!speedup && FSOUND_IsPlaying(channel_n) == TRUE)
   {
      unsigned int play = 0;
               
      for(;;)
      {
         play = FSOUND_GetCurrentPosition(channel_n);
         if(!play) 
            break;
         play <<= 2;
         if((play < sound_next_position) || (play > sound_next_position+bytes))
            break;
         
        /* if(options->reduce_cpu_usage)
         {
            Sleep(1);
         }*/
      } 
   }
      
   if(FSOUND_Sample_Lock(FSbuffer,sound_next_position,bytes,&ptr1,&ptr2,&bytes1,&bytes2) == FALSE)
      return;

   sound_next_position += bytes;
   sound_next_position = sound_next_position % sound_buffer_total_len;

   memcpy(ptr1,samples,bytes1);
   if(ptr2 != NULL)
      memcpy(ptr2,samples+bytes1,bytes2);
   
   FSOUND_Sample_Unlock(FSbuffer,ptr1,ptr2,bytes1,bytes2);
}
