#!/usr/bin/env python3
"""Convert a .glb (glTF 2.0 binary) into OBJ + MTL that tiny_obj_loader / Model3D can load.

Usage: python3 -I tools/glb2obj.py input.glb out_dir name

- One OBJ object per glTF mesh primitive (Model3D turns each into a Mesh), with node transforms applied.
- Materials without a texture get a small solid-color PNG built from baseColorFactor, because
  the scene shader always samples diffuseTexture.
- Embedded textures are not handled (the models this was written for have none).
No third-party dependencies.
"""
import json, os, struct, sys, zlib

COMPONENT = {5120: 'b', 5121: 'B', 5122: 'h', 5123: 'H', 5125: 'I', 5126: 'f'}
NCOMP = {'SCALAR': 1, 'VEC2': 2, 'VEC3': 3, 'VEC4': 4}


def read_glb(path):
    data = open(path, 'rb').read()
    magic, version, _ = struct.unpack('<4sII', data[:12])
    if magic != b'glTF' or version != 2:
        sys.exit('not a glTF 2.0 binary')
    off, gltf, binary = 12, None, b''
    while off < len(data):
        length, kind = struct.unpack('<I4s', data[off:off + 8])
        chunk = data[off + 8:off + 8 + length]
        if kind == b'JSON':
            gltf = json.loads(chunk)
        elif kind == b'BIN\x00':
            binary = chunk
        off += 8 + length
    return gltf, binary


def accessor(gltf, binary, index):
    acc = gltf['accessors'][index]
    view = gltf['bufferViews'][acc['bufferView']]
    fmt = COMPONENT[acc['componentType']]
    n = NCOMP[acc['type']]
    size = struct.calcsize(fmt)
    stride = view.get('byteStride', size * n)
    base = view.get('byteOffset', 0) + acc.get('byteOffset', 0)
    out = []
    for i in range(acc['count']):
        vals = struct.unpack_from('<' + fmt * n, binary, base + i * stride)
        out.append(vals if n > 1 else vals[0])
    return out


def mat_mul(a, b):  # 4x4, column-major lists of 16
    return [sum(a[k * 4 + r] * b[c * 4 + k] for k in range(4)) for c in range(4) for r in range(4)]


def node_matrix(node):
    if 'matrix' in node:
        return node['matrix']
    t = node.get('translation', [0, 0, 0])
    x, y, z, w = node.get('rotation', [0, 0, 0, 1])
    s = node.get('scale', [1, 1, 1])
    r = [1 - 2 * (y * y + z * z), 2 * (x * y + z * w), 2 * (x * z - y * w), 0,
         2 * (x * y - z * w), 1 - 2 * (x * x + z * z), 2 * (y * z + x * w), 0,
         2 * (x * z + y * w), 2 * (y * z - x * w), 1 - 2 * (x * x + y * y), 0,
         0, 0, 0, 1]
    for c in range(3):
        for rr in range(3):
            r[c * 4 + rr] *= s[c]
    r[12:15] = t
    return r


def transform(m, v, w):
    return tuple(sum(m[c * 4 + r] * (v[c] if c < 3 else w) for c in range(4)) for r in range(3))


def write_png(path, rgb, size=4):
    raw = b''.join(b'\x00' + bytes(rgb) * size for _ in range(size))
    def chunk(tag, payload):
        return struct.pack('>I', len(payload)) + tag + payload + struct.pack('>I', zlib.crc32(tag + payload) & 0xffffffff)
    png = b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', size, size, 8, 2, 0, 0, 0))
    png += chunk(b'IDAT', zlib.compress(raw)) + chunk(b'IEND', b'')
    open(path, 'wb').write(png)


def main():
    src, out_dir, name = sys.argv[1:4]
    gltf, binary = read_glb(src)
    os.makedirs(out_dir, exist_ok=True)

    # materials -> MTL + solid-color textures (glTF colors are linear, PNGs are sRGB)
    mats = gltf.get('materials', [])
    with open(os.path.join(out_dir, name + '.mtl'), 'w') as mtl:
        for i, m in enumerate(mats):
            color = m.get('pbrMetallicRoughness', {}).get('baseColorFactor', [1, 1, 1, 1])[:3]
            srgb = [round(255 * max(0.0, min(1.0, c)) ** (1 / 2.2)) for c in color]
            tex = '%s_mat%d.png' % (name, i)
            write_png(os.path.join(out_dir, tex), srgb)
            mtl.write('newmtl %s\nKa 0 0 0\nKd %.4f %.4f %.4f\nKs 0 0 0\nNs 1\nmap_Kd %s\n\n'
                      % (m.get('name', 'mat%d' % i), color[0], color[1], color[2], tex))

    lines = ['mtllib %s.mtl' % name]
    base = 1

    def visit(node_index, parent):
        nonlocal base
        node = gltf['nodes'][node_index]
        m = mat_mul(parent, node_matrix(node))
        if 'mesh' in node:
            mesh = gltf['meshes'][node['mesh']]
            for p, prim in enumerate(mesh['primitives']):
                if prim.get('mode', 4) != 4:
                    continue
                a = prim['attributes']
                pos = accessor(gltf, binary, a['POSITION'])
                nrm = accessor(gltf, binary, a['NORMAL']) if 'NORMAL' in a else None
                uv = accessor(gltf, binary, a['TEXCOORD_0']) if 'TEXCOORD_0' in a else None
                idx = accessor(gltf, binary, prim['indices']) if 'indices' in prim else list(range(len(pos)))
                lines.append('o %s_%d' % (mesh.get('name', 'mesh'), p))
                for v in pos:
                    lines.append('v %.6f %.6f %.6f' % transform(m, v, 1))
                for n in nrm or []:
                    t = transform(m, n, 0)
                    l = sum(c * c for c in t) ** 0.5 or 1
                    lines.append('vn %.6f %.6f %.6f' % tuple(c / l for c in t))
                for t in uv or []:
                    lines.append('vt %.6f %.6f' % (t[0], 1 - t[1]))  # glTF V runs top-down
                if 'material' in prim:
                    lines.append('usemtl %s' % mats[prim['material']].get('name', 'mat%d' % prim['material']))
                for i in range(0, len(idx), 3):
                    face = []
                    for k in idx[i:i + 3]:
                        k += base
                        face.append('%d/%s/%s' % (k, k if uv else '', k if nrm else ''))
                    lines.append('f ' + ' '.join(face))
                base += len(pos)
        for child in node.get('children', []):
            visit(child, m)

    identity = [1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1]
    for root in gltf['scenes'][gltf.get('scene', 0)]['nodes']:
        visit(root, identity)
    open(os.path.join(out_dir, name + '.obj'), 'w').write('\n'.join(lines) + '\n')
    print('wrote %s/%s.obj (%d vertices)' % (out_dir, name, base - 1))


if __name__ == '__main__':
    main()
