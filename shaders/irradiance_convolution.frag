#version 330 core
out vec4 FragColor;
in vec3 localPos;

uniform samplerCube environmentMap;

const float PI = 3.14159265359;

void main() {
    // 큐브맵의 각 픽셀 방향을 법선(N)으로 사용
    vec3 N = normalize(localPos);

    vec3 irradiance = vec3(0.0);

    // 접평면 좌표계(Tangent Space) 구성
    vec3 up    = vec3(0.0, 1.0, 0.0);
    vec3 right = normalize(cross(up, N));
    up         = cross(N, right);

    // 몬테카를로 적분 (샘플링 밀도 조절)
    // sampleDelta가 작을수록 더 정확하지만 느려짐 (0.025 정도면 적당)
    float sampleDelta = 0.025; 
    float nrSamples = 0.0;

    for(float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta) {
        for(float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta) {
            // 구면 좌표를 데카르트 좌표로 변환
            vec3 tangentSample = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
            
            // 접평면을 세계 좌표로 변환
            vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * N;

            // 코사인 가중치 적분: 환경 맵 샘플링 * cos(theta) * sin(theta)
            // sin(theta)는 구면 좌표 적분의 자코비안(Jacobian)입니다.
            irradiance += texture(environmentMap, sampleVec).rgb * cos(theta) * sin(theta);
            nrSamples++;
        }
    }

    irradiance = PI * irradiance * (1.0 / float(nrSamples));

    FragColor = vec4(irradiance, 1.0);
}