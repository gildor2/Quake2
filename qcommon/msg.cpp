#include "qcommon.h"


void sizebuf_t::Init (void *data, int length)
{
	memset (this, 0, sizeof(sizebuf_t));
	this->data = (byte*)data;
	this->maxsize = length;
}


void sizebuf_t::Write (const void *data, int length)
{
	int need = cursize + length;
	if (need > maxsize)
	{
		if (!allowoverflow)
			appError ("MSG_Write: overflow without allowoverflow (size is %d)", maxsize);

		if (length > maxsize)
			appError ("MSG_Write: %d is > full buffer size", length);

		appPrintf ("MSG_Write: overflow (max=%d, need=%d)\n", maxsize, need);
		Clear ();
		overflowed = true;
	}

	void *dst = this->data + cursize;
	cursize = need;

	memcpy (dst, data, length);
}


/*-----------------------------------------------------------------------------
	Writing functions
-----------------------------------------------------------------------------*/

void MSG_WriteChar (sizebuf_t *sb, int c)
{
	sb->Write (&c, 1);
}

void MSG_WriteByte (sizebuf_t *sb, int c)
{
	sb->Write (&c, 1);
}

void MSG_WriteShort (sizebuf_t *sb, int c)
{
	LTL(c);
	sb->Write (&c, 2);
}

void MSG_WriteLong (sizebuf_t *sb, int c)
{
	LTL(c);
	sb->Write (&c, 4);
}

void MSG_WriteFloat (sizebuf_t *sb, float f)
{
	LTL(f);
	sb->Write (&f, 4);
}

void MSG_WriteString (sizebuf_t *sb, const char *s)
{
	if (!s)	sb->Write ("", 1);
	else	sb->Write (s, strlen(s)+1);
}

void MSG_WritePos (sizebuf_t *sb, const CVec3 &pos)
{
	MSG_WriteShort (sb, appRound (pos[0]*8));
	MSG_WriteShort (sb, appRound (pos[1]*8));
	MSG_WriteShort (sb, appRound (pos[2]*8));
}

void MSG_WriteAngle (sizebuf_t *sb, float f)
{
	MSG_WriteByte (sb, appRound (f*256.0f/360) & 255);
}

void MSG_WriteAngle16 (sizebuf_t *sb, float f)
{
	MSG_WriteShort (sb, ANGLE2SHORT(f));
}


#if 0

static byte dirTable[3*8*8*8];		// value=0 -- uninitialized, else idx+1
	// 3*8 - cube: 6 planes, each subdivided to 4 squares (x<0,x>0,y<0,y>0)
	// 8*8 - coordinates inside square

static int GetDirCell (CVec3 &dir)
{
	CVec3	adir;
	float	m;
	int		sign, base, base2;

	adir[0] = fabs (dir[0]);
	adir[1] = fabs (dir[1]);
	adir[2] = fabs (dir[2]);
	sign = 0;
	if (dir[0] < 0) sign |= 1;
	if (dir[1] < 0) sign |= 2;
	if (dir[2] < 0) sign |= 4;

	if (adir[0] < adir[1])
	{
		base = 1;
		m = adir[1];
	}
	else
	{
		base = 0;
		m = adir[0];
	}
	if (m < adir[2])
	{
		base = 2;
		m = adir[2];
	}

	m = 8.0f / m;
	switch (base)
	{
	case 0:
		base2 = (appFloor (adir[1] * m) << 3) + appFloor (adir[2] * m);
		break;
	case 1:
		base2 = (appFloor (adir[0] * m) << 3) + appFloor (adir[2] * m);
		break;
	case 2:
		base2 = (appFloor (adir[0] * m) << 3) + appFloor (adir[1] * m);
		break;
	}
	return (base << 9) + (sign << 6) + base2;
}

void MSG_WriteDir (sizebuf_t *sb, const CVec3 &dir)
{
	int		cell, i, best;
	float	d, bestd;

	// find cache cell
	cell = GetDirCell (dir);
	if (dirTable[cell])	// already computed
	{
		MSG_WriteByte (sb, dirTable[cell] - 1);
		appPrintf("*");
		return;
	}
	// compute index
	bestd = best = 0;
	for (i = 0; i < NUMVERTEXNORMALS; i++)
	{
		d = dot (dir, bytedirs[i]);
		if (d > bestd)
		{
			bestd = d;
			best = i;
		}
	}
	// cache index
	dirTable[cell] = best+1;
	// write
	MSG_WriteByte (sb, best);
}

#else

void MSG_WriteDir (sizebuf_t *sb, const CVec3 &dir)
{
	float bestd = 0;
	int best = 0;
	for (int i = 0; i < NUMVERTEXNORMALS; i++)
	{
		float d = dot (dir, bytedirs[i]);
		if (d >= 0.99f)
		{
			best = i;
			break;
		}
		// remember nearest value
		if (d > bestd)
		{
			bestd = d;
			best = i;
		}
	}
	MSG_WriteByte (sb, best);
}

#endif


void MSG_ReadDir (sizebuf_t *sb, CVec3 &dir)
{
	int b = MSG_ReadByte (sb);
	if (b >= NUMVERTEXNORMALS)
		Com_DropError ("MSG_ReadDir: out of range");
	dir = bytedirs[b];
}


/*-----------------------------------------------------------------------------
	Reading functions
-----------------------------------------------------------------------------*/

// returns -1 if no more characters are available
int MSG_ReadChar (sizebuf_t *msg_read)
{
	int	c;

	if (msg_read->readcount+1 > msg_read->cursize)
		c = -1;
	else
		c = (signed char)msg_read->data[msg_read->readcount];

	msg_read->readcount++;
	return c;
}

int MSG_ReadByte (sizebuf_t *msg_read)
{
	if (msg_read->readcount+1 > msg_read->cursize)
	{
		msg_read->readcount++;
		return -1;
	}
	else
	{
		byte c = (byte)msg_read->data[msg_read->readcount];
		msg_read->readcount++;
		return c;
	}
}

int MSG_ReadShort (sizebuf_t *msg_read)
{
	if (msg_read->readcount+2 > msg_read->cursize)
	{
		msg_read->readcount += 2;
		return -1;
	}
	else
	{
		short c = *((short*)&msg_read->data[msg_read->readcount]);
		msg_read->readcount += 2;
		LTL(c);
		return c;
	}
}

int MSG_ReadLong (sizebuf_t *msg_read)
{
	if (msg_read->readcount+4 > msg_read->cursize)
	{
		msg_read->readcount += 4;
		return -1;
	}
	else
	{
		int	c = *((int*)&msg_read->data[msg_read->readcount]);
		msg_read->readcount += 4;
		LTL(c);
		return c;
	}
}

float MSG_ReadFloat (sizebuf_t *msg_read)
{
	if (msg_read->readcount+4 > msg_read->cursize)
	{
		msg_read->readcount += 4;
		return -1;
	}
	else
	{
		float f = *(float*)&msg_read->data[msg_read->readcount];
		msg_read->readcount += 4;
		LTL(f);
		return f;
	}
}

char *MSG_ReadString (sizebuf_t *msg_read)
{
	static char	string[2048];		//?? make non-static

	int l = 0;
	do
	{
		int c = MSG_ReadChar (msg_read);
		if (c == -1 || c == 0)
			break;
		string[l++] = c;
	} while (l < sizeof(string)-1);
	string[l] = 0;

	return string;
}

void MSG_ReadPos (sizebuf_t *msg_read, CVec3 &pos)
{
	pos[0] = MSG_ReadShort(msg_read) * (1.0f/8);
	pos[1] = MSG_ReadShort(msg_read) * (1.0f/8);
	pos[2] = MSG_ReadShort(msg_read) * (1.0f/8);
}

float MSG_ReadAngle (sizebuf_t *msg_read)
{
	return MSG_ReadChar(msg_read) * (360.0f/256);
}

float MSG_ReadAngle16 (sizebuf_t *msg_read)
{
	return SHORT2ANGLE(MSG_ReadShort(msg_read));
}

void MSG_ReadData (sizebuf_t *msg_read, void *data, int len)
{
	for (int i = 0; i < len; i++)
		((byte *)data)[i] = MSG_ReadByte (msg_read);
}
