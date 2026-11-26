#!/bin/sh
# Regenerates the OBJ models converted from Sketchfab .glb files. Run from the repo root.
set -e
cd "$(dirname "$0")/.."

# Sandworm: without the small human figure bundled with it.
python3 -I tools/glb2obj.py objects/sandworm/dune_sandworm.glb objects/sandworm sandworm --skip Object_5

# Camel: one solid-colour mesh.
python3 -I tools/glb2obj.py objects/camel/pink_camel.glb objects/camel camel

# Rooster: one textured mesh.
python3 -I tools/glb2obj.py objects/rooster/cock_chicken_gun.glb objects/rooster rooster

# Dragon: flies in the sky, so the seven dragon balls lying at its feet are left out.
python3 -I tools/glb2obj.py objects/dragon/shenron_dragon_ball.glb objects/dragon dragon \
	--skip "One Star_02 - Default_0" --skip "One Star_03 - Default_0" \
	--skip "Two Star_02 - Default_0" --skip "Two Star_03 - Default_0" \
	--skip "Star Three_02 - Default_0" --skip "Star Three_03 - Default_0" \
	--skip "Four Star_02 - Default_0" --skip "Four Star_03 - Default_0" \
	--skip "Five Star_02 - Default_0" --skip "Five Star_03 - Default_0" \
	--skip "Six Star_02 - Default_0" --skip "Six Star_03 - Default_0" \
	--skip "Seven Star_02 - Default_0" --skip "Seven Star_03 - Default_0"

# Black hole: the flat accretion disc keeps its glowing (emissive) texture; everything else
# (the core and the shells around it) is drawn solid black, so it needs no textures.
BH=objects/blackhole/blackhole.glb
DISC=Blackhole_ring_Blackhole_ring_0
HOLE="Blackhole_core_Blackhole_core_0 Blackhole_skin_001_Blackhole_skin_0 Blackhole_skin_002_Blackhole_core_0
      Blackhole_skin_003_Blackhole_skin_0 Blackhole_skin_004_Blackhole_core_0 Blackhole_skin_005_Blackhole_skin_0
      Blackhole_skin_006_Blackhole_skin_inner_0 Blackhole_skin_007_Blackhole_core_0
      Blackhole_core001_Blackhole_core_0 Blackhole_skin_008_Blackhole_skin_inner_0
      Blackhole_skin_009_Blackhole_skin_0 Blackhole_skin_010_Blackhole_skin_0 Blackhole_skin_011_Blackhole_core_0
      Blackhole_skin_012_Blackhole_skin_inner_0 Blackhole_skin_013_Blackhole_ring2_0
      Blackhole_core002_Blackhole_ring2_0"
skips() { for n in "$@"; do printf -- '--skip %s ' "$n"; done; }
python3 -I tools/glb2obj.py $BH objects/blackhole blackhole_hole --no-textures --skip $DISC
python3 -I tools/glb2obj.py $BH objects/blackhole blackhole_disc --emissive $(skips $HOLE)
