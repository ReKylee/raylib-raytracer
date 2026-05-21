#version 330

#define EPSILON 0.001

in vec4 fragColor;

out vec4 finalColor;

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

uniform int uDirLightCount;
uniform vec3 uDirLightDirection[MAX_LIGHTS];
uniform vec3 uDirLightIntensity[MAX_LIGHTS];

uniform int uSpotlightCount;
uniform vec3 uSpotlightPosition[MAX_LIGHTS];
uniform vec4 uSpotlightDirectionCutoff[MAX_LIGHTS]; // xyz direction, w cutoff cosine
uniform vec3 uSpotlightIntensity[MAX_LIGHTS];

#define CHECKER_A 1.0
#define CHECKER_B 0.5
#define INV_SQUARE_SIZE 2.0

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
    float diffuseMultiplier;
    float shininess;
};
HitRecord emptyHit() {
    HitRecord rec;
    rec.hit = false;
    rec.distance = 1e20;
    rec.position = vec3(0.0);
    rec.normal = vec3(0.0);
    rec.frontFace = false;
    rec.color = vec3(0.0);
    rec.diffuseMultiplier = 0.0;
    rec.shininess = 0.0;
    return rec;
}

void setFaceNormal(Ray ray, vec3 outwardNormal, inout HitRecord rec) {
    rec.frontFace = dot(ray.direction, outwardNormal) < 0.0;
    rec.normal = rec.frontFace ? outwardNormal : -outwardNormal;
}

Ray makeCameraRay() {
    vec2 fragUv = gl_FragCoord.xy / max(iResolution, vec2(1.0));
    vec2 ndc = fragUv * 2.0 - 1.0;

    vec3 cameraDirection = vec3(ndc * uCameraViewportScale, -1.0);
    vec3 origin = uCameraToWorld[3].xyz;
    vec3 direction = normalize((uCameraToWorld * vec4(cameraDirection, 0.0)).xyz);

    return Ray(origin, direction);
}

void intersectSphere(Ray ray, vec3 position, float radius, vec4 mat, inout HitRecord closest) {
    vec3 oc = ray.origin - position;

    float halfB = dot(oc, ray.direction);
    float c = dot(oc, oc) - radius * radius;

    float discriminant = halfB * halfB - c;

    if (discriminant < 0.0) {
        return;
    }

    float sqrtD = sqrt(discriminant);

    float t = -halfB - sqrtD;

    if (t <= EPSILON) {
        t = -halfB + sqrtD;
    }

    if (t <= EPSILON || t >= closest.distance) {
        return;
    }

    vec3 hitPos = ray.origin + t * ray.direction;

    closest.hit = true;
    closest.distance = t;
    closest.position = hitPos;
    setFaceNormal(ray, (hitPos - position) / radius, closest);
    closest.color = mat.xyz;
    closest.shininess = mat.w;
}

float positionToCheckers(vec3 pos, vec3 normal)
{
    vec3 an = abs(normal);

    // Choose projection by dropping the dominant normal axis.
    // useYZ = normal mostly X
    // useXZ = normal mostly Y
    // useXY = normal mostly Z
    float useYZ = step(max(an.y, an.z), an.x);
    float useXZ = (1.0 - useYZ) * step(an.z, an.y);
    float useXY = 1.0 - useYZ - useXZ;

    vec2 uv =
        useYZ * pos.yz +
            useXZ * pos.xz +
            useXY * pos.xy;

    vec2 cell = floor(uv * INV_SQUARE_SIZE);
    float checker = fract(0.5 * (cell.x + cell.y)) * 2.0;

    return mix(CHECKER_A, CHECKER_B, checker);
}

void intersectPlane(Ray ray, vec3 normal, float d, vec4 mat, inout HitRecord closest)
{
    float denom = dot(normal, ray.direction);

    if (abs(denom) < EPSILON) {
        return;
    }

    float t = -(dot(normal, ray.origin) + d) / denom;

    if (t <= EPSILON || t >= closest.distance) {
        return;
    }

    vec3 hitPos = ray.origin + t * ray.direction;
    vec3 n = normalize(normal);

    closest.hit = true;
    closest.distance = t;
    closest.position = hitPos;

    setFaceNormal(ray, n, closest);

    closest.color = mat.xyz * positionToCheckers(hitPos, n);
    closest.shininess = mat.w;
}

HitRecord hit(Ray ray) {
    HitRecord closest = emptyHit();

    for (int s = 0; s < uSphereCount; s++) {
        vec4 posRadius = uSphereData[s];
        vec4 material = uSphereColor[s];
        intersectSphere(ray, posRadius.xyz, posRadius.w, material, closest);
    }
    for (int p = 0; p < uPlaneCount; p++) {
        vec4 normalOffset = uPlaneData[p];
        vec4 material = uPlaneColor[p];
        intersectPlane(ray, normalOffset.xyz, normalOffset.w, material, closest);
    }

    return closest;
}

vec3 light(Ray ray, HitRecord record) {
    return record.color;
}

vec3 backgroundColor(Ray ray) {
    float a = 0.5 * (normalize(ray.direction).y + 1.0);
    return (1.0 - a) * vec3(1.0) + a * vec3(0.5, 0.7, 1.0);
}

vec3 rayColor(Ray ray) {
    HitRecord record = hit(ray);

    if (!record.hit) {
        return backgroundColor(ray);
    }

    return light(ray, record);
}

void main() {
    Ray ray = makeCameraRay();

    vec3 color = rayColor(ray);

    finalColor = vec4(color, 1.0);
}
