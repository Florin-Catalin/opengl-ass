#!/bin/sh
# Regenerates the OBJ models converted from Sketchfab .glb files. Run from the repo root.
set -e
cd "$(dirname "$0")/.."

# Sandworm: without the small human figure bundled with it.
python3 -I tools/glb2obj.py objects/sandworm/dune_sandworm.glb objects/sandworm sandworm --skip Object_5
