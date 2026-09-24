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

#include "input.h"
#include "GB.h"
#include "mainloop.h"
#include "config.h"

int soft_reset = 0;

int autofire_delay[4][4];

bool apply_keyboard_state(const char* buffer, int i)
{
   if(soft_reset)
   {
      if(soft_reset == 1)
      {
         GB1->button_pressed[B_START] = GB1->button_pressed[B_SELECT] = GB1->button_pressed[B_A] = GB1->button_pressed[B_B] = 0;
      } else
      {
         GB2->button_pressed[B_START] = GB2->button_pressed[B_SELECT] = GB2->button_pressed[B_A] = GB2->button_pressed[B_B] = 0;
      }

      soft_reset = 0;
      return false;
   }         
      
   if(KEYDOWN(buffer,options->multi_key_config[i][BUTTON_TURBO_A]))
   {
      if(--autofire_delay[i][0] <= 0)
      {
         autofire_delay[i][0] = options->autofire_speed;
         GB->button_pressed[B_A] = !GB->button_pressed[B_A];
      }
   }
   else
   if(KEYDOWN(buffer,options->multi_key_config[i][BUTTON_A]))
      GB->button_pressed[B_A] = 0;           
   else
   {
      GB->button_pressed[B_A] = 1;
      autofire_delay[i][0] = 0;
   }
      
   if(KEYDOWN(buffer,options->multi_key_config[i][BUTTON_TURBO_B]))
   {
      if(--autofire_delay[i][1] <= 0)
      {
         autofire_delay[i][1] = options->autofire_speed;
         GB->button_pressed[B_B] = !GB->button_pressed[B_B];
      }
   }
   else                 
   if(KEYDOWN(buffer,options->multi_key_config[i][BUTTON_B]))
      GB->button_pressed[B_B] = 0;           
   else
   {
      GB->button_pressed[B_B] = 1;
      autofire_delay[i][1] = 0;
   }
         
   if(KEYDOWN(buffer,options->multi_key_config[i][BUTTON_LEFT]))
   {                       
      GB->button_pressed[B_LEFT] = 0;
      if(!options->opposite_directions_allowed) GB->button_pressed[B_RIGHT] = 1;
   }
   else
      GB->button_pressed[B_LEFT] = 1; 
         
   if(KEYDOWN(buffer,options->multi_key_config[i][BUTTON_RIGHT]))
   {
      GB->button_pressed[B_RIGHT] = 0;                       
      if(!options->opposite_directions_allowed) GB->button_pressed[B_LEFT] = 1;
   }
   else  
      GB->button_pressed[B_RIGHT] = 1; 
          
   if(KEYDOWN(buffer,options->multi_key_config[i][BUTTON_UP]))
   {
      GB->button_pressed[B_UP] = 0;                       
      if(!options->opposite_directions_allowed) GB->button_pressed[B_DOWN] = 1;
   }
   else    
      GB->button_pressed[B_UP] = 1;   
        
   if(KEYDOWN(buffer,options->multi_key_config[i][BUTTON_DOWN]))
   {
      GB->button_pressed[B_DOWN] = 0;                       
      if(!options->opposite_directions_allowed) GB->button_pressed[B_UP] = 1;
   }
   else    
      GB->button_pressed[B_DOWN] = 1;     

   if(KEYDOWN(buffer,options->multi_key_config[i][BUTTON_TURBO_START]))
   {
      if(--autofire_delay[i][2] <= 0)
      {
         autofire_delay[i][2] = options->autofire_speed;
         GB->button_pressed[B_START] = !GB->button_pressed[B_START];
      }
   }
   else
   if(KEYDOWN(buffer,options->multi_key_config[i][BUTTON_START]))
      GB->button_pressed[B_START] = 0;           
   else
   {
      GB->button_pressed[B_START] = 1;
      autofire_delay[i][2] = 0;
   }

   if(KEYDOWN(buffer,options->multi_key_config[i][BUTTON_TURBO_SELECT]))
   {
      if(--autofire_delay[i][3] <= 0)
      {
         autofire_delay[i][3] = options->autofire_speed;
         GB->button_pressed[B_SELECT] = !GB->button_pressed[B_SELECT];
      }
   }
   else
   if(KEYDOWN(buffer,options->multi_key_config[i][BUTTON_SELECT]))
      GB->button_pressed[B_SELECT] = 0;           
   else
   {
      GB->button_pressed[B_SELECT] = 1;
      autofire_delay[i][3] = 0;
   }
         
   if(KEYDOWN(buffer,options->special_keys[BUTTON_SENSOR_UP]))
      sensor_dir[SENSOR_UP] = 1;             
   else     
      sensor_dir[SENSOR_UP] = 0;   
      
   if(KEYDOWN(buffer,options->special_keys[BUTTON_SENSOR_DOWN]))
      sensor_dir[SENSOR_DOWN] = 1;             
   else               
      sensor_dir[SENSOR_DOWN] = 0;  
                   
   if(KEYDOWN(buffer,options->special_keys[BUTTON_SENSOR_LEFT]))
      sensor_dir[SENSOR_LEFT] = 1;             
   else             
      sensor_dir[SENSOR_LEFT] = 0;     
                
   if(KEYDOWN(buffer,options->special_keys[BUTTON_SENSOR_RIGHT]))
      sensor_dir[SENSOR_RIGHT] = 1;             
   else                                                                                  
      sensor_dir[SENSOR_RIGHT] = 0;

   return true;
}
