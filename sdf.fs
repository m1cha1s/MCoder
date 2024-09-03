#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

out vec4 finalColor;

void main() {
    //float d = texture(texture0, fragTexCoord).a;
    //float aaf = fwidth(d);
    //float alpha = smoothstep(0.5-aaf, 0.5+aaf, d);

    //finalColor = vec4(fragColor.rgb*alpha, fragColor.a);
    //finalColor = fragColor*alpha;

    float d = texture(texture0, fragTexCoord).r;
    float aaf = fwidth(d);
    float alpha = smoothstep(0.5-aaf, 0.5+aaf, d);

    finalColor.r = fragColor.r * alpha;

    d = texture(texture0, fragTexCoord).g;
    aaf = fwidth(d);
    alpha = smoothstep(0.5-aaf, 0.5+aaf, d);

    finalColor.g = fragColor.g * alpha;

    d = texture(texture0, fragTexCoord).b;
    aaf = fwidth(d);
    alpha = smoothstep(0.5-aaf, 0.5+aaf, d);

    finalColor.b = fragColor.b * alpha;

    d = texture(texture0, fragTexCoord).a;
    aaf = fwidth(d);
    alpha = smoothstep(0.5-aaf, 0.5+aaf, d);

    finalColor.a = fragColor.a * alpha;
}
