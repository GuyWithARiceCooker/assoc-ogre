// Mobile-first visible demo. The smoke is now a live particle volume: particles
// drift, swirl, respawn, and at their current position receive light from all
// four projectors. Bright regions are true per-smoke-particle light sums.

const canvas = document.createElement('canvas');
const ctx = canvas.getContext('2d', { alpha: false });
document.body.prepend(canvas);

let mode = 0;
let orbit = true;
let quality = matchMedia('(pointer: coarse), (max-width: 720px)').matches ? 0 : 1;
let time = 0;
let last = performance.now();
let cameraAngle = -0.55;
let smoke = [];
let sparseRays = [];
let frameNo = 0;

const smokeCenter = { x: 0, y: 16, z: 0 };
const projectors = [
  { pos: { x: -78, y: 38, z: 70 }, color: [255, 48, 40], id: 0 },
  { pos: { x: 78, y: 34, z: 66 }, color: [50, 255, 120], id: 1 },
  { pos: { x: -72, y: 30, z: -78 }, color: [63, 116, 255], id: 2 },
  { pos: { x: 82, y: 44, z: -70 }, color: [255, 220, 66], id: 3 },
];

function resize() {
  const ratio = quality === 0 ? 0.72 : quality === 1 ? 0.92 : 1.15;
  const dpr = Math.min(devicePixelRatio || 1, ratio);
  canvas.width = Math.max(240, Math.floor(innerWidth * dpr));
  canvas.height = Math.max(320, Math.floor(innerHeight * dpr));
  canvas.style.width = '100vw';
  canvas.style.height = '100vh';
  ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
}
resize();
addEventListener('resize', () => { resize(); seedSmoke(); });

function smoothStep(a, b, x) { const t = Math.max(0, Math.min(1, (x - a) / (b - a))); return t * t * (3 - 2 * t); }
function softBand(x, center, width) { return 1 - smoothStep(width * 0.55, width, Math.abs(x - center)); }
function sub(a, b) { return { x: a.x - b.x, y: a.y - b.y, z: a.z - b.z }; }
function add(a, b) { return { x: a.x + b.x, y: a.y + b.y, z: a.z + b.z }; }
function mul(a, s) { return { x: a.x * s, y: a.y * s, z: a.z * s }; }
function dot(a, b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
function cross(a, b) { return { x: a.y*b.z - a.z*b.y, y: a.z*b.x - a.x*b.z, z: a.x*b.y - a.y*b.x }; }
function norm(a) { const l = Math.hypot(a.x, a.y, a.z) || 1; return mul(a, 1 / l); }
function clamp255(x) { return Math.max(0, Math.min(255, Math.round(x))); }
function rgba(c, a) { return `rgba(${clamp255(c[0])},${clamp255(c[1])},${clamp255(c[2])},${a})`; }

function pattern(u, v, id) {
  const r = Math.hypot(u, v);
  const a = Math.atan2(v, u);
  if (id === 0) return Math.max(softBand(r, 0.52, 0.09), softBand(u - v, 0, 0.095), softBand(u + v, 0, 0.095));
  if (id === 1) {
    const crossShape = Math.max(softBand(u, 0, 0.17), softBand(v, 0, 0.17));
    const blocks = smoothStep(0.45, 0.54, Math.abs(u)) * smoothStep(0.45, 0.54, Math.abs(v)) * (1 - smoothStep(0.84, 0.96, Math.max(Math.abs(u), Math.abs(v))));
    return Math.max(crossShape, blocks);
  }
  if (id === 2) return smoothStep(0.70, 0.95, Math.abs(Math.sin(5.0 * a + 8.0 * r))) * (1 - smoothStep(0.82, 0.98, r));
  const checker = ((Math.floor((u + 1) * 4) + Math.floor((v + 1) * 4)) % 2) === 0 ? 0.78 : 0.0;
  return Math.max(checker * (1 - smoothStep(0.86, 1.0, Math.max(Math.abs(u), Math.abs(v)))), 1 - smoothStep(0.25, 0.34, r));
}

function basis(forward) {
  const f = norm(forward);
  const upGuess = Math.abs(dot(f, { x: 0, y: 1, z: 0 })) > 0.92 ? { x: 1, y: 0, z: 0 } : { x: 0, y: 1, z: 0 };
  const right = norm(cross(f, upGuess));
  const up = norm(cross(right, f));
  return { right, up, forward: f };
}
const projectorFrames = projectors.map((proj) => {
  const forward = norm(sub(smokeCenter, proj.pos));
  return { ...proj, ...basis(forward), lens: add(proj.pos, mul(forward, 7)) };
});

function rotateY(p, a) { const s = Math.sin(a), c = Math.cos(a); return { x: p.x * c - p.z * s, y: p.y, z: p.x * s + p.z * c }; }
function project(p) {
  const w = innerWidth, h = innerHeight;
  const cam = rotateY(p, -cameraAngle);
  const z = cam.z + 135;
  const f = Math.min(w, h) * 1.15;
  return { x: w * 0.5 + cam.x * f / z, y: h * 0.54 - (cam.y - 10) * f / z, z };
}

function randn() { return Math.random() * 2 - 1; }
function randomSmokePoint() {
  // Rejection sample an ellipsoid so the cloud has real volume, not a flat sheet.
  for (;;) {
    const p = { x: randn() * 62, y: 16 + randn() * 32, z: randn() * 62 };
    const qx = p.x / 62, qy = (p.y - 16) / 34, qz = p.z / 62;
    if (qx*qx + qy*qy + qz*qz <= 1) return p;
  }
}
function seedSmoke() {
  const count = quality === 0 ? 220 : quality === 1 ? 360 : 560;
  smoke = [];
  for (let i = 0; i < count; i++) {
    const p = randomSmokePoint();
    smoke.push({
      p,
      age: Math.random(),
      life: 5 + Math.random() * 8,
      seed: Math.random() * 1000,
      size: 0.75 + Math.random() * 1.15,
      density: 0.25 + Math.random() * 0.55,
    });
  }
}
seedSmoke();

function flowVelocity(p, seed, tt) {
  // Cheap curl-ish field: horizontal swirl + slow upward thermal drift.
  const cx = p.x * 0.035, cy = p.y * 0.045, cz = p.z * 0.035;
  const a = Math.sin(cz * 2.1 + tt * 0.7 + seed) + Math.cos(cy * 1.3 - tt * 0.45);
  const b = Math.cos(cx * 1.8 - tt * 0.55 + seed * 0.7) + Math.sin(cz * 1.2 + tt * 0.35);
  const swirl = { x: -p.z * 0.018 + a * 1.2, y: 0.55 + Math.sin(cx + cz + tt) * 0.35, z: p.x * 0.018 + b * 1.2 };
  return swirl;
}
function updateSmoke(dt) {
  for (const s of smoke) {
    s.age += dt;
    const v = flowVelocity(s.p, s.seed, time);
    s.p.x += v.x * dt * 5.0;
    s.p.y += v.y * dt * 4.0;
    s.p.z += v.z * dt * 5.0;
    // Soft containment. Escaped/old particles respawn at the bottom-ish of the cloud.
    const qx = s.p.x / 72, qy = (s.p.y - 18) / 42, qz = s.p.z / 72;
    if (qx*qx + qy*qy + qz*qz > 1.35 || s.age > s.life) {
      s.p = randomSmokePoint();
      s.p.y -= 12 * Math.random();
      s.age = 0;
      s.life = 5 + Math.random() * 8;
      s.seed = Math.random() * 1000;
    }
  }
}

function particleDensity(s) {
  const qx = s.p.x / 66, qy = (s.p.y - 16) / 38, qz = s.p.z / 66;
  const ellipsoid = 1 - smoothStep(0.86, 1.22, qx*qx + qy*qy + qz*qz);
  const fade = smoothStep(0.0, 0.8, s.age) * (1 - smoothStep(s.life - 1.6, s.life, s.age));
  const wisp = 0.75 + 0.25 * Math.sin(s.p.x * 0.08 + s.p.z * 0.06 + s.seed + time * 0.8);
  return Math.max(0, ellipsoid * fade * wisp * s.density * 0.62);
}

function projectorLightAt(p) {
  const rgb = [0, 0, 0];
  let hits = 0;
  const contributors = [];
  for (const proj of projectorFrames) {
    const d = sub(p, proj.pos);
    const z = dot(d, proj.forward);
    if (z <= 2) continue;
    const fovScale = 0.42;
    const u = dot(d, proj.right) / (z * fovScale);
    const v = dot(d, proj.up) / (z * fovScale);
    if (Math.abs(u) > 1 || Math.abs(v) > 1) continue;
    const img = pattern(u, v, proj.id);
    if (img <= 0.01) continue;
    const edge = 1 - smoothStep(0.78, 1.0, Math.max(Math.abs(u), Math.abs(v)));
    const distAtten = 1 / (1 + 0.00085 * dot(d, d));
    const contrib = img * edge * distAtten * 6.2;
    rgb[0] += proj.color[0] * contrib;
    rgb[1] += proj.color[1] * contrib;
    rgb[2] += proj.color[2] * contrib;
    hits += Math.min(1, contrib * 0.55);
    contributors.push({ proj, contrib });
  }
  return { rgb, hits, contributors, energy: (rgb[0] + rgb[1] + rgb[2]) / 765 };
}

function drawLine3(a, b, color, alpha, width = 1) {
  const pa = project(a), pb = project(b);
  if (pa.z <= 1 || pb.z <= 1) return;
  ctx.strokeStyle = rgba(color, alpha);
  ctx.lineWidth = width;
  ctx.beginPath(); ctx.moveTo(pa.x, pa.y); ctx.lineTo(pb.x, pb.y); ctx.stroke();
}
function drawParticle(s) {
  const dens = particleDensity(s);
  if (dens <= 0.01) return;
  const light = projectorLightAt(s.p);
  const pp = project(s.p);
  if (pp.z <= 1) return;

  let rgb = light.rgb;
  const neutral = 30 + dens * 30;
  rgb = [rgb[0] + neutral * 0.55, rgb[1] + neutral * 0.60, rgb[2] + neutral * 0.72];
  let alpha = Math.min(0.34, dens * (0.025 + light.energy * 0.30));
  if (mode === 1) {
    const multi = smoothStep(0.8, 2.15, light.hits);
    rgb = [rgb[0] * 0.28 + 255 * multi, rgb[1] * 0.25 + 225 * multi, rgb[2] * 0.20 + 70 * multi];
    alpha = Math.min(0.46, alpha + multi * 0.22);
  } else if (mode === 2) {
    alpha *= 0.55;
  }

  const r = s.size * (quality === 0 ? 3.6 : 2.8) * Math.min(innerWidth, innerHeight) / pp.z;
  const g = ctx.createRadialGradient(pp.x, pp.y, 0, pp.x, pp.y, r * 2.4);
  g.addColorStop(0, rgba(rgb, alpha));
  g.addColorStop(1, rgba(rgb, 0));
  ctx.fillStyle = g;
  ctx.beginPath(); ctx.arc(pp.x, pp.y, r * 2.4, 0, Math.PI * 2); ctx.fill();

  if (mode === 2 && light.energy > 0.16 && sparseRays.length < 80) {
    for (const c of light.contributors) {
      if (c.contrib > 0.12) sparseRays.push({ from: c.proj.lens, to: s.p, color: c.proj.color, contrib: c.contrib });
    }
  }
}
function drawProjectorIcons() {
  for (const proj of projectorFrames) {
    const pp = project(proj.pos);
    if (pp.z <= 1) continue;
    const r = 5 * Math.min(innerWidth, innerHeight) / pp.z;
    ctx.fillStyle = rgba(proj.color, 0.85);
    ctx.beginPath(); ctx.arc(pp.x, pp.y, Math.max(3, r * 2.3), 0, Math.PI * 2); ctx.fill();
  }
}
function drawGrid() {
  for (let i = -9; i <= 9; i++) {
    drawLine3({ x: i * 10, y: 0, z: -90 }, { x: i * 10, y: 0, z: 90 }, [80,120,160], 0.14);
    drawLine3({ x: -90, y: 0, z: i * 10 }, { x: 90, y: 0, z: i * 10 }, [80,120,160], 0.14);
  }
}

function render() {
  const w = innerWidth, h = innerHeight;
  ctx.clearRect(0, 0, w, h);
  const bg = ctx.createLinearGradient(0, 0, 0, h);
  bg.addColorStop(0, '#0b1020'); bg.addColorStop(0.55, '#050912'); bg.addColorStop(1, '#02040a');
  ctx.fillStyle = bg; ctx.fillRect(0, 0, w, h);
  drawGrid();

  sparseRays = [];
  const sorted = smoke.slice().sort((a, b) => project(b.p).z - project(a.p).z);
  for (const s of sorted) drawParticle(s);
  if (mode === 2) {
    for (const ray of sparseRays) drawLine3(ray.from, ray.to, ray.color, Math.min(0.34, 0.08 + ray.contrib * 0.08), 1.1);
  }
  drawProjectorIcons();

  ctx.fillStyle = 'rgba(235,245,255,0.82)';
  ctx.font = '12px system-ui, sans-serif';
  ctx.fillText(mode === 1 ? 'elo fust: tobbszorosen megvilagitott reszecskek' : mode === 2 ? 'az aktualis fustreszecskeket megvilagito utak' : 'elo fustreszecskek pillanatnyi fenyosszege', 14, h - 70);
}

function animate(now = performance.now()) {
  requestAnimationFrame(animate);
  if (document.hidden) return;
  const dt = Math.min(0.033, (now - last) / 1000 || 0.016);
  last = now;
  time += dt;
  frameNo++;
  if (orbit) cameraAngle += dt * 0.20;
  updateSmoke(dt);
  render();
}

function setMode(next) {
  mode = next;
  document.querySelectorAll('[data-mode]').forEach(btn => btn.classList.toggle('active', Number(btn.dataset.mode) === mode));
}
function setQuality(next) {
  quality = next;
  document.getElementById('qualityBtn').textContent = ['Low', 'Med', 'High'][quality];
  document.getElementById('qualityBtn').classList.toggle('active', quality > 0);
  resize(); seedSmoke();
}

document.querySelectorAll('[data-mode]').forEach(btn => btn.addEventListener('click', () => setMode(Number(btn.dataset.mode))));
document.getElementById('orbitBtn').addEventListener('click', () => { orbit = !orbit; document.getElementById('orbitBtn').classList.toggle('active', orbit); });
document.getElementById('qualityBtn').addEventListener('click', () => setQuality((quality + 1) % 3));
addEventListener('keydown', e => { if (e.key === '1') setMode(0); if (e.key === '2') setMode(1); if (e.key === '3') setMode(2); if (e.key === ' ') orbit = !orbit; });

setMode(0); setQuality(quality); animate();
