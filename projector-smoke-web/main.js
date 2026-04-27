import * as THREE from "three";

const isMobile = matchMedia("(pointer: coarse), (max-width: 720px)").matches;
const renderer = new THREE.WebGLRenderer({ antialias: false, powerPreference: "high-performance" });
let quality = isMobile ? 0 : 1;
function qualityRatio() {
  if (quality === 0) return Math.min(devicePixelRatio, 0.62);
  if (quality === 1) return Math.min(devicePixelRatio, 0.85);
  return Math.min(devicePixelRatio, 1.15);
}
renderer.setPixelRatio(qualityRatio());
renderer.setSize(innerWidth, innerHeight, false);
renderer.outputColorSpace = THREE.SRGBColorSpace;
document.body.appendChild(renderer.domElement);

const scene = new THREE.Scene();
const camera = new THREE.PerspectiveCamera(58, innerWidth / innerHeight, 0.1, 600.0);
let t = 0.0;
let orbit = true;
let mode = 0;

const uniforms = {
  uTime: { value: 0 },
  uResolution: { value: new THREE.Vector2(innerWidth, innerHeight) },
  uMode: { value: 0 },
  uCamPos: { value: new THREE.Vector3() },
  uInvProjection: { value: new THREE.Matrix4() },
  uCamWorld: { value: new THREE.Matrix4() }
};

const vertexShader = `
varying vec2 vUv;
void main() {
  vUv = uv;
  gl_Position = vec4(position.xy, 0.0, 1.0);
}
`;

const fragmentShader = `
precision highp float;

uniform vec2 uResolution;
uniform float uTime;
uniform int uMode;
uniform vec3 uCamPos;
uniform mat4 uInvProjection;
uniform mat4 uCamWorld;
varying vec2 vUv;

const int STEPS = 46;
const float FAR_T = 155.0;
const float STEP_SIZE = FAR_T / float(STEPS);

mat2 rot(float a) {
  float s = sin(a), c = cos(a);
  return mat2(c, -s, s, c);
}

float hash(vec3 p) {
  p = fract(p * 0.3183099 + vec3(0.11, 0.17, 0.23));
  p *= 17.0;
  return fract(p.x * p.y * p.z * (p.x + p.y + p.z));
}

float noise(vec3 p) {
  vec3 i = floor(p);
  vec3 f = fract(p);
  f = f * f * (3.0 - 2.0 * f);
  float n000 = hash(i + vec3(0,0,0));
  float n100 = hash(i + vec3(1,0,0));
  float n010 = hash(i + vec3(0,1,0));
  float n110 = hash(i + vec3(1,1,0));
  float n001 = hash(i + vec3(0,0,1));
  float n101 = hash(i + vec3(1,0,1));
  float n011 = hash(i + vec3(0,1,1));
  float n111 = hash(i + vec3(1,1,1));
  float nx00 = mix(n000, n100, f.x);
  float nx10 = mix(n010, n110, f.x);
  float nx01 = mix(n001, n101, f.x);
  float nx11 = mix(n011, n111, f.x);
  float nxy0 = mix(nx00, nx10, f.y);
  float nxy1 = mix(nx01, nx11, f.y);
  return mix(nxy0, nxy1, f.z);
}

float fbm(vec3 p) {
  float a = 0.5;
  float v = 0.0;
  for (int i = 0; i < 3; ++i) {
    v += noise(p) * a;
    p = p * 2.03 + vec3(7.1, 3.3, 5.7);
    a *= 0.5;
  }
  return v;
}

float fogDensity(vec3 p) {
  // A finite smoke cloud in the middle of the room.
  vec3 q = p / vec3(48.0, 30.0, 48.0);
  float ellipsoid = 1.0 - smoothstep(0.62, 1.05, dot(q, q));
  float base = 0.22 + 0.78 * fbm(p * 0.055 + vec3(0.0, uTime * 0.035, 0.0));
  float wisps = smoothstep(0.23, 0.98, base);
  return ellipsoid * wisps;
}

float pattern(vec2 uv, int id) {
  float u = uv.x;
  float v = uv.y;
  float r = length(uv);
  float a = atan(v, u);
  if (id == 0) {
    float ring = 1.0 - smoothstep(0.045, 0.075, abs(r - 0.52));
    float diag1 = 1.0 - smoothstep(0.035, 0.07, abs(u - v));
    float diag2 = 1.0 - smoothstep(0.035, 0.07, abs(u + v));
    return max(ring, max(diag1, diag2)) * smoothstep(1.0, 0.88, r);
  }
  if (id == 1) {
    float cross = max(1.0 - smoothstep(0.08, 0.14, abs(u)), 1.0 - smoothstep(0.08, 0.14, abs(v)));
    float bx = step(0.45, abs(u)) * step(abs(u), 0.84);
    float by = step(0.45, abs(v)) * step(abs(v), 0.84);
    return max(cross, bx * by);
  }
  if (id == 2) {
    float spiral = smoothstep(0.72, 0.98, abs(sin(5.0 * a + 8.0 * r)));
    return spiral * smoothstep(0.95, 0.78, r);
  }
  float checker = mod(floor((u + 1.0) * 4.0) + floor((v + 1.0) * 4.0), 2.0);
  float board = (1.0 - checker) * step(abs(u), 0.9) * step(abs(v), 0.9);
  float disc = smoothstep(0.31, 0.25, r);
  return max(board * 0.82, disc);
}

vec3 projPos(int id) {
  if (id == 0) return vec3(-78.0, 38.0, 70.0);
  if (id == 1) return vec3(78.0, 34.0, 66.0);
  if (id == 2) return vec3(-72.0, 30.0, -78.0);
  return vec3(82.0, 44.0, -70.0);
}

vec3 projCol(int id) {
  if (id == 0) return vec3(1.0, 0.13, 0.08);
  if (id == 1) return vec3(0.12, 1.0, 0.32);
  if (id == 2) return vec3(0.18, 0.36, 1.0);
  return vec3(1.0, 0.78, 0.12);
}

vec3 projectorLightAt(vec3 p, out float hitCount) {
  vec3 center = vec3(0.0, 16.0, 0.0);
  vec3 sum = vec3(0.0);
  hitCount = 0.0;

  for (int id = 0; id < 4; ++id) {
    vec3 pp = projPos(id);
    vec3 forward = normalize(center - pp);
    vec3 upGuess = abs(dot(forward, vec3(0,1,0))) > 0.92 ? vec3(1,0,0) : vec3(0,1,0);
    vec3 right = normalize(cross(forward, upGuess));
    vec3 up = normalize(cross(right, forward));

    vec3 d = p - pp;
    float z = dot(d, forward);
    if (z <= 2.0) continue;

    // Projector camera mapping. fov decides how wide the projected image cone is.
    float fovScale = 0.42;
    vec2 uv = vec2(dot(d, right), dot(d, up)) / (z * fovScale);
    if (abs(uv.x) > 1.0 || abs(uv.y) > 1.0) continue;

    float img = pattern(uv, id);
    if (img <= 0.001) continue;

    // Beam falloff through air/fog. This is still a simplified single-scattering model,
    // but the light is evaluated at the actual sample point, not drawn as separate geometry.
    float distAtten = 1.0 / (1.0 + 0.0025 * dot(d, d));
    float coneSoft = smoothstep(1.0, 0.78, max(abs(uv.x), abs(uv.y)));
    float beamDust = 0.65 + 0.35 * fbm(p * 0.09 + float(id) * 11.7);
    float contrib = img * coneSoft * distAtten * beamDust;
    sum += projCol(id) * contrib;
    hitCount += smoothstep(0.05, 0.35, contrib);
  }
  return sum;
}

vec3 background(vec3 rd) {
  float y = max(rd.y, 0.0);
  return mix(vec3(0.015, 0.019, 0.030), vec3(0.06, 0.075, 0.11), y);
}

void main() {
  vec2 ndc = vUv * 2.0 - 1.0;
  vec4 viewRay = uInvProjection * vec4(ndc, 1.0, 1.0);
  viewRay.xyz /= viewRay.w;
  vec3 rd = normalize((uCamWorld * vec4(viewRay.xyz, 0.0)).xyz);
  vec3 ro = uCamPos;

  vec3 col = background(rd);
  vec3 accum = vec3(0.0);
  float trans = 1.0;

  // Jitter reduces banding, especially on mobile.
  float jitter = hash(vec3(gl_FragCoord.xy, uTime)) * STEP_SIZE;
  float t0 = jitter;

  for (int i = 0; i < STEPS; ++i) {
    float tt = t0 + float(i) * STEP_SIZE;
    vec3 p = ro + rd * tt;
    float dens = fogDensity(p);
    if (dens > 0.002) {
      float hitCount = 0.0;
      vec3 light = projectorLightAt(p, hitCount);

      // Mode 2 emphasizes places where multiple projector images really overlap in the same smoke sample.
      if (uMode == 1) {
        float multi = smoothstep(1.2, 2.8, hitCount);
        light = mix(light * 0.22, vec3(1.0, 0.86, 0.46) * (0.35 + 1.4 * multi), multi);
      }
      // Mode 3 shows beam pollution / paths.
      if (uMode == 2) {
        light = light * 1.65 + vec3(0.06, 0.09, 0.13);
      }

      float sigma = dens * 0.055;
      vec3 scatter = dens * light * 0.28;
      // Some neutral fog makes the smoke volume visible even between bright projected pixels.
      scatter += dens * vec3(0.018, 0.023, 0.032);
      accum += trans * scatter * STEP_SIZE;
      trans *= exp(-sigma * STEP_SIZE);
      if (trans < 0.025) break;
    }
  }

  col = col * trans + accum;
  col = col / (vec3(1.0) + col); // filmic-ish tonemap
  col = pow(col, vec3(1.0 / 2.2));
  gl_FragColor = vec4(col, 1.0);
}
`;

const material = new THREE.ShaderMaterial({ uniforms, vertexShader, fragmentShader, depthWrite: false, depthTest: false });
const quad = new THREE.Mesh(new THREE.PlaneGeometry(2, 2), material);
quad.frustumCulled = false;
scene.add(quad);

function updateCamera() {
  const yaw = orbit ? t * 0.16 : -0.45;
  camera.position.set(Math.sin(yaw) * 118, 48 + Math.sin(t * 0.28) * 5, Math.cos(yaw) * 118);
  camera.lookAt(0, 17, 0);
  camera.updateMatrixWorld();
  camera.updateProjectionMatrix();
  uniforms.uCamPos.value.copy(camera.position);
  uniforms.uInvProjection.value.copy(camera.projectionMatrixInverse);
  uniforms.uCamWorld.value.copy(camera.matrixWorld);
}

function setMode(nextMode) {
  mode = nextMode;
  uniforms.uMode.value = mode;
  document.querySelectorAll("[data-mode]").forEach((btn) => {
    btn.classList.toggle("active", Number(btn.dataset.mode) === mode);
  });
}

function setQuality(nextQuality) {
  quality = nextQuality;
  renderer.setPixelRatio(qualityRatio());
  renderer.setSize(innerWidth, innerHeight, false);
  uniforms.uResolution.value.set(innerWidth, innerHeight);
  const btn = document.getElementById("qualityBtn");
  btn.textContent = ["Low", "Med", "High"][quality];
  btn.classList.toggle("active", quality > 0);
}

document.querySelectorAll("[data-mode]").forEach((btn) => {
  btn.addEventListener("click", () => setMode(Number(btn.dataset.mode)));
});

document.getElementById("orbitBtn").addEventListener("click", () => {
  orbit = !orbit;
  document.getElementById("orbitBtn").classList.toggle("active", orbit);
});

document.getElementById("qualityBtn").addEventListener("click", () => {
  setQuality((quality + 1) % 3);
});

addEventListener("keydown", (e) => {
  if (e.key === " ") {
    orbit = !orbit;
    document.getElementById("orbitBtn").classList.toggle("active", orbit);
  }
  if (e.key === "1") setMode(0);
  if (e.key === "2") setMode(1);
  if (e.key === "3") setMode(2);
});

addEventListener("resize", () => {
  camera.aspect = innerWidth / innerHeight;
  renderer.setSize(innerWidth, innerHeight, false);
  uniforms.uResolution.value.set(innerWidth, innerHeight);
  updateCamera();
});

let last = performance.now();
function animate(now = performance.now()) {
  requestAnimationFrame(animate);
  if (document.hidden) return;
  const dt = Math.min(0.033, Math.max(0.001, (now - last) / 1000));
  last = now;
  t += dt;
  uniforms.uTime.value = t;
  updateCamera();
  renderer.render(scene, camera);
}
setMode(0);
setQuality(quality);
animate();
