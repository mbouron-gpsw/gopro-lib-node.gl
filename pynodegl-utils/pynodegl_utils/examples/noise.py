import array
import math
import random

from pynodegl import (
        AnimKeyFrameBuffer,
        AnimKeyFrameFloat,
        AnimatedBufferVec2,
        AnimatedFloat,
        BufferFloat,
        BufferVec2,
        BufferVec3,
        Geometry,
        GraphicConfig,
        Group,
        Program,
        Quad,
        Render,
        Rotate,
        Texture2D,
        UniformFloat,
        UniformVec3,
        UniformVec4,
        UniformInt,
)

from pynodegl_utils.misc import scene, get_frag


@scene(ndim={'type': 'range', 'range': [1,8]},
       nb_layers={'type': 'range', 'range': [1,8]},
       lacunarity={'type': 'range', 'range': [0.01, 10], 'unit_base': 100},
       gain={'type': 'range', 'range': [0.01, 10], 'unit_base': 100})
def noise1d(cfg, ndim=4, nb_layers=6, lacunarity=2.0, gain=0.5):
    random.seed(0)
    random_dim = 1<<ndim
    cfg.aspect_ratio = (16, 9)

    def get_rand():
        return array.array('f', [random.uniform(0, 1) for x in range(random_dim)])

    random_buf = BufferFloat(data=get_rand())
    random_tex = Texture2D(data_src=random_buf, width=random_dim, height=1)

    quad = Quad((-1, -1, 0), (2, 0, 0), (0, 2, 0))
    prog = Program(fragment=get_frag('noise'))

    utime_animkf = [AnimKeyFrameFloat(0, 0),
                    AnimKeyFrameFloat(cfg.duration, 1)]
    utime = UniformFloat(anim=AnimatedFloat(utime_animkf))

    render = Render(quad, prog)
    render.update_textures(tex0=random_tex)
    render.update_uniforms(dim=UniformInt(random_dim))
    render.update_uniforms(nb_layers=UniformInt(nb_layers))
    render.update_uniforms(time=utime)
    render.update_uniforms(profile=UniformInt(1))
    render.update_uniforms(lacunarity=UniformFloat(lacunarity))
    render.update_uniforms(gain=UniformFloat(gain))

    return render


@scene(ndim={'type': 'range', 'range': [1,8]},
       nb_layers={'type': 'range', 'range': [1,8]},
       lacunarity={'type': 'range', 'range': [0.01, 10], 'unit_base': 100},
       gain={'type': 'range', 'range': [0.01, 10], 'unit_base': 100},
       ref_color={'type': 'color'},
       nb_mountains={'type': 'range', 'range': [1, 10]})
def mountain(cfg, ndim=3, nb_layers=6, lacunarity=2.0, gain=0.5,
             ref_color=(0.5,.75,.75,1.0), nb_mountains=6):
    random.seed(0)
    random_dim = 1<<ndim
    cfg.aspect_ratio = (16, 9)
    #cfg.duration = nb_mountains ** 2

    def get_rand():
        return array.array('f', [random.uniform(0, 1) for x in range(random_dim)])

    print random_dim

    quad = Quad((-1, -1, 0), (2, 0, 0), (0, 2, 0))
    prog = Program(fragment=get_frag('mountain'))

    hscale = 1/2.

    mountains = []
    for i in range(nb_mountains):

        yoffset = (nb_mountains-i-1)/float(nb_mountains-1) * (1.0 - hscale)

        #print 'hscale=%g yoffset=%g' % (hscale, yoffset)

        white = (1.0, 1.0, 1.0)
        black = (0.0, 0.0, 0.0)
        mix = lambda a, b, x: x*a + (1.0-x)*b

        if i < nb_mountains/2:
            c0, c1 = ref_color, white
            f = (i + 1) / float(nb_mountains/2 + 1)
        else:
            c0, c1 = black, ref_color
            f = (i - nb_mountains/2) / float((nb_mountains-1)/2)
        print '%d/%d: %f' % (i+1, nb_mountains, f)
        mcolor = [mix(a, b, f) for a, b in zip(c0, c1)]

        random_buf = BufferFloat(data=get_rand())
        random_tex = Texture2D(data_src=random_buf, width=random_dim, height=1)

        utime_animkf = [AnimKeyFrameFloat(0, 0),
                        AnimKeyFrameFloat(cfg.duration, 1)]
        utime = UniformFloat(anim=AnimatedFloat(utime_animkf))

        #uyoffset_animkf = [AnimKeyFrameFloat(0, yoffset/2.),
        #                   AnimKeyFrameFloat(cfg.duration/2.0, yoffset, 'exp_in_out'),
        #                   AnimKeyFrameFloat(cfg.duration, yoffset/2., 'exp_out_in'),
        #                   ]
        #uyoffset = UniformFloat(anim=AnimatedFloat(uyoffset_animkf))
        uyoffset = UniformFloat(value=yoffset)

        render = Render(quad, prog)
        render.update_textures(tex0=random_tex)
        render.update_uniforms(dim=UniformInt(random_dim))
        render.update_uniforms(nb_layers=UniformInt(nb_layers))
        render.update_uniforms(time=utime)
        render.update_uniforms(lacunarity=UniformFloat(lacunarity))
        render.update_uniforms(gain=UniformFloat(gain))
        render.update_uniforms(mcolor=UniformVec3(mcolor))
        render.update_uniforms(yoffset=uyoffset)
        render.update_uniforms(hscale=UniformFloat(hscale))

        lacunarity *= 1.02

        mountains.append(render)

    group = Group(children=mountains)
    blend = GraphicConfig(group,
                          blend=True,
                          blend_src_factor='src_alpha',
                          blend_dst_factor='one_minus_src_alpha',
                          blend_src_factor_a='zero',
                          blend_dst_factor_a='one')
    return blend


@scene(n={'type': 'range', 'range': [1,100000]},
       gauss={'type': 'bool'})
def sphere(cfg, n=4000, gauss=True):
    random.seed(0)

    cfg.duration = 15.
    cfg.aspect_ratio = (1, 1)

    quad = Quad((-1, -1, 0), (2, 0, 0), (0, 2, 0))
    prog = Program(fragment=get_frag('color'))

    scale = 1.3
    r = []
    for i in range(n):
        if gauss:
            vec = [random.gauss(0, 1) for x in range(3)]
        else:
            vec = [random.uniform(-1, 1) for x in range(3)]
        mag = math.sqrt(sum(x*x for x in vec)) * scale
        r += [x/mag for x in vec]

    vertices_data = array.array('f', r)
    vertices_buf = BufferVec3(data=vertices_data)
    geom = Geometry(vertices_buf)
    geom.set_draw_mode('points')

    render = Render(geom, prog)
    render.update_uniforms(color=UniformVec4(value=(1,1,1,1)))

    for i in range(3):
        rot_animkf = AnimatedFloat([AnimKeyFrameFloat(0, 0),
                                    AnimKeyFrameFloat(cfg.duration, 360 * (i + 1))])
        axis = [int(i == x) for x in range(3)]
        render = Rotate(render, axis=axis, anim=rot_animkf)

    return render

def _permuted_2d_gradients(n, pad=0, r=1.0):
    step = 2. * math.pi / float(n)
    angles = [i*step for i in range(n)]
    #random.shuffle(angles)
    grad = []
    for angle in angles:
        xy = [math.cos(angle)*r, math.sin(angle)*r]
        xy = [(x + 1.0) / 2.0 for x in xy] # prevent opengl from clipping automatically during picking
        grad += xy
        grad += [0] * pad
    return grad


@scene(n={'type': 'range', 'range': [1,200]})
def distrib2d(cfg, n=100):
    random.seed(0)

    prog = Program(fragment=get_frag('color'))

    vertices = [x*2.0-1.0 for x in _permuted_2d_gradients(n, pad=1, r=.8)]
    vertices_data = array.array('f', vertices)
    vertices_buf = BufferVec3(data=vertices_data)
    geom = Geometry(vertices_buf)
    geom.set_draw_mode("points")

    render = Render(geom, prog)
    render.update_uniforms(color=UniformVec4(value=(1,1,1,1)))

    return render



@scene(ndim={'type': 'range', 'range': [0,8]},
       nb_layers={'type': 'range', 'range': [1,8]},
       lacunarity={'type': 'range', 'range': [0.01, 10], 'unit_base': 100},
       gain={'type': 'range', 'range': [0.01, 10], 'unit_base': 100})
def noise2d(cfg, ndim=2, nb_layers=6, lacunarity=2.0, gain=0.5):
    random.seed(0)
    random_dim = (1<<ndim) + 1

    cfg.duration = 3.0
    cfg.aspect_ratio = (1, 1)

    nb_gradients = random_dim**2
    print 'nb_gradients', nb_gradients
    print 'dim', random_dim
    random_data = array.array('f', _permuted_2d_gradients(nb_gradients))
    random_buf = BufferVec2(data=random_data)
    random_tex = Texture2D(data_src=random_buf, width=random_dim, height=random_dim)

    quad = Quad((-1, 1, 0), (2, 0, 0), (0, -2, 0))
    prog = Program(fragment=get_frag('noise'))

    utime_animkf = [AnimKeyFrameFloat(0, 0),
                    AnimKeyFrameFloat(cfg.duration, 1)]
    utime = UniformFloat(anim=AnimatedFloat(utime_animkf))

    render = Render(quad, prog)
    render.update_textures(tex0=random_tex)
    render.update_uniforms(dim=UniformInt(random_dim))
    render.update_uniforms(nb_layers=UniformInt(nb_layers))
    render.update_uniforms(time=utime)
    render.update_uniforms(profile=UniformInt(2))
    render.update_uniforms(lacunarity=UniformFloat(lacunarity))
    render.update_uniforms(gain=UniformFloat(gain))

    return render

# uniform distribution of vectors of unit length 1
def _get_rand(nb, nb_comp=2, pad=0):
    r = []
    for i in range(nb):
        #vec = [random.gauss(0, 1) for x in range(nb_comp)]
        vec = [random.uniform(-1, 1) for x in range(nb_comp)]
        vec += [0] * pad
        mag = math.sqrt(sum(x*x for x in vec))
        r += [x/mag for x in vec]
    return r


@scene(ndim={'type': 'range', 'range': [1,8]},
       nb_layers={'type': 'range', 'range': [1,8]},
       lacunarity={'type': 'range', 'range': [0.01, 10], 'unit_base': 100},
       gain={'type': 'range', 'range': [0.01, 10], 'unit_base': 100})
def noise3d(cfg, ndim=4, nb_layers=6, lacunarity=2.0, gain=0.5):
    cfg.duration = 5.
    cfg.aspect_ratio = (1, 1)

    random.seed(0)
    random_dim = 1<<ndim

    nb_comp = 3

    def get_rand():
        return array.array('f', _get_rand(random_dim**2, nb_comp=3))

    random_buffer = BufferVec3(data=get_rand())
    random_tex = Texture2D(data_src=random_buffer, width=random_dim, height=random_dim)

    quad = Quad((-1, -1, 0), (2, 0, 0), (0, 2, 0))
    prog = Program(fragment=get_frag('noise'))

    utime_animkf = [AnimKeyFrameFloat(0, 0),
                    AnimKeyFrameFloat(cfg.duration/2.0, 1),
                    AnimKeyFrameFloat(cfg.duration, 0)]
    utime = UniformFloat(anim=AnimatedFloat(utime_animkf))

    render = Render(quad, prog)
    render.update_textures(tex0=random_tex)
    render.update_uniforms(dim=UniformInt(random_dim))
    render.update_uniforms(nb_layers=UniformInt(nb_layers))
    render.update_uniforms(time=utime)
    render.update_uniforms(profile=UniformInt(3))
    render.update_uniforms(lacunarity=UniformFloat(lacunarity))
    render.update_uniforms(gain=UniformFloat(gain))

    return render