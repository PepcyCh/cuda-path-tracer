#pragma once

#include "../geometry/geometry.cuh"

namespace kernel {

struct AccelHitInfo {
    uint32_t instance_id;
    uint32_t primitive_id;
    glm::vec2 attribs;
    glm::mat4 transform;
    glm::mat4 transform_inv;
};

struct Bbox {
    glm::vec4 pmin;
    glm::vec4 pmax;
};

struct AccelNode {
    uint32_t lc_or_id;
    uint32_t rc;
};

namespace {

CU_DEVICE void Swap(float &a, float &b) {
    float t = a;
    a = b;
    b = t;
}

CU_DEVICE bool BboxIntersect(const glm::vec3 &pmin, const glm::vec3 &pmax, const Ray &ray) {
    auto x0 = (pmin.x - ray.origin.x) / ray.direction.x;
    auto x1 = (pmax.x - ray.origin.x) / ray.direction.x;
    if (x0 > x1) {
        Swap(x0, x1);
    }
    auto y0 = (pmin.y - ray.origin.y) / ray.direction.y;
    auto y1 = (pmax.y - ray.origin.y) / ray.direction.y;
    if (y0 > y1) {
        Swap(y0, y1);
    }
    auto z0 = (pmin.z - ray.origin.z) / ray.direction.z;
    auto z1 = (pmax.z - ray.origin.z) / ray.direction.z;
    if (z0 > z1) {
        Swap(z0, z1);
    }
    auto t0 = fmax(x0, fmax(y0, z0));
    auto t1 = fmin(x1, fmin(y1, z1));
    return t0 <= t1 && t0 < ray.tmax && t1 > ray.tmin;
}

CU_DEVICE Ray TransformRay(const Ray &ray, const glm::mat4 &trans) {
    Ray res = ray;
    res.origin = trans * glm::vec4(res.origin, 1.0f);
    res.direction = trans * glm::vec4(res.direction, 0.0f);
    return res;
}

}

struct AccelBottom {
    AccelNode *nodes;
    Bbox *bboxes;
    Geometry geometry;

    CU_DEVICE bool Intersect(Ray &ray, AccelHitInfo &hit_info) const {
        uint32_t stack[32];
        stack[0] = 0;
        uint32_t sp = 1;
        bool intersected = false;
        while (sp > 0) {
            auto u = stack[--sp];
            if (BboxIntersect(bboxes[u].pmin, bboxes[u].pmax, ray)) {
                if (nodes[u].rc == ~0u) {
                    float t;
                    if (geometry.Intersect(ray, nodes[u].lc_or_id, t, hit_info.attribs)) {
                        ray.tmax = t;
                        hit_info.primitive_id = nodes[u].lc_or_id;
                        intersected = true;
                    }
                } else {
                    stack[sp++] = nodes[u].lc_or_id;
                    stack[sp++] = nodes[u].rc;
                }
            }
        }
        return intersected;
    }

    CU_DEVICE bool Occlude(const Ray &ray) const {
        uint32_t stack[32];
        stack[0] = 0;
        uint32_t sp = 1;
        while (sp > 0) {
            auto u = stack[--sp];
            if (BboxIntersect(bboxes[u].pmin, bboxes[u].pmax, ray)) {
                if (nodes[u].rc == ~0u) {
                    float t;
                    glm::vec2 attribs;
                    if (geometry.Intersect(ray, nodes[u].lc_or_id, t, attribs)) {
                        return true;
                    }
                } else {
                    stack[sp++] = nodes[u].lc_or_id;
                    stack[sp++] = nodes[u].rc;
                }
            }
        }
        return false;
    }
};

struct AccelTop {
    AccelNode *nodes;
    Bbox *bboxes;
    struct Instance {
        AccelBottom *accel;
        glm::mat4 transform;
        glm::mat4 transform_inv;
    } *instances;

    CU_DEVICE bool Intersect(Ray &ray, AccelHitInfo &hit_info) const {
        uint32_t stack[32];
        stack[0] = 0;
        uint32_t sp = 1;
        bool intersected = false;
        while (sp > 0) {
            auto u = stack[--sp];
            if (BboxIntersect(bboxes[u].pmin, bboxes[u].pmax, ray)) {
                if (nodes[u].rc == ~0u) {
                    auto inst_id = nodes[u].lc_or_id;
                    auto &inst = instances[inst_id];
                    auto local_ray = TransformRay(ray, inst.transform_inv);
                    if (inst.accel->Intersect(local_ray, hit_info)) {
                        ray.tmax = local_ray.tmax;
                        hit_info.instance_id = inst_id;
                        hit_info.transform = inst.transform;
                        hit_info.transform_inv = inst.transform_inv;
                        intersected = true;
                    }
                } else {
                    stack[sp++] = nodes[u].lc_or_id;
                    stack[sp++] = nodes[u].rc;
                }
            }
        }
        return intersected;
    }

    CU_DEVICE bool Occlude(const Ray &ray) const {
        uint32_t stack[32];
        stack[0] = 0;
        uint32_t sp = 1;
        while (sp > 0) {
            auto u = stack[--sp];
            if (BboxIntersect(bboxes[u].pmin, bboxes[u].pmax, ray)) {
                if (nodes[u].rc == ~0u) {
                    auto inst_id = nodes[u].lc_or_id;
                    auto &inst = instances[inst_id];
                    auto local_ray = TransformRay(ray, inst.transform_inv);
                    if (inst.accel->Occlude(local_ray)) {
                        return true;
                    }
                } else {
                    stack[sp++] = nodes[u].lc_or_id;
                    stack[sp++] = nodes[u].rc;
                }
            }
        }
        return false;
    }
};

}
