1. Shader keywords
~~~~~~~~~~~~~~~~~~

  1) qer*		-- ignore
  2) q3map_sun r g b intens deg elev   (all - float)
     sunVector = {cos(elev)*cos(deg), cos(elev)*sin(deg), sin(elev)}
  3) deformVertexes:
     - projectionShadow
     - autosprite
     - autosprite2
     - text0 .. text7   (unused in Q3)
     - bulge <width> <height> <speed>   (all - float)
     - wave <div> <func> <base> <amp> <phase> <freq>
       if <div> == 0 --> error + div = 100
     - normal <amp> <freq>
     - move <x> <y> <z> <func> <base> <amp> <phase> <freq>
  4) tessSize - ignore
  5) clampTime <value>
  6) q3map*		-- ignore (note: check "q3map_surfacelight" and other "q3map_*light*")
  7) surfaceParm	-- ignore (?)
     - contents: slime, lava, playerclip, monsterclip, nodrop, origin, translucent,
       detail, structural, areaportal, clusterportal, donotenter, fog, sky, 
     - surf: nonsolid, lightfilter, alphashadow, hint, slick, noimpact, nomarks, ladder,
       nodamage, metalsteps, flesh, nosteps, nodraw, pointlight, nolightmap, nodlight, dust
  8) nomipmaps
  9) nopicmip
  10) polygonOffset
  11) entityMergable
  12) fogParms ( <r> <g> <b> ) <dist_to_opaque>
  13) portal		-- set SORT_PORTAL
  14) skyparms <farbox|-> <cloud height> <nearbox|->
  15) light		-- ignore
  16) cull
     - node, twosided, disable
     - back, backside, backsided
     (default is front)
  17) sort
     - portal		== 1
     - sky		== 2
     - opaque		== 3
     - decal		== 4
     - seeThrough	== 5
     - banner		== 6
     - underwater	== 8
     - additive		== 10
     - nearest		== 16
     - <float value>
  -------------------- FAKK2: --------------------
   17) sort (Alice, may be FAKK2 too ...)
     - portal           == 1
     - sky              == 3
     - opaque           == 4
     - decal            == 5
     - seeThrough       == 6
     - banner           == 7
     - underwater       == 9
     - additive         == 11
     - nearest          == 17
   18) if <0|1|mtex|no_mtex> / endif
   19) fogonly  (required for fog ??)
   20) light <value>    -- lightflare for surface
   21) force32bit
   22) portalsky
   23) spriteGen <parallel|parallel_oriented|parallel_upright|oriented>
   24) spriteScale <value>


2. Stage keywords
~~~~~~~~~~~~~~~~~

  Init: depthwrite == on, blend == none, alpha == none

  1) map
     - <texture>
     - $whiteimage
     - $lightmap
  2) clampmap <texture>
  3) animMap <texture1> [... <texture8>]
  4) videoMap <cin file>
  5) alphaFunc  (default -- disabled)
     - GT0
     - LT128
     - GE128
  6) depthfunc
     - lequal  (default, == 0)
     - equal
  7) detail
  8) blendfunc
     - add     (== GL_ONE, GL_ONE == src+dst)
     - filter  (== GL_DST_COLOR, GL_ZERO == src*dst)
     - blend   (== GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)
     - GL_XXX GL_XXX
     if blendfunc specified, default for depthwrite == off (use "depthwrite" keyword to override)
  9) rgbGen
     - wave <func> <base> <amp> <phase> <freq>
     - const|constant ( <r> <g> <b> )      (all - float, 0..1)
     - identity
     - identityLighting
     - entity
     - oneMinusEntity
     - vertex
     - oneMinusVertex
     - exactVertex
     - lightingDiffuse
  10) alphaGen
     - wave <func> <base> <amp> <phase> <freq>
     - const <value>              (value - float, 0..1)
     - identity
     - entity
     - oneMinusEntity
     - vertex
     - oneMinusVertex
     - lightingSpecular
     - portal <range>
       if <range> is not specified, error + <range> = 256
  11) texgen, tcGen
     - texture, base
     - lightmap
     - environment
     - vector ( <v1.x> <v1.y> <v1.z> ) ( <v2.x> <v2.y> <v2.z> )
  12) tcMod
     - turb <base> <amp> <phase> <freq>
     - scale <s_scale> <t_scale>
     - scroll <s_speed> <t_speed>
     - stretch <func> <base> <amp> <phase> <freq>
     - transform <m00> <m01> <m10> <m11> <t0> <t1>
     - rotate <deg_per_sec>
     - entityTranslate
  13) depthwrite
  -------------------- FAKK2: --------------------
  9)  rgbGen: when applied to entity, use "vertex" for lightgrid or "lightingDiffuse" for compute
     - colorwave <r> <g> <b> <func> <base> <amp> <phase> <freq>
  10) alphaGen
     - dot [min] [max]            (dot(viewDir, surfNormal); a=0 when parallel, 1 when perpendicular)
       (water, reflections)
       def: min = 0; max = 1
       expression: dst_alpha = dot(NormalVec(vieworg-vec), normal) * (max-min) + min  (if dot() < 0 dot = 0)
     - oneMinusDot
     - lightingSpecular <x y z>   ???
     - skyalpha
     - oneMinusSkyAlpha
  12) tcMod
     - offset <sOffset> <tOffset> (may be treated as initial values for "scroll")
     - parallax <sRate> <tRate>   (coords depends on vieworg ???)
     - macro <sScale> <tScale>    ???
  14) nextbundle                  (for multitexturing)


3. Wave functions
~~~~~~~~~~~~~~~~~

  1) sin
  2) square
  3) triangle
  4) sawtooth
  5) inversesawtooth
  6)* noise     (only for "deformVertexes normal" and "rgbGen [color]wave noise")

  Result = amp * (func(TIME * freq) + phase) + base

4. FAKK2 shader script manipulation
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

- tcMod scroll <fromEntity> <fromEntity>
- tcMod rotate <fromEntity>
- tcMod offset <fromEntity> <fromEntity>
- deformVertexes wave <div> <func> <fromEntity> <fromEntity> <fromEntity> <fromEntity>
- alphaGen wave <func> <fromEntity> <fromEntity> <fromEntity> <fromEntity>
- rgbGen wave <func> <fromEntity> <fromEntity> <fromEntity> <fromEntity>
- frameFromEntity                 (for animmap)

Script commands:

- shader offset [x] [y]
- shader rotation [degrees]
- shader translation [x] [y]
- shader frame [framenum]
- shader wavebase [base offset of wave function]
- shader waveamp [amplitude of wave function]
- shader wavephase [phase of wave function]
- shader wavefreq [frequency of wave function]


-----------------------------------------------

Alice:
- animMapOnce
- rgbGen:
  - global == globalColor
  - fromclient ( == vertex)
- alphaGen:
  - global == globalAlpha
  - fromentity ( == entity)
  - fromclient ( == vertex)
- deformVertexes
  - wavenormal <div> <f> <v1> <v2> <v3> <func> <base> <amp> <phase> <freq>
