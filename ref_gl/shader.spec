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
  6) q3map*		-- ignore
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
     - filter  (== GL_DST_COLOR, GL_DST_ZERO == src*dst)
     - blend   (== GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)
     - GL_XXX GL_XXX
     if blendfunc specified, default for depthwrite == off (use "depthwrite" keyword to override)
  9) rgbGen
     - wave <func> <base> <amp> <phase> <freq>
     - const ( <r> <g> <b> )      (all - float, 0..1)
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


3. Wave functions
~~~~~~~~~~~~~~~~~

  1) sin
  2) square
  3) triangle
  4) sawtooth
  5) inversesawtooth
  6)* noise     (only for "deformVertexes normal" ??)

  Result = amp * (func(TIME * freq) + phase) + base
