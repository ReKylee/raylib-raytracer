#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

out vec4 finalColor;

uniform vec2 iResolution;
uniform float iTime;

uniform vec3 uCameraPosition;
uniform vec3 uCameraForward;
uniform vec3 uCameraRight;
uniform vec3 uCameraUp;
uniform float uCameraFovY;

uniform vec3 uAmbientIntensity;

uniform int uSphereCount;
uniform vec4 uSphereData[MAX_SPHERES]; // xyz position, w radius
uniform vec4 uSphereColor[MAX_SPHERES]; // rgb color, w shininess

uniform int uPlaneCount;
uniform vec4 uPlaneData[MAX_PLANES]; // xyz normal, w offset
uniform vec4 uPlaneColor[MAX_PLANES]; // rgb color, w shininess

uniform int uDirLightCount;
uniform vec4 uDirLightDirection[MAX_LIGHTS];
uniform vec4 uDirLightIntensity[MAX_LIGHTS];

uniform int uSpotlightCount;
uniform vec4 uSpotlightPositionCutoff[MAX_LIGHTS]; // xyz position, w cutoff cosine
uniform vec4 uSpotlightDirection[MAX_LIGHTS];
uniform vec4 uSpotlightIntensity[MAX_LIGHTS];

void main() {
    vec2 uv = fragTexCoord * 2.0 - 1.0;
    uv.x *= iResolution.x / iResolution.y;

    vec3 color = 0.5 + 0.5 * cos(iTime + uv.xyx + vec3(0.0, 2.0, 4.0));

    finalColor = vec4(color, 1.0);
}
