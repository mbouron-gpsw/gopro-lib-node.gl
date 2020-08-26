/*
 * Copyright 2016 GoPro Inc.
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "NGL.h"
#include "DebugUtil.h"
#include "MathUtils.h"
#include <functional>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/type_ptr.hpp>
using namespace NGL;

typedef double easing_type;
typedef easing_type (*easing_function)(easing_type, int, const easing_type *);

#define TRANSFORM_IN(function) function(x, args_nb, args)
#define TRANSFORM_OUT(function) (1.0 - function(1.0 - x, args_nb, args))
#define TRANSFORM_IN_OUT(function) (x < 0.5 ? function(2.0 * x, args_nb, args) / 2.0 \
                                            : 1.0 - function(2.0 * (1.0 - x), args_nb, args) / 2.0)
#define TRANSFORM_OUT_IN(function) (x < 0.5 ? (1.0 - function(1.0 - 2.0 * x, args_nb, args)) / 2.0 \
                                            : (1.0 + function(2.0 * x - 1.0, args_nb, args)) / 2.0)

#define DECLARE_EASING(base_name, name, transform)                            \
static easing_type name(easing_type x, int args_nb, const easing_type *args)  \
{                                                                             \
    return transform(base_name##_helper);                                     \
}

#define DECLARE_HELPER(base_name, formula)                                                        \
static inline easing_type base_name##_helper(easing_type x, int args_nb, const easing_type *args) \
{                                                                                                 \
    return formula;                                                                               \
}

#define DECLARE_EASINGS(base_name, suffix, formula) \
DECLARE_HELPER(base_name##suffix, formula) \
DECLARE_EASING(base_name##suffix, base_name##_in##suffix,       TRANSFORM_IN)       \
DECLARE_EASING(base_name##suffix, base_name##_out##suffix,      TRANSFORM_OUT)      \
DECLARE_EASING(base_name##suffix, base_name##_in_out##suffix,   TRANSFORM_IN_OUT)   \
DECLARE_EASING(base_name##suffix, base_name##_out_in##suffix,   TRANSFORM_OUT_IN)

#define DECLARE_EASINGS_WITH_RESOLUTIONS(base_name, direct_function, resolution_function)   \
DECLARE_EASINGS(base_name,            , direct_function)                                    \
DECLARE_EASINGS(base_name, _resolution, resolution_function)

#define DEFAULT_PARAMETER(index, default_value) (args_nb > index ? args[index] : default_value)

/* Linear */

static easing_type linear(easing_type t, int args_nb, const easing_type *args)
{
    return t;
}

static easing_type linear_resolution(easing_type v, int args_nb, const easing_type *args)
{
    return v;
}

DECLARE_EASINGS_WITH_RESOLUTIONS(quadratic, x * x ,             sqrt(x))
DECLARE_EASINGS_WITH_RESOLUTIONS(cubic,     x * x * x,          pow(x, 1.0 / 3.0))
DECLARE_EASINGS_WITH_RESOLUTIONS(quartic,   x * x * x * x,      pow(x, 1.0 / 4.0))
DECLARE_EASINGS_WITH_RESOLUTIONS(quintic,   x * x * x * x * x,  pow(x, 1.0 / 5.0))

DECLARE_EASINGS_WITH_RESOLUTIONS(power, pow(x, DEFAULT_PARAMETER(0, 1.0)), pow(x, 1.0 / DEFAULT_PARAMETER(0, 1.0)))

DECLARE_EASINGS_WITH_RESOLUTIONS(sinus, 1.0 - cos(x * M_PI / 2.0), acos(1.0 - x) / M_PI * 2.0)
DECLARE_EASINGS_WITH_RESOLUTIONS(circular, 1.0 - sqrt(1.0 - x * x), sqrt(x*(2.0 - x)))


/* Exponential */

static inline easing_type exp_func(easing_type x, easing_type exp_base)
{
    return (pow(exp_base, x) - 1.0) / (exp_base - 1.0);
}

static inline easing_type exp_resolution_func(easing_type x, easing_type exp_base)
{
    return log2(x * (exp_base - 1.0) + 1.0) / log2(exp_base);
}

DECLARE_EASINGS_WITH_RESOLUTIONS(exp, exp_func(x, DEFAULT_PARAMETER(0, 1024.0)), exp_resolution_func(x, DEFAULT_PARAMETER(0, 1024.0)))


/* Bounce */

static easing_type bounce_helper(easing_type t, easing_type c, easing_type a)
{
    if (t == 1.0) {
        return c;
    } else if (t < 4.0 / 11.0) {
        return c * (7.5625 * t * t);
    } else if (t < 8.0 / 11.0) {
        t -= 6.0 / 11.0;
        return -a * (1.0 - (7.5625 * t * t + 0.75)) + c;
    } else if (t < 10.0 / 11.0) {
        t -= 9.0 / 11.0;
        return -a * (1.0 - (7.5625 * t * t + 0.9375)) + c;
    } else {
        t -= 21.0 / 22.0;
        return -a * (1.0 - (7.5625 * t * t + 0.984375)) + c;
    }
}

static easing_type bounce_in(easing_type t, int args_nb, const easing_type *args)
{
    const easing_type a = DEFAULT_PARAMETER(0, 1.70158);
    return 1.0 - bounce_helper(1.0 - t, 1.0, a);
}

static easing_type bounce_out(easing_type t, int args_nb, const easing_type *args)
{
    const easing_type a = DEFAULT_PARAMETER(0, 1.70158);
    return bounce_helper(t, 1.0, a);
}


/* Elastic */

static easing_type elastic_in_helper(easing_type t, easing_type b, easing_type c, easing_type d, easing_type a, easing_type p)
{
    if (t == 0.0)
        return b;
    easing_type t_adj = t / d;
    if (t_adj == 1.0)
        return b + c;
    easing_type s;
    if (a < fabs(c)) {
        a = c;
        s = p / 4.0;
    } else {
        s = p / (2.0 * M_PI) * asin(c / a);
    }
    t_adj -= 1.0;
    return -(a * exp2(10.0 * t_adj) * sin((t_adj * d - s) * (2.0 * M_PI) / p)) + b;
}

static easing_type elastic_in(easing_type t, int args_nb, const easing_type *args)
{
    const easing_type amplitude = DEFAULT_PARAMETER(0, 0.1);
    const easing_type period    = DEFAULT_PARAMETER(1, 0.25);
    return elastic_in_helper(t, 0.0, 1.0, 1.0, amplitude, period);
}

static easing_type elastic_out_helper(easing_type t, easing_type b, easing_type c, easing_type d, easing_type a, easing_type p)
{
    if (t <= 0.0)
        return 0.0;
    if (t >= 1.0)
        return c;
    easing_type s;
    if (a < c) {
        a = c;
        s = p / 4.0;
    } else {
        s = p / (2.0 * M_PI) * asin(c / a);
    }
    return a * exp2(-10.0 * t) * sin((t - s) * (2 * M_PI) / p) + c;
}

static easing_type elastic_out(easing_type t, int args_nb, const easing_type *args)
{
    const easing_type amplitude = DEFAULT_PARAMETER(0, 0.1);
    const easing_type period    = DEFAULT_PARAMETER(1, 0.25);
    return elastic_out_helper(t, 0.0, 1.0, 1.0, amplitude, period);
}

/* Back */

static easing_type back_in(easing_type t, int args_nb, const easing_type *args)
{
    const easing_type s = DEFAULT_PARAMETER(0, 1.70158);
    return t * t * ((s + 1.0) * t - s);
}

static easing_type back_out(easing_type t, int args_nb, const easing_type *args)
{
    const easing_type s = DEFAULT_PARAMETER(0, 1.70158);
    t -= 1.0;
    return t * t * ((s + 1.0) * t + s) + 1.0;
}

static easing_type back_in_out(easing_type t, int args_nb, const easing_type *args)
{
    const easing_type s = DEFAULT_PARAMETER(0, 1.70158) * 1.525;
    t *= 2.0;
    if (t < 1.0)
        return t * t * ((s + 1.0) * t - s) / 2.0;
    t -= 2.0;
    return (t * t * ((s + 1.0) * t + s) + 2.0) / 2.0;
}

static easing_type back_out_in(easing_type t, int args_nb, const easing_type *args)
{
    if (t < 0.5)
        return back_out(2.0 * t, args_nb, args) / 2.0;
    return (back_in(2.0 * t - 1.0, args_nb, args) + 1.0) / 2.0;
}

static const struct {
    easing_function function;
    easing_function resolution;
} easings[] = {
    {linear,                 linear_resolution},
    {quadratic_in,           quadratic_in_resolution},
    {quadratic_out,          quadratic_out_resolution},
    {quadratic_in_out,       quadratic_in_out_resolution},
    {quadratic_out_in,       quadratic_out_in_resolution},
    {cubic_in,               cubic_in_resolution},
    {cubic_out,              cubic_out_resolution},
    {cubic_in_out,           cubic_in_out_resolution},
    {cubic_out_in,           cubic_out_in_resolution},
    {quartic_in,             quartic_in_resolution},
    {quartic_out,            quartic_out_resolution},
    {quartic_in_out,         quartic_in_out_resolution},
    {quartic_out_in,         quartic_out_in_resolution},
    {quintic_in,             quintic_in_resolution},
    {quintic_out,            quintic_out_resolution},
    {quintic_in_out,         quintic_in_out_resolution},
    {quintic_out_in,         quintic_out_in_resolution},
    {power_in,               power_in_resolution},
    {power_out,              power_out_resolution},
    {power_in_out,           power_in_out_resolution},
    {power_out_in,           power_out_in_resolution},
    {sinus_in,               sinus_in_resolution},
    {sinus_out,              sinus_out_resolution},
    {sinus_in_out,           sinus_in_out_resolution},
    {sinus_out_in,           sinus_out_in_resolution},
    {exp_in,                 exp_in_resolution},
    {exp_out,                exp_out_resolution},
    {exp_in_out,             exp_in_out_resolution},
    {exp_out_in,             exp_out_in_resolution},
    {circular_in,            circular_in_resolution},
    {circular_out,           circular_out_resolution},
    {circular_in_out,        circular_in_out_resolution},
    {circular_out_in,        circular_out_in_resolution},
    {bounce_in,              NULL},
    {bounce_out,             NULL},
    {elastic_in,             NULL},
    {elastic_out,            NULL},
    {back_in,                NULL},
    {back_out,               NULL},
    {back_in_out,            NULL},
    {back_out_in,            NULL},
};

int AnimationUtil::solveEasing(EasingId easingId, double *args, int nb_args,
                     double *offsets, double v, double *t)
{
    if (!easings[easingId].resolution) {
        LOG("ERROR: no resolution available for easing %x", easingId);
        return -1;
    }
    if (offsets) {
        const easing_function eval_func = easings[easingId].function;
        const double start_value = eval_func(offsets[0], nb_args, args);
        const double end_value   = eval_func(offsets[1], nb_args, args);
        v = glm::mix(start_value, end_value, v);
    }
    double time = easings[easingId].resolution(v, nb_args, args);
    if (offsets)
        time = (time - offsets[0]) / (offsets[1] - offsets[0]);
    *t = time;
    return 0;
}


int AnimationUtil::evaluateEasing(EasingId easingId, double* args, int nb_args, double* offsets, double t, double* v) {
    if (offsets)
        t = glm::mix(offsets[0], offsets[1], t);
    const easing_function eval_func = easings[easingId].function;
    double value = eval_func(t, nb_args, args);
    if (offsets) {
        const double start_value = eval_func(offsets[0], nb_args, args);
        const double end_value   = eval_func(offsets[1], nb_args, args);
        value = (value - start_value) / (end_value - start_value);
    }
    *v = value;
    return 0;
}

void AnimKeyFrame::initOnce() {
    if (isInitialized) return;
    easingFunction   = easings[easingId].function;
    easingResolution = easings[easingId].resolution;

    if (easingOffsets[0] || easingOffsets[1] != 1.0) {
        scaleBoundaries = 1;
        boundaries[0] = easingFunction(easingOffsets[0], easingArgs.size(), easingArgs.data());
        boundaries[1] = easingFunction(easingOffsets[1], easingArgs.size(), easingArgs.data());
    }
    isInitialized = true;
}

#define COS_ALPHA_THRESHOLD 0.9995f
static auto mixQuatFn = [](const quat& q1, const quat& q2, float a) -> quat {
    quat dst;
    ngli_quat_slerp(glm::value_ptr(dst), glm::value_ptr(q1), glm::value_ptr(q2), a);
    return dst;
};

static sp<Data> makeData(void* data, uint32_t size) {
    auto r = make_shared<Data>(size);
    memcpy(r->v, data, size);
    return r;
}

#define MIX_DATA(T) \
    uint32_t count = d0->size / sizeof(T); \
    T* v0 = (T*)d0->v; \
    T* v1 = (T*)d1->v; \
    vector<T> r(count); \
    for (uint32_t j = 0; j<count; j++) { \
        r[j] = mix(*v0++, *v1++, a); \
    } \
    return makeData(r.data(), r.size() * sizeof(r[0]))

static auto mixDataFn = [](sp<Data>& d0, sp<Data> &d1, float a, DataType dataType) -> sp<Data> {
    if (dataType == DataType::FLOAT) { MIX_DATA(float); }
    else if (dataType == DataType::INT) { MIX_DATA(int); }
    else if (dataType == DataType::VEC2) { MIX_DATA(vec2); }
    else if (dataType == DataType::VEC3) { MIX_DATA(vec3); }
    else if (dataType == DataType::VEC4) { MIX_DATA(vec4); }
    else return nullptr;
};

#define DEFINE_EVALUATE_ANIMATED(AnimatedType, mixFn) \
void evaluate##AnimatedType(AnimatedType* s, float t) { \
    auto get_kf_id = [&]() -> int { \
        int ret = -1; \
        for (uint32_t i = 0; i < s->kf.size(); i++) { \
            auto keyFrame = s->kf[i]; \
            if (keyFrame->t > t) \
                break; \
            ret = i; \
        } \
        return ret; \
    }; \
    if (s->kf.size() == 0) return; \
    for (auto& kf0 : s->kf) kf0->initOnce(); \
    uint32_t kf_id = get_kf_id(); \
    if (kf_id >= 0 && kf_id < s->kf.size() - 1) { \
        auto kf0 = s->kf[kf_id]; \
        auto kf1 = s->kf[kf_id + 1]; \
        const float t0 = kf0->t; \
        const float t1 = kf1->t; \
        double tnorm = (t - t0) / (t1 - t0); \
        if (kf1->scaleBoundaries) \
            tnorm = (kf1->easingOffsets[1] - kf1->easingOffsets[0]) * tnorm + kf1->easingOffsets[0]; \
        double ratio = kf1->easingFunction(tnorm, kf1->easingArgs.size(), kf1->easingArgs.data()); \
        if (kf1->scaleBoundaries) \
            ratio = (ratio - kf1->boundaries[0]) / (kf1->boundaries[1] - kf1->boundaries[0]); \
        s->value = mixFn; \
    } else { \
        auto kf0 = s->kf[0]; \
        auto kfn = s->kf[s->kf.size() - 1]; \
        auto kf  = t < kf0->t ? kf0 : kfn; \
        s->value = kf->v; \
    } \
}

DEFINE_EVALUATE_ANIMATED(AnimatedBuffer, mixDataFn(kf0->v, kf1->v, ratio, s->dataType))
DEFINE_EVALUATE_ANIMATED(AnimatedInt, mix(kf0->v, kf1->v, ratio))
DEFINE_EVALUATE_ANIMATED(AnimatedFloat, mix(kf0->v, kf1->v, ratio))
DEFINE_EVALUATE_ANIMATED(AnimatedTime, mix(kf0->v, kf1->v, ratio))
DEFINE_EVALUATE_ANIMATED(AnimatedVec2, mix(kf0->v, kf1->v, ratio))
DEFINE_EVALUATE_ANIMATED(AnimatedVec3, mix(kf0->v, kf1->v, ratio))
DEFINE_EVALUATE_ANIMATED(AnimatedVec4, mix(kf0->v, kf1->v, ratio))
DEFINE_EVALUATE_ANIMATED(AnimatedQuat, mixQuatFn(kf0->v, kf1->v, ratio))

void AnimatedBuffer::evaluate(float t) {
    evaluateAnimatedBuffer(this, t);
}
void AnimatedInt::evaluate(float t) {
    evaluateAnimatedInt(this, t);
}
void AnimatedTime::evaluate(float t) {
    evaluateAnimatedTime(this, t);
}
void AnimatedFloat::evaluate(float t) {
    evaluateAnimatedFloat(this, t);
}
void AnimatedVec2::evaluate(float t) {
    evaluateAnimatedVec2(this, t);
}
void AnimatedVec3::evaluate(float t) {
    evaluateAnimatedVec3(this, t);
}
void AnimatedVec4::evaluate(float t) {
    evaluateAnimatedVec4(this, t);
}
void AnimatedQuat::evaluate(float t) {
    evaluateAnimatedQuat(this, t);
}
