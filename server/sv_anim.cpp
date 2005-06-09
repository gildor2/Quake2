#include "server.h"

/*-----------------------------------------------------------------------------
	Animation support
-----------------------------------------------------------------------------*/

/* Quake2 animation frames:
	name		frames	num	loop stop simple	torso		legs
	---------------------------------------------------------------------------
	stand	 	0-39	40	+			+		STAND[2]	IDLE
	run			40-45	6	+			+		STAND[2]	** (WALK,BACKWALK,RUN,BACK)
	attack		46-53	8						ATTACK[2]	**
	pain1		54-57	4						(angle)		**
	pain2		58-61	4						(angle)		**
	pain3		62-65	4						(angle)		**
	jump		66-71	6						IDLE		** (JUMP,LAND, JUMPB, LANDB)
	flip		72-83	12						GESTURE		**
	salute		84-94	11						GESTURE		**
	taunt		95-111	17						GESTURE		**
	wave		112-122	11						GESTURE		**
	point		123-134	12						GESTURE		**
	crstnd		135-153	19	+			+		STAND[2]	IDLECR
	crwalk		154-159	6	+			+		STAND[2]	WALKCR
	crattack	160-168	9						ATTACK[2]	** (IDLECR,WALKCR+BACKCR)
	crpain		169-172	4						(angle)		----//----
	crdeath		173-177	5		  +		+		? (BOTH_DEATHn ?)
	death1		178-183	6		  +		+		BOTH_DEATH1->DEAD1
	death2		184-189	6		  +		+		BOTH_DEATH2->DEAD2
	death3		190-197	8		  +		+		BOTH_DEATH3->DEAD3
	---------------------------------------------------------------------------
	stop - stop on last frame (death animation)
	simple - basic animation, almost (or completely) idle; other (!simple) - may be combined with simple animation
	TORSO_STAND2, TORSO_ATTACK2 - for special weapons only (gauntlet)
	** - compute from movement
	(angle) - pain animation in Q3 made with torso movement
	JUMP,LAND - forward, JUMPB,LANDB - backward movement

	"crpain" and "pain3" (in std q2 game) may be used in reverse order to signal weapon dropping/changing
	"wave" may be used in reverse order for grenade throwing (1st 8 frames)
	"jump" used 3-6 frames; frame #2 - for "in air" state (static); strange code: game/p_view.c :: G_SetClientFrame()

	Unused in Q2, but present in Q3: LEGS_SWIM, LEGS_TURN, TORSO_DROP, TORSO_RAISE
*/

struct simpleAnim_t
{
	short first, last;
	short anim;
};


void SV_ComputeAnimation (player_state_t *ps, entityStateEx_t &ent, entityStateEx_t *oldent, edict_t *edict)
{
	int f = ent.frame;		// Quake2 animation frame
	int i;

	/*----------------------- Death animation -------------------------------*/

	static const simpleAnim_t deaths[] = {
		{173, 177, BOTH_DEATH1},	// crdeath
		{178, 183, BOTH_DEATH1},	// death1
		{184, 189, BOTH_DEATH2},	// death2
		{190, 197, BOTH_DEATH3}		// death3
	};

	for (i = 0; i < ARRAY_COUNT(deaths); i++)
	{
		const simpleAnim_t &p = deaths[i];
		if (f >= p.first && f <= p.last)
		{
			int n = p.anim;
			// allow DEATH animation to complete; if entity already not linked to player
			// (player respawned) - switch to corresponding DEAD animation
			if (ps == NULL) n++;	// BOTH_DEATHn -> BOTH_DEADn
			ent.SetAnim (n, n);
			return;
		}
	}

	// validate ps
	if (ps)
	{
		if (!(ps->pmove.origin[0] | ps->pmove.origin[1] | ps->pmove.origin[2] |
			ps->pmove.velocity[0] | ps->pmove.velocity[1] | ps->pmove.velocity[2]))
			ps = NULL;				// this may be with bots: zero player_state_t; needs to recompute
	}

	player_state_t calcPs;
#if 1
	if (ps == NULL)
#else
	// test ps-computation mode
	EXEC_ONCE(Com_WPrintf("TEST !!!!!!!!!!!!\n");)
#endif
	{
		/* try to re-create player_state_t
		 * here used following fields:
		 *	ps->pmove.velocity
		 *	ps->pmove.pm_flags & PMF_ON_GROUND/PMF_DUCKED
		 *	ps->viewangles
		 *	ps->rdflags & RDF_UNDERWATER
		 */
		memset (&calcPs, 0, sizeof(player_state_t));
		// detect crouch
		if (f >= 135 && f <= 172)		// crstnd/crwalk/crattack/crpain
			calcPs.pmove.pm_flags |= PMF_DUCKED;
		// detect on-ground
		CVec3 down = ent.origin;
		down[2] -= 0.25f;
		trace_t trace;
		// consts from pmove.cpp:
		static const CVec3 tMins = {-16, -16, -24};
		static const CVec3 tMaxs = { 16,  16,   4}; // as ducked
		SV_Trace (trace, ent.origin, down, tMins, tMaxs, edict, MASK_PLAYERSOLID);
		if (trace.fraction < 1.0f || trace.startsolid)
			calcPs.pmove.pm_flags |= PMF_ON_GROUND;
		// check underwater
		if (SV_PointContents (ent.origin) & MASK_WATER)
			calcPs.rdflags |= RDF_UNDERWATER;
		// viewangles will be same, as ent.angles ...
		calcPs.viewangles = ent.angles;
		if (calcPs.viewangles[PITCH] < -30 || calcPs.viewangles[PITCH] > 30)
			calcPs.viewangles[PITCH] = 0;
		// compute velocity
		if (oldent)
			for (int i = 0; i < 3; i++)
				calcPs.pmove.velocity[i] = appRound((ent.origin[i] - oldent->origin[i]) * 8 * 10);	// 8 - quant, 10 - sv_fps
		// change ps pointer
		ps = &calcPs;
	}

	/*------------------------ Legs animation -------------------------------*/

	CVec3 velHorz;
	velHorz[0] = ps->pmove.velocity[0] * 0.125f;
	velHorz[1] = ps->pmove.velocity[1] * 0.125f;
	velHorz[2] = 0;		// no vertical component

	CVec3 forward, right;
	AngleVectors (ps->viewangles, &forward, &right, NULL);
	float velForward = dot (forward, velHorz);
	float velRight   = dot (right, velHorz);
	float absVelHorz = SQRTFAST(velForward*velForward + velRight*velRight);

	// acquire previous frame info
	int prevLegs = ANIM_NOCHANGE, prevTorso = ANIM_NOCHANGE, prevAngle = LEGS_NEUTRAL, prevPitch = 0;
	if (oldent) oldent->GetAnim (prevLegs, prevTorso, prevAngle, prevPitch);

	//?? torso angles: note, that Q3 angles is for legs (torso rotated), but in Q2 angles for
	//??  torso (because legs are fixed) - legs rotated

	int legs = ANIM_NOCHANGE;

	if (!(ps->pmove.pm_flags & PMF_ON_GROUND))
	{
		if (ps->rdflags & RDF_UNDERWATER)
			legs = LEGS_SWIM;
		else
		{
			legs = (velForward > -20) ? LEGS_JUMP : LEGS_JUMPB;
			// but: when one of jump animations already started, do not change it
			if (prevLegs == LEGS_JUMP || prevLegs == LEGS_JUMPB)
				legs = prevLegs;
			// land animation: automatically launched by client, when JUMP -> IDLE/RUN/WALK
		}
	}
	else if (ps->pmove.pm_flags & PMF_DUCKED)
	{
		// duck
		if (absVelHorz < 20)
			legs = LEGS_IDLECR;
		else if (velForward > -10)
			legs = LEGS_WALKCR;
		else
			legs = LEGS_BACKCR;
	}
	else
	{
		// stand
		if (absVelHorz > 150)
		{
			// run
			legs = velForward > -10 ? LEGS_RUN : LEGS_BACK;
		}
		else if (absVelHorz > 20)
		{
			// walk
			legs = velForward > -20 ? LEGS_WALK : LEGS_BACKWALK;
		}
		else
			legs = LEGS_IDLE;
	}
	// choose legs angle
	int legsAngle = LEGS_NEUTRAL;
	if (legs != LEGS_IDLE && legs != LEGS_IDLECR && legs != LEGS_SWIM)
	{
		if (abs(velForward) < 20)
		{
			// side movement ?
			if (velRight > 20)
				legsAngle = LEGS_RIGHT_90;
			else if (velRight < -20)
				legsAngle = LEGS_LEFT_90;
		}
		else if (abs(velRight) > 20)
		{
			// |velRight| > 20, |velForward| > 20 -- +/- 45 degrees
			if (IsNegative (velRight) ^ IsNegative (velForward))
				legsAngle = LEGS_LEFT_45;
			else
				legsAngle = LEGS_RIGHT_45;
		}
	}

	/*----------------------- Torso animation -------------------------------*/

	int torso = TORSO_STAND;	//?? this animation should be on IDLE: some Q3 anims may be longer, than
								//?? Q2 analogs - should allow them to complete

#define A2(first,last,num,anim)		\
		{first, first+num-1, anim},	\
		{first+num, last, ANIM_NOCHANGE}
	static const simpleAnim_t torses[] = {
		// NOTE: we use 1st 3 frames to detect animation; other frames will left animation unchanged
		A2( 46,  53, 3, TORSO_ATTACK),	// attack
		A2(160, 168, 3, TORSO_ATTACK),	// crattack
		A2( 72,  83, 3, TORSO_GESTURE),	// flip
		A2( 84,  94, 3, TORSO_GESTURE),	// salute
		A2( 95, 111, 3, TORSO_GESTURE),	// taunt
		A2(112, 122, 3, TORSO_GESTURE),	// wave
		A2(123, 134, 3, TORSO_GESTURE)	// point
	};

	for (i = 0; i < ARRAY_COUNT(torses); i++)
	{
		const simpleAnim_t &p = torses[i];
		if (f >= p.first && f <= p.last)
		{
			torso = p.anim;
			break;
		}
	}

	// weapon changing animation
	// uses reversed crpain (172->169->172) / pain3 (65->62->65)
#if 0
	if (oldent && (
		(oldent->frame == 170 && f == 169) ||
		(oldent->frame == 63  && f == 62)))
#else
	if (oldent && ((ent.skinnum ^ oldent->skinnum) & 0xFF00))
#endif
		torso = TORSO_DROP;

	int pitch = bound(ps->viewangles[PITCH], -90, 90);

	//!! pain: 54-57,58-61,62-65; crpain: 169-172 -> angles (may be, mix rotate + lean angles?)

	// send
	ent.SetAnim (legs, torso, legsAngle, pitch);
}
