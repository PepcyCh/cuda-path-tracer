#include "path.cuh"

#include "common.cuh"

namespace kernel {

namespace {

CU_DEVICE glm::vec3 Trace(const PathTracer::Params &params, Ray ray, SamplerState &sampler) {
    AccelHitInfo hit_info;
    if (!params.scene.accel->Intersect(ray, hit_info)) {
        // TODO - envmap
        return glm::vec3(0.0f);
    }
    auto surface = params.scene.instances[hit_info.instance_id].GetShadingSurface(hit_info);

    glm::vec3 color = surface.bsdf.emission;
    glm::vec3 throughput(1.0f);
    for (uint32_t depth = 0; depth < params.max_depth; depth++) {
        Frame frame(surface.vertex.normal);
        auto wo = frame.ToLocal(-ray.direction);

        if (!surface.bsdf.IsDelta()) {
            float light_sample_pdf;
            auto light = params.scene.light_sampler.Sample(surface.vertex.position, sampler.Next1D(), light_sample_pdf);
            auto light_samp = light.Sample(surface.vertex.position, sampler.Next2D());
            light_samp.pdf *= light_sample_pdf;
            light_samp.weight /= light_sample_pdf;
            if (light_samp.pdf > 0.0f) {
                Ray shadow_ray(surface.vertex.position, light_samp.dir);
                shadow_ray.tmax = light_samp.dist - Ray::kShadowRayEps;
                if (!params.scene.accel->Occlude(shadow_ray)) {
                    auto wi = frame.ToLocal(light_samp.dir);
                    float mis_weight = 1.0f;
                    if (!light.IsDelta()) {
                        auto bsdf_pdf = surface.bsdf.Pdf(wo, wi);
                        mis_weight = PowerHeuristic(light_samp.pdf, bsdf_pdf);
                    }
                    color += throughput * mis_weight * light_samp.weight * surface.bsdf.Eval(wo, wi);
                }
            }
        }

        auto bsdf_samp = surface.bsdf.Sample(wo, sampler.Next1D(), sampler.Next2D());
        if (bsdf_samp.pdf == 0.0f) {
            break;
        }
        throughput *= bsdf_samp.weight;
        ray = Ray(surface.vertex.position, frame.ToWorld(bsdf_samp.wi));

        if (!params.scene.accel->Intersect(ray, hit_info)) {
            // TODO - envmap
            break;
        }

        surface = params.scene.instances[hit_info.instance_id].GetShadingSurface(hit_info);
        if (surface.bsdf.emission != glm::vec3(0.0f) && glm::dot(ray.direction, surface.vertex.normal) < 0.0f) {
            float mis_weight = 1.0f;
            if (bsdf_samp.lobe.type != BsdfLobe::Type::eSpecular) {
                const auto &light = params.scene.instances[hit_info.instance_id].light;
                auto light_pdf = surface.vertex.pdf
                    * PathToSolidAngleJacobian(ray.origin, surface.vertex.position, surface.vertex.normal)
                    * params.scene.light_sampler.Pdf(ray.origin, light);
                mis_weight = PowerHeuristic(bsdf_samp.pdf, light_pdf);
            }
            color += throughput * mis_weight * surface.bsdf.emission;
        }

        float rr_prop = glm::clamp(Luminance(throughput), 0.01f, 0.95f);
        if (sampler.Next1D() > rr_prop) {
            break;
        }
        throughput /= rr_prop;
    }

    return color;
}

CU_GLOBAL void RenderKernel(PathTracer::Params params) {
    glm::uvec2 pixel_coord(blockIdx.x * blockDim.x + threadIdx.x, blockIdx.y * blockDim.y + threadIdx.y);
    if (pixel_coord.x >= params.screen_width || pixel_coord.y >= params.screen_height) {
        return;
    }
    auto pixel_index = (params.screen_height - 1 - pixel_coord.y) * params.screen_width + pixel_coord.x;

    auto sampler = SamplerState::Create(pixel_index, params.spp);

    auto subpixel = params.spp == 1 ? glm::vec2(0.5f) : sampler.Next2D();
    auto ray = params.scene.camera.SampleRay(static_cast<float>(params.screen_width) / params.screen_height,
        (glm::vec2(pixel_coord) + subpixel) / glm::vec2(params.screen_width, params.screen_height), sampler.Next2D());

    auto color = Trace(params, ray, sampler);
    if (glm::any(glm::isnan(color)) || glm::any(glm::isinf(color))) {
        color = glm::vec3(0.0f);
    }
    auto prev_color = params.output[pixel_index];
    auto mixed_color = glm::mix(prev_color, glm::vec4(color, 1.0f), 1.0f / params.spp);
    params.output[pixel_index] = mixed_color;
}

}

void PathTracer::Render(const Params &params) {
    dim3 threads(16, 16, 1);
    dim3 grids((params.screen_width + threads.x - 1) / threads.x, (params.screen_height + threads.y - 1) / threads.y);
    RenderKernel<<<grids, threads>>>(params);
    auto r = cudaDeviceSynchronize();
    assert(r == 0);
}

}
