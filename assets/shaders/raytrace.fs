#version 330

// Fragment output

out vec4 finalColor;

// Uniforms

uniform vec2 iResolution;
uniform float iTime;

uniform mat4 uCameraToWorld;
uniform vec2 uCameraViewportScale;

uniform vec3 uAmbientIntensity;

uniform int uSphereCount;
uniform vec4 uSphereData[MAX_SPHERES]; // xyz position, w radius
uniform vec4 uSphereColor[MAX_SPHERES]; // rgb color, w shininess

uniform int uPlaneCount;
uniform vec4 uPlaneData[MAX_PLANES]; // xyz normal, w offset
uniform vec4 uPlaneColor[MAX_PLANES]; // rgb color, w shininess

uniform int uDirectionalLightCount;
uniform vec3 uDirectionalLightDirection[MAX_LIGHTS];
uniform vec3 uDirectionalLightIntensity[MAX_LIGHTS];

uniform int uSpotlightCount;
uniform vec3 uSpotlightPosition[MAX_LIGHTS];
uniform vec4 uSpotlightDirectionCutoff[MAX_LIGHTS]; // xyz direction, w cutoff cosine
uniform vec3 uSpotlightIntensity[MAX_LIGHTS];

// Constants

#define EPSILON 0.001
#define CHECKER_A 1.0
#define CHECKER_B 0.5
#define INV_SQUARE_SIZE 2.0
#define FAR_DISTANCE 1e20
#define MAX_REFLECTION_BOUNCES 2
#define REFLECTION_EPSILON 0.001
#define DEFAULT_SPHERE_REFLECTIVITY 0.25
#define DEFAULT_PLANE_REFLECTIVITY 0.0
#define MIN_LIGHT_DISTANCE 0.1

// Types

struct Ray {
    vec3 origin;
    vec3 direction;
};

struct HitRecord {
    bool hit;
    float distance;
    vec3 position;
    vec3 normal;
    bool frontFace;
    vec3 color;
    float shininess;
    float reflectivity;
};

// Hit helpers

HitRecord emptyHit() {
    HitRecord record;
    record.hit = false;
    record.distance = FAR_DISTANCE;
    record.position = vec3(0.0);
    record.normal = vec3(0.0);
    record.frontFace = false;
    record.color = vec3(0.0);
    record.shininess = 0.0;
    record.reflectivity = 0.0;
    return record;
}

void setFaceNormal(Ray ray, vec3 outwardNormal, inout HitRecord record) {
    record.frontFace = dot(ray.direction, outwardNormal) < 0.0;
    record.normal = record.frontFace ? outwardNormal : -outwardNormal;
}

// Camera

Ray makeCameraRay() {
    vec2 fragUv = gl_FragCoord.xy / max(iResolution, vec2(1.0));
    vec2 ndc = fragUv * 2.0 - 1.0;

    vec3 cameraDirection = vec3(ndc * uCameraViewportScale, -1.0);
    vec3 origin = uCameraToWorld[3].xyz;
    vec3 direction = normalize((uCameraToWorld * vec4(cameraDirection, 0.0)).xyz);

    return Ray(origin, direction);
}

// Intersection

void intersectSphere(
    Ray ray,
    vec3 spherePosition,
    float sphereRadius,
    vec4 material,
    float reflectivity,
    inout HitRecord closest
) {
    vec3 originToCenter = ray.origin - spherePosition;

    float halfB = dot(originToCenter, ray.direction);
    float c = dot(originToCenter, originToCenter) - sphereRadius * sphereRadius;
    float discriminant = halfB * halfB - c;

    if (discriminant < 0.0) {
        return;
    }

    float sqrtDiscriminant = sqrt(discriminant);
    float distance = -halfB - sqrtDiscriminant;

    if (distance <= EPSILON) {
        distance = -halfB + sqrtDiscriminant;
    }

    if (distance <= EPSILON || distance >= closest.distance) {
        return;
    }

    vec3 hitPosition = ray.origin + distance * ray.direction;

    closest.hit = true;
    closest.distance = distance;
    closest.position = hitPosition;
    setFaceNormal(ray, (hitPosition - spherePosition) / sphereRadius, closest);
    closest.color = material.xyz;
    closest.shininess = material.w;
    closest.reflectivity = reflectivity;
}

float checkerMultiplierAt(vec3 position, vec3 normal) {
    vec3 absNormal = abs(normal);

    float useYz = step(max(absNormal.y, absNormal.z), absNormal.x);
    float useXz = (1.0 - useYz) * step(absNormal.z, absNormal.y);
    float useXy = 1.0 - useYz - useXz;

    vec2 uv = useYz * position.yz + useXz * position.xz + useXy * position.xy;
    vec2 cell = floor(uv * INV_SQUARE_SIZE);
    float checker = fract(0.5 * (cell.x + cell.y)) * 2.0;

    return mix(CHECKER_A, CHECKER_B, checker);
}

void intersectPlane(
    Ray ray,
    vec3 planeNormal,
    float planeOffset,
    vec4 material,
    float reflectivity,
    inout HitRecord closest
) {
    vec3 normal = normalize(planeNormal);
    float denominator = dot(normal, ray.direction);

    if (abs(denominator) < EPSILON) {
        return;
    }

    float distance = -(dot(normal, ray.origin) + planeOffset) / denominator;

    if (distance <= EPSILON || distance >= closest.distance) {
        return;
    }

    vec3 hitPosition = ray.origin + distance * ray.direction;

    closest.hit = true;
    closest.distance = distance;
    closest.position = hitPosition;
    setFaceNormal(ray, normal, closest);
    closest.color = material.xyz * checkerMultiplierAt(hitPosition, normal);
    closest.shininess = material.w;
    closest.reflectivity = reflectivity;
}

HitRecord intersectScene(Ray ray) {
    HitRecord closest = emptyHit();

    for (int sphereIndex = 0; sphereIndex < uSphereCount; sphereIndex++) {
        vec4 sphereData = uSphereData[sphereIndex];
        vec4 material = uSphereColor[sphereIndex];
        intersectSphere(
            ray,
            sphereData.xyz,
            sphereData.w,
            material,
            DEFAULT_SPHERE_REFLECTIVITY,
            closest
        );
    }

    for (int planeIndex = 0; planeIndex < uPlaneCount; planeIndex++) {
        vec4 planeData = uPlaneData[planeIndex];
        vec4 material = uPlaneColor[planeIndex];
        intersectPlane(
            ray,
            planeData.xyz,
            planeData.w,
            material,
            DEFAULT_PLANE_REFLECTIVITY,
            closest
        );
    }

    return closest;
}

// Shadows

Ray makeShadowRay(vec3 position, vec3 directionToLight) {
    return Ray(position + directionToLight * EPSILON * 2.0, directionToLight);
}

float directionalLightVisibility(vec3 position, vec3 directionToLight) {
    HitRecord shadowHit = intersectScene(makeShadowRay(position, directionToLight));
    return shadowHit.hit ? 0.0 : 1.0;
}

float spotlightVisibility(vec3 position, vec3 directionToLight, float distanceToLight) {
    HitRecord shadowHit = intersectScene(makeShadowRay(position, directionToLight));
    return shadowHit.hit && shadowHit.distance < distanceToLight ? 0.0 : 1.0;
}

float spotlightAttenuation(float distanceToLight) {
    float safeDistance = max(distanceToLight, MIN_LIGHT_DISTANCE);
    return 1.0 / (safeDistance * safeDistance);
}

// Lighting

vec3 phongLightContribution(
    HitRecord record,
    vec3 directionToLight,
    vec3 lightIntensity,
    vec3 directionToCamera,
    float shadowVisibility
) {
    const vec3 specularColor = vec3(0.7);

    float diffuseStrength = max(dot(record.normal, directionToLight), 0.0);
    vec3 reflectedLightDirection = reflect(-directionToLight, record.normal);
    float specularStrength = pow(max(dot(reflectedLightDirection, directionToCamera), 0.0), record.shininess);

    specularStrength *= step(0.0, diffuseStrength);

    return shadowVisibility * lightIntensity *
        (record.color * diffuseStrength + specularColor * specularStrength);
}

vec3 shadeHit(Ray ray, HitRecord record) {
    vec3 shadedColor = record.color * uAmbientIntensity;
    vec3 directionToCamera = normalize(-ray.direction);

    for (int lightIndex = 0; lightIndex < uDirectionalLightCount; lightIndex++) {
        vec3 directionToLight = normalize(-uDirectionalLightDirection[lightIndex]);
        float shadowVisibility = directionalLightVisibility(record.position, directionToLight);

        shadedColor += phongLightContribution(
                record,
                directionToLight,
                uDirectionalLightIntensity[lightIndex],
                directionToCamera,
                shadowVisibility
            );
    }

    for (int lightIndex = 0; lightIndex < uSpotlightCount; lightIndex++) {
        vec3 directionFromLight = record.position - uSpotlightPosition[lightIndex];
        float distanceToLight = length(directionFromLight);
        vec3 directionToLight = -directionFromLight / max(distanceToLight, EPSILON);

        vec3 spotlightDirection = normalize(uSpotlightDirectionCutoff[lightIndex].xyz);
        float spotlightCutoff = uSpotlightDirectionCutoff[lightIndex].w;
        float coneVisibility = step(spotlightCutoff, dot(spotlightDirection, normalize(directionFromLight)));
        float shadowVisibility = spotlightVisibility(record.position, directionToLight, distanceToLight);
        vec3 lightIntensity = uSpotlightIntensity[lightIndex] * coneVisibility * spotlightAttenuation(distanceToLight);

        shadedColor += phongLightContribution(
                record,
                directionToLight,
                lightIntensity,
                directionToCamera,
                shadowVisibility
            );
    }

    return shadedColor;
}

vec3 backgroundColor(Ray ray) {
    float blendFactor = 0.5 * (normalize(ray.direction).y + 1.0);
    return (1.0 - blendFactor) * vec3(1.0) + blendFactor * vec3(0.5, 0.7, 1.0);
}

// Ray tracing

vec3 raytrace(Ray initialRay) {
    Ray ray = initialRay;

    vec3 accumulatedColor = vec3(0.0);
    vec3 reflectionThroughput = vec3(1.0);

    for (int bounce = 0; bounce <= MAX_REFLECTION_BOUNCES; bounce++) {
        HitRecord hit = intersectScene(ray);

        if (!hit.hit) {
            accumulatedColor += reflectionThroughput * backgroundColor(ray);
            break;
        }

        vec3 localColor = shadeHit(ray, hit);
        float reflectivity = clamp(hit.reflectivity, 0.0, 1.0);

        accumulatedColor += reflectionThroughput * (1.0 - reflectivity) * localColor;

        if (reflectivity <= 0.001) {
            break;
        }

        reflectionThroughput *= reflectivity;

        vec3 reflectedDirection = reflect(ray.direction, hit.normal);

        ray.origin = hit.position + hit.normal * REFLECTION_EPSILON;
        ray.direction = normalize(reflectedDirection);
    }

    return accumulatedColor;
}

// Entry point

void main() {
    Ray ray = makeCameraRay();
    vec3 color = raytrace(ray);

    finalColor = vec4(color, 1.0);
}
