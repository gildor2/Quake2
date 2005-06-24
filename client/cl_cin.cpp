//!! TODO: cin_t -> class

#include "client.h"

#define CIN_SPEED	14		// do not change: video format depends on in (number of sound samples)
							// to change video playback speed, edit RunCinematic() function (CIN_MSEC_PER_FRAME)

//?? remove?
typedef struct
{
	byte	*data;
	int		count;
} cblock_t;

typedef struct
{
	// sound info
	bool	restartSound;
	int		s_rate;
	int		s_width;
	int		s_channels;

	// video info
	int		width;
	int		height;
	int		frameSize;					// width*height
	unsigned palette[256];

	// file data
	QFILE	*file;						// if NULL, display pic
	char	imageName[MAX_OSPATH];		// used for "map image.pcx"

	int		frame;
	double	frameTime;					// == cls.realtime for current cinematic frame; float precision for correct
										// video/audio synchronization; why: S_RawSamples() will not restart sound, but sound
										// buffer will be filled every 1000/CIN_SPEED ~ 71.4 msecs; when round this value to
										// 71 or 72 will get desynchronization about 0.5/71 == 0.007 => ~0.42 sec per minute ...)

	// image to display
	byte	*buf[2];					// 2 buffers, size = width*height; 1 for current frame, 1 for next (precaching)
	byte	activeBuf;					// index of current frame buffer

	// fields for Huffman decoding
	int		*huffNodes;					// [256][256][2];
	int		numHuffNodes[256];
} cinematics_t;

static cinematics_t cin;


//?? helper function (should place to files.cpp)
static int ReadCinInt (void)
{
	int		tmp;
	FS_Read (&tmp, 4, cin.file);
	//?? may perform error checking here, but FS_Read() already throw FatalError() when cannot read file
	return LittleLong (tmp);
}


/*-----------------------------------------------------------------------------
	Decompression of .CIN files
-----------------------------------------------------------------------------*/

static int SmallestNode1 (int numhnodes, bool *h_used, int *h_count)
{
	int best = BIG_NUMBER;
	int bestnode = -1;
	for (int i = 0; i < numhnodes; i++)
	{
		if (h_used[i] || !h_count[i])
			continue;
		if (h_count[i] < best)
		{
			best = h_count[i];
			bestnode = i;
		}
	}

	if (bestnode >= 0)
		h_used[bestnode] = true;
	return bestnode;
}


// .CIN compression uses huffman encoding with dependency on previous byte
// This function will create 256 huffman tables (for each previous byte)
static void InitHuffTable (void)
{
	//?? if make cin dynamically allocated, can place huffNodes[] as static
	cin.huffNodes = new int [256*256*2];		// 128K integers
	memset (cin.huffNodes, 0, 256*256*2*sizeof(int));

	for (int prev = 0; prev < 256; prev++)
	{
		bool	h_used[512];
		int		h_count[512];
		memset (h_used, 0, sizeof(h_used));

		// read a row of counts and convert byte[]->int[]
		byte	counts[256];
		FS_Read (counts, sizeof(counts), cin.file);
		for (int j = 0; j < 256; j++)
			h_count[j] = counts[j];

		// build the nodes
		int *node = cin.huffNodes + prev*256*2;
		for (int numhnodes = 256; numhnodes < 511; numhnodes++, node += 2)
		{
			// pick two lowest counts
			node[0] = SmallestNode1 (numhnodes, h_used, h_count);
			if (node[0] == -1) break;	// no more
			node[1] = SmallestNode1 (numhnodes, h_used, h_count);
			if (node[1] == -1) break;	// no more
			h_count[numhnodes] = h_count[node[0]] + h_count[node[1]];
		}

		cin.numHuffNodes[prev] = numhnodes-1;
	}
}


static void HuffDecompress (cblock_t *in, cblock_t *out, byte *buffer, int count)
{
	byte *input = in->data;
	out->data = buffer;

#define NEXT_TABLE(prevByte)	\
	hnodes = cin.huffNodes + ((prevByte-1)*256*2); \
	nodenum = cin.numHuffNodes[prevByte];

	// hnodes -> huffman table for current decoding byte
	int		*hnodes, nodenum;
	// init decompression; assume previous byte is 0
	NEXT_TABLE(0);

	byte bit = 0;
	byte inbyte;
	while (true)
	{
		if (nodenum < 256)
		{
			// leaf - place output byte
			*buffer++ = nodenum;
			if (!--count) break;
			// prepare to decode next byte
			NEXT_TABLE(nodenum);
		}
		// acquire next data byte when needed
		if (!bit)
		{
			inbyte = *input++;
			bit = 8;
		}
		// analyze next bit
		nodenum = hnodes[nodenum*2 + (inbyte&1)];
		inbyte >>= 1;
		bit--;
	}

	int readCount = input - in->data;
	if (readCount != in->count && readCount != in->count + 1)
		Com_WPrintf ("Decompression overread by %d\n", readCount - in->count);

	out->count = buffer - out->data;
	return;
#undef NEXT_TABLE
}


/*-----------------------------------------------------------------------------
	Cinematic control
-----------------------------------------------------------------------------*/

static void FreeCinematic (void)
{
	if (cin.buf[0])		delete cin.buf[0];
	if (cin.buf[1])		delete cin.buf[1];
	if (cin.file)		FS_FCloseFile (cin.file);
	if (cin.huffNodes)	delete cin.huffNodes;
	if (cin.restartSound) CL_Snd_Restart_f ();
	memset (&cin, 0, sizeof(cin));

	cl.cinematicActive = false;
}


void SCR_StopCinematic ()
{
	// tell the server to advance to the next map / cinematic
	MSG_WriteByte (&cls.netchan.message, clc_stringcmd);
	MSG_WriteString (&cls.netchan.message, va("nextserver %d\n", cl.servercount));

	FreeCinematic ();
}


/*
==================
ReadNextFrame
==================
*/
static bool ReadNextFrame (byte *buffer)
{
	int		command;
	byte	samples[22050 * 4];
	byte	compressed[0x20000];
	int		size;
	byte	*pic;
	cblock_t in, huf1;
	int		start, end, count;

	guard(ReadNextFrame);

	// read the next frame
	command = ReadCinInt ();
	if (command == 2) return false;			// last frame marker

	if (command == 1)
	{
		byte	palette[256*3];
		// read palette
		FS_Read (palette, sizeof(palette), cin.file);
		// convert 3-byte RGB palette to 4-byte RGBA for renderer
		byte *src = palette;
		byte *dst = (byte*) cin.palette;
		for (int i = 0; i < 256; i++)
		{
			*dst++ = *src++;
			*dst++ = *src++;
			*dst++ = *src++;
			*dst++ = 255;
		}
	}

	// decompress the next frame
	size = ReadCinInt () - 4;
	if (size > sizeof(compressed) || size < 1)
		Com_DropError ("Bad compressed frame size: %d\n", size);
	// read and verify check decompressed frame size
	if (ReadCinInt() != cin.frameSize)
		Com_FatalError ("bad cinematic frame size");

	FS_Read (compressed, size, cin.file);

	// read sound
	start = cin.frame * cin.s_rate / CIN_SPEED;
	end = (cin.frame + 1) * cin.s_rate / CIN_SPEED;
	count = end - start;

	FS_Read (samples, count*cin.s_width*cin.s_channels, cin.file);
	S_RawSamples (count, cin.s_rate, cin.s_width, cin.s_channels, samples);

	in.data = compressed;
	in.count = size;

	HuffDecompress (&in, &huf1, buffer, cin.frameSize);
	pic = huf1.data;

	return true;

	unguard;
}


static void RunCinematic (void)
{
	guard(RunCinematic);

	if (cls.key_dest != key_game)
	{
		// pause if menu or console is up
		cin.frameTime = cls.realtime;
		// disable sound looping
		S_StopAllSounds_f ();
		return;
	}

#if 0
	// use timescale
	float CIN_MSEC_PER_FRAME = (timescale->value) ? 1000.0f/CIN_SPEED/timescale->value : 0x7FFFFFF;
#else
#define CIN_MSEC_PER_FRAME	(1000.0f/CIN_SPEED)
#endif

	if (!timedemo->integer)
	{
		int frameDelta = appFloor ((cls.realtime - cin.frameTime) / CIN_MSEC_PER_FRAME);
		if (frameDelta == 0) return;			// display previous frame
#if 1
		if (frameDelta < 0)
		{
			Com_DPrintf ("RunCinematic: fix %d\n", frameDelta);
			cin.frameTime = cls.realtime;
			return;
		}
#endif

		if (frameDelta >= 2)
		{
			Com_DPrintf ("RunCinematic: dropped %d frames\n", frameDelta - 1);
			cin.frameTime = cls.realtime;
		}
		cin.frameTime += CIN_MSEC_PER_FRAME;
	}
	else
		cin.frameTime = cls.realtime;

	cin.frame++;

	cin.activeBuf ^= 1;
	if (!ReadNextFrame (cin.buf[cin.activeBuf ^ 1]))
		SCR_StopCinematic ();

	unguard;
}


bool SCR_DrawCinematic (void)
{
	if (!cl.cinematicActive)
		return false;

	if (cin.imageName[0]) {
		// static image
		RE_DrawDetailedPic (0, 0, viddef.width, viddef.height, cin.imageName);
	} else if (cin.file) {
		// cinematic frame
		RE_DrawStretchRaw8 (0, 0, viddef.width, viddef.height, cin.width, cin.height, cin.buf[cin.activeBuf], cin.palette);
		RunCinematic ();
	} else
		return false;
	return true;
}


void SCR_PlayCinematic (char *filename)
{
	guard(SCR_PlayCinematic);

	// make sure CD isn't playing music
	CDAudio_Stop ();

	if (RE_GetCaps() & REF_CONSOLE_ONLY)
	{
		// no cinematic for text-only mode
		SCR_StopCinematic ();
		return;
	}

	FreeCinematic ();

	const char *ext = strchr (filename, '.');
	if (ext && strcmp (ext, ".cin"))
	{
		// not ".cin" extension - try static image
		appCopyFilename (cin.imageName, filename, sizeof(cin.imageName));
		cin.imageName[ext - filename] = 0;	// cut extension
		CBasicImage *img = RE_RegisterPic (cin.imageName);
		if (img)
		{
			cin.width = img->width;
			cin.height = img->height;
		}
		else
		{
			Com_WPrintf ("%s not found\n", filename);
			SCR_StopCinematic ();			// launch next server
			return;
		}
	}
	else
	{
		// .cin file
		cin.file = FS_FOpenFile (va("video/%s", filename));
		if (!cin.file)
		{
			Com_DPrintf ("Cinematic %s not found\n", filename);
			SCR_StopCinematic ();			// launch next server
			return;
		}

		cin.width = ReadCinInt ();
		cin.height = ReadCinInt ();
		cin.s_rate = ReadCinInt ();
		cin.s_width = ReadCinInt ();
		cin.s_channels = ReadCinInt ();

		cin.frameSize = cin.width * cin.height;
		cin.buf[0] = new byte [cin.frameSize];
		cin.buf[1] = new byte [cin.frameSize];
		cin.activeBuf = 0;

		InitHuffTable ();

		// switch to cinematic sound rate if it is higher then current one
		int oldKhz = Cvar_VariableInt ("s_khz");
		if (oldKhz < cin.s_rate / 1000)
		{
			cin.restartSound = true;
			Cvar_SetInteger ("s_khz", cin.s_rate / 1000);
			CL_Snd_Restart_f ();
			Cvar_SetInteger ("s_khz", oldKhz);
		}
		// prepare 2 display frames
		ReadNextFrame (cin.buf[0]);
		ReadNextFrame (cin.buf[1]);
		cin.frame = 0;
		cin.frameTime = cls.realtime;
	}

	SCR_EndLoadingPlaque (true);
	cls.state = ca_active;
	cl.cinematicActive = true;

	unguard;
}
