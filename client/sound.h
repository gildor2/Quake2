/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

struct sfx_t;

void S_Init (void);
void S_Shutdown (void);

// if origin is NULL, the sound will be dynamically sourced from the entity
void S_StartSound (const CVec3 *origin, int entnum, int entchannel, sfx_t *sfx, float fvol,  float attenuation, float timeofs);
void S_StartLocalSound (const char *s);

void S_RawSamples (int samples, int rate, int width, int channels, byte *data);

void S_StopAllSounds_f (void);
void S_Update (const CVec3 &origin, const CVec3 &v_right);

void S_Activate (bool active);

void S_BeginRegistration (void);
sfx_t *S_RegisterSound (const char *sample);
void S_EndRegistration (void);
