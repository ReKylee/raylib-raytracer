#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

out vec4 finalColor;

uniform vec2 iResolution;
uniform float iTime;

uniform mat4 uCameraToWorld;
uniform float uCameraFovY;

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

struct Ray {
    vec3 origin;
    vec3 direction;
};

struct HitRecord {
    bool hit;
    float distance;
    vec3 position;
    vec3 normal;
    vec3 color;
    float shininess;
};

Ray makeCameraRay(vec2 fragUv) {
    vec2 ndc = fragUv * 2.0 - 1.0;
    ndc.x *= iResolution.x / iResolution.y;

    float fovScale = tan(radians(uCameraFovY) * 0.5);
    vec3 cameraDirection = normalize(vec3(
                ndc * fovScale,
                1.0
            ));

    vec3 origin = uCameraToWorld[3].xyz;
    vec3 direction = normalize((uCameraToWorld * vec4(cameraDirection, 0.0)).xyz);

    return Ray(origin, direction);
}

HitRecord hit(Ray ray) {
    HitRecord record;
    record.hit = false;
    record.distance = 0.0;
    record.position = vec3(0.0);
    record.normal = vec3(0.0, 1.0, 0.0);
    record.color = vec3(0.0);
    record.shininess = 0.0;

    return record;
}

vec3 light(Ray ray, HitRecord record) {
    return record.color;
}

vec3 background_color(Ray ray) {
    vec3 a = 0.5 * (ray.direction + 1.0);
    return (1.0 - a) * vec3(1.0, 1.0, 1.0) + a * vec3(0.5, 0.7, 1.0);
}

vec3 ray_color(Ray ray) {
    HitRecord record = hit(ray);

    if (!record.hit) {
        return background_color(ray);
    }

    return light(ray, record);
}

void main() {
    Ray ray = makeCameraRay(fragTexCoord);

    vec3 color = ray_color(ray);

    finalColor = vec4(color, 1.0);
}
