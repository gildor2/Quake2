
//#define BRUSH_PROFILE		1

#if MAX_DEBUG
#undef BRUSH_PROFILE
#define BRUSH_PROFILE		1
#endif


#if BRUSH_PROFILE
#define BRUSH_STAT(x)		x
#else
#define BRUSH_STAT(x)
#endif


struct CBrushVert
{
	CVec3	*v;
	CBrushVert *next;		// inside side
};


struct CBrushSide
{
	CBrushSide *next;		// inside brush
	// plane normal looks outside the brush volume
	CPlane *plane;
	CBrushVert *verts;		// all verts should belongs to `plane'
	bool	back;			// when "true", uses inverted `plane'
};


struct CBrush
{
	CBrushSide *sides;
	unsigned contents;		//!! should be set outside
	CBrush ()
	{}
	CBrush (const CBox &Bounds);
	CBrush *Split (CPlane *plane);
	void GetBounds (CBox &bounds);
	void AddBevels (CPlane* (*GetPlane)(const CVec3&, float));
	// memory pool for all split operations
	static CMemoryChain *mem;
#if 1
	void Dump ()
	{
		for (CBrushSide *s = sides; s; s = s->next)
		{
			appPrintf ("plane: n={%g %g %g} d=%g%s\n", VECTOR_ARG(s->plane->normal),
				s->plane->dist, s->back ? " (back)" : "");
			for (CBrushVert *v = s->verts; v; v = v->next)
				appPrintf ("  v={%g %g %g}\n", VECTOR_ARG((*v->v)));
		}
	}
#endif
};
