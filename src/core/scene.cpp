
/*
    pbrt source code is Copyright(c) 1998-2016
                        Matt Pharr, Greg Humphreys, and Wenzel Jakob.

    This file is part of pbrt.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are
    met:

    - Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

    - Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
    IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
    TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
    PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
    HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
    LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 */


// core/scene.cpp*
#include "scene.h"
#include "stats.h"

#include "accelerators/bvh.h"
#include "shapes/triangle.h"
#include <typeinfo>

namespace pbrt {

STAT_COUNTER("Intersections/Regular ray intersection tests",
             nIntersectionTests);
STAT_COUNTER("Intersections/Shadow ray intersection tests", nShadowTests);

void Scene::DumpPrimitives() {
    //https://stackoverflow.com/questions/1358143/downcasting-shared-ptrbase-to-shared-ptrderived
    std::shared_ptr<BVHAccel> agg = std::static_pointer_cast<BVHAccel>(aggregate);

    Bounds3f bounds = agg->WorldBound();
    //printf("------\n(%f, %f, %f)\n", bounds[0][0], bounds[0][1], bounds[0][2]);
    //printf("(%f, %f, %f)\n------\n", bounds[1][0], bounds[1][1], bounds[1][2]);

    Vector3f dir(0, 0, 1);
    //Vector3f invDir(1.f / dir.x, 1.f / dir.y, 1.f / dir.z);
    //int dirIsNeg[3] = {invDir.x < 0, invDir.y < 0, invDir.z < 0};

    std::vector<std::shared_ptr<Primitive>>& primitives = *(agg->Primitives());
    //printf("primitive type: %s\n", typeid(*primitives[0]).name());
    for (int i = 0; i < primitives.size(); i++) {
      if (primitives[i]->PrimType() == 0) { // is GeometricPrimitive
        std::shared_ptr<GeometricPrimitive> gp = std::static_pointer_cast<GeometricPrimitive>(primitives[i]);
        if (gp->GetShape()->ShapeType() == 1) { // is Triangle
          std::shared_ptr<Triangle> tr = std::static_pointer_cast<Triangle>(gp->GetShape());
          printf("%f, %f, %f\n", tr->GetVertexCoor(0)[0], tr->GetVertexCoor(0)[1], tr->GetVertexCoor(0)[2]);
          //printf("%f, %f, %f\n", tr->GetVertexCoor(1)[0], tr->GetVertexCoor(1)[1], tr->GetVertexCoor(1)[2]);
          //printf("%f, %f, %f\n", tr->GetVertexCoor(2)[0], tr->GetVertexCoor(2)[1], tr->GetVertexCoor(2)[2]);
          Ray ray(tr->GetVertexCoor(0), dir);
          //bounds.IntersectP(ray, invDir, dirIsNeg);
          float h0, h1;
          if (bounds.IntersectP(ray, &h0, &h1)) {
            float h = (h0 == 0) ? h1 : h0;
            printf("%f, %f, %f\n", ray(h)[0], ray(h)[1], ray(h)[2]);
          }
        }
      }
    }
}

// Scene Method Definitions
bool Scene::Intersect(const Ray &ray, SurfaceInteraction *isect) const {
    ++nIntersectionTests;
    DCHECK_NE(ray.d, Vector3f(0,0,0));
    return aggregate->Intersect(ray, isect);
}

bool Scene::IntersectP(const Ray &ray) const {
    ++nShadowTests;
    DCHECK_NE(ray.d, Vector3f(0,0,0));
    return aggregate->IntersectP(ray);
}

bool Scene::IntersectTr(Ray ray, Sampler &sampler, SurfaceInteraction *isect,
                        Spectrum *Tr) const {
    *Tr = Spectrum(1.f);
    while (true) {
        bool hitSurface = Intersect(ray, isect);
        // Accumulate beam transmittance for ray segment
        if (ray.medium) *Tr *= ray.medium->Tr(ray, sampler);

        // Initialize next ray segment or terminate transmittance computation
        if (!hitSurface) return false;
        if (isect->primitive->GetMaterial() != nullptr) return true;
        ray = isect->SpawnRay(ray.d);
    }
}

}  // namespace pbrt
