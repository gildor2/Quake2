
void CL_AddEffects();
void CL_ClearEffects();


//---------------- particles ---------------------

particle_t *CL_AllocParticle();
beam_t *CL_AllocParticleBeam(const CVec3 &start, const CVec3 &end, float radius, float fadeTime);
void CL_MetalSparks(const CVec3 &pos, const CVec3 &dir, int count);

extern particle_t *active_particles;
extern beam_t	*active_beams;

#define	PARTICLE_GRAVITY			80
#define INSTANT_PARTICLE			-10000.0	//??

//--------------- lightstyles --------------------

void CL_ClearLightStyles();
void CL_SetLightstyle(int i, const char *s);
extern lightstyle_t cl_lightstyles[MAX_LIGHTSTYLES];


//----------------- dlights ----------------------

struct cdlight_t
{
	int		key;				// so entities can reuse same entry
	CVec3	color;
	CVec3	origin;
	float	radius;
	float	die;				// stop lighting after this time
};

cdlight_t *CL_AllocDlight(int key, const CVec3 &origin);


//---------------- network -----------------------

void CL_ParseMuzzleFlash();
void CL_ParseMuzzleFlash2();
void CL_EntityEvent(clEntityState_t *ent);


/*-----------------------------------------------------------------------------
	Effects
-----------------------------------------------------------------------------*/

// simple static effects
void CL_ParticleEffect(const CVec3 &org, const CVec3 &dir, int color, int count);
void CL_ParticleEffect2(const CVec3 &org, const CVec3 &dir, int color, int count);
void CL_ParticleEffect3(const CVec3 &org, const CVec3 &dir, int color, int count);
void CL_TeleporterParticles(clEntityState_t *ent);
void CL_LogoutEffect(const CVec3 &org, int type);
void CL_ItemRespawnParticles(const CVec3 &org);
void CL_ExplosionParticles(const CVec3 &org);
void CL_BFGExplosionParticles(const CVec3 &org);
void CL_TrackerExplosionParticles(const CVec3 &org);
void CL_BlasterParticles(const CVec3 &org, const CVec3 &dir, unsigned color);

// simple trails
void CL_BlasterTrail(centity_t &ent);
void CL_BlasterTrail2(centity_t &ent);
void CL_FlagTrail(centity_t &ent, int color);
void CL_RocketTrail(centity_t &ent);
void CL_BubbleTrail(const CVec3 &start, const CVec3 &end);
void CL_BubbleTrail2(const CVec3 &start, const CVec3 &end);
void CL_TagTrail(centity_t &ent);
void CL_DebugTrail(const CVec3 &start, const CVec3 &end);
void CL_ForceWall(const CVec3 &start, const CVec3 &end, int color);

// complex effects
void CL_BigTeleportParticles(const CVec3 &org);
void CL_BfgParticles(const CVec3 &origin);
void CL_FlyEffect(centity_t &ent);
void CL_DiminishingTrail(centity_t &ent, int flags);
void CL_RailTrail(const CVec3 &start, const CVec3 &end);
void CL_RailTrailExt(const CVec3 &start, const CVec3 &end, byte rType, byte rColor);
void CL_IonripperTrail(centity_t &ent);
void CL_TrapParticles(const CVec3 &origin);
void CL_TeleportParticles(const CVec3 &org);
void CL_Heatbeam(const CVec3 &start, const CVec3 &forward);
void CL_ParticleSteamEffect(const CVec3 &org, const CVec3 &dir, int color, int count, int magnitude);
void CL_ParticleSmokeEffect(const CVec3 &org, const CVec3 &dir);
void CL_TrackerTrail(centity_t &ent);
void CL_TrackerShell(const CVec3 &origin);
void CL_MonsterPlasmaShell(const CVec3 &origin);
void CL_WidowSplash(const CVec3 &org);
