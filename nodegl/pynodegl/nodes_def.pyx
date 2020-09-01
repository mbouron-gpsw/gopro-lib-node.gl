from libc.stdlib cimport free
from libc.stdint cimport uintptr_t
from cpython cimport array
cdef extern from "nodegl.h":
    cdef int NGL_NODE_ANIMATEDBUFFERFLOAT
    cdef int NGL_NODE_ANIMATEDBUFFERVEC2
    cdef int NGL_NODE_ANIMATEDBUFFERVEC3
    cdef int NGL_NODE_ANIMATEDBUFFERVEC4
    cdef int NGL_NODE_ANIMATEDTIME
    cdef int NGL_NODE_ANIMATEDFLOAT
    cdef int NGL_NODE_ANIMATEDVEC2
    cdef int NGL_NODE_ANIMATEDVEC3
    cdef int NGL_NODE_ANIMATEDVEC4
    cdef int NGL_NODE_ANIMATEDQUAT
    cdef int NGL_NODE_ANIMKEYFRAMEFLOAT
    cdef int NGL_NODE_ANIMKEYFRAMEVEC2
    cdef int NGL_NODE_ANIMKEYFRAMEVEC3
    cdef int NGL_NODE_ANIMKEYFRAMEVEC4
    cdef int NGL_NODE_ANIMKEYFRAMEQUAT
    cdef int NGL_NODE_ANIMKEYFRAMEBUFFER
    cdef int NGL_NODE_BLOCK
    cdef int NGL_NODE_BUFFERBYTE
    cdef int NGL_NODE_BUFFERBVEC2
    cdef int NGL_NODE_BUFFERBVEC3
    cdef int NGL_NODE_BUFFERBVEC4
    cdef int NGL_NODE_BUFFERINT
    cdef int NGL_NODE_BUFFERINT64
    cdef int NGL_NODE_BUFFERIVEC2
    cdef int NGL_NODE_BUFFERIVEC3
    cdef int NGL_NODE_BUFFERIVEC4
    cdef int NGL_NODE_BUFFERSHORT
    cdef int NGL_NODE_BUFFERSVEC2
    cdef int NGL_NODE_BUFFERSVEC3
    cdef int NGL_NODE_BUFFERSVEC4
    cdef int NGL_NODE_BUFFERUBYTE
    cdef int NGL_NODE_BUFFERUBVEC2
    cdef int NGL_NODE_BUFFERUBVEC3
    cdef int NGL_NODE_BUFFERUBVEC4
    cdef int NGL_NODE_BUFFERUINT
    cdef int NGL_NODE_BUFFERUIVEC2
    cdef int NGL_NODE_BUFFERUIVEC3
    cdef int NGL_NODE_BUFFERUIVEC4
    cdef int NGL_NODE_BUFFERUSHORT
    cdef int NGL_NODE_BUFFERUSVEC2
    cdef int NGL_NODE_BUFFERUSVEC3
    cdef int NGL_NODE_BUFFERUSVEC4
    cdef int NGL_NODE_BUFFERFLOAT
    cdef int NGL_NODE_BUFFERVEC2
    cdef int NGL_NODE_BUFFERVEC3
    cdef int NGL_NODE_BUFFERVEC4
    cdef int NGL_NODE_BUFFERMAT4
    cdef int NGL_NODE_CAMERA
    cdef int NGL_NODE_CIRCLE
    cdef int NGL_NODE_COMPUTE
    cdef int NGL_NODE_COMPUTEPROGRAM
    cdef int NGL_NODE_GEOMETRY
    cdef int NGL_NODE_GRAPHICCONFIG
    cdef int NGL_NODE_GROUP
    cdef int NGL_NODE_HUD
    cdef int NGL_NODE_IDENTITY
    cdef int NGL_NODE_MEDIA
    cdef int NGL_NODE_PROGRAM
    cdef int NGL_NODE_QUAD
    cdef int NGL_NODE_RENDER
    cdef int NGL_NODE_RENDERTOTEXTURE
    cdef int NGL_NODE_ROTATE
    cdef int NGL_NODE_ROTATEQUAT
    cdef int NGL_NODE_SCALE
    cdef int NGL_NODE_TEXT
    cdef int NGL_NODE_TEXTURE2D
    cdef int NGL_NODE_TEXTURE3D
    cdef int NGL_NODE_TEXTURECUBE
    cdef int NGL_NODE_TIMERANGEFILTER
    cdef int NGL_NODE_TIMERANGEMODECONT
    cdef int NGL_NODE_TIMERANGEMODENOOP
    cdef int NGL_NODE_TIMERANGEMODEONCE
    cdef int NGL_NODE_TRANSFORM
    cdef int NGL_NODE_TRANSLATE
    cdef int NGL_NODE_TRIANGLE
    cdef int NGL_NODE_STREAMEDINT
    cdef int NGL_NODE_STREAMEDFLOAT
    cdef int NGL_NODE_STREAMEDVEC2
    cdef int NGL_NODE_STREAMEDVEC3
    cdef int NGL_NODE_STREAMEDVEC4
    cdef int NGL_NODE_STREAMEDMAT4
    cdef int NGL_NODE_STREAMEDBUFFERINT
    cdef int NGL_NODE_STREAMEDBUFFERFLOAT
    cdef int NGL_NODE_STREAMEDBUFFERVEC2
    cdef int NGL_NODE_STREAMEDBUFFERVEC3
    cdef int NGL_NODE_STREAMEDBUFFERVEC4
    cdef int NGL_NODE_STREAMEDBUFFERMAT4
    cdef int NGL_NODE_UNIFORMINT
    cdef int NGL_NODE_UNIFORMMAT4
    cdef int NGL_NODE_UNIFORMFLOAT
    cdef int NGL_NODE_UNIFORMVEC2
    cdef int NGL_NODE_UNIFORMVEC3
    cdef int NGL_NODE_UNIFORMVEC4
    cdef int NGL_NODE_UNIFORMQUAT
    cdef int NGL_NODE_USERSWITCH


cdef class _Node:
    cdef ngl_node *ctx

    @property
    def cptr(self):
        return <uintptr_t>self.ctx

    def serialize(self, filename=""):
        return _ret_pystr(ngl_node_serialize(self.ctx, <const char *>filename))

    def dot(self):
        return _ret_pystr(ngl_node_dot(self.ctx))

    def __dealloc__(self):
        ngl_node_unrefp(&self.ctx)

    def _update_dict(self, field_name, arg=None, **kwargs):
        cdef ngl_node *node
        data_dict = {}
        if arg is not None:
            if not isinstance(arg, dict):
                raise TypeError("%s must be of type dict" % field_name)
            data_dict.update(arg)
        data_dict.update(**kwargs)
        for key, val in data_dict.items():
            if not isinstance(key, str) or (val is not None and not isinstance(val, _Node)):
                raise TypeError("update_%s() takes a dictionary of <string, Node>" % field_name)
            node = (<_Node>val).ctx if val is not None else NULL
            ret = ngl_node_param_set(self.ctx, field_name, <const char *>key, node)
            if ret < 0:
                return ret
        return 0

    def _init_params(self, label=None):

        if label is not None:
            self.set_label(label)

    def _add_nodelist(self, field_name, *elems):
        if hasattr(elems[0], '__iter__'):
            raise Exception("add_%s() takes elements as "
                            "positional arguments, not list" %
                            field_name)
        cdef int nb_elems = len(elems)
        elems_c = <ngl_node **>calloc(len(elems), sizeof(ngl_node *))
        if elems_c is NULL:
            raise MemoryError()
        cdef int i
        for i, item in enumerate(elems):
            elems_c[i] = (<_Node>item).ctx
        ret = ngl_node_param_add(self.ctx, field_name, nb_elems, elems_c)
        free(elems_c)
        return ret

    def _add_doublelist(self, field_name, *elems):
        if hasattr(elems[0], '__iter__'):
            raise Exception("add_%s() takes elements as "
                            "positional arguments, not list" %
                            field_name)
        cdef int nb_elems = len(elems)
        elems_c = <double*>calloc(len(elems), sizeof(double))
        if elems_c is NULL:
            raise MemoryError()
        cdef int i
        for i, item in enumerate(elems):
            elems_c[i] = <double>item
        ret = ngl_node_param_add(self.ctx, field_name, nb_elems, elems_c)
        free(elems_c)
        return ret

    def set_label(self, const char * label):
        return ngl_node_param_set(self.ctx, "label", label)


cdef class _AnimatedBuffer(_Node):

    def _init_params(self, keyframes=None, *args, **kwargs):
        if keyframes is not None:
            self.add_keyframes(*keyframes)
        _Node._init_params(self, *args, **kwargs)

    def add_keyframes(self, *keyframes):
        return self._add_nodelist("keyframes", *keyframes)


cdef class AnimatedBufferFloat(_AnimatedBuffer):

    def __init__(self, *args, **kwargs):
        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_ANIMATEDBUFFERFLOAT)
        if self.ctx is NULL:
            raise MemoryError()
        self._init_params(*args, **kwargs)


cdef class AnimatedBufferVec2(_AnimatedBuffer):

    def __init__(self, *args, **kwargs):
        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_ANIMATEDBUFFERVEC2)
        if self.ctx is NULL:
            raise MemoryError()
        self._init_params(*args, **kwargs)


cdef class AnimatedBufferVec3(_AnimatedBuffer):

    def __init__(self, *args, **kwargs):
        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_ANIMATEDBUFFERVEC3)
        if self.ctx is NULL:
            raise MemoryError()
        self._init_params(*args, **kwargs)


cdef class AnimatedBufferVec4(_AnimatedBuffer):

    def __init__(self, *args, **kwargs):
        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_ANIMATEDBUFFERVEC4)
        if self.ctx is NULL:
            raise MemoryError()
        self._init_params(*args, **kwargs)


cdef class AnimatedTime(_Node):

    def __init__(self, keyframes=None, *args, **kwargs):
        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_ANIMATEDTIME)
        if self.ctx is NULL:
            raise MemoryError()
        _Node._init_params(self, *args, **kwargs)

        if keyframes is not None:
            self.add_keyframes(*keyframes)

    def add_keyframes(self, *keyframes):
        return self._add_nodelist("keyframes", *keyframes)


cdef class AnimatedFloat(_Node):

    def __init__(self, keyframes=None, *args, **kwargs):
        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_ANIMATEDFLOAT)
        if self.ctx is NULL:
            raise MemoryError()
        _Node._init_params(self, *args, **kwargs)

        if keyframes is not None:
            self.add_keyframes(*keyframes)

    def evaluate(self, t):
        cdef float[1] vec
        ngl_anim_evaluate(self.ctx, vec, t)
        return vec[0]

    def add_keyframes(self, *keyframes):
        return self._add_nodelist("keyframes", *keyframes)


cdef class AnimatedVec2(_Node):

    def __init__(self, keyframes=None, *args, **kwargs):
        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_ANIMATEDVEC2)
        if self.ctx is NULL:
            raise MemoryError()
        _Node._init_params(self, *args, **kwargs)

        if keyframes is not None:
            self.add_keyframes(*keyframes)

    def evaluate(self, t):
        cdef float[2] vec
        ngl_anim_evaluate(self.ctx, vec, t)
        return (vec[0], vec[1])

    def add_keyframes(self, *keyframes):
        return self._add_nodelist("keyframes", *keyframes)


cdef class AnimatedVec3(_Node):

    def __init__(self, keyframes=None, *args, **kwargs):
        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_ANIMATEDVEC3)
        if self.ctx is NULL:
            raise MemoryError()
        _Node._init_params(self, *args, **kwargs)

        if keyframes is not None:
            self.add_keyframes(*keyframes)

    def evaluate(self, t):
        cdef float[3] vec
        ngl_anim_evaluate(self.ctx, vec, t)
        return (vec[0], vec[1], vec[2])

    def add_keyframes(self, *keyframes):
        return self._add_nodelist("keyframes", *keyframes)


cdef class AnimatedVec4(_Node):

    def __init__(self, keyframes=None, *args, **kwargs):
        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_ANIMATEDVEC4)
        if self.ctx is NULL:
            raise MemoryError()
        _Node._init_params(self, *args, **kwargs)

        if keyframes is not None:
            self.add_keyframes(*keyframes)

    def evaluate(self, t):
        cdef float[4] vec
        ngl_anim_evaluate(self.ctx, vec, t)
        return (vec[0], vec[1], vec[2], vec[3])

    def add_keyframes(self, *keyframes):
        return self._add_nodelist("keyframes", *keyframes)


cdef class AnimatedQuat(_Node):

    def __init__(self, keyframes=None, as_mat4=None, *args, **kwargs):
        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_ANIMATEDQUAT)
        if self.ctx is NULL:
            raise MemoryError()
        _Node._init_params(self, *args, **kwargs)

        if keyframes is not None:
            self.add_keyframes(*keyframes)
        if as_mat4 is not None:
            self.set_as_mat4(as_mat4)

    def evaluate(self, t):
        cdef float[4] vec
        ngl_anim_evaluate(self.ctx, vec, t)
        return (vec[0], vec[1], vec[2], vec[3])

    def add_keyframes(self, *keyframes):
        return self._add_nodelist("keyframes", *keyframes)

    def set_as_mat4(self, bint as_mat4):
        return ngl_node_param_set(self.ctx, "as_mat4", as_mat4)


cdef class AnimKeyFrameFloat(_Node):

    def __init__(self, double time, double value, easing=None, easing_args=None, easing_start_offset=None, easing_end_offset=None, *args, **kwargs):
        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_ANIMKEYFRAMEFLOAT, time, value)
        if self.ctx is NULL:
            raise MemoryError()
        _Node._init_params(self, *args, **kwargs)

        if easing is not None:
            self.set_easing(easing)
        if easing_args is not None:
            self.add_easing_args(*easing_args)
        if easing_start_offset is not None:
            self.set_easing_start_offset(easing_start_offset)
        if easing_end_offset is not None:
            self.set_easing_end_offset(easing_end_offset)

    def set_easing(self, const char * easing):
        return ngl_node_param_set(self.ctx, "easing", easing)

    def add_easing_args(self, *easing_args):
        return self._add_doublelist("easing_args", *easing_args)

    def set_easing_start_offset(self, double easing_start_offset):
        return ngl_node_param_set(self.ctx, "easing_start_offset", easing_start_offset)

    def set_easing_end_offset(self, double easing_end_offset):
        return ngl_node_param_set(self.ctx, "easing_end_offset", easing_end_offset)


cdef class AnimKeyFrameVec2(_Node):

    def __init__(self, double time, value, easing=None, easing_args=None, easing_start_offset=None, easing_end_offset=None, *args, **kwargs):
        cdef float[2] value_c
        cdef int value_i
        if len(value) != 2:
            raise TypeError("%s parameter is expected to be vec%d but got %d values" % (
                            "value", 2, len(value)))
        for value_i in range(2):
            value_c[value_i] = value[value_i]

        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_ANIMKEYFRAMEVEC2, time, value_c)
        if self.ctx is NULL:
            raise MemoryError()
        _Node._init_params(self, *args, **kwargs)

        if easing is not None:
            self.set_easing(easing)
        if easing_args is not None:
            self.add_easing_args(*easing_args)
        if easing_start_offset is not None:
            self.set_easing_start_offset(easing_start_offset)
        if easing_end_offset is not None:
            self.set_easing_end_offset(easing_end_offset)

    def set_easing(self, const char * easing):
        return ngl_node_param_set(self.ctx, "easing", easing)

    def add_easing_args(self, *easing_args):
        return self._add_doublelist("easing_args", *easing_args)

    def set_easing_start_offset(self, double easing_start_offset):
        return ngl_node_param_set(self.ctx, "easing_start_offset", easing_start_offset)

    def set_easing_end_offset(self, double easing_end_offset):
        return ngl_node_param_set(self.ctx, "easing_end_offset", easing_end_offset)


cdef class AnimKeyFrameVec3(_Node):

    def __init__(self, double time, value, easing=None, easing_args=None, easing_start_offset=None, easing_end_offset=None, *args, **kwargs):
        cdef float[3] value_c
        cdef int value_i
        if len(value) != 3:
            raise TypeError("%s parameter is expected to be vec%d but got %d values" % (
                            "value", 3, len(value)))
        for value_i in range(3):
            value_c[value_i] = value[value_i]

        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_ANIMKEYFRAMEVEC3, time, value_c)
        if self.ctx is NULL:
            raise MemoryError()
        _Node._init_params(self, *args, **kwargs)

        if easing is not None:
            self.set_easing(easing)
        if easing_args is not None:
            self.add_easing_args(*easing_args)
        if easing_start_offset is not None:
            self.set_easing_start_offset(easing_start_offset)
        if easing_end_offset is not None:
            self.set_easing_end_offset(easing_end_offset)

    def set_easing(self, const char * easing):
        return ngl_node_param_set(self.ctx, "easing", easing)

    def add_easing_args(self, *easing_args):
        return self._add_doublelist("easing_args", *easing_args)

    def set_easing_start_offset(self, double easing_start_offset):
        return ngl_node_param_set(self.ctx, "easing_start_offset", easing_start_offset)

    def set_easing_end_offset(self, double easing_end_offset):
        return ngl_node_param_set(self.ctx, "easing_end_offset", easing_end_offset)


cdef class AnimKeyFrameVec4(_Node):

    def __init__(self, double time, value, easing=None, easing_args=None, easing_start_offset=None, easing_end_offset=None, *args, **kwargs):
        cdef float[4] value_c
        cdef int value_i
        if len(value) != 4:
            raise TypeError("%s parameter is expected to be vec%d but got %d values" % (
                            "value", 4, len(value)))
        for value_i in range(4):
            value_c[value_i] = value[value_i]

        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_ANIMKEYFRAMEVEC4, time, value_c)
        if self.ctx is NULL:
            raise MemoryError()
        _Node._init_params(self, *args, **kwargs)

        if easing is not None:
            self.set_easing(easing)
        if easing_args is not None:
            self.add_easing_args(*easing_args)
        if easing_start_offset is not None:
            self.set_easing_start_offset(easing_start_offset)
        if easing_end_offset is not None:
            self.set_easing_end_offset(easing_end_offset)

    def set_easing(self, const char * easing):
        return ngl_node_param_set(self.ctx, "easing", easing)

    def add_easing_args(self, *easing_args):
        return self._add_doublelist("easing_args", *easing_args)

    def set_easing_start_offset(self, double easing_start_offset):
        return ngl_node_param_set(self.ctx, "easing_start_offset", easing_start_offset)

    def set_easing_end_offset(self, double easing_end_offset):
        return ngl_node_param_set(self.ctx, "easing_end_offset", easing_end_offset)


cdef class AnimKeyFrameQuat(_Node):

    def __init__(self, double time, quat, easing=None, easing_args=None, easing_start_offset=None, easing_end_offset=None, *args, **kwargs):
        cdef float[4] quat_c
        cdef int quat_i
        if len(quat) != 4:
            raise TypeError("%s parameter is expected to be vec%d but got %d values" % (
                            "quat", 4, len(quat)))
        for quat_i in range(4):
            quat_c[quat_i] = quat[quat_i]

        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_ANIMKEYFRAMEQUAT, time, quat_c)
        if self.ctx is NULL:
            raise MemoryError()
        _Node._init_params(self, *args, **kwargs)

        if easing is not None:
            self.set_easing(easing)
        if easing_args is not None:
            self.add_easing_args(*easing_args)
        if easing_start_offset is not None:
            self.set_easing_start_offset(easing_start_offset)
        if easing_end_offset is not None:
            self.set_easing_end_offset(easing_end_offset)

    def set_easing(self, const char * easing):
        return ngl_node_param_set(self.ctx, "easing", easing)

    def add_easing_args(self, *easing_args):
        return self._add_doublelist("easing_args", *easing_args)

    def set_easing_start_offset(self, double easing_start_offset):
        return ngl_node_param_set(self.ctx, "easing_start_offset", easing_start_offset)

    def set_easing_end_offset(self, double easing_end_offset):
        return ngl_node_param_set(self.ctx, "easing_end_offset", easing_end_offset)


cdef class AnimKeyFrameBuffer(_Node):

    def __init__(self, double time, data=None, easing=None, easing_args=None, easing_start_offset=None, easing_end_offset=None, *args, **kwargs):
        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_ANIMKEYFRAMEBUFFER, time)
        if self.ctx is NULL:
            raise MemoryError()
        _Node._init_params(self, *args, **kwargs)

        if data is not None:
            self.set_data(data)
        if easing is not None:
            self.set_easing(easing)
        if easing_args is not None:
            self.add_easing_args(*easing_args)
        if easing_start_offset is not None:
            self.set_easing_start_offset(easing_start_offset)
        if easing_end_offset is not None:
            self.set_easing_end_offset(easing_end_offset)

    def set_data(self, array.array data):
        return ngl_node_param_set(self.ctx,
                                  "data",
                                  <int>(data.buffer_info()[1] * data.itemsize),
                                  <void *>(data.data.as_voidptr))


    def set_easing(self, const char * easing):
        return ngl_node_param_set(self.ctx, "easing", easing)

    def add_easing_args(self, *easing_args):
        return self._add_doublelist("easing_args", *easing_args)

    def set_easing_start_offset(self, double easing_start_offset):
        return ngl_node_param_set(self.ctx, "easing_start_offset", easing_start_offset)

    def set_easing_end_offset(self, double easing_end_offset):
        return ngl_node_param_set(self.ctx, "easing_end_offset", easing_end_offset)


cdef class Block(_Node):

    def __init__(self, fields=None, layout=None, *args, **kwargs):
        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_BLOCK)
        if self.ctx is NULL:
            raise MemoryError()
        _Node._init_params(self, *args, **kwargs)

        if fields is not None:
            self.update_fields(fields)
        if layout is not None:
            self.set_layout(layout)
            
    def update_fields(self, arg=None, **kwargs):
        return self._update_dict("fields", arg, **kwargs)

    def set_layout(self, const char * layout):
        return ngl_node_param_set(self.ctx, "layout", layout)


cdef class _Buffer(_Node):

    def _init_params(self, count=None, data=None, filename=None, block=None, block_field=None, *args, **kwargs):
        if count is not None:
            self.set_count(count)
        if data is not None:
            self.set_data(data)
        if filename is not None:
            self.set_filename(filename)
        if block is not None:
            self.set_block(block)
        if block_field is not None:
            self.set_block_field(block_field)
        _Node._init_params(self, *args, **kwargs)

    def set_count(self, int count):
        return ngl_node_param_set(self.ctx, "count", count)

    def set_data(self, array.array data):
        return ngl_node_param_set(self.ctx,
                                  "data",
                                  <int>(data.buffer_info()[1] * data.itemsize),
                                  <void *>(data.data.as_voidptr))


    def set_filename(self, const char * filename):
        return ngl_node_param_set(self.ctx, "filename", filename)

    def set_block(self, _Node block):
        return ngl_node_param_set(self.ctx, "block", block.ctx)

    def set_block_field(self, const char* block_field):
        return ngl_node_param_set(self.ctx, "block_field", block_field)


cdef class BufferByte(_Buffer):

    def __init__(self, *args, **kwargs):
        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_BUFFERBYTE)
        if self.ctx is NULL:
            raise MemoryError()
        self._init_params(*args, **kwargs)


cdef class BufferBVec2(_Buffer):

    def __init__(self, *args, **kwargs):
        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_BUFFERBVEC2)
        if self.ctx is NULL:
            raise MemoryError()
        self._init_params(*args, **kwargs)


cdef class BufferBVec3(_Buffer):

    def __init__(self, *args, **kwargs):
        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_BUFFERBVEC3)
        if self.ctx is NULL:
            raise MemoryError()
        self._init_params(*args, **kwargs)


cdef class BufferBVec4(_Buffer):

    def __init__(self, *args, **kwargs):
        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_BUFFERBVEC4)
        if self.ctx is NULL:
            raise MemoryError()
        self._init_params(*args, **kwargs)


cdef class BufferInt(_Buffer):

    def __init__(self, *args, **kwargs):
        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_BUFFERINT)
        if self.ctx is NULL:
            raise MemoryError()
        self._init_params(*args, **kwargs)


cdef class BufferInt64(_Buffer):

    def __init__(self, *args, **kwargs):
        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_BUFFERINT64)
        if self.ctx is NULL:
            raise MemoryError()
        self._init_params(*args, **kwargs)


cdef class BufferIVec2(_Buffer):

    def __init__(self, *args, **kwargs):
        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_BUFFERIVEC2)
        if self.ctx is NULL:
            raise MemoryError()
        self._init_params(*args, **kwargs)


cdef class BufferIVec3(_Buffer):

    def __init__(self, *args, **kwargs):
        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_BUFFERIVEC3)
        if self.ctx is NULL:
            raise MemoryError()
        self._init_params(*args, **kwargs)


cdef class BufferIVec4(_Buffer):

    def __init__(self, *args, **kwargs):
        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_BUFFERIVEC4)
        if self.ctx is NULL:
            raise MemoryError()
        self._init_params(*args, **kwargs)


cdef class BufferShort(_Buffer):

    def __init__(self, *args, **kwargs):
        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_BUFFERSHORT)
        if self.ctx is NULL:
            raise MemoryError()
        self._init_params(*args, **kwargs)


cdef class BufferSVec2(_Buffer):

    def __init__(self, *args, **kwargs):
        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_BUFFERSVEC2)
        if self.ctx is NULL:
            raise MemoryError()
        self._init_params(*args, **kwargs)


cdef class BufferSVec3(_Buffer):

    def __init__(self, *args, **kwargs):
        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_BUFFERSVEC3)
        if self.ctx is NULL:
            raise MemoryError()
        self._init_params(*args, **kwargs)


cdef class BufferSVec4(_Buffer):

    def __init__(self, *args, **kwargs):
        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_BUFFERSVEC4)
        if self.ctx is NULL:
            raise MemoryError()
        self._init_params(*args, **kwargs)


cdef class BufferUByte(_Buffer):

    def __init__(self, *args, **kwargs):
        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_BUFFERUBYTE)
        if self.ctx is NULL:
            raise MemoryError()
        self._init_params(*args, **kwargs)


cdef class BufferUBVec2(_Buffer):

    def __init__(self, *args, **kwargs):
        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_BUFFERUBVEC2)
        if self.ctx is NULL:
            raise MemoryError()
        self._init_params(*args, **kwargs)


cdef class BufferUBVec3(_Buffer):

    def __init__(self, *args, **kwargs):
        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_BUFFERUBVEC3)
        if self.ctx is NULL:
            raise MemoryError()
        self._init_params(*args, **kwargs)


cdef class BufferUBVec4(_Buffer):

    def __init__(self, *args, **kwargs):
        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_BUFFERUBVEC4)
        if self.ctx is NULL:
            raise MemoryError()
        self._init_params(*args, **kwargs)


cdef class BufferUInt(_Buffer):

    def __init__(self, *args, **kwargs):
        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_BUFFERUINT)
        if self.ctx is NULL:
            raise MemoryError()
        self._init_params(*args, **kwargs)


cdef class BufferUIVec2(_Buffer):

    def __init__(self, *args, **kwargs):
        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_BUFFERUIVEC2)
        if self.ctx is NULL:
            raise MemoryError()
        self._init_params(*args, **kwargs)


cdef class BufferUIVec3(_Buffer):

    def __init__(self, *args, **kwargs):
        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_BUFFERUIVEC3)
        if self.ctx is NULL:
            raise MemoryError()
        self._init_params(*args, **kwargs)


cdef class BufferUIVec4(_Buffer):

    def __init__(self, *args, **kwargs):
        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_BUFFERUIVEC4)
        if self.ctx is NULL:
            raise MemoryError()
        self._init_params(*args, **kwargs)


cdef class BufferUShort(_Buffer):

    def __init__(self, *args, **kwargs):
        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_BUFFERUSHORT)
        if self.ctx is NULL:
            raise MemoryError()
        self._init_params(*args, **kwargs)


cdef class BufferUSVec2(_Buffer):

    def __init__(self, *args, **kwargs):
        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_BUFFERUSVEC2)
        if self.ctx is NULL:
            raise MemoryError()
        self._init_params(*args, **kwargs)


cdef class BufferUSVec3(_Buffer):

    def __init__(self, *args, **kwargs):
        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_BUFFERUSVEC3)
        if self.ctx is NULL:
            raise MemoryError()
        self._init_params(*args, **kwargs)


cdef class BufferUSVec4(_Buffer):

    def __init__(self, *args, **kwargs):
        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_BUFFERUSVEC4)
        if self.ctx is NULL:
            raise MemoryError()
        self._init_params(*args, **kwargs)


cdef class BufferFloat(_Buffer):

    def __init__(self, *args, **kwargs):
        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_BUFFERFLOAT)
        if self.ctx is NULL:
            raise MemoryError()
        self._init_params(*args, **kwargs)


cdef class BufferVec2(_Buffer):

    def __init__(self, *args, **kwargs):
        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_BUFFERVEC2)
        if self.ctx is NULL:
            raise MemoryError()
        self._init_params(*args, **kwargs)


cdef class BufferVec3(_Buffer):

    def __init__(self, *args, **kwargs):
        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_BUFFERVEC3)
        if self.ctx is NULL:
            raise MemoryError()
        self._init_params(*args, **kwargs)


cdef class BufferVec4(_Buffer):

    def __init__(self, *args, **kwargs):
        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_BUFFERVEC4)
        if self.ctx is NULL:
            raise MemoryError()
        self._init_params(*args, **kwargs)


cdef class BufferMat4(_Buffer):

    def __init__(self, *args, **kwargs):
        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_BUFFERMAT4)
        if self.ctx is NULL:
            raise MemoryError()
        self._init_params(*args, **kwargs)


cdef class Camera(_Node):

    def __init__(self, _Node child, eye=None, center=None, up=None, perspective=None, orthographic=None, clipping=None, eye_transform=None, center_transform=None, up_transform=None, fov_anim=None, *args, **kwargs):
        assert child is not None

        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_CAMERA, child.ctx)
        if self.ctx is NULL:
            raise MemoryError()
        _Node._init_params(self, *args, **kwargs)

        if eye is not None:
            self.set_eye(*eye)
        if center is not None:
            self.set_center(*center)
        if up is not None:
            self.set_up(*up)
        if perspective is not None:
            self.set_perspective(*perspective)
        if orthographic is not None:
            self.set_orthographic(*orthographic)
        if clipping is not None:
            self.set_clipping(*clipping)
        if eye_transform is not None:
            self.set_eye_transform(eye_transform)
        if center_transform is not None:
            self.set_center_transform(center_transform)
        if up_transform is not None:
            self.set_up_transform(up_transform)
        if fov_anim is not None:
            self.set_fov_anim(fov_anim)

    def set_eye(self, *eye):
        cdef float[3] eye_c
        cdef int eye_i
        if len(eye) != 3:
            raise TypeError("%s parameter is expected to be vec%d but got %d values" % (
                            "eye", 3, len(eye)))
        for eye_i in range(3):
            eye_c[eye_i] = eye[eye_i]

        return ngl_node_param_set(self.ctx, "eye", eye_c)

    def set_center(self, *center):
        cdef float[3] center_c
        cdef int center_i
        if len(center) != 3:
            raise TypeError("%s parameter is expected to be vec%d but got %d values" % (
                            "center", 3, len(center)))
        for center_i in range(3):
            center_c[center_i] = center[center_i]

        return ngl_node_param_set(self.ctx, "center", center_c)

    def set_up(self, *up):
        cdef float[3] up_c
        cdef int up_i
        if len(up) != 3:
            raise TypeError("%s parameter is expected to be vec%d but got %d values" % (
                            "up", 3, len(up)))
        for up_i in range(3):
            up_c[up_i] = up[up_i]

        return ngl_node_param_set(self.ctx, "up", up_c)

    def set_perspective(self, *perspective):
        cdef float[2] perspective_c
        cdef int perspective_i
        if len(perspective) != 2:
            raise TypeError("%s parameter is expected to be vec%d but got %d values" % (
                            "perspective", 2, len(perspective)))
        for perspective_i in range(2):
            perspective_c[perspective_i] = perspective[perspective_i]

        return ngl_node_param_set(self.ctx, "perspective", perspective_c)

    def set_orthographic(self, *orthographic):
        cdef float[4] orthographic_c
        cdef int orthographic_i
        if len(orthographic) != 4:
            raise TypeError("%s parameter is expected to be vec%d but got %d values" % (
                            "orthographic", 4, len(orthographic)))
        for orthographic_i in range(4):
            orthographic_c[orthographic_i] = orthographic[orthographic_i]

        return ngl_node_param_set(self.ctx, "orthographic", orthographic_c)

    def set_clipping(self, *clipping):
        cdef float[2] clipping_c
        cdef int clipping_i
        if len(clipping) != 2:
            raise TypeError("%s parameter is expected to be vec%d but got %d values" % (
                            "clipping", 2, len(clipping)))
        for clipping_i in range(2):
            clipping_c[clipping_i] = clipping[clipping_i]

        return ngl_node_param_set(self.ctx, "clipping", clipping_c)

    def set_eye_transform(self, _Node eye_transform):
        return ngl_node_param_set(self.ctx, "eye_transform", eye_transform.ctx)

    def set_center_transform(self, _Node center_transform):
        return ngl_node_param_set(self.ctx, "center_transform", center_transform.ctx)

    def set_up_transform(self, _Node up_transform):
        return ngl_node_param_set(self.ctx, "up_transform", up_transform.ctx)

    def set_fov_anim(self, _Node fov_anim):
        return ngl_node_param_set(self.ctx, "fov_anim", fov_anim.ctx)


cdef class Circle(_Node):

    def __init__(self, radius=None, npoints=None, *args, **kwargs):
        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_CIRCLE)
        if self.ctx is NULL:
            raise MemoryError()
        _Node._init_params(self, *args, **kwargs)

        if radius is not None:
            self.set_radius(radius)
        if npoints is not None:
            self.set_npoints(npoints)

    def set_radius(self, double radius):
        return ngl_node_param_set(self.ctx, "radius", radius)

    def set_npoints(self, int npoints):
        return ngl_node_param_set(self.ctx, "npoints", npoints)


cdef class Compute(_Node):

    def __init__(self, int nb_group_x, int nb_group_y, int nb_group_z, 
			int threads_per_group_x, int threads_per_group_y, int threads_per_group_z,
			_Node program, textures=None, uniforms=None, blocks=None, *args, **kwargs):
        assert program is not None

        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_COMPUTE, nb_group_x, nb_group_y, nb_group_z, 
			threads_per_group_x, threads_per_group_y, threads_per_group_z, program.ctx)
        if self.ctx is NULL:
            raise MemoryError()
        _Node._init_params(self, *args, **kwargs)

        if textures is not None:
            self.update_textures(textures)
        if uniforms is not None:
            self.update_uniforms(uniforms)
        if blocks is not None:
            self.update_blocks(blocks)

    def update_textures(self, arg=None, **kwargs):
        return self._update_dict("textures", arg, **kwargs)

    def update_uniforms(self, arg=None, **kwargs):
        return self._update_dict("uniforms", arg, **kwargs)

    def update_blocks(self, arg=None, **kwargs):
        return self._update_dict("blocks", arg, **kwargs)


cdef class ComputeProgram(_Node):

    def __init__(self, const char *compute, *args, **kwargs):
        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_COMPUTEPROGRAM, compute)
        if self.ctx is NULL:
            raise MemoryError()
        _Node._init_params(self, *args, **kwargs)



cdef class Geometry(_Node):

    def __init__(self, _Node vertices, uvcoords=None, normals=None, indices=None, topology=None, *args, **kwargs):
        assert vertices is not None

        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_GEOMETRY, vertices.ctx)
        if self.ctx is NULL:
            raise MemoryError()
        _Node._init_params(self, *args, **kwargs)

        if uvcoords is not None:
            self.set_uvcoords(uvcoords)
        if normals is not None:
            self.set_normals(normals)
        if indices is not None:
            self.set_indices(indices)
        if topology is not None:
            self.set_topology(topology)

    def set_uvcoords(self, _Node uvcoords):
        return ngl_node_param_set(self.ctx, "uvcoords", uvcoords.ctx)

    def set_normals(self, _Node normals):
        return ngl_node_param_set(self.ctx, "normals", normals.ctx)

    def set_indices(self, _Node indices):
        return ngl_node_param_set(self.ctx, "indices", indices.ctx)

    def set_topology(self, const char * topology):
        return ngl_node_param_set(self.ctx, "topology", topology)


cdef class GraphicConfig(_Node):

    def __init__(self, _Node child, blend=None, blend_src_factor=None, blend_dst_factor=None, blend_src_factor_a=None, blend_dst_factor_a=None, blend_op=None, blend_op_a=None, color_write_mask=None, depth_test=None, depth_write_mask=None, depth_func=None, stencil_test=None, stencil_write_mask=None, stencil_func=None, stencil_ref=None, stencil_read_mask=None, stencil_fail=None, stencil_depth_fail=None, stencil_depth_pass=None, cull_face=None, cull_face_mode=None, scissor_test=None, scissor=None, *args, **kwargs):
        assert child is not None

        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_GRAPHICCONFIG, child.ctx)
        if self.ctx is NULL:
            raise MemoryError()
        _Node._init_params(self, *args, **kwargs)

        if blend is not None:
            self.set_blend(blend)
        if blend_src_factor is not None:
            self.set_blend_src_factor(blend_src_factor)
        if blend_dst_factor is not None:
            self.set_blend_dst_factor(blend_dst_factor)
        if blend_src_factor_a is not None:
            self.set_blend_src_factor_a(blend_src_factor_a)
        if blend_dst_factor_a is not None:
            self.set_blend_dst_factor_a(blend_dst_factor_a)
        if blend_op is not None:
            self.set_blend_op(blend_op)
        if blend_op_a is not None:
            self.set_blend_op_a(blend_op_a)
        if color_write_mask is not None:
            self.set_color_write_mask(color_write_mask)
        if depth_test is not None:
            self.set_depth_test(depth_test)
        if depth_write_mask is not None:
            self.set_depth_write_mask(depth_write_mask)
        if depth_func is not None:
            self.set_depth_func(depth_func)
        if stencil_test is not None:
            self.set_stencil_test(stencil_test)
        if stencil_write_mask is not None:
            self.set_stencil_write_mask(stencil_write_mask)
        if stencil_func is not None:
            self.set_stencil_func(stencil_func)
        if stencil_ref is not None:
            self.set_stencil_ref(stencil_ref)
        if stencil_read_mask is not None:
            self.set_stencil_read_mask(stencil_read_mask)
        if stencil_fail is not None:
            self.set_stencil_fail(stencil_fail)
        if stencil_depth_fail is not None:
            self.set_stencil_depth_fail(stencil_depth_fail)
        if stencil_depth_pass is not None:
            self.set_stencil_depth_pass(stencil_depth_pass)
        if cull_face is not None:
            self.set_cull_face(cull_face)
        if cull_face_mode is not None:
            self.set_cull_face_mode(cull_face_mode)
        if scissor_test is not None:
            self.set_scissor_test(scissor_test)
        if scissor is not None:
            self.set_scissor(*scissor)

    def set_blend(self, bint blend):
        return ngl_node_param_set(self.ctx, "blend", blend)

    def set_blend_src_factor(self, const char * blend_src_factor):
        return ngl_node_param_set(self.ctx, "blend_src_factor", blend_src_factor)

    def set_blend_dst_factor(self, const char * blend_dst_factor):
        return ngl_node_param_set(self.ctx, "blend_dst_factor", blend_dst_factor)

    def set_blend_src_factor_a(self, const char * blend_src_factor_a):
        return ngl_node_param_set(self.ctx, "blend_src_factor_a", blend_src_factor_a)

    def set_blend_dst_factor_a(self, const char * blend_dst_factor_a):
        return ngl_node_param_set(self.ctx, "blend_dst_factor_a", blend_dst_factor_a)

    def set_blend_op(self, const char * blend_op):
        return ngl_node_param_set(self.ctx, "blend_op", blend_op)

    def set_blend_op_a(self, const char * blend_op_a):
        return ngl_node_param_set(self.ctx, "blend_op_a", blend_op_a)

    def set_color_write_mask(self, const char * color_write_mask):
        return ngl_node_param_set(self.ctx, "color_write_mask", color_write_mask)

    def set_depth_test(self, bint depth_test):
        return ngl_node_param_set(self.ctx, "depth_test", depth_test)

    def set_depth_write_mask(self, bint depth_write_mask):
        return ngl_node_param_set(self.ctx, "depth_write_mask", depth_write_mask)

    def set_depth_func(self, const char * depth_func):
        return ngl_node_param_set(self.ctx, "depth_func", depth_func)

    def set_stencil_test(self, bint stencil_test):
        return ngl_node_param_set(self.ctx, "stencil_test", stencil_test)

    def set_stencil_write_mask(self, int stencil_write_mask):
        return ngl_node_param_set(self.ctx, "stencil_write_mask", stencil_write_mask)

    def set_stencil_func(self, const char * stencil_func):
        return ngl_node_param_set(self.ctx, "stencil_func", stencil_func)

    def set_stencil_ref(self, int stencil_ref):
        return ngl_node_param_set(self.ctx, "stencil_ref", stencil_ref)

    def set_stencil_read_mask(self, int stencil_read_mask):
        return ngl_node_param_set(self.ctx, "stencil_read_mask", stencil_read_mask)

    def set_stencil_fail(self, const char * stencil_fail):
        return ngl_node_param_set(self.ctx, "stencil_fail", stencil_fail)

    def set_stencil_depth_fail(self, const char * stencil_depth_fail):
        return ngl_node_param_set(self.ctx, "stencil_depth_fail", stencil_depth_fail)

    def set_stencil_depth_pass(self, const char * stencil_depth_pass):
        return ngl_node_param_set(self.ctx, "stencil_depth_pass", stencil_depth_pass)

    def set_cull_face(self, bint cull_face):
        return ngl_node_param_set(self.ctx, "cull_face", cull_face)

    def set_cull_face_mode(self, const char * cull_face_mode):
        return ngl_node_param_set(self.ctx, "cull_face_mode", cull_face_mode)

    def set_scissor_test(self, bint scissor_test):
        return ngl_node_param_set(self.ctx, "scissor_test", scissor_test)

    def set_scissor(self, *scissor):
        cdef float[4] scissor_c
        cdef int scissor_i
        if len(scissor) != 4:
            raise TypeError("%s parameter is expected to be vec%d but got %d values" % (
                            "scissor", 4, len(scissor)))
        for scissor_i in range(4):
            scissor_c[scissor_i] = scissor[scissor_i]

        return ngl_node_param_set(self.ctx, "scissor", scissor_c)


cdef class Group(_Node):

    def __init__(self, children=None, *args, **kwargs):
        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_GROUP)
        if self.ctx is NULL:
            raise MemoryError()
        _Node._init_params(self, *args, **kwargs)

        if children is not None:
            self.add_children(*children)

    def add_children(self, *children):
        return self._add_nodelist("children", *children)


cdef class HUD(_Node):

    def __init__(self, _Node child, measure_window=None, refresh_rate=None, export_filename=None, bg_color=None, aspect_ratio=None, *args, **kwargs):
        assert child is not None

        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_HUD, child.ctx)
        if self.ctx is NULL:
            raise MemoryError()
        _Node._init_params(self, *args, **kwargs)

        if measure_window is not None:
            self.set_measure_window(measure_window)
        if refresh_rate is not None:
            self.set_refresh_rate(refresh_rate)
        if export_filename is not None:
            self.set_export_filename(export_filename)
        if bg_color is not None:
            self.set_bg_color(*bg_color)
        if aspect_ratio is not None:
            self.set_aspect_ratio(aspect_ratio)

    def set_measure_window(self, int measure_window):
        return ngl_node_param_set(self.ctx, "measure_window", measure_window)

    def set_refresh_rate(self, tuple refresh_rate):
        return ngl_node_param_set(self.ctx,
                                  "refresh_rate",
                                  <int>refresh_rate[0],
                                  <int>refresh_rate[1]);

    def set_export_filename(self, const char * export_filename):
        return ngl_node_param_set(self.ctx, "export_filename", export_filename)

    def set_bg_color(self, *bg_color):
        cdef float[4] bg_color_c
        cdef int bg_color_i
        if len(bg_color) != 4:
            raise TypeError("%s parameter is expected to be vec%d but got %d values" % (
                            "bg_color", 4, len(bg_color)))
        for bg_color_i in range(4):
            bg_color_c[bg_color_i] = bg_color[bg_color_i]

        return ngl_node_param_set(self.ctx, "bg_color", bg_color_c)

    def set_aspect_ratio(self, tuple aspect_ratio):
        return ngl_node_param_set(self.ctx,
                                  "aspect_ratio",
                                  <int>aspect_ratio[0],
                                  <int>aspect_ratio[1]);


cdef class Identity(_Node):

    def __init__(self, *args, **kwargs):
        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_IDENTITY)
        if self.ctx is NULL:
            raise MemoryError()
        _Node._init_params(self, *args, **kwargs)



cdef class Media(_Node):

    def __init__(self, const char *filename, sxplayer_min_level=None, time_anim=None, audio_tex=None, max_nb_packets=None, max_nb_frames=None, max_nb_sink=None, max_pixels=None, stream_idx=None, *args, **kwargs):
        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_MEDIA, filename)
        if self.ctx is NULL:
            raise MemoryError()
        _Node._init_params(self, *args, **kwargs)

        if sxplayer_min_level is not None:
            self.set_sxplayer_min_level(sxplayer_min_level)
        if time_anim is not None:
            self.set_time_anim(time_anim)
        if audio_tex is not None:
            self.set_audio_tex(audio_tex)
        if max_nb_packets is not None:
            self.set_max_nb_packets(max_nb_packets)
        if max_nb_frames is not None:
            self.set_max_nb_frames(max_nb_frames)
        if max_nb_sink is not None:
            self.set_max_nb_sink(max_nb_sink)
        if max_pixels is not None:
            self.set_max_pixels(max_pixels)
        if stream_idx is not None:
            self.set_stream_idx(stream_idx)

    def set_sxplayer_min_level(self, const char * sxplayer_min_level):
        return ngl_node_param_set(self.ctx, "sxplayer_min_level", sxplayer_min_level)

    def set_time_anim(self, _Node time_anim):
        return ngl_node_param_set(self.ctx, "time_anim", time_anim.ctx)

    def set_audio_tex(self, bint audio_tex):
        return ngl_node_param_set(self.ctx, "audio_tex", audio_tex)

    def set_max_nb_packets(self, int max_nb_packets):
        return ngl_node_param_set(self.ctx, "max_nb_packets", max_nb_packets)

    def set_max_nb_frames(self, int max_nb_frames):
        return ngl_node_param_set(self.ctx, "max_nb_frames", max_nb_frames)

    def set_max_nb_sink(self, int max_nb_sink):
        return ngl_node_param_set(self.ctx, "max_nb_sink", max_nb_sink)

    def set_max_pixels(self, int max_pixels):
        return ngl_node_param_set(self.ctx, "max_pixels", max_pixels)

    def set_stream_idx(self, int stream_idx):
        return ngl_node_param_set(self.ctx, "stream_idx", stream_idx)


cdef class Program(_Node):

    def __init__(self, vertex='default.vert', fragment='default.frag', *args, **kwargs):
        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_PROGRAM)
        if self.ctx is NULL:
            raise MemoryError()
        _Node._init_params(self, *args, **kwargs)

        if vertex is not None:
            self.set_vertex(vertex)
        if fragment is not None:
            self.set_fragment(fragment)

    def set_vertex(self, const char * vertex):
        return ngl_node_param_set(self.ctx, "vertex", vertex)

    def set_fragment(self, const char * fragment):
        return ngl_node_param_set(self.ctx, "fragment", fragment)


cdef class Quad(_Node):

    def __init__(self, corner=None, width=None, height=None, uv_corner=None, uv_width=None, uv_height=None, *args, **kwargs):
        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_QUAD)
        if self.ctx is NULL:
            raise MemoryError()
        _Node._init_params(self, *args, **kwargs)

        if corner is not None:
            self.set_corner(*corner)
        if width is not None:
            self.set_width(*width)
        if height is not None:
            self.set_height(*height)
        if uv_corner is not None:
            self.set_uv_corner(*uv_corner)
        if uv_width is not None:
            self.set_uv_width(*uv_width)
        if uv_height is not None:
            self.set_uv_height(*uv_height)

    def set_corner(self, *corner):
        cdef float[3] corner_c
        cdef int corner_i
        if len(corner) != 3:
            raise TypeError("%s parameter is expected to be vec%d but got %d values" % (
                            "corner", 3, len(corner)))
        for corner_i in range(3):
            corner_c[corner_i] = corner[corner_i]

        return ngl_node_param_set(self.ctx, "corner", corner_c)

    def set_width(self, *width):
        cdef float[3] width_c
        cdef int width_i
        if len(width) != 3:
            raise TypeError("%s parameter is expected to be vec%d but got %d values" % (
                            "width", 3, len(width)))
        for width_i in range(3):
            width_c[width_i] = width[width_i]

        return ngl_node_param_set(self.ctx, "width", width_c)

    def set_height(self, *height):
        cdef float[3] height_c
        cdef int height_i
        if len(height) != 3:
            raise TypeError("%s parameter is expected to be vec%d but got %d values" % (
                            "height", 3, len(height)))
        for height_i in range(3):
            height_c[height_i] = height[height_i]

        return ngl_node_param_set(self.ctx, "height", height_c)

    def set_uv_corner(self, *uv_corner):
        cdef float[2] uv_corner_c
        cdef int uv_corner_i
        if len(uv_corner) != 2:
            raise TypeError("%s parameter is expected to be vec%d but got %d values" % (
                            "uv_corner", 2, len(uv_corner)))
        for uv_corner_i in range(2):
            uv_corner_c[uv_corner_i] = uv_corner[uv_corner_i]

        return ngl_node_param_set(self.ctx, "uv_corner", uv_corner_c)

    def set_uv_width(self, *uv_width):
        cdef float[2] uv_width_c
        cdef int uv_width_i
        if len(uv_width) != 2:
            raise TypeError("%s parameter is expected to be vec%d but got %d values" % (
                            "uv_width", 2, len(uv_width)))
        for uv_width_i in range(2):
            uv_width_c[uv_width_i] = uv_width[uv_width_i]

        return ngl_node_param_set(self.ctx, "uv_width", uv_width_c)

    def set_uv_height(self, *uv_height):
        cdef float[2] uv_height_c
        cdef int uv_height_i
        if len(uv_height) != 2:
            raise TypeError("%s parameter is expected to be vec%d but got %d values" % (
                            "uv_height", 2, len(uv_height)))
        for uv_height_i in range(2):
            uv_height_c[uv_height_i] = uv_height[uv_height_i]

        return ngl_node_param_set(self.ctx, "uv_height", uv_height_c)


cdef class Render(_Node):

    def __init__(self, _Node geometry, program=None, textures=None, uniforms=None, blocks=None, attributes=None, instance_attributes=None, nb_instances=None, *args, **kwargs):
        assert geometry is not None

        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_RENDER, geometry.ctx)
        if self.ctx is NULL:
            raise MemoryError()
        _Node._init_params(self, *args, **kwargs)

        if program is not None:
            self.set_program(program)
        if textures is not None:
            self.update_textures(textures)
        if uniforms is not None:
            self.update_uniforms(uniforms)
        if blocks is not None:
            self.update_blocks(blocks)
        if attributes is not None:
            self.update_attributes(attributes)
        if instance_attributes is not None:
            self.update_instance_attributes(instance_attributes)
        if nb_instances is not None:
            self.set_nb_instances(nb_instances)

    def set_program(self, _Node program):
        return ngl_node_param_set(self.ctx, "program", program.ctx)

    def update_textures(self, arg=None, **kwargs):
        return self._update_dict("textures", arg, **kwargs)

    def update_uniforms(self, arg=None, **kwargs):
        return self._update_dict("uniforms", arg, **kwargs)

    def update_blocks(self, arg=None, **kwargs):
        return self._update_dict("blocks", arg, **kwargs)

    def update_attributes(self, arg=None, **kwargs):
        return self._update_dict("attributes", arg, **kwargs)

    def update_instance_attributes(self, arg=None, **kwargs):
        return self._update_dict("instance_attributes", arg, **kwargs)

    def set_nb_instances(self, int nb_instances):
        return ngl_node_param_set(self.ctx, "nb_instances", nb_instances)


cdef class RenderToTexture(_Node):

    def __init__(self, _Node child, color_textures=None, depth_texture=None, samples=None, clear_color=None, features=None, vflip=None, *args, **kwargs):
        assert child is not None

        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_RENDERTOTEXTURE, child.ctx)
        if self.ctx is NULL:
            raise MemoryError()
        _Node._init_params(self, *args, **kwargs)

        if color_textures is not None:
            self.add_color_textures(*color_textures)
        if depth_texture is not None:
            self.set_depth_texture(depth_texture)
        if samples is not None:
            self.set_samples(samples)
        if clear_color is not None:
            self.set_clear_color(*clear_color)
        if features is not None:
            self.set_features(features)
        if vflip is not None:
            self.set_vflip(vflip)

    def add_color_textures(self, *color_textures):
        return self._add_nodelist("color_textures", *color_textures)

    def set_depth_texture(self, _Node depth_texture):
        return ngl_node_param_set(self.ctx, "depth_texture", depth_texture.ctx)

    def set_samples(self, int samples):
        return ngl_node_param_set(self.ctx, "samples", samples)

    def set_clear_color(self, *clear_color):
        cdef float[4] clear_color_c
        cdef int clear_color_i
        if len(clear_color) != 4:
            raise TypeError("%s parameter is expected to be vec%d but got %d values" % (
                            "clear_color", 4, len(clear_color)))
        for clear_color_i in range(4):
            clear_color_c[clear_color_i] = clear_color[clear_color_i]

        return ngl_node_param_set(self.ctx, "clear_color", clear_color_c)

    def set_features(self, const char * features):
        return ngl_node_param_set(self.ctx, "features", features)

    def set_vflip(self, bint vflip):
        return ngl_node_param_set(self.ctx, "vflip", vflip)


cdef class Rotate(_Node):

    def __init__(self, _Node child, angle=None, axis=None, anchor=None, anim=None, *args, **kwargs):
        assert child is not None

        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_ROTATE, child.ctx)
        if self.ctx is NULL:
            raise MemoryError()
        _Node._init_params(self, *args, **kwargs)

        if angle is not None:
            self.set_angle(angle)
        if axis is not None:
            self.set_axis(*axis)
        if anchor is not None:
            self.set_anchor(*anchor)
        if anim is not None:
            self.set_anim(anim)

    def set_angle(self, double angle):
        return ngl_node_param_set(self.ctx, "angle", angle)

    def set_axis(self, *axis):
        cdef float[3] axis_c
        cdef int axis_i
        if len(axis) != 3:
            raise TypeError("%s parameter is expected to be vec%d but got %d values" % (
                            "axis", 3, len(axis)))
        for axis_i in range(3):
            axis_c[axis_i] = axis[axis_i]

        return ngl_node_param_set(self.ctx, "axis", axis_c)

    def set_anchor(self, *anchor):
        cdef float[3] anchor_c
        cdef int anchor_i
        if len(anchor) != 3:
            raise TypeError("%s parameter is expected to be vec%d but got %d values" % (
                            "anchor", 3, len(anchor)))
        for anchor_i in range(3):
            anchor_c[anchor_i] = anchor[anchor_i]

        return ngl_node_param_set(self.ctx, "anchor", anchor_c)

    def set_anim(self, _Node anim):
        return ngl_node_param_set(self.ctx, "anim", anim.ctx)


cdef class RotateQuat(_Node):

    def __init__(self, _Node child, quat=None, anchor=None, anim=None, *args, **kwargs):
        assert child is not None

        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_ROTATEQUAT, child.ctx)
        if self.ctx is NULL:
            raise MemoryError()
        _Node._init_params(self, *args, **kwargs)

        if quat is not None:
            self.set_quat(*quat)
        if anchor is not None:
            self.set_anchor(*anchor)
        if anim is not None:
            self.set_anim(anim)

    def set_quat(self, *quat):
        cdef float[4] quat_c
        cdef int quat_i
        if len(quat) != 4:
            raise TypeError("%s parameter is expected to be vec%d but got %d values" % (
                            "quat", 4, len(quat)))
        for quat_i in range(4):
            quat_c[quat_i] = quat[quat_i]

        return ngl_node_param_set(self.ctx, "quat", quat_c)

    def set_anchor(self, *anchor):
        cdef float[3] anchor_c
        cdef int anchor_i
        if len(anchor) != 3:
            raise TypeError("%s parameter is expected to be vec%d but got %d values" % (
                            "anchor", 3, len(anchor)))
        for anchor_i in range(3):
            anchor_c[anchor_i] = anchor[anchor_i]

        return ngl_node_param_set(self.ctx, "anchor", anchor_c)

    def set_anim(self, _Node anim):
        return ngl_node_param_set(self.ctx, "anim", anim.ctx)


cdef class Scale(_Node):

    def __init__(self, _Node child, factors=None, anchor=None, anim=None, *args, **kwargs):
        assert child is not None

        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_SCALE, child.ctx)
        if self.ctx is NULL:
            raise MemoryError()
        _Node._init_params(self, *args, **kwargs)

        if factors is not None:
            self.set_factors(*factors)
        if anchor is not None:
            self.set_anchor(*anchor)
        if anim is not None:
            self.set_anim(anim)

    def set_factors(self, *factors):
        cdef float[3] factors_c
        cdef int factors_i
        if len(factors) != 3:
            raise TypeError("%s parameter is expected to be vec%d but got %d values" % (
                            "factors", 3, len(factors)))
        for factors_i in range(3):
            factors_c[factors_i] = factors[factors_i]

        return ngl_node_param_set(self.ctx, "factors", factors_c)

    def set_anchor(self, *anchor):
        cdef float[3] anchor_c
        cdef int anchor_i
        if len(anchor) != 3:
            raise TypeError("%s parameter is expected to be vec%d but got %d values" % (
                            "anchor", 3, len(anchor)))
        for anchor_i in range(3):
            anchor_c[anchor_i] = anchor[anchor_i]

        return ngl_node_param_set(self.ctx, "anchor", anchor_c)

    def set_anim(self, _Node anim):
        return ngl_node_param_set(self.ctx, "anim", anim.ctx)


cdef class Text(_Node):

    def __init__(self, const char *text, fg_color=None, bg_color=None, box_corner=None, box_width=None, box_height=None, padding=None, font_scale=None, valign=None, halign=None, aspect_ratio=None, min_filter=None, mag_filter=None, mipmap_filter=None, *args, **kwargs):
        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_TEXT, text)
        if self.ctx is NULL:
            raise MemoryError()
        _Node._init_params(self, *args, **kwargs)

        if fg_color is not None:
            self.set_fg_color(*fg_color)
        if bg_color is not None:
            self.set_bg_color(*bg_color)
        if box_corner is not None:
            self.set_box_corner(*box_corner)
        if box_width is not None:
            self.set_box_width(*box_width)
        if box_height is not None:
            self.set_box_height(*box_height)
        if padding is not None:
            self.set_padding(padding)
        if font_scale is not None:
            self.set_font_scale(font_scale)
        if valign is not None:
            self.set_valign(valign)
        if halign is not None:
            self.set_halign(halign)
        if aspect_ratio is not None:
            self.set_aspect_ratio(aspect_ratio)
        if min_filter is not None:
            self.set_min_filter(min_filter)
        if mag_filter is not None:
            self.set_mag_filter(mag_filter)
        if mipmap_filter is not None:
            self.set_mipmap_filter(mipmap_filter)

    def set_fg_color(self, *fg_color):
        cdef float[4] fg_color_c
        cdef int fg_color_i
        if len(fg_color) != 4:
            raise TypeError("%s parameter is expected to be vec%d but got %d values" % (
                            "fg_color", 4, len(fg_color)))
        for fg_color_i in range(4):
            fg_color_c[fg_color_i] = fg_color[fg_color_i]

        return ngl_node_param_set(self.ctx, "fg_color", fg_color_c)

    def set_bg_color(self, *bg_color):
        cdef float[4] bg_color_c
        cdef int bg_color_i
        if len(bg_color) != 4:
            raise TypeError("%s parameter is expected to be vec%d but got %d values" % (
                            "bg_color", 4, len(bg_color)))
        for bg_color_i in range(4):
            bg_color_c[bg_color_i] = bg_color[bg_color_i]

        return ngl_node_param_set(self.ctx, "bg_color", bg_color_c)

    def set_box_corner(self, *box_corner):
        cdef float[3] box_corner_c
        cdef int box_corner_i
        if len(box_corner) != 3:
            raise TypeError("%s parameter is expected to be vec%d but got %d values" % (
                            "box_corner", 3, len(box_corner)))
        for box_corner_i in range(3):
            box_corner_c[box_corner_i] = box_corner[box_corner_i]

        return ngl_node_param_set(self.ctx, "box_corner", box_corner_c)

    def set_box_width(self, *box_width):
        cdef float[3] box_width_c
        cdef int box_width_i
        if len(box_width) != 3:
            raise TypeError("%s parameter is expected to be vec%d but got %d values" % (
                            "box_width", 3, len(box_width)))
        for box_width_i in range(3):
            box_width_c[box_width_i] = box_width[box_width_i]

        return ngl_node_param_set(self.ctx, "box_width", box_width_c)

    def set_box_height(self, *box_height):
        cdef float[3] box_height_c
        cdef int box_height_i
        if len(box_height) != 3:
            raise TypeError("%s parameter is expected to be vec%d but got %d values" % (
                            "box_height", 3, len(box_height)))
        for box_height_i in range(3):
            box_height_c[box_height_i] = box_height[box_height_i]

        return ngl_node_param_set(self.ctx, "box_height", box_height_c)

    def set_padding(self, int padding):
        return ngl_node_param_set(self.ctx, "padding", padding)

    def set_font_scale(self, double font_scale):
        return ngl_node_param_set(self.ctx, "font_scale", font_scale)

    def set_valign(self, const char * valign):
        return ngl_node_param_set(self.ctx, "valign", valign)

    def set_halign(self, const char * halign):
        return ngl_node_param_set(self.ctx, "halign", halign)

    def set_aspect_ratio(self, tuple aspect_ratio):
        return ngl_node_param_set(self.ctx,
                                  "aspect_ratio",
                                  <int>aspect_ratio[0],
                                  <int>aspect_ratio[1]);

    def set_min_filter(self, const char * min_filter):
        return ngl_node_param_set(self.ctx, "min_filter", min_filter)

    def set_mag_filter(self, const char * mag_filter):
        return ngl_node_param_set(self.ctx, "mag_filter", mag_filter)

    def set_mipmap_filter(self, const char * mipmap_filter):
        return ngl_node_param_set(self.ctx, "mipmap_filter", mipmap_filter)


cdef class Texture2D(_Node):

    def __init__(self, format=None, width=None, height=None, min_filter=None, mag_filter=None, mipmap_filter=None, wrap_s=None, wrap_t=None, access=None, data_src=None, direct_rendering=None, *args, **kwargs):
        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_TEXTURE2D)
        if self.ctx is NULL:
            raise MemoryError()
        _Node._init_params(self, *args, **kwargs)

        if format is not None:
            self.set_format(format)
        if width is not None:
            self.set_width(width)
        if height is not None:
            self.set_height(height)
        if min_filter is not None:
            self.set_min_filter(min_filter)
        if mag_filter is not None:
            self.set_mag_filter(mag_filter)
        if mipmap_filter is not None:
            self.set_mipmap_filter(mipmap_filter)
        if wrap_s is not None:
            self.set_wrap_s(wrap_s)
        if wrap_t is not None:
            self.set_wrap_t(wrap_t)
        if access is not None:
            self.set_access(access)
        if data_src is not None:
            self.set_data_src(data_src)
        if direct_rendering is not None:
            self.set_direct_rendering(direct_rendering)

    def set_format(self, const char * format):
        return ngl_node_param_set(self.ctx, "format", format)

    def set_width(self, int width):
        return ngl_node_param_set(self.ctx, "width", width)

    def set_height(self, int height):
        return ngl_node_param_set(self.ctx, "height", height)

    def set_min_filter(self, const char * min_filter):
        return ngl_node_param_set(self.ctx, "min_filter", min_filter)

    def set_mag_filter(self, const char * mag_filter):
        return ngl_node_param_set(self.ctx, "mag_filter", mag_filter)

    def set_mipmap_filter(self, const char * mipmap_filter):
        return ngl_node_param_set(self.ctx, "mipmap_filter", mipmap_filter)

    def set_wrap_s(self, const char * wrap_s):
        return ngl_node_param_set(self.ctx, "wrap_s", wrap_s)

    def set_wrap_t(self, const char * wrap_t):
        return ngl_node_param_set(self.ctx, "wrap_t", wrap_t)

    def set_access(self, const char * access):
        return ngl_node_param_set(self.ctx, "access", access)

    def set_data_src(self, _Node data_src):
        return ngl_node_param_set(self.ctx, "data_src", data_src.ctx)

    def set_direct_rendering(self, bint direct_rendering):
        return ngl_node_param_set(self.ctx, "direct_rendering", direct_rendering)


cdef class Texture3D(_Node):

    def __init__(self, format=None, width=None, height=None, depth=None, min_filter=None, mag_filter=None, mipmap_filter=None, wrap_s=None, wrap_t=None, wrap_r=None, access=None, data_src=None, *args, **kwargs):
        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_TEXTURE3D)
        if self.ctx is NULL:
            raise MemoryError()
        _Node._init_params(self, *args, **kwargs)

        if format is not None:
            self.set_format(format)
        if width is not None:
            self.set_width(width)
        if height is not None:
            self.set_height(height)
        if depth is not None:
            self.set_depth(depth)
        if min_filter is not None:
            self.set_min_filter(min_filter)
        if mag_filter is not None:
            self.set_mag_filter(mag_filter)
        if mipmap_filter is not None:
            self.set_mipmap_filter(mipmap_filter)
        if wrap_s is not None:
            self.set_wrap_s(wrap_s)
        if wrap_t is not None:
            self.set_wrap_t(wrap_t)
        if wrap_r is not None:
            self.set_wrap_r(wrap_r)
        if access is not None:
            self.set_access(access)
        if data_src is not None:
            self.set_data_src(data_src)

    def set_format(self, const char * format):
        return ngl_node_param_set(self.ctx, "format", format)

    def set_width(self, int width):
        return ngl_node_param_set(self.ctx, "width", width)

    def set_height(self, int height):
        return ngl_node_param_set(self.ctx, "height", height)

    def set_depth(self, int depth):
        return ngl_node_param_set(self.ctx, "depth", depth)

    def set_min_filter(self, const char * min_filter):
        return ngl_node_param_set(self.ctx, "min_filter", min_filter)

    def set_mag_filter(self, const char * mag_filter):
        return ngl_node_param_set(self.ctx, "mag_filter", mag_filter)

    def set_mipmap_filter(self, const char * mipmap_filter):
        return ngl_node_param_set(self.ctx, "mipmap_filter", mipmap_filter)

    def set_wrap_s(self, const char * wrap_s):
        return ngl_node_param_set(self.ctx, "wrap_s", wrap_s)

    def set_wrap_t(self, const char * wrap_t):
        return ngl_node_param_set(self.ctx, "wrap_t", wrap_t)

    def set_wrap_r(self, const char * wrap_r):
        return ngl_node_param_set(self.ctx, "wrap_r", wrap_r)

    def set_access(self, const char * access):
        return ngl_node_param_set(self.ctx, "access", access)

    def set_data_src(self, _Node data_src):
        return ngl_node_param_set(self.ctx, "data_src", data_src.ctx)


cdef class TextureCube(_Node):

    def __init__(self, format=None, size=None, min_filter=None, mag_filter=None, mipmap_filter=None, wrap_s=None, wrap_t=None, wrap_r=None, access=None, data_src=None, *args, **kwargs):
        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_TEXTURECUBE)
        if self.ctx is NULL:
            raise MemoryError()
        _Node._init_params(self, *args, **kwargs)

        if format is not None:
            self.set_format(format)
        if size is not None:
            self.set_size(size)
        if min_filter is not None:
            self.set_min_filter(min_filter)
        if mag_filter is not None:
            self.set_mag_filter(mag_filter)
        if mipmap_filter is not None:
            self.set_mipmap_filter(mipmap_filter)
        if wrap_s is not None:
            self.set_wrap_s(wrap_s)
        if wrap_t is not None:
            self.set_wrap_t(wrap_t)
        if wrap_r is not None:
            self.set_wrap_r(wrap_r)
        if access is not None:
            self.set_access(access)
        if data_src is not None:
            self.set_data_src(data_src)

    def set_format(self, const char * format):
        return ngl_node_param_set(self.ctx, "format", format)

    def set_size(self, int size):
        return ngl_node_param_set(self.ctx, "size", size)

    def set_min_filter(self, const char * min_filter):
        return ngl_node_param_set(self.ctx, "min_filter", min_filter)

    def set_mag_filter(self, const char * mag_filter):
        return ngl_node_param_set(self.ctx, "mag_filter", mag_filter)

    def set_mipmap_filter(self, const char * mipmap_filter):
        return ngl_node_param_set(self.ctx, "mipmap_filter", mipmap_filter)

    def set_wrap_s(self, const char * wrap_s):
        return ngl_node_param_set(self.ctx, "wrap_s", wrap_s)

    def set_wrap_t(self, const char * wrap_t):
        return ngl_node_param_set(self.ctx, "wrap_t", wrap_t)

    def set_wrap_r(self, const char * wrap_r):
        return ngl_node_param_set(self.ctx, "wrap_r", wrap_r)

    def set_access(self, const char * access):
        return ngl_node_param_set(self.ctx, "access", access)

    def set_data_src(self, _Node data_src):
        return ngl_node_param_set(self.ctx, "data_src", data_src.ctx)


cdef class TimeRangeFilter(_Node):

    def __init__(self, _Node child, ranges=None, prefetch_time=None, max_idle_time=None, *args, **kwargs):
        assert child is not None

        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_TIMERANGEFILTER, child.ctx)
        if self.ctx is NULL:
            raise MemoryError()
        _Node._init_params(self, *args, **kwargs)

        if ranges is not None:
            self.add_ranges(*ranges)
        if prefetch_time is not None:
            self.set_prefetch_time(prefetch_time)
        if max_idle_time is not None:
            self.set_max_idle_time(max_idle_time)

    def add_ranges(self, *ranges):
        return self._add_nodelist("ranges", *ranges)

    def set_prefetch_time(self, double prefetch_time):
        return ngl_node_param_set(self.ctx, "prefetch_time", prefetch_time)

    def set_max_idle_time(self, double max_idle_time):
        return ngl_node_param_set(self.ctx, "max_idle_time", max_idle_time)


cdef class TimeRangeModeCont(_Node):

    def __init__(self, double start_time, *args, **kwargs):
        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_TIMERANGEMODECONT, start_time)
        if self.ctx is NULL:
            raise MemoryError()
        _Node._init_params(self, *args, **kwargs)



cdef class TimeRangeModeNoop(_Node):

    def __init__(self, double start_time, *args, **kwargs):
        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_TIMERANGEMODENOOP, start_time)
        if self.ctx is NULL:
            raise MemoryError()
        _Node._init_params(self, *args, **kwargs)



cdef class TimeRangeModeOnce(_Node):

    def __init__(self, double start_time, double render_time, *args, **kwargs):
        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_TIMERANGEMODEONCE, start_time, render_time)
        if self.ctx is NULL:
            raise MemoryError()
        _Node._init_params(self, *args, **kwargs)



cdef class Transform(_Node):

    def __init__(self, _Node child, matrix=None, *args, **kwargs):
        assert child is not None

        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_TRANSFORM, child.ctx)
        if self.ctx is NULL:
            raise MemoryError()
        _Node._init_params(self, *args, **kwargs)

        if matrix is not None:
            self.set_matrix(*matrix)

    def set_matrix(self, *matrix):
        cdef float[16] matrix_c
        cdef int matrix_i
        if len(matrix) != 16:
            raise TypeError("%s parameter is expected to be vec%d but got %d values" % (
                            "matrix", 16, len(matrix)))
        for matrix_i in range(16):
            matrix_c[matrix_i] = matrix[matrix_i]

        return ngl_node_param_set(self.ctx, "matrix", matrix_c)


cdef class Translate(_Node):

    def __init__(self, _Node child, vector=None, anim=None, *args, **kwargs):
        assert child is not None

        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_TRANSLATE, child.ctx)
        if self.ctx is NULL:
            raise MemoryError()
        _Node._init_params(self, *args, **kwargs)

        if vector is not None:
            self.set_vector(*vector)
        if anim is not None:
            self.set_anim(anim)

    def set_vector(self, *vector):
        cdef float[3] vector_c
        cdef int vector_i
        if len(vector) != 3:
            raise TypeError("%s parameter is expected to be vec%d but got %d values" % (
                            "vector", 3, len(vector)))
        for vector_i in range(3):
            vector_c[vector_i] = vector[vector_i]

        return ngl_node_param_set(self.ctx, "vector", vector_c)

    def set_anim(self, _Node anim):
        return ngl_node_param_set(self.ctx, "anim", anim.ctx)


cdef class Triangle(_Node):

    def __init__(self, edge0, edge1, edge2, uv_edge0=None, uv_edge1=None, uv_edge2=None, *args, **kwargs):
        cdef float[3] edge0_c
        cdef int edge0_i
        if len(edge0) != 3:
            raise TypeError("%s parameter is expected to be vec%d but got %d values" % (
                            "edge0", 3, len(edge0)))
        for edge0_i in range(3):
            edge0_c[edge0_i] = edge0[edge0_i]

        cdef float[3] edge1_c
        cdef int edge1_i
        if len(edge1) != 3:
            raise TypeError("%s parameter is expected to be vec%d but got %d values" % (
                            "edge1", 3, len(edge1)))
        for edge1_i in range(3):
            edge1_c[edge1_i] = edge1[edge1_i]

        cdef float[3] edge2_c
        cdef int edge2_i
        if len(edge2) != 3:
            raise TypeError("%s parameter is expected to be vec%d but got %d values" % (
                            "edge2", 3, len(edge2)))
        for edge2_i in range(3):
            edge2_c[edge2_i] = edge2[edge2_i]

        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_TRIANGLE, edge0_c, edge1_c, edge2_c)
        if self.ctx is NULL:
            raise MemoryError()
        _Node._init_params(self, *args, **kwargs)

        if uv_edge0 is not None:
            self.set_uv_edge0(*uv_edge0)
        if uv_edge1 is not None:
            self.set_uv_edge1(*uv_edge1)
        if uv_edge2 is not None:
            self.set_uv_edge2(*uv_edge2)

    def set_uv_edge0(self, *uv_edge0):
        cdef float[2] uv_edge0_c
        cdef int uv_edge0_i
        if len(uv_edge0) != 2:
            raise TypeError("%s parameter is expected to be vec%d but got %d values" % (
                            "uv_edge0", 2, len(uv_edge0)))
        for uv_edge0_i in range(2):
            uv_edge0_c[uv_edge0_i] = uv_edge0[uv_edge0_i]

        return ngl_node_param_set(self.ctx, "uv_edge0", uv_edge0_c)

    def set_uv_edge1(self, *uv_edge1):
        cdef float[2] uv_edge1_c
        cdef int uv_edge1_i
        if len(uv_edge1) != 2:
            raise TypeError("%s parameter is expected to be vec%d but got %d values" % (
                            "uv_edge1", 2, len(uv_edge1)))
        for uv_edge1_i in range(2):
            uv_edge1_c[uv_edge1_i] = uv_edge1[uv_edge1_i]

        return ngl_node_param_set(self.ctx, "uv_edge1", uv_edge1_c)

    def set_uv_edge2(self, *uv_edge2):
        cdef float[2] uv_edge2_c
        cdef int uv_edge2_i
        if len(uv_edge2) != 2:
            raise TypeError("%s parameter is expected to be vec%d but got %d values" % (
                            "uv_edge2", 2, len(uv_edge2)))
        for uv_edge2_i in range(2):
            uv_edge2_c[uv_edge2_i] = uv_edge2[uv_edge2_i]

        return ngl_node_param_set(self.ctx, "uv_edge2", uv_edge2_c)


cdef class StreamedInt(_Node):

    def __init__(self, _Node timestamps, _Node buffer, timebase=None, time_anim=None, *args, **kwargs):
        assert timestamps is not None

        assert buffer is not None

        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_STREAMEDINT, timestamps.ctx, buffer.ctx)
        if self.ctx is NULL:
            raise MemoryError()
        _Node._init_params(self, *args, **kwargs)

        if timebase is not None:
            self.set_timebase(timebase)
        if time_anim is not None:
            self.set_time_anim(time_anim)

    def set_timebase(self, tuple timebase):
        return ngl_node_param_set(self.ctx,
                                  "timebase",
                                  <int>timebase[0],
                                  <int>timebase[1]);

    def set_time_anim(self, _Node time_anim):
        return ngl_node_param_set(self.ctx, "time_anim", time_anim.ctx)


cdef class StreamedFloat(_Node):

    def __init__(self, _Node timestamps, _Node buffer, timebase=None, time_anim=None, *args, **kwargs):
        assert timestamps is not None

        assert buffer is not None

        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_STREAMEDFLOAT, timestamps.ctx, buffer.ctx)
        if self.ctx is NULL:
            raise MemoryError()
        _Node._init_params(self, *args, **kwargs)

        if timebase is not None:
            self.set_timebase(timebase)
        if time_anim is not None:
            self.set_time_anim(time_anim)

    def set_timebase(self, tuple timebase):
        return ngl_node_param_set(self.ctx,
                                  "timebase",
                                  <int>timebase[0],
                                  <int>timebase[1]);

    def set_time_anim(self, _Node time_anim):
        return ngl_node_param_set(self.ctx, "time_anim", time_anim.ctx)


cdef class StreamedVec2(_Node):

    def __init__(self, _Node timestamps, _Node buffer, timebase=None, time_anim=None, *args, **kwargs):
        assert timestamps is not None

        assert buffer is not None

        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_STREAMEDVEC2, timestamps.ctx, buffer.ctx)
        if self.ctx is NULL:
            raise MemoryError()
        _Node._init_params(self, *args, **kwargs)

        if timebase is not None:
            self.set_timebase(timebase)
        if time_anim is not None:
            self.set_time_anim(time_anim)

    def set_timebase(self, tuple timebase):
        return ngl_node_param_set(self.ctx,
                                  "timebase",
                                  <int>timebase[0],
                                  <int>timebase[1]);

    def set_time_anim(self, _Node time_anim):
        return ngl_node_param_set(self.ctx, "time_anim", time_anim.ctx)


cdef class StreamedVec3(_Node):

    def __init__(self, _Node timestamps, _Node buffer, timebase=None, time_anim=None, *args, **kwargs):
        assert timestamps is not None

        assert buffer is not None

        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_STREAMEDVEC3, timestamps.ctx, buffer.ctx)
        if self.ctx is NULL:
            raise MemoryError()
        _Node._init_params(self, *args, **kwargs)

        if timebase is not None:
            self.set_timebase(timebase)
        if time_anim is not None:
            self.set_time_anim(time_anim)

    def set_timebase(self, tuple timebase):
        return ngl_node_param_set(self.ctx,
                                  "timebase",
                                  <int>timebase[0],
                                  <int>timebase[1]);

    def set_time_anim(self, _Node time_anim):
        return ngl_node_param_set(self.ctx, "time_anim", time_anim.ctx)


cdef class StreamedVec4(_Node):

    def __init__(self, _Node timestamps, _Node buffer, timebase=None, time_anim=None, *args, **kwargs):
        assert timestamps is not None

        assert buffer is not None

        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_STREAMEDVEC4, timestamps.ctx, buffer.ctx)
        if self.ctx is NULL:
            raise MemoryError()
        _Node._init_params(self, *args, **kwargs)

        if timebase is not None:
            self.set_timebase(timebase)
        if time_anim is not None:
            self.set_time_anim(time_anim)

    def set_timebase(self, tuple timebase):
        return ngl_node_param_set(self.ctx,
                                  "timebase",
                                  <int>timebase[0],
                                  <int>timebase[1]);

    def set_time_anim(self, _Node time_anim):
        return ngl_node_param_set(self.ctx, "time_anim", time_anim.ctx)


cdef class StreamedMat4(_Node):

    def __init__(self, _Node timestamps, _Node buffer, timebase=None, time_anim=None, *args, **kwargs):
        assert timestamps is not None

        assert buffer is not None

        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_STREAMEDMAT4, timestamps.ctx, buffer.ctx)
        if self.ctx is NULL:
            raise MemoryError()
        _Node._init_params(self, *args, **kwargs)

        if timebase is not None:
            self.set_timebase(timebase)
        if time_anim is not None:
            self.set_time_anim(time_anim)

    def set_timebase(self, tuple timebase):
        return ngl_node_param_set(self.ctx,
                                  "timebase",
                                  <int>timebase[0],
                                  <int>timebase[1]);

    def set_time_anim(self, _Node time_anim):
        return ngl_node_param_set(self.ctx, "time_anim", time_anim.ctx)


cdef class StreamedBufferInt(_Node):

    def __init__(self, int count, _Node timestamps, _Node buffer, timebase=None, time_anim=None, *args, **kwargs):
        assert timestamps is not None

        assert buffer is not None

        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_STREAMEDBUFFERINT, count, timestamps.ctx, buffer.ctx)
        if self.ctx is NULL:
            raise MemoryError()
        _Node._init_params(self, *args, **kwargs)

        if timebase is not None:
            self.set_timebase(timebase)
        if time_anim is not None:
            self.set_time_anim(time_anim)

    def set_timebase(self, tuple timebase):
        return ngl_node_param_set(self.ctx,
                                  "timebase",
                                  <int>timebase[0],
                                  <int>timebase[1]);

    def set_time_anim(self, _Node time_anim):
        return ngl_node_param_set(self.ctx, "time_anim", time_anim.ctx)


cdef class StreamedBufferFloat(_Node):

    def __init__(self, int count, _Node timestamps, _Node buffer, timebase=None, time_anim=None, *args, **kwargs):
        assert timestamps is not None

        assert buffer is not None

        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_STREAMEDBUFFERFLOAT, count, timestamps.ctx, buffer.ctx)
        if self.ctx is NULL:
            raise MemoryError()
        _Node._init_params(self, *args, **kwargs)

        if timebase is not None:
            self.set_timebase(timebase)
        if time_anim is not None:
            self.set_time_anim(time_anim)

    def set_timebase(self, tuple timebase):
        return ngl_node_param_set(self.ctx,
                                  "timebase",
                                  <int>timebase[0],
                                  <int>timebase[1]);

    def set_time_anim(self, _Node time_anim):
        return ngl_node_param_set(self.ctx, "time_anim", time_anim.ctx)


cdef class StreamedBufferVec2(_Node):

    def __init__(self, int count, _Node timestamps, _Node buffer, timebase=None, time_anim=None, *args, **kwargs):
        assert timestamps is not None

        assert buffer is not None

        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_STREAMEDBUFFERVEC2, count, timestamps.ctx, buffer.ctx)
        if self.ctx is NULL:
            raise MemoryError()
        _Node._init_params(self, *args, **kwargs)

        if timebase is not None:
            self.set_timebase(timebase)
        if time_anim is not None:
            self.set_time_anim(time_anim)

    def set_timebase(self, tuple timebase):
        return ngl_node_param_set(self.ctx,
                                  "timebase",
                                  <int>timebase[0],
                                  <int>timebase[1]);

    def set_time_anim(self, _Node time_anim):
        return ngl_node_param_set(self.ctx, "time_anim", time_anim.ctx)


cdef class StreamedBufferVec3(_Node):

    def __init__(self, int count, _Node timestamps, _Node buffer, timebase=None, time_anim=None, *args, **kwargs):
        assert timestamps is not None

        assert buffer is not None

        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_STREAMEDBUFFERVEC3, count, timestamps.ctx, buffer.ctx)
        if self.ctx is NULL:
            raise MemoryError()
        _Node._init_params(self, *args, **kwargs)

        if timebase is not None:
            self.set_timebase(timebase)
        if time_anim is not None:
            self.set_time_anim(time_anim)

    def set_timebase(self, tuple timebase):
        return ngl_node_param_set(self.ctx,
                                  "timebase",
                                  <int>timebase[0],
                                  <int>timebase[1]);

    def set_time_anim(self, _Node time_anim):
        return ngl_node_param_set(self.ctx, "time_anim", time_anim.ctx)


cdef class StreamedBufferVec4(_Node):

    def __init__(self, int count, _Node timestamps, _Node buffer, timebase=None, time_anim=None, *args, **kwargs):
        assert timestamps is not None

        assert buffer is not None

        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_STREAMEDBUFFERVEC4, count, timestamps.ctx, buffer.ctx)
        if self.ctx is NULL:
            raise MemoryError()
        _Node._init_params(self, *args, **kwargs)

        if timebase is not None:
            self.set_timebase(timebase)
        if time_anim is not None:
            self.set_time_anim(time_anim)

    def set_timebase(self, tuple timebase):
        return ngl_node_param_set(self.ctx,
                                  "timebase",
                                  <int>timebase[0],
                                  <int>timebase[1]);

    def set_time_anim(self, _Node time_anim):
        return ngl_node_param_set(self.ctx, "time_anim", time_anim.ctx)


cdef class StreamedBufferMat4(_Node):

    def __init__(self, int count, _Node timestamps, _Node buffer, timebase=None, time_anim=None, *args, **kwargs):
        assert timestamps is not None

        assert buffer is not None

        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_STREAMEDBUFFERMAT4, count, timestamps.ctx, buffer.ctx)
        if self.ctx is NULL:
            raise MemoryError()
        _Node._init_params(self, *args, **kwargs)

        if timebase is not None:
            self.set_timebase(timebase)
        if time_anim is not None:
            self.set_time_anim(time_anim)

    def set_timebase(self, tuple timebase):
        return ngl_node_param_set(self.ctx,
                                  "timebase",
                                  <int>timebase[0],
                                  <int>timebase[1]);

    def set_time_anim(self, _Node time_anim):
        return ngl_node_param_set(self.ctx, "time_anim", time_anim.ctx)


cdef class UniformInt(_Node):

    def __init__(self, value=None, *args, **kwargs):
        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_UNIFORMINT)
        if self.ctx is NULL:
            raise MemoryError()
        _Node._init_params(self, *args, **kwargs)

        if value is not None:
            self.set_value(value)

    def set_value(self, int value):
        return ngl_node_param_set(self.ctx, "value", value)


cdef class UniformMat4(_Node):

    def __init__(self, value=None, transform=None, *args, **kwargs):
        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_UNIFORMMAT4)
        if self.ctx is NULL:
            raise MemoryError()
        _Node._init_params(self, *args, **kwargs)

        if value is not None:
            self.set_value(*value)
        if transform is not None:
            self.set_transform(transform)

    def set_value(self, *value):
        cdef float[16] value_c
        cdef int value_i
        if len(value) != 16:
            raise TypeError("%s parameter is expected to be vec%d but got %d values" % (
                            "value", 16, len(value)))
        for value_i in range(16):
            value_c[value_i] = value[value_i]

        return ngl_node_param_set(self.ctx, "value", value_c)

    def set_transform(self, _Node transform):
        return ngl_node_param_set(self.ctx, "transform", transform.ctx)


cdef class UniformFloat(_Node):

    def __init__(self, value=None, *args, **kwargs):
        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_UNIFORMFLOAT)
        if self.ctx is NULL:
            raise MemoryError()
        _Node._init_params(self, *args, **kwargs)

        if value is not None:
            self.set_value(value)

    def set_value(self, double value):
        return ngl_node_param_set(self.ctx, "value", value)


cdef class UniformVec2(_Node):

    def __init__(self, value=None, *args, **kwargs):
        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_UNIFORMVEC2)
        if self.ctx is NULL:
            raise MemoryError()
        _Node._init_params(self, *args, **kwargs)

        if value is not None:
            self.set_value(*value)

    def set_value(self, *value):
        cdef float[2] value_c
        cdef int value_i
        if len(value) != 2:
            raise TypeError("%s parameter is expected to be vec%d but got %d values" % (
                            "value", 2, len(value)))
        for value_i in range(2):
            value_c[value_i] = value[value_i]

        return ngl_node_param_set(self.ctx, "value", value_c)


cdef class UniformVec3(_Node):

    def __init__(self, value=None, *args, **kwargs):
        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_UNIFORMVEC3)
        if self.ctx is NULL:
            raise MemoryError()
        _Node._init_params(self, *args, **kwargs)

        if value is not None:
            self.set_value(*value)

    def set_value(self, *value):
        cdef float[3] value_c
        cdef int value_i
        if len(value) != 3:
            raise TypeError("%s parameter is expected to be vec%d but got %d values" % (
                            "value", 3, len(value)))
        for value_i in range(3):
            value_c[value_i] = value[value_i]

        return ngl_node_param_set(self.ctx, "value", value_c)


cdef class UniformVec4(_Node):

    def __init__(self, value=None, *args, **kwargs):
        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_UNIFORMVEC4)
        if self.ctx is NULL:
            raise MemoryError()
        _Node._init_params(self, *args, **kwargs)

        if value is not None:
            self.set_value(*value)

    def set_value(self, *value):
        cdef float[4] value_c
        cdef int value_i
        if len(value) != 4:
            raise TypeError("%s parameter is expected to be vec%d but got %d values" % (
                            "value", 4, len(value)))
        for value_i in range(4):
            value_c[value_i] = value[value_i]

        return ngl_node_param_set(self.ctx, "value", value_c)


cdef class UniformQuat(_Node):

    def __init__(self, value=None, as_mat4=None, *args, **kwargs):
        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_UNIFORMQUAT)
        if self.ctx is NULL:
            raise MemoryError()
        _Node._init_params(self, *args, **kwargs)

        if value is not None:
            self.set_value(*value)
        if as_mat4 is not None:
            self.set_as_mat4(as_mat4)

    def set_value(self, *value):
        cdef float[4] value_c
        cdef int value_i
        if len(value) != 4:
            raise TypeError("%s parameter is expected to be vec%d but got %d values" % (
                            "value", 4, len(value)))
        for value_i in range(4):
            value_c[value_i] = value[value_i]

        return ngl_node_param_set(self.ctx, "value", value_c)

    def set_as_mat4(self, bint as_mat4):
        return ngl_node_param_set(self.ctx, "as_mat4", as_mat4)


cdef class UserSwitch(_Node):

    def __init__(self, _Node child, enabled=None, *args, **kwargs):
        assert child is not None

        assert self.ctx is NULL
        self.ctx = ngl_node_create(NGL_NODE_USERSWITCH, child.ctx)
        if self.ctx is NULL:
            raise MemoryError()
        _Node._init_params(self, *args, **kwargs)

        if enabled is not None:
            self.set_enabled(enabled)

    def set_enabled(self, bint enabled):
        return ngl_node_param_set(self.ctx, "enabled", enabled)

